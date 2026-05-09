// SPDX-License-Identifier: TBD
// TH25-UI: UNIT-101 PrescriptionEditor implementation
//          (Inc.4 で本格化、本 v0.1 では空殻).
//
// SDD-TH25-001 v0.1.1 §3.2 で予告された UNIT-101 PrescriptionEditor の空殻
// 実装. 本 v0.1 範囲では IF 骨格 + 常時拒否プレースホルダ (submit_prescription()
// が常に ErrorCode::InternalUnexpectedState を返却) + submit_count_ 集計のみ.
// Inc.4 SDD 改訂 (SDD v0.4) で §4.15 UNIT-101 として正式追記される予定.
// Inc.4 で実 UI 表示 + 入力バリデーション + IPC 経由 PrescriptionSet メッセージ
// 送信 + AuditLogger 転送を追加予定.

#include "th25_ui/prescription_editor.hpp"

namespace th25_ui {

// ============================================================================
// SDD §3.2 公開 API (本 v0.1 範囲、Inc.4 SDD 改訂で確定予定).
// ============================================================================

auto PrescriptionEditor::submit_prescription(
    th25_ctrl::TreatmentMode mode,
    const std::variant<th25_ctrl::Energy_MeV, th25_ctrl::Energy_MV>& energy,
    th25_ctrl::DoseUnit_cGy target_dose) noexcept
    -> th25_ctrl::Result<void, th25_ctrl::ErrorCode> {
    // 空殻実装: 引数を参照せず常時 ErrorCode::InternalUnexpectedState を返却.
    // Inc.4 で実 UI 実装 (TreatmentMode / Energy / DoseUnit_cGy 入力ウィジェット
    // + 入力バリデーション + IPC 経由 PrescriptionSet メッセージ生成・送信
    // + AuditLogger 転送) に発展.
    //
    // memory ordering / synchronization:
    //   - submit_count_.fetch_add(release): submit_count() reader との
    //     happens-before 関係確立 + read-modify-write のアトミック性.
    //
    // 引数 mode / energy / target_dose は本 v0.1 範囲では未使用 (常時拒否
    // のため). cast to void でコンパイラの未使用引数警告を抑制
    // (Inc.4 で実 UI ロジック実装時に削除予定).
    static_cast<void>(mode);
    static_cast<void>(energy);
    static_cast<void>(target_dose);

    submit_count_.fetch_add(1U, std::memory_order_release);

    // 本 v0.1 範囲は UI 層が未実装のため、操作者入力を受付不可とする.
    // ErrorCode::InternalUnexpectedState は SDD §6.1 ErrorCode 階層の
    // Internal 系 (0xFF) Critical (fail-stop) として定義されており、
    // UI 層が未実装のまま操作者入力を受付ようとすると不定動作になる
    // ことを構造的に予防する意味で本 ErrorCode を採用 (Inc.4 で本格 UI
    // 実装時に Auth 系 / Beam 系等の適切な ErrorCode に変更予定).
    return th25_ctrl::Result<void, th25_ctrl::ErrorCode>::error(
        th25_ctrl::ErrorCode::InternalUnexpectedState);
}

// ============================================================================
// 補助 API (UT / Inc.4 拡張点).
// ============================================================================

auto PrescriptionEditor::submit_count() const noexcept -> std::uint64_t {
    return submit_count_.load(std::memory_order_acquire);
}

auto PrescriptionEditor::reset_for_test() noexcept -> void {
    submit_count_.store(0U, std::memory_order_release);
}

}  // namespace th25_ui
