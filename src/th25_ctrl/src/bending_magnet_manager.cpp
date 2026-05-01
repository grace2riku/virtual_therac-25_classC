// SPDX-License-Identifier: TBD
// TH25-CTRL: UNIT-206 BendingMagnetManager implementation.
//
// SDD-TH25-001 v0.1.1 §4.7 と完全一致する HZ-005 部分対応 + RCM-008 中核ユニット.

#include "th25_ctrl/bending_magnet_manager.hpp"

#include "th25_ctrl/treatment_mode_manager.hpp"  // is_electron_energy_in_range / is_xray_energy_in_range

namespace th25_ctrl {

// ============================================================================
// BendingMagnetManager 公開 API.
// ============================================================================

auto BendingMagnetManager::set_current_for_energy(
    TreatmentMode mode, Energy_MeV electron_energy) noexcept -> Result<void, ErrorCode> {
    // SDD §4.7 / SRS-002 整合: 電子線エネルギー指定なら mode は Electron でなければならない.
    if (mode != TreatmentMode::Electron) {
        return Result<void, ErrorCode>::error(ErrorCode::ModeInvalidTransition);
    }
    // SRS-004 範囲: 1.0 ≤ MeV ≤ 25.0 (UNIT-202 で確立済の判定を再利用).
    if (!TreatmentModeManager::is_electron_energy_in_range(electron_energy)) {
        return Result<void, ErrorCode>::error(ErrorCode::ModeInvalidTransition);
    }

    // 対応表 + 線形補間で目標電流を計算.
    const auto target = EnergyMagnetMap::compute_target_current_electron(electron_energy);

    // SRS-D-006 範囲外時は安全側拒否 (Magnet 系 0x05).
    // 線形補間係数のプレースホルダ値 (slope=2.5 A/MeV) は 25 MeV で 62.5 A となり
    // 500 A 上限以下だが、SDP §6.1 確定後の校正データ次第では上限到達可能性あり.
    if (!is_current_in_range(target)) {
        return Result<void, ErrorCode>::error(ErrorCode::MagnetCurrentDeviation);
    }

    // Step 24 範囲: target_current_ に release store + target_set_ を立てる.
    // 実 IPC 経由 BendingMagnetSim 指令送信 + ±5% 到達待機 (タイムアウト 200 ms) は
    // Step 25+ で UNIT-302/402 結線時に完成.
    target_current_.store(target.value(), std::memory_order_release);
    target_set_.store(true, std::memory_order_release);
    return Result<void, ErrorCode>::ok();
}

auto BendingMagnetManager::set_current_for_energy(
    TreatmentMode mode, Energy_MV xray_energy) noexcept -> Result<void, ErrorCode> {
    if (mode != TreatmentMode::XRay) {
        return Result<void, ErrorCode>::error(ErrorCode::ModeInvalidTransition);
    }
    if (!TreatmentModeManager::is_xray_energy_in_range(xray_energy)) {
        return Result<void, ErrorCode>::error(ErrorCode::ModeInvalidTransition);
    }

    const auto target = EnergyMagnetMap::compute_target_current_xray(xray_energy);

    if (!is_current_in_range(target)) {
        return Result<void, ErrorCode>::error(ErrorCode::MagnetCurrentDeviation);
    }

    target_current_.store(target.value(), std::memory_order_release);
    target_set_.store(true, std::memory_order_release);
    return Result<void, ErrorCode>::ok();
}

auto BendingMagnetManager::current_actual() const noexcept -> MagnetCurrent_A {
    return MagnetCurrent_A{actual_current_.load(std::memory_order_acquire)};
}

auto BendingMagnetManager::is_within_tolerance() const noexcept -> bool {
    // 事前条件: target が一度でも設定済であること.
    // 未設定時は in-tolerance と認めない (安全側、SDD §4.7 / SRS-ALM-004).
    if (!target_set_.load(std::memory_order_acquire)) {
        return false;
    }
    const MagnetCurrent_A target{target_current_.load(std::memory_order_acquire)};
    const MagnetCurrent_A actual{actual_current_.load(std::memory_order_acquire)};
    return is_current_within_tolerance(target, actual);
}

auto BendingMagnetManager::inject_actual_current(MagnetCurrent_A actual) noexcept -> void {
    actual_current_.store(actual.value(), std::memory_order_release);
}

auto BendingMagnetManager::current_target() const noexcept -> MagnetCurrent_A {
    return MagnetCurrent_A{target_current_.load(std::memory_order_acquire)};
}

auto BendingMagnetManager::is_target_set() const noexcept -> bool {
    return target_set_.load(std::memory_order_acquire);
}

}  // namespace th25_ctrl
