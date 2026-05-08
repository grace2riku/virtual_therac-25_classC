// SPDX-License-Identifier: TBD
// TH25-CTRL: UNIT-210 CoreAuthenticationGateway implementation
//            (Inc.4 で本格化、本 v0.1 では空殻).
//
// SDD-TH25-001 v0.1.1 §4.11 と整合する操作者認証ゲートウェイユニット.
// 本 v0.1 範囲では IF 骨格 + 常時拒否プレースホルダ (verify() が常に
// ErrorCode::AuthRequired を返却) + verify_count_ 集計のみ.
// Inc.4 で実認証ロジック + MessageBus 経由非同期化 + AuthenticationResult
// dispatch + AuditLogger 転送を追加予定.

#include "th25_ctrl/core_authentication_gateway.hpp"

namespace th25_ctrl {

// ============================================================================
// SDD §4.11 公開 API.
// ============================================================================

auto CoreAuthenticationGateway::verify(const OperatorId& id,
                                       const Credential& cred) noexcept
    -> Result<void, ErrorCode> {
    // 空殻実装: 引数を参照せず常時 ErrorCode::AuthRequired を返却.
    // Inc.4 で実認証ロジック (ID/パスワード照合 + 期限管理 + AuditLogger 転送)
    // に発展.
    //
    // memory ordering / synchronization:
    //   - verify_count_.fetch_add(release): verify_count() reader との
    //     happens-before 関係確立 + read-modify-write のアトミック性.
    //
    // 引数 id / cred は本 v0.1 範囲では未使用 (常時拒否のため). cast to void で
    // コンパイラの未使用引数警告を抑制 (Inc.4 で実認証ロジック実装時に削除予定).
    static_cast<void>(id);
    static_cast<void>(cred);

    verify_count_.fetch_add(1U, std::memory_order_release);

    return Result<void, ErrorCode>::error(ErrorCode::AuthRequired);
}

// ============================================================================
// 補助 API (UT / Inc.4 拡張点).
// ============================================================================

auto CoreAuthenticationGateway::verify_count() const noexcept -> std::uint64_t {
    return verify_count_.load(std::memory_order_acquire);
}

auto CoreAuthenticationGateway::reset_for_test() noexcept -> void {
    verify_count_.store(0U, std::memory_order_release);
}

}  // namespace th25_ctrl
