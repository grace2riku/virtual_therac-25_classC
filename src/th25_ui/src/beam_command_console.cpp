// SPDX-License-Identifier: TBD
// TH25-UI: UNIT-102 BeamCommandConsole implementation
//          (Inc.4 で本格化、本 v0.1 では空殻).
//
// SDD-TH25-001 v0.1.1 §3.2 で予告された UNIT-102 BeamCommandConsole の空殻
// 実装. 本 v0.1 範囲では IF 骨格 + 常時拒否プレースホルダ
// (submit_beam_command() が常に ErrorCode::InternalUnexpectedState を返却) +
// submit_count_ 集計のみ.
// Inc.4 SDD 改訂 (SDD v0.4) で §4.16 UNIT-102 として正式追記される予定.
// Inc.4 で実 UI 表示 + 致死的エラーバイパス試行拒否 (RCM-010) + IPC 経由
// BeamCommand メッセージ送信 + AuditLogger 転送を追加予定.

#include "th25_ui/beam_command_console.hpp"

namespace th25_ui {

// ============================================================================
// SDD §3.2 公開 API (本 v0.1 範囲、Inc.4 SDD 改訂で確定予定).
// ============================================================================

auto BeamCommandConsole::submit_beam_command(BeamCommand command) noexcept
    -> th25_ctrl::Result<void, th25_ctrl::ErrorCode> {
    // 空殻実装: 引数を参照せず常時 ErrorCode::InternalUnexpectedState を返却.
    // Inc.4 で実 UI 実装 (ビームオン/オフ物理ボタン or ソフトボタン + 致死的
    // エラー判定 + バイパス試行拒否 (RCM-010) + IPC 経由 BeamCommand メッセージ
    // 生成・送信 + AuditLogger 転送) に発展.
    //
    // memory ordering / synchronization:
    //   - submit_count_.fetch_add(release): submit_count() reader との
    //     happens-before 関係確立 + read-modify-write のアトミック性.
    //
    // 引数 command は本 v0.1 範囲では未使用 (常時拒否のため). cast to void で
    // コンパイラの未使用引数警告を抑制 (Inc.4 で BeamCommand 分岐ロジック実装時
    // に削除予定).
    static_cast<void>(command);

    submit_count_.fetch_add(1U, std::memory_order_release);

    // 本 v0.1 範囲は UI 層が未実装のため、操作者ビームオン/オフ要求を受付不可
    // とする.
    // ErrorCode::InternalUnexpectedState は SDD §6.1 ErrorCode 階層の
    // Internal 系 (0xFF) Critical (fail-stop) として定義されており、
    // UI 層が未実装のまま操作者ビーム指令を受付ようとすると不定動作になる
    // ことを構造的に予防する意味で本 ErrorCode を採用 (Inc.4 で本格 UI
    // 実装時に Beam 系 0x02 / Auth 系 0x07 等の適切な ErrorCode に変更予定).
    return th25_ctrl::Result<void, th25_ctrl::ErrorCode>::error(
        th25_ctrl::ErrorCode::InternalUnexpectedState);
}

// ============================================================================
// 補助 API (UT / Inc.4 拡張点).
// ============================================================================

auto BeamCommandConsole::submit_count() const noexcept -> std::uint64_t {
    return submit_count_.load(std::memory_order_acquire);
}

auto BeamCommandConsole::reset_for_test() noexcept -> void {
    submit_count_.store(0U, std::memory_order_release);
}

}  // namespace th25_ui
