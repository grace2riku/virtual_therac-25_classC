// SPDX-License-Identifier: TBD (See LICENSE in repo root once defined)
// TH25-SIM: UNIT-303 TurntableSim (故障注入機能あり)
//
// IEC 62304 Class C, C++20.
// Implementation of SDD-TH25-001 v0.1.1 §4.12 (UNIT-303 TurntableSim 表).
//
// 役割: 仮想ターンテーブル (3 系統独立センサ) の応答模擬 +
//       故障注入機能 (StuckAt / Delay / NoResponse) (SAD §3 ARCH-003.3 Hardware Simulator 層).
// **Hardware Simulator 層の 3 ユニット目** (UNIT-301 ElectronGunSim / UNIT-302 BendingMagnetSim
// に続く). UNIT-303 は故障注入機能を持つため Sim 層内で初の「異常系試験用ハーネス基盤」.
//
// SDD §4.12 公開 API (3 種、SRS-IF-003 と完全一致):
//   - command_position(Position_mm)              : 目標位置を内部状態に保存 (50 mm/s 線形遷移、
//                                                  Step 28 範囲では構造のみ)
//   - read_sensor(SensorId) const -> Position_mm : 指定センサ値読取 (3 系統独立)
//   - inject_fault(SensorId, FaultMode)          : 故障注入 (試験用)
//
// 補助 API (UT / シミュレーション用):
//   - inject_stuck_at_value(SensorId, Position_mm) : StuckAt 時の凍結値設定
//   - enable_sensor_noise(uint32_t seed)           : ±0.1 mm センサノイズ有効化 (default OFF)
//   - disable_sensor_noise()                       : センサノイズ無効化
//   - is_sensor_noise_enabled() const              : ノイズ状態取得
//   - current_commanded() const                    : commanded_position 読取 (debug 用)
//   - current_fault_mode(SensorId) const           : 指定センサの故障モード読取 (debug 用)
//
// 静的純粋関数 (UT 用):
//   - is_position_in_range(Position_mm)  : SRS-D-007 範囲 (-100.0〜+100.0 mm)
//   - clamp_to_range(Position_mm)        : 範囲飽和
//   - sensor_index(SensorId)             : enum → 配列 index 変換
//
// SDD §4.12 設計判断 (Step 28 範囲):
//   - 内部状態 commanded_position_mm_ を std::atomic<double> で保持 (release-acquire).
//   - SRS-D-007 範囲外指令は clamp_to_range で物理的飽和を模擬 (実物理デバイス挙動).
//   - 3 系統センサ ±0.1 mm ばらつきは UT 安定化のため default OFF.
//     enable_sensor_noise(seed) で明示的に有効化、std::atomic<std::uint32_t> rng_state_ で
//     簡易 LCG 状態保持 (UNIT-301/302 と同パターン、Sim 層設計の一貫性).
//   - 50 mm/s 線形遷移は Step 28 範囲では構造のみ (実時間 sleep は実装しない).
//     実時間遷移は Inc.1 完了時の IT-104 「UNIT-205 move_to 500 ms タイムアウト + ±0.5 mm 到達」
//     で検証.
//   - SRS-IF-003 の `ITurntable` 抽象 IF は Step 28 では未定義 (Inc.1 では UNIT-303 を
//     具体実装として直接利用). Inc.2/3 で複数 Sim 並行実装時に抽象化を検討.
//
// 故障モード設計 (SDD §4.12 表「StuckAt / Delay / NoResponse」):
//   - FaultMode::None        : 正常 (実位置 ± 0.1 mm noise 任意, default の挙動)
//   - FaultMode::StuckAt     : 補助 API inject_stuck_at_value(s, value) で事前設定した
//                              凍結値を返却 (IT/ST 用、Therac-25 風「センサ故着」模擬)
//   - FaultMode::Delay       : 「前回 commanded_position」を返却 (Step 28 範囲では実時間
//                              遅延なし、構造のみ)
//   - FaultMode::NoResponse  : 0.0 mm 固定返却 (応答なし)
//
// Step 28 範囲制約:
//   SDD §4.12「移動模擬 (50 mm/s で線形遷移)」は Step 28 では構造のみ. UNIT-205
//   TurntableManager との結線は Inc.1 後半 SafetyCoreOrchestrator dispatch 機構整備時に完成
//   (UNIT-205 の inject_sensor_readings を本 Sim の read_sensor 呼出に置き換え、API 不変).
//   UNIT-208 StartupSelfCheck からの実読取置換は同 Step で UNIT-208 内部の inject API を
//   本 Sim の read_sensor 呼出に置き換え (API 不変).
//
// Therac-25 hazard mapping:
//   - HZ-001 (mode/target mismatch): 3 系統独立センサ + StuckAt 故障注入で UNIT-205 の
//     med-of-3 集約 + has_discrepancy 判定を IT/ST で機械検証可能化 (Tyler 事故型
//     「ターンテーブル位置不整合」を構造的検出経路の試験ハーネスとして提供).
//   - HZ-002 (race condition): 三重 atomic + std::atomic<FaultMode> + UT 並行 TSan で機械検証.
//   - HZ-007 (legacy preconditions): static_assert(is_always_lock_free) 四重表明 を 12 ユニット目に拡大
//     (UNIT-200/401/201/202/203/204/205/206/208/301/302/303、本ユニットで FaultMode atomic を追加).

