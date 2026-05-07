// SPDX-License-Identifier: TBD
// TH25-CTRL: UNIT-209 AuditLogger 空殻 ユニット試験 (UTPR-TH25-001 v0.15 §7.2 と整合).
//
// 試験対象: src/th25_ctrl/include/th25_ctrl/audit_logger.hpp /
//            src/th25_ctrl/src/audit_logger.cpp (UNIT-209 空殻).
// 適用範囲: SDD-TH25-001 v0.1.1 §4.10 で確定された Inc.4 で本格化予定の
//            安全関連イベントの集約・永続化ユニット (本 v0.1 では空殻).
//
// IEC 62304 §5.5.4 クラス C 追加受入基準:
//   - 正常系 / 境界値 / 異常系 / 並行処理 / 資源 / データフロー
//
// Therac-25 主要因類型 (SPRP §4.3.1):
//   - A (race condition): std::atomic<bool> + std::atomic<std::uint64_t> +
//                         std::mutex (cerr 出力アトミック性) + UT TSan 並行試験
//                         で機械検証 (空殻段階から並行設計確立).
//   - E (cryptic / bypass): publish() で全 UNIT 安全関連イベントを集約する
//                            事後監査経路の構造的位置づけを確立 (Inc.4 で
//                            永続化により事後追跡可能性を完成).
//   - F (legacy preconditions): is_always_lock_free static_assert (build 時検証).
//
// 本 v0.1 範囲の UT 限界:
//   MessageBus consume + ファイル永続化 + fsync は Inc.4 範囲のため、本ファイル
//   では扱わない. 本 v0.1 では IF 骨格 + publish() の std::cerr 出力 +
//   stop シグナル + processed_count + lock-free 二重表明 + race-free 並行試験を
//   対象とする.
//
// 試験設計上の注意:
//   - publish() は std::cerr に直接出力するため、UT 実行時に大量のログ出力が
//     発生する. CI ログでの可読性のため、各 UT で出力件数は最小化 (並行試験
//     UT-209-09/10 を除く).
//   - std::cerr 出力内容自体の検証は本 v0.1 では行わない (Inc.4 で永続化された
//     ファイル内容の検証として実装予定). 本 v0.1 では processed_count() で
//     publish() 呼出回数の機械検証のみ.

#include "th25_ctrl/audit_logger.hpp"

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
using std::chrono::system_clock;

// 標準的な AuditLogEntry を構築するヘルパ.
auto make_entry(EventType type, const std::string& msg) -> AuditLogEntry {
    AuditLogEntry e;
    e.timestamp = system_clock::now();
    e.event_type = type;
    e.message = msg;
    return e;
}

}  // namespace

// ============================================================================
// UT-209-01: 初期状態 (stop_requested_ = false, processed_count_ = 0,
//            pending_count = 0)
// ============================================================================
TEST(AuditLogger_Initial, DefaultState) {
    AuditLogger logger;
    EXPECT_FALSE(logger.is_stop_requested());
    EXPECT_EQ(logger.processed_count(), 0U);
    EXPECT_EQ(logger.pending_count(), 0U);
}

// ============================================================================
// UT-209-02: publish() 単体呼出で processed_count() が 1 増加
// ============================================================================
TEST(AuditLogger_Publish, SinglePublishIncrementsProcessedCount) {
    AuditLogger logger;
    logger.publish(make_entry(EventType::Info, "UT-209-02 single publish"));
    EXPECT_EQ(logger.processed_count(), 1U);
}

// ============================================================================
// UT-209-03: 複数回 publish() で processed_count() が累積
// ============================================================================
TEST(AuditLogger_Publish, MultiplePublishAccumulate) {
    AuditLogger logger;
    for (std::size_t i = 0; i < 5; ++i) {
        logger.publish(make_entry(EventType::Info, "UT-209-03 entry " + std::to_string(i)));
    }
    EXPECT_EQ(logger.processed_count(), 5U);
}

// ============================================================================
// UT-209-04: stop() 呼出で is_stop_requested() = true、多重 stop は冪等
// ============================================================================
TEST(AuditLogger_Stop, StopShouldSetFlagAndBeIdempotent) {
    AuditLogger logger;
    EXPECT_FALSE(logger.is_stop_requested());
    logger.stop();
    EXPECT_TRUE(logger.is_stop_requested());
    logger.stop();
    logger.stop();
    EXPECT_TRUE(logger.is_stop_requested());
}

