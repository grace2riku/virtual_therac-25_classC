// SPDX-License-Identifier: TBD
// TH25-UI: UNIT-102 BeamCommandConsole 空殻 ユニット試験
//          (UTPR-TH25-001 v0.20 §7.2 と整合).
//
// 試験対象: src/th25_ui/include/th25_ui/beam_command_console.hpp /
//            src/th25_ui/src/beam_command_console.cpp (UNIT-102 空殻).
// 適用範囲: SDD-TH25-001 v0.1.1 §3.2 で予告された Inc.4 で本格化予定の
//            ビームオン/オフ操作 UI ユニット (本 v0.1 では空殻 + 常時拒否).
//            SDD §4 独立セクションは Inc.4 SDD 改訂 (SDD v0.4) で §4.16 として
//            正式追記される予定.
//
// IEC 62304 §5.5.4 クラス C 追加受入基準:
//   - 正常系 / 境界値 / 異常系 / 並行処理 / 資源 / データフロー
//
// Therac-25 主要因類型 (SPRP §4.3.1):
//   - A (race condition): std::atomic<std::uint64_t> + UT TSan 並行試験で
//                         機械検証 (空殻段階から並行設計確立).
//   - E (cryptic / bypass): submit_beam_command() が同期 API として常時
//                            ErrorCode::InternalUnexpectedState を返す構造により
//                            「UI 層未実装時は致死的エラーバイパス試行を含めて
//                            ビーム指令を受付不可」インタフェース契約を IF
//                            骨格段階で確立 (RCM-010 中核の構造的位置づけ).
//   - F (legacy preconditions): is_always_lock_free static_assert (build 時検証).
//
// 本 v0.1 範囲の UT 限界:
//   実 UI 表示 + 致死的エラーバイパス試行拒否 + IPC 経由 BeamCommand メッセージ
//   送信 + AuditLogger 転送は Inc.4 範囲のため、本ファイルでは扱わない. 本 v0.1
//   では IF 骨格 + submit_beam_command() の常時拒否 + submit_count_ 集計 +
//   lock-free 表明 + race-free 並行試験を対象とする.

#include "th25_ui/beam_command_console.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <type_traits>
#include <vector>

#include "th25_ctrl/common_types.hpp"

namespace th25_ui {

// ============================================================================
// UT-102-01: 初期状態 (submit_count_ = 0)
// ============================================================================
TEST(BeamCommandConsole_Initial, DefaultState) {
    BeamCommandConsole console;
    EXPECT_EQ(console.submit_count(), 0U);
}

// ============================================================================
// UT-102-02: submit_beam_command() 単体呼出で常時 ErrorCode::InternalUnexpectedState
//            を返す (空殻範囲: UI 層未実装時は受付不可)
// ============================================================================
TEST(BeamCommandConsole_Submit, AlwaysReturnsInternalUnexpectedState) {
    BeamCommandConsole console;
    const auto result = console.submit_beam_command(BeamCommand::On);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error_code(), th25_ctrl::ErrorCode::InternalUnexpectedState);
}

// ============================================================================
// UT-102-03: submit_beam_command() 呼出で submit_count() が 1 増加
// ============================================================================
TEST(BeamCommandConsole_Submit, SingleSubmitIncrementsCount) {
    BeamCommandConsole console;
    EXPECT_EQ(console.submit_count(), 0U);
    static_cast<void>(console.submit_beam_command(BeamCommand::On));
    EXPECT_EQ(console.submit_count(), 1U);
}

// ============================================================================
// UT-102-04: 累積 submit_beam_command() 呼出で submit_count() が呼出回数だけ増加
// ============================================================================
TEST(BeamCommandConsole_Submit, MultipleSubmitAccumulatesCount) {
    BeamCommandConsole console;
    constexpr std::uint64_t kIterations = 100U;
    for (std::uint64_t i = 0U; i < kIterations; ++i) {
        const auto result = console.submit_beam_command(
            (i % 2U == 0U) ? BeamCommand::On : BeamCommand::Off);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error_code(),
                  th25_ctrl::ErrorCode::InternalUnexpectedState);
    }
    EXPECT_EQ(console.submit_count(), kIterations);
}

// ============================================================================
// UT-102-05: 全 BeamCommand 値 (On / Off) で常時 InternalUnexpectedState
//            (空殻範囲: 入力依存しない構造、RCM-010 中核の構造的位置づけ)
//
// 本 UT は Therac-25 East Texas 事故型「致死的エラー発生時の P キー押下による
// 照射継続」の構造的不可能化を空殻段階から確立: BeamCommand 全値で UI 層が
// 未実装な間は受付不可. Inc.4 で本格 UI 実装時に致死的エラー判定 + バイパス
// 試行拒否 (RCM-010) を追加する.
// ============================================================================
TEST(BeamCommandConsole_Submit, RejectsAllBeamCommandValues) {
    BeamCommandConsole console;

    constexpr std::array<BeamCommand, 2U> kCommands = {
        BeamCommand::On,
        BeamCommand::Off,
    };

    for (const auto cmd : kCommands) {
        const auto result = console.submit_beam_command(cmd);
        ASSERT_FALSE(result.has_value())
            << "submit_beam_command should reject in v0.1 (Inc.4 で本格 UI 実装)";
        EXPECT_EQ(result.error_code(),
                  th25_ctrl::ErrorCode::InternalUnexpectedState);
    }
    EXPECT_EQ(console.submit_count(), kCommands.size());
}

