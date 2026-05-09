// SPDX-License-Identifier: TBD
// TH25-UI: UNIT-103 AlarmDisplay 空殻 ユニット試験
//          (UTPR-TH25-001 v0.23 §7.2 と整合).
//
// 試験対象: src/th25_ui/include/th25_ui/alarm_display.hpp /
//            src/th25_ui/src/alarm_display.cpp (UNIT-103 空殻).
// 適用範囲: SDD-TH25-001 v0.1.1 §3.2 で予告された Inc.4 で本格化予定の
//            アラーム/エラー表示 UI ユニット (本 v0.1 では空殻 + 常時拒否).
//            SDD §4 独立セクションは Inc.4 SDD 改訂 (SDD v0.4) で §4.17 として
//            正式追記される予定.
//
// IEC 62304 §5.5.4 クラス C 追加受入基準:
//   - 正常系 / 境界値 / 異常系 / 並行処理 / 資源 / データフロー
//
// Therac-25 主要因類型 (SPRP §4.3.1):
//   - A (race condition): std::atomic<std::uint64_t> + UT TSan 並行試験で
//                         機械検証 (空殻段階から並行設計確立).
//   - E (cryptic / bypass): display_alarm() が同期 API として常時
//                            ErrorCode::InternalUnexpectedState を返す構造により
//                            「UI 層未実装時はアラーム表示要求を受付不可」
//                            インタフェース契約を IF 骨格段階で確立 (RCM-009
//                            中核の構造的位置づけ、Therac-25 East Texas 事故型
//                            「MALFUNCTION 54 単独表示」廃止に向けた設計).
//   - F (legacy preconditions): is_always_lock_free static_assert (build 時検証).
//
// 本 v0.1 範囲の UT 限界:
//   実 UI 表示 + 人間可読メッセージ生成 + 操作者承認待ち管理 + Severity::Critical
//   時の LifecycleState::Halted 連携 + IPC 経由アラーム配信受信 + AuditLogger
//   転送は Inc.4 範囲のため、本ファイルでは扱わない. 本 v0.1 では IF 骨格 +
//   display_alarm() の常時拒否 + display_count_ 集計 + lock-free 表明 + race-free
//   並行試験を対象とする.

#include "th25_ui/alarm_display.hpp"

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <thread>
#include <type_traits>
#include <vector>

#include "th25_ctrl/common_types.hpp"

namespace th25_ui {

// ============================================================================
// UT-103-01: 初期状態 (display_count_ = 0)
// ============================================================================
TEST(AlarmDisplay_Initial, DefaultState) {
    AlarmDisplay display;
    EXPECT_EQ(display.display_count(), 0U);
}

// ============================================================================
// UT-103-02: display_alarm() 単体呼出で常時 ErrorCode::InternalUnexpectedState
//            を返す (空殻範囲: UI 層未実装時は受付不可)
// ============================================================================
TEST(AlarmDisplay_Display, AlwaysReturnsInternalUnexpectedState) {
    AlarmDisplay display;
    const auto result = display.display_alarm(
        th25_ctrl::Severity::Critical,
        th25_ctrl::ErrorCode::ModeInvalidTransition);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error_code(), th25_ctrl::ErrorCode::InternalUnexpectedState);
}

// ============================================================================
// UT-103-03: display_alarm() 呼出で display_count() が 1 増加
// ============================================================================
TEST(AlarmDisplay_Display, SingleDisplayIncrementsCount) {
    AlarmDisplay display;
    EXPECT_EQ(display.display_count(), 0U);
    static_cast<void>(display.display_alarm(
        th25_ctrl::Severity::High,
        th25_ctrl::ErrorCode::TurntableOutOfPosition));
    EXPECT_EQ(display.display_count(), 1U);
}

// ============================================================================
// UT-103-04: 累積 display_alarm() 呼出で display_count() が呼出回数だけ増加
// ============================================================================
TEST(AlarmDisplay_Display, MultipleDisplayAccumulatesCount) {
    AlarmDisplay display;
    constexpr std::uint64_t kIterations = 100U;
    for (std::uint64_t i = 0U; i < kIterations; ++i) {
        const auto result = display.display_alarm(
            th25_ctrl::Severity::Critical,
            th25_ctrl::ErrorCode::DoseTargetExceeded);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error_code(),
                  th25_ctrl::ErrorCode::InternalUnexpectedState);
    }
    EXPECT_EQ(display.display_count(), kIterations);
}

