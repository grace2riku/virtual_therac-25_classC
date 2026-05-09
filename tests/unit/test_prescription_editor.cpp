// SPDX-License-Identifier: TBD
// TH25-UI: UNIT-101 PrescriptionEditor 空殻 ユニット試験
//          (UTPR-TH25-001 v0.19 §7.2 と整合).
//
// 試験対象: src/th25_ui/include/th25_ui/prescription_editor.hpp /
//            src/th25_ui/src/prescription_editor.cpp (UNIT-101 空殻).
// 適用範囲: SDD-TH25-001 v0.1.1 §3.2 で予告された Inc.4 で本格化予定の
//            治療計画入力 UI ユニット (本 v0.1 では空殻 + 常時拒否).
//            SDD §4 独立セクションは Inc.4 SDD 改訂 (SDD v0.4) で §4.15 として
//            正式追記される予定.
//
// IEC 62304 §5.5.4 クラス C 追加受入基準:
//   - 正常系 / 境界値 / 異常系 / 並行処理 / 資源 / データフロー
//
// Therac-25 主要因類型 (SPRP §4.3.1):
//   - A (race condition): std::atomic<std::uint64_t> + UT TSan 並行試験で
//                         機械検証 (空殻段階から並行設計確立).
//   - E (cryptic / bypass): submit_prescription() が同期 API として常時
//                            ErrorCode::InternalUnexpectedState を返す構造により
//                            「UI 層未実装時は治療計画送信を受付不可」
//                            インタフェース契約を IF 骨格段階で確立.
//   - F (legacy preconditions): is_always_lock_free static_assert (build 時検証).
//
// 本 v0.1 範囲の UT 限界:
//   実 UI 表示 + 入力バリデーション + IPC 経由 PrescriptionSet メッセージ送信 +
//   AuditLogger 転送は Inc.4 範囲のため、本ファイルでは扱わない. 本 v0.1 では
//   IF 骨格 + submit_prescription() の常時拒否 + submit_count_ 集計 + lock-free
//   表明 + race-free 並行試験を対象とする.

#include "th25_ui/prescription_editor.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <type_traits>
#include <variant>
#include <vector>

#include "th25_ctrl/common_types.hpp"

namespace th25_ui {

// ============================================================================
// UT-101-01: 初期状態 (submit_count_ = 0)
// ============================================================================
TEST(PrescriptionEditor_Initial, DefaultState) {
    PrescriptionEditor editor;
    EXPECT_EQ(editor.submit_count(), 0U);
}

// ============================================================================
// UT-101-02: submit_prescription() 単体呼出で常時 ErrorCode::InternalUnexpectedState
//            を返す (空殻範囲: UI 層未実装時は受付不可)
// ============================================================================
TEST(PrescriptionEditor_Submit, AlwaysReturnsInternalUnexpectedState) {
    PrescriptionEditor editor;
    const auto result = editor.submit_prescription(
        th25_ctrl::TreatmentMode::Electron,
        std::variant<th25_ctrl::Energy_MeV, th25_ctrl::Energy_MV>{
            th25_ctrl::Energy_MeV{10.0}},
        th25_ctrl::DoseUnit_cGy{100.0});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error_code(), th25_ctrl::ErrorCode::InternalUnexpectedState);
}

// ============================================================================
// UT-101-03: submit_prescription() 呼出で submit_count() が 1 増加
// ============================================================================
TEST(PrescriptionEditor_Submit, SingleSubmitIncrementsCount) {
    PrescriptionEditor editor;
    EXPECT_EQ(editor.submit_count(), 0U);
    static_cast<void>(editor.submit_prescription(
        th25_ctrl::TreatmentMode::Electron,
        std::variant<th25_ctrl::Energy_MeV, th25_ctrl::Energy_MV>{
            th25_ctrl::Energy_MeV{10.0}},
        th25_ctrl::DoseUnit_cGy{100.0}));
    EXPECT_EQ(editor.submit_count(), 1U);
}

// ============================================================================
// UT-101-04: 累積 submit_prescription() 呼出で submit_count() が呼出回数だけ増加
// ============================================================================
TEST(PrescriptionEditor_Submit, MultipleSubmitAccumulatesCount) {
    PrescriptionEditor editor;
    constexpr std::uint64_t kIterations = 100U;
    for (std::uint64_t i = 0U; i < kIterations; ++i) {
        const auto result = editor.submit_prescription(
            th25_ctrl::TreatmentMode::Electron,
            std::variant<th25_ctrl::Energy_MeV, th25_ctrl::Energy_MV>{
                th25_ctrl::Energy_MeV{10.0}},
            th25_ctrl::DoseUnit_cGy{100.0});
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error_code(),
                  th25_ctrl::ErrorCode::InternalUnexpectedState);
    }
    EXPECT_EQ(editor.submit_count(), kIterations);
}

