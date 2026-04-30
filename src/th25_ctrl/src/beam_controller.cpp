// SPDX-License-Identifier: TBD
// TH25-CTRL: UNIT-203 BeamController implementation.
//
// SDD-TH25-001 v0.1.1 §4.4 と完全一致する RCM-017 中核ユニット.

#include "th25_ctrl/beam_controller.hpp"

namespace th25_ctrl {

auto BeamController::current_state() const noexcept -> BeamState {
    return state_.load(std::memory_order_acquire);
}

auto BeamController::set_beam_on_permission(bool permitted) noexcept -> void {
    beam_on_permitted_.store(permitted, std::memory_order_release);
}

auto BeamController::is_beam_on_permitted() const noexcept -> bool {
    return beam_on_permitted_.load(std::memory_order_acquire);
}

auto BeamController::request_beam_on(LifecycleState lifecycle_state) noexcept
    -> Result<void, ErrorCode> {
    // SDD §4.4 事前条件: lifecycle_state == Ready.
    if (lifecycle_state != LifecycleState::Ready) {
        return Result<void, ErrorCode>::error(ErrorCode::BeamOnNotPermitted);
    }

    // SDD §4.4 事前条件: BeamState == Off (再エントラント防止).
    // 既に Arming/On/Stopping の場合は不正状態.
    {
        BeamState expected_state = BeamState::Off;
        if (!state_.compare_exchange_strong(
                expected_state, BeamState::Arming,
                std::memory_order_acq_rel, std::memory_order_acquire)) {
            return Result<void, ErrorCode>::error(ErrorCode::BeamOnNotPermitted);
        }
    }

    // SDD §4.4 事前条件: ビームオン許可フラグ true.
    // **compare_exchange_strong で 1 回限り消費**. これにより:
    //   1. SRS-002 シーケンス未完了状態でのビームオン要求は構造的に拒否
    //   2. 1 回のビームオン要求で 1 回しか照射されない (二重照射不可)
    //   3. race condition は発生しない (atomic + acq_rel ordering)
    // RCM-017 の中核機構、Therac-25 Tyler 事故型「許可確認の race condition」への
    // 構造的対応.
    {
        bool expected_permission = true;
        if (!beam_on_permitted_.compare_exchange_strong(
                expected_permission, false,
                std::memory_order_acq_rel, std::memory_order_acquire)) {
            // 許可フラグが false だった: state_ を Arming → Off に巻き戻し、
            // 拒否 ErrorCode を返す.
            state_.store(BeamState::Off, std::memory_order_release);
            return Result<void, ErrorCode>::error(ErrorCode::BeamOnNotPermitted);
        }
    }

    // 全条件 OK: Arming → On に進める.
    // SDD §4.4「成功時 BeamState = Arming → On、InterProcessChannel 経由で
    // ElectronGunCurrent_mA を電子銃に設定」については、UNIT-301/402 が未実装の
    // ため Step 21 では state 遷移のみ. Step 22+ で結線.
    state_.store(BeamState::On, std::memory_order_release);
    return Result<void, ErrorCode>::ok();
}

auto BeamController::request_beam_off() noexcept -> Result<void, ErrorCode> {
    // SDD §4.4 「事前条件: 任意」「事後条件: BeamState = On → Stopping → Off」.
    // 任意状態から呼出可能. 既に Off の場合は no-op で Off のまま.
    //
    // SDD §4.4 「< 10 ms で電子銃電流 = 0 mA」(SRS-101) は UNIT-301/402 結線後の
    // IT-101 で実測する. Step 21 では state 遷移のみを実装.
    //
    // 状態遷移:
    //   Off       → Off       (no-op)
    //   Arming    → Off       (Stopping を経由せずに即時遷移、フェイルセーフ)
    //   On        → Stopping → Off
    //   Stopping  → Off       (継続中の遷移を完了)
    const BeamState current = state_.load(std::memory_order_acquire);
    switch (current) {
        case BeamState::On:
            // On → Stopping → Off の 2 段階遷移.
            // 中間状態 Stopping を可視化 (SDD §4.1 BeamState 4 状態).
            state_.store(BeamState::Stopping, std::memory_order_release);
            state_.store(BeamState::Off, std::memory_order_release);
            break;
        case BeamState::Arming:
        case BeamState::Stopping:
        case BeamState::Off:
        default:
            // フェイルセーフ: いかなる状態からも Off に遷移可能.
            state_.store(BeamState::Off, std::memory_order_release);
            break;
    }

    // ビームオフ後は許可フラグも安全側に倒す (次回 beam_on は permission 再設定が必要).
    // SDD §4.4「許可フラグは UNIT-202 が SRS-002 シーケンス完了時のみ true に設定」と整合.
    beam_on_permitted_.store(false, std::memory_order_release);

    return Result<void, ErrorCode>::ok();
}

}  // namespace th25_ctrl
