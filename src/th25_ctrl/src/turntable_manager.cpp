// SPDX-License-Identifier: TBD
// TH25-CTRL: UNIT-205 TurntableManager implementation.
//
// SDD-TH25-001 v0.1.1 §4.6 と完全一致する RCM-005 中核ユニット.

#include "th25_ctrl/turntable_manager.hpp"

namespace th25_ctrl {

// ============================================================================
// TurntablePosition member functions (SDD §4.6 サンプルそのまま).
// ============================================================================

auto TurntablePosition::median() const noexcept -> Position_mm {
    // 3 値の中央値 (med-of-3) を sorted の中央要素として算出.
    return Position_mm{TurntableManager::compute_median(
        sensor_0.value(), sensor_1.value(), sensor_2.value())};
}

auto TurntablePosition::has_discrepancy(double threshold_mm) const noexcept -> bool {
    return TurntableManager::has_discrepancy_values(
        sensor_0.value(), sensor_1.value(), sensor_2.value(), threshold_mm);
}

// ============================================================================
// TurntableManager 公開 API.
// ============================================================================

auto TurntableManager::move_to(Position_mm target) noexcept -> Result<void, ErrorCode> {
    // SDD §4.6 / SRS-D-007 範囲検証 [-100.0, +100.0] mm.
    if (!is_position_in_range(target)) {
        return Result<void, ErrorCode>::error(ErrorCode::TurntableOutOfPosition);
    }

    // Step 23 範囲: target_ に atomic release store.
    // 実際の InterProcessChannel 経由 TurntableSim 指令 + 3 系統センサ ±0.5 mm 到達待機
    // (SDD §4.6) は Step 24+ で UNIT-303/402 結線時に完成.
    target_.store(target.value(), std::memory_order_release);
    return Result<void, ErrorCode>::ok();
}

auto TurntableManager::read_position() const noexcept -> Result<TurntablePosition, ErrorCode> {
    // 3 系統センサを atomic acquire load.
    // 各 load は独立に行われるため、3 値は厳密には同一時刻のスナップショットではない可能性
    // があるが、is_in_position の判定は has_discrepancy + median の合成で安全側に倒れる.
    const double s0 = sensor_0_.load(std::memory_order_acquire);
    const double s1 = sensor_1_.load(std::memory_order_acquire);
    const double s2 = sensor_2_.load(std::memory_order_acquire);

    // Step 23 範囲: 失敗ケースなし (将来 UNIT-402 結線時に IpcChannelClosed を返す可能性).
    return Result<TurntablePosition, ErrorCode>(TurntablePosition{
        Position_mm{s0}, Position_mm{s1}, Position_mm{s2}});
}

auto TurntableManager::is_in_position(Position_mm expected) const noexcept -> bool {
    // SDD §4.6 サンプル実装: read_position → has_discrepancy チェック → median.abs_diff.
    auto result = read_position();
    if (!result.has_value()) {
        // 読み取りエラー (Step 23 範囲では発生しないが、将来 IpcChannelClosed 等を想定).
        return false;
    }
    const auto& pos = result.value();
    if (pos.has_discrepancy()) {
        // 3 系統不一致は in-position と認めない (SDD §4.6).
        return false;
    }
    // median と expected の偏差 ≤ 0.5 mm で in-position.
    return pos.median().abs_diff(expected) <= Position_mm{kInPositionToleranceMm};
}

auto TurntableManager::inject_sensor_readings(
    Position_mm s0, Position_mm s1, Position_mm s2) noexcept -> void {
    // UT / シミュレーション用 setter.
    // 順序: 各 sensor を release store. read_position 側で 3 値を独立に acquire load する.
    sensor_0_.store(s0.value(), std::memory_order_release);
    sensor_1_.store(s1.value(), std::memory_order_release);
    sensor_2_.store(s2.value(), std::memory_order_release);
}

auto TurntableManager::current_target() const noexcept -> Position_mm {
    return Position_mm{target_.load(std::memory_order_acquire)};
}

}  // namespace th25_ctrl