// ============================================================================
// UT-101-05: 多様な引数組合せ (TreatmentMode 3 種 × Energy 2 variant) でも
//            常時 InternalUnexpectedState (空殻範囲: 入力依存しない構造)
// ============================================================================
TEST(PrescriptionEditor_Submit, RejectsAllInputsRegardlessOfArgs) {
    PrescriptionEditor editor;

    using EnergyVariant =
        std::variant<th25_ctrl::Energy_MeV, th25_ctrl::Energy_MV>;

    struct Input {
        th25_ctrl::TreatmentMode mode;
        EnergyVariant energy;
        th25_ctrl::DoseUnit_cGy dose;
    };

    const std::vector<Input> kInputs = {
        // Light mode (エネルギー無関係だが UT 駆動上値を入れる)
        {th25_ctrl::TreatmentMode::Light, EnergyVariant{th25_ctrl::Energy_MeV{0.0}},
         th25_ctrl::DoseUnit_cGy{0.0}},
        // Electron mode + 境界値
        {th25_ctrl::TreatmentMode::Electron,
         EnergyVariant{th25_ctrl::Energy_MeV{1.0}},
         th25_ctrl::DoseUnit_cGy{0.01}},
        {th25_ctrl::TreatmentMode::Electron,
         EnergyVariant{th25_ctrl::Energy_MeV{25.0}},
         th25_ctrl::DoseUnit_cGy{10000.0}},
        // XRay mode + 境界値
        {th25_ctrl::TreatmentMode::XRay,
         EnergyVariant{th25_ctrl::Energy_MV{5.0}},
         th25_ctrl::DoseUnit_cGy{0.01}},
        {th25_ctrl::TreatmentMode::XRay,
         EnergyVariant{th25_ctrl::Energy_MV{25.0}},
         th25_ctrl::DoseUnit_cGy{10000.0}},
    };

    for (const auto& inp : kInputs) {
        const auto result =
            editor.submit_prescription(inp.mode, inp.energy, inp.dose);
        ASSERT_FALSE(result.has_value())
            << "submit_prescription should reject in v0.1 (Inc.4 で本格 UI 実装)";
        EXPECT_EQ(result.error_code(),
                  th25_ctrl::ErrorCode::InternalUnexpectedState);
    }
    EXPECT_EQ(editor.submit_count(), kInputs.size());
}

// ============================================================================
// UT-101-06: reset_for_test() で submit_count_ が 0 に戻る
// ============================================================================
TEST(PrescriptionEditor_Reset, ResetClearsSubmitCount) {
    PrescriptionEditor editor;
    static_cast<void>(editor.submit_prescription(
        th25_ctrl::TreatmentMode::Electron,
        std::variant<th25_ctrl::Energy_MeV, th25_ctrl::Energy_MV>{
            th25_ctrl::Energy_MeV{10.0}},
        th25_ctrl::DoseUnit_cGy{100.0}));
    static_cast<void>(editor.submit_prescription(
        th25_ctrl::TreatmentMode::XRay,
        std::variant<th25_ctrl::Energy_MeV, th25_ctrl::Energy_MV>{
            th25_ctrl::Energy_MV{15.0}},
        th25_ctrl::DoseUnit_cGy{200.0}));
    EXPECT_EQ(editor.submit_count(), 2U);

    editor.reset_for_test();
    EXPECT_EQ(editor.submit_count(), 0U);

    // reset 後も submit_prescription() は引き続き InternalUnexpectedState を返す.
    const auto result = editor.submit_prescription(
        th25_ctrl::TreatmentMode::Light,
        std::variant<th25_ctrl::Energy_MeV, th25_ctrl::Energy_MV>{
            th25_ctrl::Energy_MeV{0.0}},
        th25_ctrl::DoseUnit_cGy{0.0});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error_code(),
              th25_ctrl::ErrorCode::InternalUnexpectedState);
    EXPECT_EQ(editor.submit_count(), 1U);
}