// ============================================================================
// UT-102-06: reset_for_test() で submit_count_ が 0 に戻る
// ============================================================================
TEST(BeamCommandConsole_Reset, ResetClearsSubmitCount) {
    BeamCommandConsole console;
    static_cast<void>(console.submit_beam_command(BeamCommand::On));
    static_cast<void>(console.submit_beam_command(BeamCommand::Off));
    EXPECT_EQ(console.submit_count(), 2U);

    console.reset_for_test();
    EXPECT_EQ(console.submit_count(), 0U);

    // reset 後も submit_beam_command() は引き続き InternalUnexpectedState を返す.
    const auto result = console.submit_beam_command(BeamCommand::On);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error_code(),
              th25_ctrl::ErrorCode::InternalUnexpectedState);
    EXPECT_EQ(console.submit_count(), 1U);
}

// ============================================================================
// UT-102-07: ErrorCode::InternalUnexpectedState は Internal 系 (0xFF) に属する
//            (SDD §6.1 ErrorCode 階層との整合確認、空殻範囲の選択根拠を機械検証)
// ============================================================================
TEST(BeamCommandConsole_ErrorCategory, InternalUnexpectedStateBelongsToInternalCategory) {
    constexpr std::uint8_t kInternalCategory = 0xFFU;
    static_assert(
        th25_ctrl::error_category(th25_ctrl::ErrorCode::InternalUnexpectedState) ==
            kInternalCategory,
        "InternalUnexpectedState must belong to Internal category 0xFF.");

    BeamCommandConsole console;
    const auto result = console.submit_beam_command(BeamCommand::On);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(th25_ctrl::error_category(result.error_code()), kInternalCategory);
}

// ============================================================================
// UT-102-08: ErrorCode::InternalUnexpectedState は Severity::Critical
//            (SDD §6.2 Severity 階層との整合確認)
// ============================================================================
TEST(BeamCommandConsole_Severity, InternalUnexpectedStateIsCritical) {
    static_assert(
        th25_ctrl::severity_of(th25_ctrl::ErrorCode::InternalUnexpectedState) ==
            th25_ctrl::Severity::Critical,
        "InternalUnexpectedState must be Critical severity (fail-stop).");
    SUCCEED();
}

// ============================================================================
// UT-102-09: Copy/Move 禁止 (compile-time 検証、所有権独立、内部 atomic 保持)
// ============================================================================
TEST(BeamCommandConsole_Structure, NonCopyableNonMovable) {
    static_assert(!std::is_copy_constructible_v<BeamCommandConsole>);
    static_assert(!std::is_copy_assignable_v<BeamCommandConsole>);
    static_assert(!std::is_move_constructible_v<BeamCommandConsole>);
    static_assert(!std::is_move_assignable_v<BeamCommandConsole>);
    SUCCEED();
}

// ============================================================================
// UT-102-10: Default 構築可能 (compile-time)
// ============================================================================
TEST(BeamCommandConsole_Structure, DefaultConstructible) {
    static_assert(std::is_default_constructible_v<BeamCommandConsole>);
    static_assert(std::is_nothrow_default_constructible_v<BeamCommandConsole>);
    SUCCEED();
}

// ============================================================================
// UT-102-11: 並行 N threads × M submit_beam_command() race-free
//            (HZ-002 機械検証、tsan プリセット必須)
//
// 設計:
//   - 4 threads が並行に submit_beam_command() を 500 回ずつ呼出
//     (合計 2000 回、半数 On / 半数 Off).
//   - 各 submit_beam_command() は InternalUnexpectedState を返却
//     + submit_count_ を 1 増加.
//   - 終了時 submit_count() == 4 * 500 == 2000 を機械検証.
//   - tsan プリセットで data race detection 0 を機械検証.
//
// 教訓水平展開 (CR-0021 / Step 33):
//   reader/checker パターンを使わない (本 UT は writer のみ並行) ため
//   do-while パターンは適用対象外. ただし将来 reader 並行を追加する場合は
//   PRB-0005/PRB-0006/CR-0021 教訓に従い do-while パターンを最初から採用.
// ============================================================================
TEST(BeamCommandConsole_Concurrency, MultiThreadSubmitRaceFree) {
    BeamCommandConsole console;

    constexpr int kThreads = 4;
    constexpr int kIterations = 500;

    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(kThreads));

    std::atomic<int> internal_state_hits{0};

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&console, &internal_state_hits, t]() {
            for (int i = 0; i < kIterations; ++i) {
                // thread index + 反復で On/Off を交互に発行
                const auto cmd =
                    ((t + i) % 2 == 0) ? BeamCommand::On : BeamCommand::Off;
                const auto result = console.submit_beam_command(cmd);
                if (!result.has_value() &&
                    result.error_code() ==
                        th25_ctrl::ErrorCode::InternalUnexpectedState) {
                    internal_state_hits.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    constexpr std::uint64_t kTotal =
        static_cast<std::uint64_t>(kThreads) *
        static_cast<std::uint64_t>(kIterations);
    EXPECT_EQ(console.submit_count(), kTotal);
    EXPECT_EQ(internal_state_hits.load(std::memory_order_relaxed),
              static_cast<int>(kTotal));
}

// ============================================================================
// UT-102-12: HZ-007 lock-free 単一表明
//            (std::atomic<std::uint64_t>::is_always_lock_free)
//
// 18 ユニット目に拡大. compiler / 標準ライブラリ更新で is_always_lock_free が
// false になった場合、ビルド時点で fail-stop. (本 UT は static_assert を
// テストファイルからも明示確認することで二重防御).
// ============================================================================
TEST(BeamCommandConsole_HZ007, LockFreeAssertion) {
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
        "std::atomic<std::uint64_t> must be always lock-free for "
        "BeamCommandConsole submit_count_ "
        "(SDD §3.2, RCM-002, HZ-002/HZ-007 structural prevention).");
    SUCCEED();
}

}  // namespace th25_ui
