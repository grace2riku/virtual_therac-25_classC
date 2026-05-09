// SPDX-License-Identifier: TBD
// TH25-UI: UNIT-103 AlarmDisplay implementation
//          (Inc.4 で本格化、本 v0.1 では空殻).
//
// SDD-TH25-001 v0.1.1 §3.2 で予告された UNIT-103 AlarmDisplay の空殻実装.
// 本 v0.1 範囲では IF 骨格 + 常時拒否プレースホルダ (display_alarm() が常に
// ErrorCode::InternalUnexpectedState を返却) + display_count_ 集計のみ.
// Inc.4 SDD 改訂 (SDD v0.4) で §4.17 UNIT-103 として正式追記される予定.
// Inc.4 で実 UI 表示 + 人間可読メッセージ生成 + 操作者承認待ち管理 +
// Severity::Critical 時の Halted 連携 + AuditLogger 転送を追加予定.

#include "th25_ui/alarm_display.hpp"

namespace th25_ui {

// ============================================================================
// SDD §3.2 公開 API (本 v0.1 範囲、Inc.4 SDD 改訂で確定予定).
// ============================================================================

auto AlarmDisplay::display_alarm(th25_ctrl::Severity severity,
                                 th25_ctrl::ErrorCode code) noexcept
    -> th25_ctrl::Result<void, th25_ctrl::ErrorCode> {
    // 空殻実装: 引数を参照せず常時 ErrorCode::InternalUnexpectedState を返却.
    // Inc.4 で実 UI 実装 (アラーム表示ウィジェット + 人間可読メッセージ生成 +
    // 操作者承認待ち管理 + Severity::Critical 時の LifecycleState::Halted 連携
    // + IPC 経由アラーム配信受信 + AuditLogger 転送) に発展.
    //
    // memory ordering / synchronization:
    //   - display_count_.fetch_add(release): display_count() reader との
    //     happens-before 関係確立 + read-modify-write のアトミック性.
    //
    // 引数 severity / code は本 v0.1 範囲では未使用 (常時拒否のため). cast to
    // void でコンパイラの未使用引数警告を抑制 (Inc.4 で人間可読メッセージ
    // 生成ロジック + Severity 別 UI 表示分岐実装時に削除予定).
    static_cast<void>(severity);
    static_cast<void>(code);

    display_count_.fetch_add(1U, std::memory_order_release);

    // 本 v0.1 範囲は UI 層が未実装のため、アラーム表示要求を受付不可とする.
    // ErrorCode::InternalUnexpectedState は SDD §6.1 ErrorCode 階層の
    // Internal 系 (0xFF) Critical (fail-stop) として定義されており、
    // UI 層が未実装のままアラーム表示要求を受付ようとすると不定動作になる
    // ことを構造的に予防する意味で本 ErrorCode を採用 (Inc.4 で本格 UI
    // 実装時に適切な ErrorCode に変更予定).
    return th25_ctrl::Result<void, th25_ctrl::ErrorCode>::error(
        th25_ctrl::ErrorCode::InternalUnexpectedState);
}

// ============================================================================
// 補助 API (UT / Inc.4 拡張点).
// ============================================================================

auto AlarmDisplay::display_count() const noexcept -> std::uint64_t {
    return display_count_.load(std::memory_order_acquire);
}

auto AlarmDisplay::reset_for_test() noexcept -> void {
    display_count_.store(0U, std::memory_order_release);
}

}  // namespace th25_ui