// ============================================================================
// UT-101-07: ErrorCode::InternalUnexpectedState は Internal 系 (0xFF) に属する
//            (SDD §6.1 ErrorCode 階層との整合確認、空殻範囲の選択根拠を機械検証)
// ============================================================================
TEST(PrescriptionEditor_ErrorCategory, InternalUnexpectedStateBelongsToInternalCategory) {
    constexpr std::uint8_t kInternalCategory = 0xFFU;
    static_assert(
        th25_ctrl::error_category(th25_ctrl::ErrorCode::InternalUnexpectedState) ==
            kInternalCategory,
        "InternalUnexpectedState must belong to Internal category 0xFF.");

    PrescriptionEditor editor;
    const auto result = editor.submit_prescription(
        th25_ctrl::TreatmentMode::Electron,
        std::variant<th25_ctrl::Energy_MeV, th25_ctrl::Energy_MV>{
            th25_ctrl::Energy_MeV{10.0}},
        th25_ctrl::DoseUnit_cGy{100.0});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(th25_ctrl::error_category(result.error_code()), kInternalCategory);
}

// ============================================================================
// UT-101-08: ErrorCode::InternalUnexpectedState は Severity::Critical
//            (SDD §6.2 Severity 階層との整合確認)
// ============================================================================
TEST(PrescriptionEditor_Severity, InternalUnexpectedStateIsCritical) {
    static_assert(
        th25_ctrl::severity_of(th25_ctrl::ErrorCode::InternalUnexpectedState) ==
            th25_ctrl::Severity::Critical,
        "InternalUnexpectedState must be Critical severity (fail-stop).");
    SUCCEED();
}

// ============================================================================
// UT-101-09: Copy/Move 禁止 (compile-time 検証、所有権独立、内部 atomic 保持)
// ============================================================================
TEST(PrescriptionEditor_Structure, NonCopyableNonMovable) {
    static_assert(!std::is_copy_constructible_v<PrescriptionEditor>);
    static_assert(!std::is_copy_assignable_v<PrescriptionEditor>);
    static_assert(!std::is_move_constructible_v<PrescriptionEditor>);
    static_assert(!std::is_move_assignable_v<PrescriptionEditor>);
    SUCCEED();
}

// ============================================================================
// UT-101-10: Default 構築可能 (compile-time)
// ============================================================================
TEST(PrescriptionEditor_Structure, DefaultConstructible) {
    static_assert(std::is_default_constructible_v<PrescriptionEditor>);
    static_assert(std::is_nothrow_default_constructible_v<PrescriptionEditor>);
    SUCCEED();
}

// ============================================================================
// UT-101-11: 並行 N threads × M submit_prescription() race-free
//            (HZ-002 機械検証、tsan プリセット必須)
//
// 設計:
//   - 4 threads が並行に submit_prescription() を 500 回ずつ呼出
//     (合計 2000 回).
//   - 各 submit_prescription() は InternalUnexpectedState を返却
//     + submit_count_ を 1 増加.
//   - 終了時 submit_count() == 4 * 500 == 2000 を機械検証.
//   - tsan プリセットで data race detection 0 を機械検証.
//
// 教訓水平展開 (CR-0021 / Step 33):
//   reader/checker パターンを使わない (本 UT は writer のみ並行) ため
//   do-while パターンは適用対象外. ただし将来 reader 並行を追加する場合は
//   PRB-0005/PRB-0006/CR-0021 教訓に従い do-while パターンを最初から採用.
// ============================================================================
TEST(PrescriptionEditor_Concurrency, MultiThreadSubmitRaceFree) {
    PrescriptionEditor editor;

    constexpr int kThreads = 4;
    constexpr int kIterations = 500;

    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(kThreads));

    std::atomic<int> internal_state_hits{0};

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&editor, &internal_state_hits]() {
            for (int i = 0; i < kIterations; ++i) {
                const auto result = editor.submit_prescription(
                    th25_ctrl::TreatmentMode::Electron,
                    std::variant<th25_ctrl::Energy_MeV, th25_ctrl::Energy_MV>{
                        th25_ctrl::Energy_MeV{10.0}},
                    th25_ctrl::DoseUnit_cGy{100.0});
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
    EXPECT_EQ(editor.submit_count(), kTotal);
    EXPECT_EQ(internal_state_hits.load(std::memory_order_relaxed),
              static_cast<int>(kTotal));
}

// ============================================================================
// UT-101-12: HZ-007 lock-free 単一表明
//            (std::atomic<std::uint64_t>::is_always_lock_free)
//
// 17 ユニット目に拡大. compiler / 標準ライブラリ更新で is_always_lock_free が
// false になった場合、ビルド時点で fail-stop. (本 UT は static_assert を
// テストファイルからも明示確認することで二重防御).
// ============================================================================
TEST(PrescriptionEditor_HZ007, LockFreeAssertion) {
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
        "std::atomic<std::uint64_t> must be always lock-free for "
        "PrescriptionEditor submit_count_ "
        "(SDD §3.2, RCM-002, HZ-002/HZ-007 structural prevention).");
    SUCCEED();
}

}  // namespace th25_ui