// ============================================================================
// UT-103-05: 多様な Severity × ErrorCode 組合せで常時 InternalUnexpectedState
//            (空殻範囲: 入力依存しない構造、RCM-009 中核の構造的位置づけ)
//
// 本 UT は Therac-25 East Texas 事故型「MALFUNCTION 54 単独表示 + バイパス
// 常態化」の構造的予防を空殻段階から確立: Severity 全 4 値 × 代表的 ErrorCode
// 各系統で UI 層が未実装な間は受付不可. Inc.4 で本格 UI 実装時に人間可読
// メッセージ生成 (RCM-009) + Severity::Critical 時の Halted 連携 (RCM-010 +
// SRS-UX-002) を追加する.
// ============================================================================
TEST(AlarmDisplay_Display, RejectsAllSeverityErrorCodeCombinations) {
    AlarmDisplay display;

    struct Input {
        th25_ctrl::Severity severity;
        th25_ctrl::ErrorCode code;
    };

    const std::vector<Input> kInputs = {
        // Critical (Mode/Beam/Dose/Internal 系)
        {th25_ctrl::Severity::Critical,
         th25_ctrl::ErrorCode::ModeInvalidTransition},
        {th25_ctrl::Severity::Critical,
         th25_ctrl::ErrorCode::BeamOnNotPermitted},
        {th25_ctrl::Severity::Critical,
         th25_ctrl::ErrorCode::DoseOverflow},
        {th25_ctrl::Severity::Critical,
         th25_ctrl::ErrorCode::InternalUnexpectedState},
        // High (Turntable/IPC 系)
        {th25_ctrl::Severity::High,
         th25_ctrl::ErrorCode::TurntableOutOfPosition},
        {th25_ctrl::Severity::High,
         th25_ctrl::ErrorCode::IpcChannelClosed},
        // Medium (Magnet 系)
        {th25_ctrl::Severity::Medium,
         th25_ctrl::ErrorCode::MagnetCurrentDeviation},
        // Low (代表)
        {th25_ctrl::Severity::Low,
         th25_ctrl::ErrorCode::TurntableMoveTimeout},
    };

    for (const auto& inp : kInputs) {
        const auto result = display.display_alarm(inp.severity, inp.code);
        ASSERT_FALSE(result.has_value())
            << "display_alarm should reject in v0.1 (Inc.4 で本格 UI 実装)";
        EXPECT_EQ(result.error_code(),
                  th25_ctrl::ErrorCode::InternalUnexpectedState);
    }
    EXPECT_EQ(display.display_count(), kInputs.size());
}

// ============================================================================
// UT-103-06: reset_for_test() で display_count_ が 0 に戻る
// ============================================================================
TEST(AlarmDisplay_Reset, ResetClearsDisplayCount) {
    AlarmDisplay display;
    static_cast<void>(display.display_alarm(
        th25_ctrl::Severity::Critical,
        th25_ctrl::ErrorCode::ModeInvalidTransition));
    static_cast<void>(display.display_alarm(
        th25_ctrl::Severity::High,
        th25_ctrl::ErrorCode::TurntableOutOfPosition));
    EXPECT_EQ(display.display_count(), 2U);

    display.reset_for_test();
    EXPECT_EQ(display.display_count(), 0U);

    // reset 後も display_alarm() は引き続き InternalUnexpectedState を返す.
    const auto result = display.display_alarm(
        th25_ctrl::Severity::Low,
        th25_ctrl::ErrorCode::TurntableMoveTimeout);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error_code(),
              th25_ctrl::ErrorCode::InternalUnexpectedState);
    EXPECT_EQ(display.display_count(), 1U);
}

// ============================================================================
// UT-103-07: ErrorCode::InternalUnexpectedState は Internal 系 (0xFF) に属する
//            (SDD §6.1 ErrorCode 階層との整合確認、空殻範囲の選択根拠を機械検証)
// ============================================================================
TEST(AlarmDisplay_ErrorCategory, InternalUnexpectedStateBelongsToInternalCategory) {
    constexpr std::uint8_t kInternalCategory = 0xFFU;
    static_assert(
        th25_ctrl::error_category(th25_ctrl::ErrorCode::InternalUnexpectedState) ==
            kInternalCategory,
        "InternalUnexpectedState must belong to Internal category 0xFF.");

    AlarmDisplay display;
    const auto result = display.display_alarm(
        th25_ctrl::Severity::Critical,
        th25_ctrl::ErrorCode::ModeInvalidTransition);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(th25_ctrl::error_category(result.error_code()), kInternalCategory);
}

