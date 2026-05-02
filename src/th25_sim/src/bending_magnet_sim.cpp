// SPDX-License-Identifier: TBD
// TH25-SIM: UNIT-302 BendingMagnetSim implementation.
//
// SDD-TH25-001 v0.1.1 §4.13 と完全一致する Hardware Simulator 層の 2 ユニット目
// (UNIT-301 ElectronGunSim と同パターン、ノイズ ±2% / SRS-D-006 範囲 0.0〜500.0 A).

#include "th25_sim/bending_magnet_sim.hpp"

namespace th25_sim {

namespace {

// 簡易 LCG (Numerical Recipes 推奨係数). 周期 2^32.
// rng_state' = rng_state * 1664525 + 1013904223
// UNIT-301 ElectronGunSim と同パターンで Sim 層内に閉じて再宣言する
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
// BendingMagnetSim 公開 API.
// ============================================================================

auto BendingMagnetSim::set_current(th25_ctrl::MagnetCurrent_A commanded) noexcept
    -> void {
    // SRS-D-006 範囲外は物理的飽和を模擬 (実物理デバイス挙動).
    const auto clamped = clamp_to_range(commanded);
    commanded_current_a_.store(clamped.value(), std::memory_order_release);
}

auto BendingMagnetSim::read_actual_current() const noexcept
    -> th25_ctrl::MagnetCurrent_A {
    const double commanded = commanded_current_a_.load(std::memory_order_acquire);

    if (!noise_enabled_.load(std::memory_order_acquire)) {
        // ノイズ無効: commanded 値そのまま返却 (UT 安定化).
        return th25_ctrl::MagnetCurrent_A{commanded};
    }

    // ノイズ有効: 簡易 LCG で疑似乱数を生成し ±2% に正規化.
    // fetch_add で rng_state_ を次状態に進める (release_acquire で thread-safe).
    const std::uint32_t prev_state =
        rng_state_.fetch_add(kLcgIncrement, std::memory_order_acq_rel);
    const std::uint32_t next_state = prev_state * kLcgMultiplier + kLcgIncrement;
    const double normalized = normalize_to_unit_range(next_state);  // [-1, +1)
    const double noise_delta = commanded * kBendingMagnetNoiseFraction * normalized;
    const double noisy = commanded + noise_delta;

    // SRS-I-007 準拠: 計測値も 0.0〜500.0 A に飽和.
    return clamp_to_range(th25_ctrl::MagnetCurrent_A{noisy});
}

// ============================================================================
// 補助 API.
// ============================================================================

auto BendingMagnetSim::enable_noise(std::uint32_t seed) noexcept -> void {
    rng_state_.store(seed, std::memory_order_release);
    noise_enabled_.store(true, std::memory_order_release);
}

auto BendingMagnetSim::disable_noise() noexcept -> void {
    noise_enabled_.store(false, std::memory_order_release);
}

auto BendingMagnetSim::is_noise_enabled() const noexcept -> bool {
    return noise_enabled_.load(std::memory_order_acquire);
}

auto BendingMagnetSim::current_commanded() const noexcept
    -> th25_ctrl::MagnetCurrent_A {
    return th25_ctrl::MagnetCurrent_A{
        commanded_current_a_.load(std::memory_order_acquire)};
}

}  // namespace th25_sim
