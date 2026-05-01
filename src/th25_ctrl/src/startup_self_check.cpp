// SPDX-License-Identifier: TBD
// TH25-CTRL: UNIT-208 StartupSelfCheck implementation.
//
// SDD-TH25-001 v0.1.1 §4.9 と完全一致する RCM-013 中核 + HZ-009 部分対応ユニット.

#include "th25_ctrl/startup_self_check.hpp"

namespace th25_ctrl {

// ============================================================================
// StartupSelfCheck コンストラクタ.
// ============================================================================

StartupSelfCheck::StartupSelfCheck(
    TurntableManager& turntable,
    BendingMagnetManager& bending_magnet,
    DoseManager& dose_manager) noexcept
    : turntable_(turntable),
      bending_magnet_(bending_magnet),
      dose_manager_(dose_manager) {}

// ============================================================================
// 公開 API.
// ============================================================================

auto StartupSelfCheck::perform_self_check() const noexcept -> Result<void, ErrorCode> {
    // 1. 電子銃電流確認 (SRS-012 / SDD §4.9 項目 1).
    //    Step 25 範囲: inject_electron_gun_current で設定された値を atomic 経由で取得.
    //    将来 UNIT-301 結線時: ElectronGunSim::read_actual_current() に置換.
    const ElectronGunCurrent_mA electron_gun{
        electron_gun_current_ma_.load(std::memory_order_acquire)};
    if (!is_electron_gun_in_zero(electron_gun)) {
        return Result<void, ErrorCode>::error(ErrorCode::ElectronGunCurrentOutOfRange);
    }

    // 2. ターンテーブル位置確認 (SRS-012 / SDD §4.9 項目 2).
    //    UNIT-205::is_in_position は内部で「3 系統センサ acquire load -> has_discrepancy
    //    判定 (1.0 mm 偏差) -> median と expected の差 ≤ 0.5 mm」を実施するため、SRS-012
    //    の「±0.5 mm 以内」+「3 系統偏差 < 1.0 mm」の両判定を同時に行える.
    if (!turntable_.is_in_position(kLightPositionMm)) {
        return Result<void, ErrorCode>::error(ErrorCode::TurntableOutOfPosition);
    }

    // 3. ベンディング磁石電流確認 (SRS-012 / SDD §4.9 項目 3).
    //    UNIT-206::current_actual で計測値取得 (release-acquire ordering).
    if (!is_bending_magnet_in_zero(bending_magnet_.current_actual())) {
        return Result<void, ErrorCode>::error(ErrorCode::MagnetCurrentDeviation);
    }

    // 4. ドーズ積算カウンタ初期化確認 (SRS-012 / SDD §4.9 項目 4).
    //    UNIT-204::current_accumulated は PulseCounter::load() * rate_ で計算され、
    //    accumulated_pulses_ = 0 なら 0.0 cGy が正確に返る (浮動小数表現で 0 は exact).
    if (!is_dose_zero(dose_manager_.current_accumulated())) {
        return Result<void, ErrorCode>::error(ErrorCode::InternalUnexpectedState);
    }

    // 4 項目全 Pass.
    return Result<void, ErrorCode>::ok();
}

// ============================================================================
// 補助 API (UT / シミュレーション用).
// ============================================================================

auto StartupSelfCheck::inject_electron_gun_current(ElectronGunCurrent_mA actual) noexcept
    -> void {
    electron_gun_current_ma_.store(actual.value(), std::memory_order_release);
}

auto StartupSelfCheck::current_electron_gun_current() const noexcept -> ElectronGunCurrent_mA {
    return ElectronGunCurrent_mA{electron_gun_current_ma_.load(std::memory_order_acquire)};
}

}  // namespace th25_ctrl
