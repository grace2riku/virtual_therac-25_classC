// SPDX-License-Identifier: TBD
// TH25-CTRL: UNIT-202 TreatmentModeManager implementation.
//
// SDD-TH25-001 v0.1.1 §4.3 と完全一致する HZ-001 直接対応の中核ユニット.

#include "th25_ctrl/treatment_mode_manager.hpp"

namespace th25_ctrl {

namespace {

// SDD §4.3 表: モード遷移許可表 (SRS-002).
// Light/Electron/XRay の任意の遷移を許可 (同モード = no-op、SDD §4.3 注記).
// 本テーブルは将来 SDD で禁止遷移が追加された場合に拡張する想定. 現時点では
// 全 3 × 3 = 9 通りすべて許可.
[[nodiscard]] auto compute_mode_transition_allowed(
    TreatmentMode from, TreatmentMode to) noexcept -> bool {
    // 列挙値の妥当性確認のみ (SDD §4.1 で TreatmentMode は 3 値固定).
    // 不明な値 (キャスト経由の不正値) は拒否する防御的チェック.
    auto is_known = [](TreatmentMode m) noexcept -> bool {
        return m == TreatmentMode::Electron || m == TreatmentMode::XRay ||
               m == TreatmentMode::Light;
    };
    if (!is_known(from) || !is_known(to)) {
        return false;
    }
    return true;
}

}  // namespace

TreatmentModeManager::TreatmentModeManager(TreatmentMode initial_mode) noexcept
    : current_mode_(initial_mode) {}

auto TreatmentModeManager::current_mode() const noexcept -> TreatmentMode {
    return current_mode_.load(std::memory_order_acquire);
}

auto TreatmentModeManager::is_mode_transition_allowed(
    TreatmentMode from, TreatmentMode to) noexcept -> bool {
    return compute_mode_transition_allowed(from, to);
}

auto TreatmentModeManager::is_electron_energy_in_range(
    Energy_MeV energy) noexcept -> bool {
    const double v = energy.value();
    return (v >= kElectronEnergyMinMeV) && (v <= kElectronEnergyMaxMeV);
}

auto TreatmentModeManager::is_xray_energy_in_range(
    Energy_MV energy) noexcept -> bool {
    const double v = energy.value();
    return (v >= kXrayEnergyMinMV) && (v <= kXrayEnergyMaxMV);
}

auto TreatmentModeManager::request_mode_change(
    TreatmentMode new_mode,
    Energy_MeV electron_energy,
    BeamState current_beam_state) noexcept -> Result<void, ErrorCode> {
    // SDD §4.3 事前条件: BeamState == Off (SRS-002 step 1, RCM-001 中核).
    // Therac-25 Tyler 事故では BeamState 確認漏れが事故の一因だった.
    if (current_beam_state != BeamState::Off) {
        return Result<void, ErrorCode>::error(ErrorCode::ModeBeamOnNotAllowed);
    }

    // 本 API は Electron モード専用オーバーロード.
    if (new_mode != TreatmentMode::Electron) {
        return Result<void, ErrorCode>::error(ErrorCode::ModeInvalidTransition);
    }

    // SRS-004 エネルギー範囲チェック (RCM-008 強い型 + 範囲検証).
    // SDD §4.3「エネルギー範囲外時 Mode 系該当 ErrorCode」を ModeInvalidTransition
    // で実現 (SDD §4.1.1 の Mode 系には専用コードがないため、Mode 系の汎用
    // 不正値として ModeInvalidTransition を流用. 専用コード ModeEnergyOutOfRange
    // 等の追加は将来 SDD 改訂検討事項).
    if (!is_electron_energy_in_range(electron_energy)) {
        return Result<void, ErrorCode>::error(ErrorCode::ModeInvalidTransition);
    }

    // SDD §4.3 表 遷移許可チェック (列挙値の妥当性も含む).
    const TreatmentMode from = current_mode_.load(std::memory_order_acquire);
    if (!is_mode_transition_allowed(from, new_mode)) {
        return Result<void, ErrorCode>::error(ErrorCode::ModeInvalidTransition);
    }

    // 状態更新.
    current_mode_.store(new_mode, std::memory_order_release);

    // SDD §4.3 事後条件「TurntableManager::move_to + BendingMagnetManager::
    // set_current がスケジュール済」については、UNIT-205/206 が未実装のため
    // 本 Step では構造のみを成立 (Step 21+ で結線). 本 Step では
    // electron_energy をペンディング状態として保持しないが、Step 21+ で
    // 校正データ (EnergyMagnetMap) に基づく磁石電流 + ターンテーブル位置を
    // ペンディング操作リストに記録する設計を追加予定.
    static_cast<void>(electron_energy);

    return Result<void, ErrorCode>::ok();
}

auto TreatmentModeManager::request_mode_change(
    TreatmentMode new_mode,
    Energy_MV xray_energy,
    BeamState current_beam_state) noexcept -> Result<void, ErrorCode> {
    // SDD §4.3 事前条件 (SRS-002 step 1).
    if (current_beam_state != BeamState::Off) {
        return Result<void, ErrorCode>::error(ErrorCode::ModeBeamOnNotAllowed);
    }

    // 本 API は XRay モード専用オーバーロード.
    if (new_mode != TreatmentMode::XRay) {
        return Result<void, ErrorCode>::error(ErrorCode::ModeInvalidTransition);
    }

    // SRS-004 エネルギー範囲チェック.
    if (!is_xray_energy_in_range(xray_energy)) {
        return Result<void, ErrorCode>::error(ErrorCode::ModeInvalidTransition);
    }

    // SDD §4.3 表 遷移許可チェック.
    const TreatmentMode from = current_mode_.load(std::memory_order_acquire);
    if (!is_mode_transition_allowed(from, new_mode)) {
        return Result<void, ErrorCode>::error(ErrorCode::ModeInvalidTransition);
    }

    current_mode_.store(new_mode, std::memory_order_release);
    static_cast<void>(xray_energy);

    return Result<void, ErrorCode>::ok();
}

auto TreatmentModeManager::request_light_mode(
    BeamState current_beam_state) noexcept -> Result<void, ErrorCode> {
    // SDD §4.3 事前条件 (SRS-002 step 1).
    // Light モードは「治療終了時、ビームオフ確認後にターンテーブル位置のみ
    // 移動」と SDD §4.3 表で定義されているため、BeamState == Off を要求する.
    if (current_beam_state != BeamState::Off) {
        return Result<void, ErrorCode>::error(ErrorCode::ModeBeamOnNotAllowed);
    }

    // SDD §4.3 表 遷移許可チェック (Light への遷移は常に許可).
    const TreatmentMode from = current_mode_.load(std::memory_order_acquire);
    if (!is_mode_transition_allowed(from, TreatmentMode::Light)) {
        return Result<void, ErrorCode>::error(ErrorCode::ModeInvalidTransition);
    }

    current_mode_.store(TreatmentMode::Light, std::memory_order_release);
    return Result<void, ErrorCode>::ok();
}

auto TreatmentModeManager::verify_mode_consistency(
    TreatmentMode requested_mode) const noexcept -> Result<void, ErrorCode> {
    // SDD §4.3「センサ 3 系統と要求モードの不整合時 ModePositionMismatch」.
    // Step 20 範囲: 内部状態の整合性チェックのみ. Step 21+ で TurntableManager
    // 結線時に 3 系統センサ依存判定を追加し、本関数で「センサ平均位置と
    // requested_mode の対応位置 (TurntablePositions) の偏差判定」を行う設計に
    // 拡張する.
    const TreatmentMode current = current_mode_.load(std::memory_order_acquire);
    if (current != requested_mode) {
        return Result<void, ErrorCode>::error(ErrorCode::ModePositionMismatch);
    }
    return Result<void, ErrorCode>::ok();
}

}  // namespace th25_ctrl
