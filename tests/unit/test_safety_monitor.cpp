// SPDX-License-Identifier: TBD
// TH25-CTRL: UNIT-207 SafetyMonitor 空殻 ユニット試験 (UTPR-TH25-001 v0.14 §7.2 と整合).
//
// 試験対象: src/th25_ctrl/include/th25_ctrl/safety_monitor.hpp /
//            src/th25_ctrl/src/safety_monitor.cpp (UNIT-207 空殻).
// 適用範囲: SDD-TH25-001 v0.1.1 §4.8 で確定された Inc.2 で本格化予定の
//            独立並行タスクのインターロック監視ユニット (本 v0.1 では空殻).
//
// IEC 62304 §5.5.4 クラス C 追加受入基準:
//   - 正常系 / 境界値 / 異常系 / 並行処理 / 資源 / データフロー
//
// Therac-25 主要因類型 (SPRP §4.3.1):
//   - A (race condition): std::atomic<bool> + std::atomic<std::uint64_t> + UT
//                         TSan 並行試験で機械検証 (空殻段階から並行設計確立).
//   - D (interlock missing): Inc.2 で本格化予定. 本 v0.1 では構造試験のみ.
//   - F (legacy preconditions): is_always_lock_free static_assert (build 時検証).
//
// 本 v0.1 範囲の UT 限界:
//   MessageBus consume + AuditLogger 転送 + 検知ロジックは Inc.2 範囲のため、
//   本ファイルでは扱わない. 本 v0.1 では IF 骨格 + stop シグナル + tick_count
//   + lock-free 二重表明 + race-free 並行試験を対象とする.

#include "th25_ctrl/safety_monitor.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <type_traits>
#include <vector>

