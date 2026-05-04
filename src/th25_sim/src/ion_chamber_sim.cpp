// SPDX-License-Identifier: TBD
// TH25-SIM: UNIT-304 IonChamberSim implementation.
//
// SDD-TH25-001 v0.1.1 §4.12 と完全一致する Hardware Simulator 層の 4 ユニット目
// (本層最終ユニット、UNIT-301〜303 と同パターン + 故障注入 Saturation/ChannelFailure +
// 2 系統独立).

#include "th25_sim/ion_chamber_sim.hpp"

namespace th25_sim {

// ============================================================================
// IonChamberSim 公開 API.
// ============================================================================

auto IonChamberSim::read_dose(ChannelId channel) const noexcept
    -> th25_ctrl::DoseUnit_cGy {
    const std::size_t idx = channel_index(channel);
    const FaultMode mode = fault_mode_[idx].load(std::memory_order_acquire);

    // ----- 故障モード分岐 -----
    switch (mode) {
        case FaultMode::Saturation: {
            // 上限張り付き模擬 (Therac-25 ドーズ計測異常型「上限張り付き」、IT/ST 用).
            return th25_ctrl::DoseUnit_cGy{kSaturationValueCgy};
        }
        case FaultMode::ChannelFailure: {
            // 片系失陥模擬 (応答消失、0.0 cGy 固定).
            return th25_ctrl::DoseUnit_cGy{0.0};
        }
        case FaultMode::None:
        default:
            break;
    }

    // ----- FaultMode::None: 通常応答 (累積ドーズ返却) -----
    const double v = accumulated_dose_cgy_[idx].load(std::memory_order_acquire);
    return th25_ctrl::DoseUnit_cGy{v};
}

auto IonChamberSim::inject_saturation(ChannelId channel) noexcept -> void {
    const std::size_t idx = channel_index(channel);
    fault_mode_[idx].store(FaultMode::Saturation, std::memory_order_release);
}

auto IonChamberSim::inject_channel_failure(ChannelId channel) noexcept -> void {
    const std::size_t idx = channel_index(channel);
    fault_mode_[idx].store(FaultMode::ChannelFailure, std::memory_order_release);
}

// ============================================================================
// 補助 API.
// ============================================================================

auto IonChamberSim::inject_dose_increment(ChannelId channel,
                                          th25_ctrl::DoseUnit_cGy delta) noexcept
    -> void {
    const std::size_t idx = channel_index(channel);
    // 既存累積に delta を加算後、SRS-I-006 範囲 (0.0〜10000.0 cGy) に飽和.
    // load → 加算 → clamp → store の順 (本 UT 駆動 API は単一 incrementer 前提、
    // 並行 incrementer の安全性は要求しない、UT-304-23/24 で 1 incrementer + N reader
    // 構成で race-free を検証).
    const double prev = accumulated_dose_cgy_[idx].load(std::memory_order_acquire);
    const double next_raw = prev + delta.value();
    const auto clamped = clamp_to_range(th25_ctrl::DoseUnit_cGy{next_raw});
    accumulated_dose_cgy_[idx].store(clamped.value(), std::memory_order_release);
}

auto IonChamberSim::reset_channel(ChannelId channel) noexcept -> void {
    const std::size_t idx = channel_index(channel);
    accumulated_dose_cgy_[idx].store(0.0, std::memory_order_release);
}

auto IonChamberSim::clear_fault(ChannelId channel) noexcept -> void {
    const std::size_t idx = channel_index(channel);
    fault_mode_[idx].store(FaultMode::None, std::memory_order_release);
}

auto IonChamberSim::current_fault_mode(ChannelId channel) const noexcept
    -> FaultMode {
    const std::size_t idx = channel_index(channel);
    return fault_mode_[idx].load(std::memory_order_acquire);
}

auto IonChamberSim::current_accumulated(ChannelId channel) const noexcept
    -> th25_ctrl::DoseUnit_cGy {
    const std::size_t idx = channel_index(channel);
    return th25_ctrl::DoseUnit_cGy{
        accumulated_dose_cgy_[idx].load(std::memory_order_acquire)};
}

}  // namespace th25_sim