#pragma once

#include "th25_ctrl/common_types.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace th25_sim {

// ============================================================================
// SRS-D-007 範囲: Position_mm -100.0〜+100.0 mm (SDD §4.12 / SRS-I-005).
// ============================================================================
inline constexpr double kTurntableMinMm{-100.0};
inline constexpr double kTurntableMaxMm{100.0};

// ============================================================================
// SDD §4.12「実位置 ± 0.1 mm」のセンサノイズ許容率 (絶対値).
// (UNIT-301 ±1% / UNIT-302 ±2% は乗算ノイズだったが、ターンテーブルセンサは
//  位置の絶対誤差仕様のため絶対値ノイズを採用).
// ============================================================================
inline constexpr double kTurntableSensorNoiseAbsMm{0.1};

// ============================================================================
// 3 系統独立センサ数 (SRS-IF-003: SensorId ∈ {0, 1, 2}).
// ============================================================================
inline constexpr std::size_t kSensorCount{3};

// ============================================================================
// SensorId 列挙 (SRS-IF-003).
// std::uint8_t 基底で std::atomic<SensorId> の lock-free 性を保証.
// ============================================================================
enum class SensorId : std::uint8_t {
    Sensor0 = 0,
    Sensor1 = 1,
    Sensor2 = 2,
};

// ============================================================================
// FaultMode 列挙 (SDD §4.12「StuckAt / Delay / NoResponse」+ None).
// std::uint8_t 基底で std::atomic<FaultMode> の lock-free 性を保証.
//
// - None       : 正常 (実位置 ± 0.1 mm noise 任意)
// - StuckAt    : 凍結値固定返却 (inject_stuck_at_value で事前設定)
// - Delay      : 前回 commanded_position を返却 (Step 28 範囲では構造のみ)
// - NoResponse : 0.0 mm 固定返却 (応答なし)
// ============================================================================
enum class FaultMode : std::uint8_t {
    None = 0,
    StuckAt = 1,
    Delay = 2,
    NoResponse = 3,
};

// ============================================================================
// TurntableSim (SDD §4.12 UNIT-303、Hardware Simulator 層の 3 ユニット目).
// ============================================================================
class TurntableSim {
public:
    TurntableSim() noexcept = default;

    // 所有権を一意に保つためコピー/ムーブ禁止 (内部 atomic を含むため).
    TurntableSim(const TurntableSim&) = delete;
    TurntableSim(TurntableSim&&) = delete;
    auto operator=(const TurntableSim&) -> TurntableSim& = delete;
    auto operator=(TurntableSim&&) -> TurntableSim& = delete;
    ~TurntableSim() = default;

    // ----- SDD §4.12 公開 API (SRS-IF-003 と完全一致) -----

    // ターンテーブル目標位置指令.
    // 事前条件: なし (任意の値を受け取り、SRS-D-007 範囲外は物理的飽和を模擬).
    // 事後条件: clamp_to_range で -100.0〜+100.0 mm に飽和した値を内部状態に保存
    //   (release store). Delay 故障モード参照用に「前回 commanded_position」も別変数で保持.
    // 注: 50 mm/s 線形遷移は Step 28 範囲では構造のみ (実時間 sleep なし).
    auto command_position(th25_ctrl::Position_mm target) noexcept -> void;

    // 指定センサ値読取 (SRS-IF-003: SensorId ∈ {0, 1, 2}).
    // 事後条件 (故障モード別):
    //   - None       : commanded_position ± noise (noise default OFF) を返却
    //   - StuckAt    : inject_stuck_at_value で設定された凍結値を返却
    //   - Delay      : 前回 commanded_position (previous_commanded_position_mm_) を返却
    //   - NoResponse : 0.0 mm を返却
    [[nodiscard]] auto read_sensor(SensorId sensor) const noexcept
        -> th25_ctrl::Position_mm;

    // 故障注入 (試験用、SDD §4.12 表).
    // 事後条件: 指定センサの故障モードを更新 (release store).
    // StuckAt の凍結値は事前に inject_stuck_at_value(sensor, value) で設定する運用.
    auto inject_fault(SensorId sensor, FaultMode mode) noexcept -> void;

    // ----- 補助 API (UT / シミュレーション用) -----

    // StuckAt 時の凍結値設定 (inject_fault(sensor, StuckAt) と組合せて使用).
    // 事後条件: 指定センサの stuck_at_value を更新 (release store).
    auto inject_stuck_at_value(SensorId sensor,
                               th25_ctrl::Position_mm value) noexcept -> void;

