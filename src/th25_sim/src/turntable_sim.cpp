// SPDX-License-Identifier: TBD
// TH25-SIM: UNIT-303 TurntableSim implementation.
//
// SDD-TH25-001 v0.1.1 §4.12 と完全一致する Hardware Simulator 層の 3 ユニット目
// (UNIT-301/302 と同パターン + 故障注入機能 + 3 系統センサ).

#include "th25_sim/turntable_sim.hpp"

namespace th25_sim {

namespace {

// 簡易 LCG (Numerical Recipes 推奨係数). 周期 2^32.
// rng_state' = rng_state * 1664525 + 1013904223
// UNIT-301/302 と同パターンで Sim 層内に閉じて再宣言する
// (Sim ユニット間の独立性を保つため).
constexpr std::uint32_t kLcgMultiplier{1664525U};
constexpr std::uint32_t kLcgIncrement{1013904223U};

// uint32_t 値を [-1.0, +1.0] の double に正規化.
[[nodiscard]] auto normalize_to_unit_range(std::uint32_t state) noexcept -> double {
    // 上位 24 bit を [0, 1] にマップして 2 * x - 1 で [-1, +1] に変換.
    // 24 bit にすることで double 仮数部 (52 bit) を超えない.
    constexpr double kInv24bit{1.0 / static_cast<double>(1U << 24U)};
    const double normalized = static_cast<double>(state >> 8U) * kInv24bit;  // [0, 1)
    return (normalized * 2.0) - 1.0;                                         // [-1, +1)
}

}  // namespace

// ============================================================================
// TurntableSim 公開 API.
// ============================================================================

auto TurntableSim::command_position(th25_ctrl::Position_mm target) noexcept -> void {
    // SRS-D-007 範囲外は物理的飽和を模擬 (実物理デバイス挙動).
    const auto clamped = clamp_to_range(target);

    // Delay 故障モード参照用に、直前の commanded_position を previous に退避.
    // (load → store 順、acq_rel 不要だが意図を明示するため acquire / release).
    const double prev = commanded_position_mm_.load(std::memory_order_acquire);
    previous_commanded_position_mm_.store(prev, std::memory_order_release);

    // 50 mm/s 線形遷移は Step 28 範囲では構造のみ (実時間 sleep なし).
    commanded_position_mm_.store(clamped.value(), std::memory_order_release);
}

auto TurntableSim::read_sensor(SensorId sensor) const noexcept
    -> th25_ctrl::Position_mm {
    const std::size_t idx = sensor_index(sensor);
    const FaultMode mode = fault_mode_[idx].load(std::memory_order_acquire);

    // ----- 故障モード分岐 -----
    switch (mode) {
        case FaultMode::StuckAt: {
            // 凍結値固定返却 (IT/ST 用、Therac-25 風「センサ故着」模擬).
            const double v = stuck_at_value_mm_[idx].load(std::memory_order_acquire);
            return th25_ctrl::Position_mm{v};
        }
        case FaultMode::Delay: {
            // 前回 commanded_position 返却 (Step 28 範囲では実時間遅延なし、構造のみ).
            const double v =
                previous_commanded_position_mm_.load(std::memory_order_acquire);
            return th25_ctrl::Position_mm{v};
        }
        case FaultMode::NoResponse: {
            // 応答なし固定 (0.0 mm).
            return th25_ctrl::Position_mm{0.0};
        }
        case FaultMode::None:
        default:
            break;
    }

    // ----- FaultMode::None: 通常応答 -----
    const double commanded =
        commanded_position_mm_.load(std::memory_order_acquire);

    if (!sensor_noise_enabled_.load(std::memory_order_acquire)) {
        // ノイズ無効: commanded 値そのまま返却 (UT 安定化).
        return th25_ctrl::Position_mm{commanded};
    }

    // ノイズ有効: 簡易 LCG で疑似乱数を生成し ±0.1 mm の絶対値ノイズを乗せる.
    // fetch_add で rng_state_ を次状態に進める (acq_rel で thread-safe).
    // 3 系統センサ間で rng_state_ を共有するが、各 read_sensor 呼出ごとに
    // 別の値が生成されるため、3 系統独立性は保たれる (むしろ独立 RNG より強い).
    const std::uint32_t prev_state =
        rng_state_.fetch_add(kLcgIncrement, std::memory_order_acq_rel);
    const std::uint32_t next_state = prev_state * kLcgMultiplier + kLcgIncrement;
    const double normalized = normalize_to_unit_range(next_state);  // [-1, +1)
    const double noise_delta = kTurntableSensorNoiseAbsMm * normalized;
    const double noisy = commanded + noise_delta;

    // SRS-I-005 準拠: センサ値も -100.0〜+100.0 mm に飽和 (clamp).
    return clamp_to_range(th25_ctrl::Position_mm{noisy});
}

auto TurntableSim::inject_fault(SensorId sensor, FaultMode mode) noexcept -> void {
    const std::size_t idx = sensor_index(sensor);
    fault_mode_[idx].store(mode, std::memory_order_release);
}

// ============================================================================
// 補助 API.
// ============================================================================

auto TurntableSim::inject_stuck_at_value(SensorId sensor,
                                         th25_ctrl::Position_mm value) noexcept
    -> void {
    const std::size_t idx = sensor_index(sensor);
    stuck_at_value_mm_[idx].store(value.value(), std::memory_order_release);
}

auto TurntableSim::enable_sensor_noise(std::uint32_t seed) noexcept -> void {
    rng_state_.store(seed, std::memory_order_release);
    sensor_noise_enabled_.store(true, std::memory_order_release);
}

auto TurntableSim::disable_sensor_noise() noexcept -> void {
    sensor_noise_enabled_.store(false, std::memory_order_release);
}

auto TurntableSim::is_sensor_noise_enabled() const noexcept -> bool {
    return sensor_noise_enabled_.load(std::memory_order_acquire);
}

auto TurntableSim::current_commanded() const noexcept -> th25_ctrl::Position_mm {
    return th25_ctrl::Position_mm{
        commanded_position_mm_.load(std::memory_order_acquire)};
}

auto TurntableSim::current_fault_mode(SensorId sensor) const noexcept -> FaultMode {
    const std::size_t idx = sensor_index(sensor);
    return fault_mode_[idx].load(std::memory_order_acquire);
}

}  // namespace th25_sim
