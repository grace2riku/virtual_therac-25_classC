// SPDX-License-Identifier: TBD
// TH25-CTRL: UNIT-204 DoseManager implementation.
//
// SDD-TH25-001 v0.1.1 §4.5 と完全一致する HZ-005 直接 + HZ-003 構造的排除の中核ユニット.

#include "th25_ctrl/dose_manager.hpp"

#include <cmath>
#include <cstdint>
#include <limits>

namespace th25_ctrl {

namespace {

// target_dose / rate を整数 pulses に変換するヘルパ.
// 浮動小数演算結果が uint64_t に安全に収まる範囲かをチェック.
//   - 負値・非有限・uint64_t::max() より大きい場合は false を返す.
//   - 有効な場合は out_pulses にセットして true.
[[nodiscard]] auto compute_target_pulses(DoseUnit_cGy target,
                                         DoseRatePerPulse_cGy_per_pulse rate,
                                         std::uint64_t& out_pulses) noexcept -> bool {
    const double rate_value = rate.value();
    if (!(rate_value > 0.0) || !std::isfinite(rate_value)) {
        return false;
    }
    const double target_value = target.value();
    if (!(target_value >= 0.0) || !std::isfinite(target_value)) {
        return false;
    }
    const double pulses_dbl = target_value / rate_value;
    if (!std::isfinite(pulses_dbl) || pulses_dbl < 0.0) {
        return false;
    }
    // uint64_t::max() を超える値は保持できない (HZ-003 構造的予防の前提).
    constexpr double kMaxRepresentable =
        static_cast<double>(std::numeric_limits<std::uint64_t>::max());
    if (pulses_dbl > kMaxRepresentable) {
        return false;
    }
    out_pulses = static_cast<std::uint64_t>(pulses_dbl);
    return true;
}

}  // namespace

DoseManager::DoseManager(DoseRatePerPulse_cGy_per_pulse rate) noexcept : rate_(rate) {}

auto DoseManager::set_dose_target(DoseUnit_cGy target,
                                  LifecycleState lifecycle_state) noexcept
    -> Result<void, ErrorCode> {
    // SDD §4.5 事前条件: lifecycle_state ∈ {PrescriptionSet, Ready}.
    // UNIT-201 init_subsystems と同パターンで InternalUnexpectedState 返却.
    if (lifecycle_state != LifecycleState::PrescriptionSet
        && lifecycle_state != LifecycleState::Ready) {
        return Result<void, ErrorCode>::error(ErrorCode::InternalUnexpectedState);
    }

    // SDD §4.5 / SRS-008 範囲検証 [0.01, 10000.0] cGy.
    if (!is_dose_target_in_range(target)) {
        return Result<void, ErrorCode>::error(ErrorCode::DoseOutOfRange);
    }

    // target / rate_ を整数 pulses に変換. オーバフロー / 不正値時は DoseOverflow.
    std::uint64_t target_pulses_value = 0;
    if (!compute_target_pulses(target, rate_, target_pulses_value)) {
        return Result<void, ErrorCode>::error(ErrorCode::DoseOverflow);
    }

    // 内部状態を整合的に更新する.
    // 順序: (1) 累積リセット → (2) target_pulses_ / target_reached_ をクリア → (3) target_set_ true.
    // target_set_ を最後に設定することで、on_dose_pulse が部分初期化状態を観測しないことを保証.
    accumulated_pulses_.reset();
    target_reached_.store(false, std::memory_order_release);
    target_pulses_.store(target_pulses_value, std::memory_order_release);
    target_set_.store(true, std::memory_order_release);

    return Result<void, ErrorCode>::ok();
}

auto DoseManager::on_dose_pulse(PulseCount pulse_delta) noexcept -> void {
    // SDD §4.5 「1 kHz で IonChamberSim から呼出」.
    // 全パスは noexcept、共有変数は atomic 経由のみ (SAD §9 SEP-003 / RCM-002 / RCM-019).
    //
    // 累積を進める (PulseCounter 内 std::atomic<uint64_t>::fetch_add(acq_rel)).
    // 戻り値は加算前の累積 (atomic::fetch_add 仕様).
    const PulseCount before = accumulated_pulses_.fetch_add(pulse_delta);
    const std::uint64_t after = before.value() + pulse_delta.value();

    // target 未設定時は到達検知を実施しない (未設定保護).
    if (!target_set_.load(std::memory_order_acquire)) {
        return;
    }

    const std::uint64_t target = target_pulses_.load(std::memory_order_acquire);
    if (after >= target) {
        // SDD §4.5 「目標到達時 IF-U-002 経由で BeamOff 要求送信」.
        // Step 22 範囲では target_reached_ フラグまで実装. UNIT-201/203 への結線は
        // Step 23+ で SafetyCoreOrchestrator dispatch 機構整備時に完成.
        target_reached_.store(true, std::memory_order_release);
    }
}

auto DoseManager::current_accumulated() const noexcept -> DoseUnit_cGy {
    return pulse_count_to_dose(accumulated_pulses_.load());
}

auto DoseManager::current_target() const noexcept -> DoseUnit_cGy {
    if (!target_set_.load(std::memory_order_acquire)) {
        return DoseUnit_cGy{0.0};
    }
    const std::uint64_t pulses = target_pulses_.load(std::memory_order_acquire);
    // pulses → target_dose は逆算. target_pulses_ は target / rate を切り捨てた整数値であり、
    // 元の target_dose に対し最大 1 パルス分の精度誤差を含むことに注意 (UT-204-XX で許容範囲を検証).
    return DoseUnit_cGy{static_cast<double>(pulses) * rate_.value()};
}

auto DoseManager::is_target_reached() const noexcept -> bool {
    return target_reached_.load(std::memory_order_acquire);
}

auto DoseManager::pulse_count_to_dose(PulseCount count) const noexcept -> DoseUnit_cGy {
    // 単位変換: count [pulses] × rate [cGy/pulse] = dose [cGy].
    return DoseUnit_cGy{static_cast<double>(count.value()) * rate_.value()};
}

auto DoseManager::reset() noexcept -> void {
    // 順序: target_set_ を先に false にすることで、on_dose_pulse が中途半端な状態を
    // 観測しないことを保証. その後、累積 / 到達 / target_pulses_ を初期化.
    target_set_.store(false, std::memory_order_release);
    target_reached_.store(false, std::memory_order_release);
    target_pulses_.store(std::numeric_limits<std::uint64_t>::max(), std::memory_order_release);
    accumulated_pulses_.reset();
}

auto DoseManager::target_pulses_for_test() const noexcept -> std::uint64_t {
    return target_pulses_.load(std::memory_order_acquire);
}

}  // namespace th25_ctrl