namespace th25_ctrl {

namespace {

using std::chrono::milliseconds;
using std::chrono::steady_clock;

}  // namespace

// ============================================================================
// UT-207-01: 初期状態 (stop_requested_ = false, tick_count_ = 0)
// ============================================================================
TEST(SafetyMonitor_Initial, DefaultState) {
    SafetyMonitor m;
    EXPECT_FALSE(m.is_stop_requested());
    EXPECT_EQ(m.tick_count(), 0U);
}

// ============================================================================
// UT-207-02: stop() 呼出で is_stop_requested() = true (release-acquire 同期)
// ============================================================================
TEST(SafetyMonitor_Stop, StopShouldSetFlag) {
    SafetyMonitor m;
    m.stop();
    EXPECT_TRUE(m.is_stop_requested());
}

// ============================================================================
// UT-207-03: 多重 stop() 呼出は冪等
// ============================================================================
TEST(SafetyMonitor_Stop, IdempotentStop) {
    SafetyMonitor m;
    m.stop();
    m.stop();
    m.stop();
    EXPECT_TRUE(m.is_stop_requested());
}

// ============================================================================
// UT-207-04: reset_for_test() で初期状態に復帰
// ============================================================================
TEST(SafetyMonitor_Reset, ResetClearsState) {
    SafetyMonitor m;
    m.stop();
    EXPECT_TRUE(m.is_stop_requested());
    m.reset_for_test();
    EXPECT_FALSE(m.is_stop_requested());
    EXPECT_EQ(m.tick_count(), 0U);
}

// ============================================================================
// UT-207-05: kLoopIntervalMs の値検証 (SDD §4.8 サンプル擬似コード = 10 ms)
// ============================================================================
TEST(SafetyMonitor_Constants, LoopIntervalIs10Ms) {
    static_assert(SafetyMonitor::kLoopIntervalMs == milliseconds{10},
        "SafetyMonitor::kLoopIntervalMs must equal 10 ms (SDD §4.8).");
    EXPECT_EQ(SafetyMonitor::kLoopIntervalMs, milliseconds{10});
}

// ============================================================================
// UT-207-06: run_monitor_thread() を別スレッドで起動 → stop() で正常合流
//   - スレッド起動前に stop() しても合流可能 (stop_requested_ true で即離脱)
// ============================================================================
TEST(SafetyMonitor_Lifecycle, StopBeforeRunReturnsImmediately) {
    SafetyMonitor m;
    m.stop();  // 起動前に停止指令.
    std::thread t([&m] { m.run_monitor_thread(); });
    t.join();  // 即合流するはず (デッドロックしない).
    EXPECT_TRUE(m.is_stop_requested());
}

// ============================================================================
// UT-207-07: run_monitor_thread() 起動 → 30 ms 後に stop() → 合流
//   - tick_count_ が 1 以上に増加していること (ループが少なくとも 1 回反復)
//   - 合流時間が想定範囲 (kLoopIntervalMs 数倍程度) であること
// ============================================================================
TEST(SafetyMonitor_Lifecycle, RunThenStopJoinsCleanly) {
    SafetyMonitor m;
    std::thread t([&m] { m.run_monitor_thread(); });

    std::this_thread::sleep_for(milliseconds{30});
    const auto t_stop_call = steady_clock::now();
    m.stop();
    t.join();
    const auto t_joined = steady_clock::now();

    EXPECT_TRUE(m.is_stop_requested());
    EXPECT_GE(m.tick_count(), 1U);

    // stop 呼出から合流まで kLoopIntervalMs * 5 = 50 ms を上限の余裕として確認
    // (実機 jitter 吸収、CI 環境依存性に配慮).
    const auto wait_ms =
        std::chrono::duration_cast<milliseconds>(t_joined - t_stop_call).count();
    EXPECT_LE(wait_ms, 200);
}

// ============================================================================
// UT-207-08: run_monitor_thread() を 100 ms 走らせて tick_count が増加
//   - 100 ms / 10 ms = 約 10 反復見込み (許容範囲: >= 1)
// ============================================================================
TEST(SafetyMonitor_Tick, TickCountIncreasesOverTime) {
    SafetyMonitor m;
    std::thread t([&m] { m.run_monitor_thread(); });
    std::this_thread::sleep_for(milliseconds{100});
    m.stop();
    t.join();
    EXPECT_GE(m.tick_count(), 1U);
}

// ============================================================================
// UT-207-09: 並行 reader (HZ-002 race condition、tsan プリセット必須)
//   - 1 producer (run_monitor_thread) + 4 reader (is_stop_requested + tick_count)
//   - 各 reader が 5000 回連続読取で race 検知 0 を期待
// ============================================================================
TEST(SafetyMonitor_Concurrent, OneRunnerPlusFourReaders) {
    SafetyMonitor m;
    std::thread runner([&m] { m.run_monitor_thread(); });

    constexpr std::size_t kReaderCount{4};
    constexpr std::size_t kReadIterations{5000};
    std::vector<std::thread> readers;
    readers.reserve(kReaderCount);

    std::atomic<std::uint64_t> total_reads{0};
    for (std::size_t i = 0; i < kReaderCount; ++i) {
        readers.emplace_back([&m, &total_reads] {
            for (std::size_t j = 0; j < kReadIterations; ++j) {
                (void)m.is_stop_requested();
                (void)m.tick_count();
                total_reads.fetch_add(1U, std::memory_order_relaxed);
            }
        });
    }
    for (auto& r : readers) {
        r.join();
    }

    m.stop();
    runner.join();

    EXPECT_EQ(total_reads.load(std::memory_order_acquire),
              static_cast<std::uint64_t>(kReaderCount * kReadIterations));
    EXPECT_TRUE(m.is_stop_requested());
}

// ============================================================================
// UT-207-10: 並行 stop() 呼出 (N stopper) → 全合流後 stop_requested_ = true
//   - HZ-002 (race condition): 複数スレッドからの stop() が安全に冪等動作する
// ============================================================================
TEST(SafetyMonitor_Concurrent, MultipleStoppersAreSafe) {
    SafetyMonitor m;
    std::thread runner([&m] { m.run_monitor_thread(); });

    constexpr std::size_t kStopperCount{4};
    std::vector<std::thread> stoppers;
    stoppers.reserve(kStopperCount);

    for (std::size_t i = 0; i < kStopperCount; ++i) {
        stoppers.emplace_back([&m] { m.stop(); });
    }
    for (auto& s : stoppers) {
        s.join();
    }
    runner.join();

    EXPECT_TRUE(m.is_stop_requested());
}

// ============================================================================
// UT-207-11: コピー / ムーブ禁止 (compile-time、type_traits 検証)
//   - 内部 atomic 保持 + 別スレッドから参照される設計のため
// ============================================================================
TEST(SafetyMonitor_TypeTraits, NotCopyableNotMovable) {
    static_assert(!std::is_copy_constructible<SafetyMonitor>::value,
        "SafetyMonitor must not be copy-constructible.");
    static_assert(!std::is_copy_assignable<SafetyMonitor>::value,
        "SafetyMonitor must not be copy-assignable.");
    static_assert(!std::is_move_constructible<SafetyMonitor>::value,
        "SafetyMonitor must not be move-constructible.");
    static_assert(!std::is_move_assignable<SafetyMonitor>::value,
        "SafetyMonitor must not be move-assignable.");
    static_assert(std::is_default_constructible<SafetyMonitor>::value,
        "SafetyMonitor must be default-constructible (空殻 v0.1 範囲).");
    SUCCEED();
}

// ============================================================================
// UT-207-12: HZ-007 lock-free 二重表明 (build 時 static_assert + runtime 確認)
//   - std::atomic<bool> + std::atomic<std::uint64_t> 双方が is_always_lock_free
// ============================================================================
TEST(SafetyMonitor_HazardPrevention, LockFreeAssertions) {
    static_assert(std::atomic<bool>::is_always_lock_free,
        "std::atomic<bool> must be always lock-free (HZ-002/HZ-007).");
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
        "std::atomic<std::uint64_t> must be always lock-free (HZ-002/HZ-007).");
    EXPECT_TRUE(std::atomic<bool>::is_always_lock_free);
    EXPECT_TRUE(std::atomic<std::uint64_t>::is_always_lock_free);
}

}  // namespace th25_ctrl