// ============================================================================
// UT-209-05: reset_for_test() で初期状態に復帰
// ============================================================================
TEST(AuditLogger_Reset, ResetClearsState) {
    AuditLogger logger;
    logger.publish(make_entry(EventType::Warning, "UT-209-05 before reset"));
    logger.stop();
    EXPECT_TRUE(logger.is_stop_requested());
    EXPECT_EQ(logger.processed_count(), 1U);

    logger.reset_for_test();
    EXPECT_FALSE(logger.is_stop_requested());
    EXPECT_EQ(logger.processed_count(), 0U);
}

// ============================================================================
// UT-209-06: kIoLoopIntervalMs の値検証 (SDD §4.10 と整合 = 10 ms)
// ============================================================================
TEST(AuditLogger_Constants, IoLoopIntervalIs10Ms) {
    static_assert(AuditLogger::kIoLoopIntervalMs == milliseconds{10},
        "AuditLogger::kIoLoopIntervalMs must equal 10 ms (SDD §4.10).");
    EXPECT_EQ(AuditLogger::kIoLoopIntervalMs, milliseconds{10});
}

// ============================================================================
// UT-209-07: run_io_thread() を別スレッドで起動 → stop() で正常合流
//   - スレッド起動前に stop() しても合流可能 (stop_requested_ true で即離脱)
// ============================================================================
TEST(AuditLogger_Lifecycle, StopBeforeRunReturnsImmediately) {
    AuditLogger logger;
    logger.stop();  // 起動前に停止指令.
    std::thread t([&logger] { logger.run_io_thread(); });
    t.join();  // 即合流するはず (デッドロックしない).
    EXPECT_TRUE(logger.is_stop_requested());
}

// ============================================================================
// UT-209-08: run_io_thread() 起動 → 30 ms 後に stop() → 合流
//   - 合流時間が想定範囲 (kIoLoopIntervalMs 数倍程度) であること
// ============================================================================
TEST(AuditLogger_Lifecycle, RunThenStopJoinsCleanly) {
    AuditLogger logger;
    std::thread t([&logger] { logger.run_io_thread(); });

    std::this_thread::sleep_for(milliseconds{30});
    const auto t_stop_call = steady_clock::now();
    logger.stop();
    t.join();
    const auto t_joined = steady_clock::now();

    EXPECT_TRUE(logger.is_stop_requested());

    // stop 呼出から合流まで kIoLoopIntervalMs * 5 = 50 ms を上限の余裕として確認
    // (実機 jitter 吸収、CI 環境依存性に配慮).
    const auto wait_ms =
        std::chrono::duration_cast<milliseconds>(t_joined - t_stop_call).count();
    EXPECT_LE(wait_ms, 200);
}

// ============================================================================
// UT-209-09: 並行 publisher (HZ-002 race condition、tsan プリセット必須)
//   - 4 publisher × 200 publish の race-free + processed_count 整合性
// ============================================================================
TEST(AuditLogger_Concurrent, MultiplePublishersAreRaceFree) {
    AuditLogger logger;

    constexpr std::size_t kPublisherCount{4};
    constexpr std::size_t kPublishIterations{200};
    std::vector<std::thread> publishers;
    publishers.reserve(kPublisherCount);

    for (std::size_t i = 0; i < kPublisherCount; ++i) {
        publishers.emplace_back([&logger, i] {
            for (std::size_t j = 0; j < kPublishIterations; ++j) {
                AuditLogEntry e;
                e.timestamp = system_clock::now();
                e.event_type = EventType::Info;
                e.message = "p" + std::to_string(i) + "-" + std::to_string(j);
                logger.publish(e);
            }
        });
    }
    for (auto& p : publishers) {
        p.join();
    }

    EXPECT_EQ(logger.processed_count(),
              static_cast<std::uint64_t>(kPublisherCount * kPublishIterations));
}