    // ±0.1 mm センサノイズ有効化 (default OFF). seed で簡易 LCG 初期状態を設定.
    auto enable_sensor_noise(std::uint32_t seed) noexcept -> void;

    // センサノイズ無効化 (read_sensor が commanded_position そのまま返却するモードに戻す).
    auto disable_sensor_noise() noexcept -> void;

    // ノイズ有効/無効状態取得 (acquire load).
    [[nodiscard]] auto is_sensor_noise_enabled() const noexcept -> bool;

    // 設定済 commanded_position 値取得 (UT / debug 用).
    [[nodiscard]] auto current_commanded() const noexcept
        -> th25_ctrl::Position_mm;

    // 指定センサの故障モード読取 (UT / debug 用、acquire load).
    [[nodiscard]] auto current_fault_mode(SensorId sensor) const noexcept
        -> FaultMode;

    // ----- 静的純粋関数 (UT 用) -----

    // SRS-D-007 範囲チェック.
    [[nodiscard]] static constexpr auto is_position_in_range(
        th25_ctrl::Position_mm position) noexcept -> bool {
        const double v = position.value();
        return v >= kTurntableMinMm && v <= kTurntableMaxMm;
    }

    // SRS-D-007 範囲飽和 (実物理デバイスの飽和挙動を模擬).
    [[nodiscard]] static constexpr auto clamp_to_range(
        th25_ctrl::Position_mm position) noexcept -> th25_ctrl::Position_mm {
        const double v = position.value();
        if (v < kTurntableMinMm) {
            return th25_ctrl::Position_mm{kTurntableMinMm};
        }
        if (v > kTurntableMaxMm) {
            return th25_ctrl::Position_mm{kTurntableMaxMm};
        }
        return position;
    }

    // SensorId enum → 配列 index 変換 (UT / 内部用、constexpr).
    [[nodiscard]] static constexpr auto sensor_index(SensorId sensor) noexcept
        -> std::size_t {
        return static_cast<std::size_t>(sensor);
    }

private:
    // 内部状態 (release-acquire ordering).
    // commanded_position_mm_: 最新 command_position の値 (clamp 後).
    std::atomic<double> commanded_position_mm_{0.0};

    // previous_commanded_position_mm_: 前回 command_position の値 (Delay 故障モード参照用).
    // command_position 呼出時に commanded_position_mm_ → previous_commanded_position_mm_
    // にロード前の値を退避 (これにより「前回値」が Delay モードで返る挙動を実現).
    std::atomic<double> previous_commanded_position_mm_{0.0};

    // 各センサの故障モード (3 系統独立).
    std::atomic<FaultMode> fault_mode_[kSensorCount]{
        std::atomic<FaultMode>{FaultMode::None},
        std::atomic<FaultMode>{FaultMode::None},
        std::atomic<FaultMode>{FaultMode::None},
    };

    // 各センサの StuckAt 凍結値 (3 系統独立).
    std::atomic<double> stuck_at_value_mm_[kSensorCount]{
        std::atomic<double>{0.0},
        std::atomic<double>{0.0},
        std::atomic<double>{0.0},
    };

    // ノイズ有効/無効 + 簡易 LCG 状態.
    std::atomic<bool> sensor_noise_enabled_{false};

    // mutable: read_sensor() は論理的 const (commanded を変更しない) だが
    // ノイズ計算の rng 内部状態は更新する必要があるため、std::atomic + mutable で
    // 標準的なパターンを採用 (UNIT-301/302 と同パターン、Sim 層設計の一貫性).
    mutable std::atomic<std::uint32_t> rng_state_{0U};

    // ----- HZ-007 構造的予防 (SDD §7 SOUP-003/004 機能要求) -----
    // UNIT-200/401/201/202/203/204/205/206/208/301/302 と同パターンを 12 ユニット目に拡大.
    // 本ユニットでは FaultMode atomic を追加し四重表明 (従来三重 + FaultMode).
    static_assert(std::atomic<double>::is_always_lock_free,
        "std::atomic<double> must be always lock-free for TurntableSim "
        "commanded_position_mm_ / previous_commanded_position_mm_ / stuck_at_value_mm_ "
        "(SDD §4.12, RCM-002, HZ-002/HZ-007 structural prevention).");
    static_assert(std::atomic<bool>::is_always_lock_free,
        "std::atomic<bool> must be always lock-free for TurntableSim sensor_noise_enabled_ "
        "(SDD §4.12, RCM-002, HZ-002/HZ-007 structural prevention).");
    static_assert(std::atomic<std::uint32_t>::is_always_lock_free,
        "std::atomic<std::uint32_t> must be always lock-free for TurntableSim rng_state_ "
        "(SDD §4.12, RCM-002, HZ-002/HZ-007 structural prevention).");
    static_assert(std::atomic<FaultMode>::is_always_lock_free,
        "std::atomic<FaultMode> must be always lock-free for TurntableSim fault_mode_ "
        "(SDD §4.12, RCM-002, HZ-002/HZ-007 structural prevention).");
};

}  // namespace th25_sim
