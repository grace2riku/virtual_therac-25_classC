// SPDX-License-Identifier: TBD (See LICENSE in repo root once defined)
// TH25-SIM: UNIT-302 BendingMagnetSim
//
// IEC 62304 Class C, C++20.
// Implementation of SDD-TH25-001 v0.1.1 §4.13 (UNIT-302 BendingMagnetSim 表).
//
// 役割: 仮想ベンディング磁石の応答模擬 (SAD §3 ARCH-003.2 Hardware Simulator 層).
// **Hardware Simulator 層の 2 ユニット目** (UNIT-301 ElectronGunSim に続く).
//
// SDD §4.13 公開 API (2 種、SRS-IF-002 と完全一致):
//   - set_current(MagnetCurrent_A)              : 内部状態に保存
//   - read_actual_current() const               : 計測値取得 (設定値 ± 2% noise 任意)
//
// 補助 API (UT / シミュレーション用):
//   - enable_noise(uint32_t seed) : ノイズ有効化 (default OFF、UT 安定化)
//   - disable_noise()             : ノイズ無効化
//   - is_noise_enabled() const    : ノイズ状態取得
//
// 静的純粋関数 (UT 用):
//   - is_current_in_range(MagnetCurrent_A) : SRS-D-006 範囲 (0.0〜500.0 A)
//   - clamp_to_range(MagnetCurrent_A)      : 範囲飽和
//
// SDD §4.13 設計判断 (Step 27 範囲):
//   - 内部状態 commanded_current_a_ を std::atomic<double> で保持 (release-acquire).
//   - SRS-D-006 範囲外指令は clamp_to_range で物理的飽和を模擬 (実物理デバイス挙動).
//   - 設定値 ± 2% ノイズ模擬は UT 安定化のため default OFF. enable_noise(seed) で
//     明示的に有効化、std::atomic<std::uint32_t> rng_state_ で簡易 LCG 状態保持.
//   - 応答遅延 50 ms 模擬 (電流安定待機模擬) は Step 27 範囲では構造のみ
//     (実時間 sleep は実装しない). 実時間遅延は Inc.1 完了時の IT-101
//     「ビームオフ < 10 ms」と並行する磁石位置決め試験で検証.
//   - SRS-IF-002 の `IBendingMagnet` 抽象 IF は Step 27 では未定義 (Inc.1 では
//     UNIT-302 を具体実装として直接利用). Inc.2/3 で複数 Sim 並行実装時に抽象化を検討.
//
// Step 27 範囲制約:
//   SDD §4.13「応答遅延 50 ms 模擬」は Step 27 では構造のみ. UNIT-206
//   BendingMagnetManager との結線は Inc.1 後半 SafetyCoreOrchestrator dispatch 機構
//   整備時に完成. UNIT-208 StartupSelfCheck からの実読取置換は同 Step で UNIT-208
//   内部の inject API を本 Sim の `read_actual_current()` 呼出に置き換え (API 不変).
//
// Therac-25 hazard mapping:
//   - HZ-001 (mode/target mismatch): UNIT-206 BendingMagnetManager 結線時に
//     「磁石電流偏差 ±5%」経路で計測値と指令値の整合を検証することで
//     起動時不整合・走行時偏差を構造的検出 (Inc.1 後半完成).
//   - HZ-002 (race condition): 三重 atomic (commanded / noise_enabled / rng_state) +
//     UT 並行 TSan で機械検証.
//   - HZ-007 (legacy preconditions): static_assert(is_always_lock_free) 三重表明を
//     11 ユニット目に拡大 (UNIT-200/401/201/202/203/204/205/206/208/301/302).

#pragma once

#include "th25_ctrl/common_types.hpp"

#include <atomic>
#include <cstdint>

namespace th25_sim {

// ============================================================================
// SRS-D-006 範囲: MagnetCurrent_A 0.0〜500.0 A (SDD §4.13).
//
// 注: UNIT-206 BendingMagnetManager 側 (kMagnetCurrentMinA / kMagnetCurrentMaxA)
// と同一の SRS-D-006 範囲を、Sim 層独自定数として再宣言する.
// 理由: th25_sim ライブラリは th25_ctrl にリンクするが、Sim 層の独立した
// 設計表明として「物理デバイスの飽和範囲」を Sim 層スコープで明示する.
// ============================================================================
inline constexpr double kBendingMagnetMinA{0.0};
inline constexpr double kBendingMagnetMaxA{500.0};

// ============================================================================
// SDD §4.13 設定値 ± 2% ノイズ模擬の許容率.
// (2.0% = 0.02、UNIT-301 ElectronGunSim ±1% に対し ±2%)
// 磁石電流測定の物理実態 (より大きな揺らぎ) に整合させる.
// ============================================================================
inline constexpr double kBendingMagnetNoiseFraction{0.02};

// ============================================================================
// BendingMagnetSim (SDD §4.13 UNIT-302、Hardware Simulator 層の 2 ユニット目).
// ============================================================================
class BendingMagnetSim {
public:
    BendingMagnetSim() noexcept = default;