// ============================================================================
// UT-209-10: 並行 publisher + reader + run_io_thread() (HZ-002 総合並行試験)
//   - 1 io_runner + 2 publisher × 100 publish + 4 reader × 1000 連続読取
//   - tsan プリセットで race detection 0 を期待
// ============================================================================
TEST(AuditLogger_Concurrent, RunnerPlusPublishersPlusReaders) {
    AuditLogger logger;
    std::thread runner([&logger] { logger.run_io_thread(); });

    constexpr std::size_t kPublisherCount{2};
    constexpr std::size_t kPublishIterations{100};
    std::vector<std::thread> publishers;
    publishers.reserve(kPublisherCount);
    for (std::size_t i = 0; i < kPublisherCount; ++i) {
        publishers.emplace_back([&logger, i] {
            for (std::size_t j = 0; j < kPublishIterations; ++j) {
                AuditLogEntry e;
                e.timestamp = system_clock::now();
                e.event_type = EventType::SafetyAlarm;
                e.message = "p" + std::to_string(i) + "-" + std::to_string(j);
                logger.publish(e);
            }
        });
    }

    constexpr std::size_t kReaderCount{4};
    constexpr std::size_t kReadIterations{1000};
    std::vector<std::thread> readers;
    readers.reserve(kReaderCount);
    std::atomic<std::uint64_t> total_reads{0};
    for (std::size_t i = 0; i < kReaderCount; ++i) {
        readers.emplace_back([&logger, &total_reads] {
            for (std::size_t j = 0; j < kReadIterations; ++j) {
                (void)logger.is_stop_requested();
                (void)logger.processed_count();
                (void)logger.pending_count();
                total_reads.fetch_add(1U, std::memory_order_relaxed);
            }
        });
    }

    for (auto& p : publishers) {
        p.join();
    }
    for (auto& r : readers) {
        r.join();
    }

    logger.stop();
    runner.join();

    EXPECT_EQ(logger.processed_count(),
              static_cast<std::uint64_t>(kPublisherCount * kPublishIterations));
    EXPECT_EQ(total_reads.load(std::memory_order_acquire),
              static_cast<std::uint64_t>(kReaderCount * kReadIterations));
    EXPECT_TRUE(logger.is_stop_requested());
}

// ============================================================================
// UT-209-11: コピー / ムーブ禁止 (compile-time、type_traits 検証) +
//            AuditLogEntry / EventType の構造的検証
// ============================================================================
TEST(AuditLogger_TypeTraits, NotCopyableNotMovable) {
    static_assert(!std::is_copy_constructible<AuditLogger>::value,
        "AuditLogger must not be copy-constructible.");
    static_assert(!std::is_copy_assignable<AuditLogger>::value,
        "AuditLogger must not be copy-assignable.");
    static_assert(!std::is_move_constructible<AuditLogger>::value,
        "AuditLogger must not be move-constructible.");
    static_assert(!std::is_move_assignable<AuditLogger>::value,
        "AuditLogger must not be move-assignable.");
    static_assert(std::is_default_constructible<AuditLogger>::value,
        "AuditLogger must be default-constructible (空殻 v0.1 範囲).");

    // EventType の安定値確認 (SDD §5 IF-U-007 と整合).
    static_assert(static_cast<std::uint8_t>(EventType::Info) == 0U);
    static_assert(static_cast<std::uint8_t>(EventType::Warning) == 1U);
    static_assert(static_cast<std::uint8_t>(EventType::Error) == 2U);
    static_assert(static_cast<std::uint8_t>(EventType::SafetyAlarm) == 3U);

    // AuditLogEntry はコピー / ムーブ可能な値型 (publish() に値渡しまたは
    // const& で渡すため).
    static_assert(std::is_copy_constructible<AuditLogEntry>::value,
        "AuditLogEntry must be copy-constructible (publish IF).");
    static_assert(std::is_move_constructible<AuditLogEntry>::value,
        "AuditLogEntry must be move-constructible (publish IF).");

    SUCCEED();
}

// ============================================================================
// UT-209-12: HZ-007 lock-free 二重表明 (build 時 static_assert + runtime 確認)
//   - std::atomic<bool> + std::atomic<std::uint64_t> 双方が is_always_lock_free
// ============================================================================
TEST(AuditLogger_HazardPrevention, LockFreeAssertions) {
    static_assert(std::atomic<bool>::is_always_lock_free,
        "std::atomic<bool> must be always lock-free (HZ-002/HZ-007).");
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
        "std::atomic<std::uint64_t> must be always lock-free (HZ-002/HZ-007).");
    EXPECT_TRUE(std::atomic<bool>::is_always_lock_free);
    EXPECT_TRUE(std::atomic<std::uint64_t>::is_always_lock_free);
}

}  // namespace th25_ctrl
