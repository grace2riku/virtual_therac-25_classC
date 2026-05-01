// SPDX-License-Identifier: TBD
// TH25-SIM: UNIT-301 ElectronGunSim implementation.
//
// SDD-TH25-001 v0.1.1 §4.12 と完全一致する Hardware Simulator 層の最初の具体実装.

#include "th25_sim/electron_gun_sim.hpp"

namespace th25_sim {

namespace {

// 簡易 LCG (Numerical Recipes 推奨係数). 周期 2^32.
// rng_state' = rng_state * 1664525 + 1013904223
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
// ElectronGunSim 公開 API.
// ============================================================================

auto ElectronGunSim::set_current(th25_ctrl::ElectronGunCurrent_mA commanded) noexcept
    -> void {
    // SRS-D-008 範囲外は物理的飽和を模擬 (実物理デバイス挙動).
    const auto clamped = clamp_to_range(commanded);
    commanded_current_ma_.store(clamped.value(), std::memory_order_release);
}

auto ElectronGunSim::read_actual_current() const noexcept
    -> th25_ctrl::ElectronGunCurrent_mA {
    const double commanded = commanded_current_ma_.load(std::memory_order_acquire);

    if (!noise_enabled_.load(std::memory_order_acquire)) {
        // ノイズ無効: commanded 値そのまま返却 (UT 安定化).
        return th25_ctrl::ElectronGunCurrent_mA{commanded};
    }

    // ノイズ有効: 簡易 LCG で疑似乱数を生成し ±1% に正規化.
    // fetch_add で rng_state_ を次状態に進める (release_acquire で thread-safe).
    const std::uint32_t prev_state =
        rng_state_.fetch_add(kLcgIncrement, std::memory_order_acq_rel);
    const std::uint32_t next_state = prev_state * kLcgMultiplier + kLcgIncrement;
    const double normalized = normalize_to_unit_range(next_state);  // [-1, +1)
    const double noise_delta = commanded * kElectronGunNoiseFraction * normalized;
    const double noisy = commanded + noise_delta;

    // SRS-I-008 準拠: 計測値も 0.0〜10.0 mA に飽和.
    return clamp_to_range(th25_ctrl::ElectronGunCurrent_mA{noisy});
}

// ============================================================================
// 補助 API.
// ============================================================================

auto ElectronGunSim::enable_noise(std::uint32_t seed) noexcept -> void {
    rng_state_.store(seed, std::memory_order_release);
    noise_enabled_.store(true, std::memory_order_release);
}

auto ElectronGunSim::disable_noise() noexcept -> void {
    noise_enabled_.store(false, std::memory_order_release);
}

auto ElectronGunSim::is_noise_enabled() const noexcept -> bool {
    return noise_enabled_.load(std::memory_order_acquire);
}

auto ElectronGunSim::current_commanded() const noexcept
    -> th25_ctrl::ElectronGunCurrent_mA {
    return th25_ctrl::ElectronGunCurrent_mA{
        commanded_current_ma_.load(std::memory_order_acquire)};
}

}  // namespace th25_sim