// ============================================================================
// UT-103-08: ErrorCode::InternalUnexpectedState は Severity::Critical
//            (SDD §6.2 Severity 階層との整合確認)
// ============================================================================
TEST(AlarmDisplay_Severity, InternalUnexpectedStateIsCritical) {
    static_assert(
        th25_ctrl::severity_of(th25_ctrl::ErrorCode::InternalUnexpectedState) ==
            th25_ctrl::Severity::Critical,
        "InternalUnexpectedState must be Critical severity (fail-stop).");
    SUCCEED();
}

// ============================================================================
// UT-103-09: Copy/Move 禁止 (compile-time 検証、所有権独立、内部 atomic 保持)
// ============================================================================
TEST(AlarmDisplay_Structure, NonCopyableNonMovable) {
    static_assert(!std::is_copy_constructible_v<AlarmDisplay>);
    static_assert(!std::is_copy_assignable_v<AlarmDisplay>);
    static_assert(!std::is_move_constructible_v<AlarmDisplay>);
    static_assert(!std::is_move_assignable_v<AlarmDisplay>);
    SUCCEED();
}

// ============================================================================
// UT-103-10: Default 構築可能 (compile-time)
// ============================================================================
TEST(AlarmDisplay_Structure, DefaultConstructible) {
    static_assert(std::is_default_constructible_v<AlarmDisplay>);
    static_assert(std::is_nothrow_default_constructible_v<AlarmDisplay>);
    SUCCEED();
}

// ============================================================================
// UT-103-11: 並行 N threads × M display_alarm() race-free
//            (HZ-002 機械検証、tsan プリセット必須)
//
// 設計:
//   - 4 threads が並行に display_alarm() を 500 回ずつ呼出
//     (合計 2000 回、Severity 4 値を循環).
//   - 各 display_alarm() は InternalUnexpectedState を返却
//     + display_count_ を 1 増加.
//   - 終了時 display_count() == 4 * 500 == 2000 を機械検証.
//   - tsan プリセットで data race detection 0 を機械検証.
//
// 教訓水平展開 (CR-0021 / Step 33):
//   reader/checker パターンを使わない (本 UT は writer のみ並行) ため
//   do-while パターンは適用対象外. ただし将来 reader 並行を追加する場合は
//   PRB-0005/PRB-0006/CR-0021 教訓に従い do-while パターンを最初から採用.
// ============================================================================
TEST(AlarmDisplay_Concurrency, MultiThreadDisplayRaceFree) {
    AlarmDisplay display;

    constexpr int kThreads = 4;
    constexpr int kIterations = 500;

    constexpr std::array<th25_ctrl::Severity, 4> kSeverities = {
        th25_ctrl::Severity::Critical,
        th25_ctrl::Severity::High,
        th25_ctrl::Severity::Medium,
        th25_ctrl::Severity::Low,
    };

    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(kThreads));

    std::atomic<int> internal_state_hits{0};

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&display, &internal_state_hits, &kSeverities, t]() {
            for (int i = 0; i < kIterations; ++i) {
                // thread index + 反復で Severity を循環
                const auto severity =
                    kSeverities[static_cast<std::size_t>((t + i) % 4)];
                const auto result = display.display_alarm(
                    severity,
                    th25_ctrl::ErrorCode::ModeInvalidTransition);
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
    EXPECT_EQ(display.display_count(), kTotal);
    EXPECT_EQ(internal_state_hits.load(std::memory_order_relaxed),
              static_cast<int>(kTotal));
}

// ============================================================================
// UT-103-12: HZ-007 lock-free 単一表明
//            (std::atomic<std::uint64_t>::is_always_lock_free)
//
// 19 ユニット目に拡大. compiler / 標準ライブラリ更新で is_always_lock_free が
// false になった場合、ビルド時点で fail-stop. (本 UT は static_assert を
// テストファイルからも明示確認することで二重防御).
// ============================================================================
TEST(AlarmDisplay_HZ007, LockFreeAssertion) {
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
        "std::atomic<std::uint64_t> must be always lock-free for "
        "AlarmDisplay display_count_ "
        "(SDD §3.2, RCM-002, HZ-002/HZ-007 structural prevention).");
    SUCCEED();
}

}  // namespace th25_ui
