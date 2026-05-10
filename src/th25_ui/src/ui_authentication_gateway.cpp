// SPDX-License-Identifier: TBD
// TH25-UI: UNIT-104 UIAuthenticationGateway implementation
//          (Inc.4 で本格化、本 v0.1 では空殻).
//
// SDD-TH25-001 v0.1.1 §3.2 で予告された UNIT-104 UIAuthenticationGateway の
// 空殻実装. 本 v0.1 範囲では IF 骨格 + 常時拒否プレースホルダ
// (request_authentication() が常に ErrorCode::AuthRequired を返却) +
// attempt_count_ 集計のみ.
// Inc.4 SDD 改訂 (SDD v0.4) で §4.18 UNIT-104 として正式追記される予定.
// Inc.4 で実 UI ID/パスワード入力 + Core 側 UNIT-210 への IF-U-010 経由
// AuthenticationVerify 送信 + AuthenticationResult 待ち + 操作者セッション
// 確立 + AuditLogger 転送を追加予定.

#include "th25_ui/ui_authentication_gateway.hpp"

namespace th25_ui {

// ============================================================================
// SDD §3.2 公開 API (本 v0.1 範囲、Inc.4 SDD 改訂で確定予定).
// ============================================================================

auto UIAuthenticationGateway::request_authentication(
    const th25_ctrl::OperatorId& id,
    const th25_ctrl::Credential& cred) noexcept
    -> th25_ctrl::Result<void, th25_ctrl::ErrorCode> {
    // 空殻実装: 引数を参照せず常時 ErrorCode::AuthRequired を返却.
    // Inc.4 で実 UI 実装 (ID/パスワード入力ダイアログ + SecureString 化 +
    // Core 側 UNIT-210 への IF-U-010 AuthenticationVerify 送信 +
    // AuthenticationResult 待ち + 認証成功時の操作者セッション確立 +
    // AuditLogger 転送) に発展.
    //
    // memory ordering / synchronization:
    //   - attempt_count_.fetch_add(release): attempt_count() reader との
    //     happens-before 関係確立 + read-modify-write のアトミック性.
    //
    // 引数 id / cred は本 v0.1 範囲では未使用 (常時拒否のため). cast to void
    // でコンパイラの未使用引数警告を抑制 (Inc.4 で実認証ロジック実装時に削除予定).
    static_cast<void>(id);
    static_cast<void>(cred);

    attempt_count_.fetch_add(1U, std::memory_order_release);

    // 本 v0.1 範囲は UI 層 + Core 連携が未実装のため、認証要求を常時拒否する.
    // ErrorCode::AuthRequired は SDD §6.1 ErrorCode 階層の Auth 系 (0x07)
    // に属し SDD §6.2 で Severity::Medium にマップされている. UNIT-210
    // CoreAuthenticationGateway (Step 32 / CR-0020 で空殻実装済) が常時
    // AuthRequired を返す空殻と一対の整合を確立する意味で本 ErrorCode を採用
    // (Inc.4 で本格認証ロジック実装時には「未認証時 = AuthRequired を返す」
    //  という Inc.4 後の本来挙動と空殻挙動が同一 ErrorCode で表現可能、
    //  後方互換性).
    return th25_ctrl::Result<void, th25_ctrl::ErrorCode>::error(
        th25_ctrl::ErrorCode::AuthRequired);
}

// ============================================================================
// 補助 API (UT / Inc.4 拡張点).
// ============================================================================

auto UIAuthenticationGateway::attempt_count() const noexcept -> std::uint64_t {
    return attempt_count_.load(std::memory_order_acquire);
}

auto UIAuthenticationGateway::reset_for_test() noexcept -> void {
    attempt_count_.store(0U, std::memory_order_release);
}

}  // namespace th25_ui