    // 所有権を一意に保つためコピー/ムーブ禁止 (内部 atomic を含むため).
    BendingMagnetSim(const BendingMagnetSim&) = delete;
    BendingMagnetSim(BendingMagnetSim&&) = delete;
    auto operator=(const BendingMagnetSim&) -> BendingMagnetSim& = delete;
    auto operator=(BendingMagnetSim&&) -> BendingMagnetSim& = delete;
    ~BendingMagnetSim() = default;

    // ----- SDD §4.13 公開 API (SRS-IF-002 と完全一致) -----

    // ベンディング磁石電流指令.
    // 事前条件: なし (任意の値を受け取り、SRS-D-006 範囲外は物理的飽和を模擬).
    // 事後条件: clamp_to_range で 0.0〜500.0 A に飽和した値を内部状態に保存
    //   (release store).
    auto set_current(th25_ctrl::MagnetCurrent_A commanded) noexcept -> void;

    // 計測値取得.
    // 事後条件:
    //   - noise_enabled = false (default): commanded 値そのまま返却
    //   - noise_enabled = true: commanded ± 2% の疑似乱数ノイズを乗せて返却
    //     (rng_state_ を fetch_add で次状態に進める、簡易 LCG ベース)
    //   - 結果は clamp_to_range で 0.0〜500.0 A に飽和 (SRS-I-007 準拠).
    [[nodiscard]] auto read_actual_current() const noexcept
        -> th25_ctrl::MagnetCurrent_A;

    // ----- 補助 API (UT / シミュレーション用) -----

    // ノイズ有効化 (default OFF). seed で簡易 LCG の初期状態を設定.
    auto enable_noise(std::uint32_t seed) noexcept -> void;

    // ノイズ無効化 (read_actual_current が commanded 値そのまま返却するモードに戻す).
    auto disable_noise() noexcept -> void;

    // ノイズ有効/無効状態取得 (acquire load).
    [[nodiscard]] auto is_noise_enabled() const noexcept -> bool;

    // 設定済 commanded 値取得 (UT / debug 用).
    [[nodiscard]] auto current_commanded() const noexcept
        -> th25_ctrl::MagnetCurrent_A;

    // ----- 静的純粋関数 (UT 用) -----

    // SRS-D-006 範囲チェック.
    [[nodiscard]] static constexpr auto is_current_in_range(
        th25_ctrl::MagnetCurrent_A current) noexcept -> bool {
        const double v = current.value();
        return v >= kBendingMagnetMinA && v <= kBendingMagnetMaxA;
    }

    // SRS-D-006 範囲飽和 (実物理デバイスの飽和挙動を模擬).
    [[nodiscard]] static constexpr auto clamp_to_range(
        th25_ctrl::MagnetCurrent_A current) noexcept -> th25_ctrl::MagnetCurrent_A {
        const double v = current.value();
        if (v < kBendingMagnetMinA) {
            return th25_ctrl::MagnetCurrent_A{kBendingMagnetMinA};
        }
        if (v > kBendingMagnetMaxA) {
            return th25_ctrl::MagnetCurrent_A{kBendingMagnetMaxA};
        }
        return current;
    }

private:
    // 内部状態 (release-acquire ordering).
    std::atomic<double> commanded_current_a_{0.0};
    std::atomic<bool> noise_enabled_{false};

    // 簡易 LCG (Linear Congruential Generator) 状態.
    // read_actual_current() 内で fetch_add(増分定数) で次状態に進めることで、
    // const メソッドかつ thread-safe な疑似乱数生成を実現.
    // mutable: read_actual_current() は論理的 const (commanded を変更しない) だが
    // ノイズ計算の rng 内部状態は更新する必要があるため、std::atomic + mutable で
    // 標準的なパターンを採用 (UNIT-301 ElectronGunSim と同パターン、
    // C++ FAQ / Hennessy & Patterson 推奨).
    mutable std::atomic<std::uint32_t> rng_state_{0U};

    // ----- HZ-007 構造的予防 (SDD §7 SOUP-003/004 機能要求) -----
    // UNIT-200/401/201/202/203/204/205/206/208/301 と同パターンを 11 ユニット目に拡大.
    static_assert(std::atomic<double>::is_always_lock_free,
        "std::atomic<double> must be always lock-free for BendingMagnetSim "
        "commanded_current_a_ "
        "(SDD §4.13, RCM-002, HZ-002/HZ-007 structural prevention).");
    static_assert(std::atomic<bool>::is_always_lock_free,
        "std::atomic<bool> must be always lock-free for BendingMagnetSim noise_enabled_ "
        "(SDD §4.13, RCM-002, HZ-002/HZ-007 structural prevention).");
    static_assert(std::atomic<std::uint32_t>::is_always_lock_free,
        "std::atomic<std::uint32_t> must be always lock-free for BendingMagnetSim "
        "rng_state_ "
        "(SDD §4.13, RCM-002, HZ-002/HZ-007 structural prevention).");
};

}  // namespace th25_sim
