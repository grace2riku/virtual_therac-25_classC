// SPDX-License-Identifier: TBD (See LICENSE in repo root once defined)
// TH25-SIM: UNIT-301 ElectronGunSim
//
// IEC 62304 Class C, C++20.
// Implementation of SDD-TH25-001 v0.1.1 §4.12 (UNIT-301).
//
// 役割: 仮想電子銃の応答模擬 (SAD §3 ARCH-003.1 Hardware Simulator 層).
// **Hardware Simulator 層の最初の具体実装ユニット**.
//
// SDD §4.12 公開 API (2 種、SRS-IF-001 と完全一致):
//   - set_current(ElectronGunCurrent_mA)        : 内部状態に保存
//   - read_actual_current() const               : 計測値取得 (設定値 ± 1% noise 任意)
//
// 補助 API (UT / シミュレーション用):
//   - enable_noise(uint32_t seed) : ノイズ有効化 (default OFF、UT 安定化)
//   - disable_noise()             : ノイズ無効化
//   - is_noise_enabled() const    : ノイズ状態取得
//
// 静的純粋関数 (UT 用):
//   - is_current_in_range(ElectronGunCurrent_mA)  : SRS-D-008 範囲 (0.0〜10.0 mA)
//   - clamp_to_range(ElectronGunCurrent_mA)       : 範囲飽和
//
// SDD §4.12 設計判断 (Step 26 範囲):
//   - 内部状態 commanded_current_ma_ を std::atomic<double> で保持 (release-acquire).
//   - SRS-D-008 範囲外指令は clamp_to_range で物理的飽和を模擬 (実物理デバイス挙動).
//   - 設定値 ± 1% ノイズ模擬は UT 安定化のため default OFF. enable_noise(seed) で
//     明示的に有効化、std::atomic<std::uint32_t> rng_state_ で簡易 LCG 状態保持.
//   - 応答遅延 1 ms 模擬は Step 26 範囲では構造のみ (実時間 sleep は実装しない).
//     実時間遅延は Inc.1 完了時の IT-101「ビームオフ < 10 ms」測定で検証.
//   - SRS-IF-001 の `IElectronGun` 抽象 IF は Step 26 では未定義 (Inc.1 では UNIT-301
//     を具体実装として直接利用). Inc.2/3 で複数 Sim 並行実装時に抽象化を検討.
//
// Step 26 範囲制約:
//   SDD §4.12「応答遅延 1 ms 模擬」は Step 26 では構造のみ. UNIT-203 BeamController
//   との結線は Inc.1 後半 SafetyCoreOrchestrator dispatch 機構整備時に完成. UNIT-208
//   StartupSelfCheck からの実読取置換は同 Step で UNIT-208 内部の inject API を本 Sim
//   の `read_actual_current()` 呼出に置き換え (API 不変).
//
// Therac-25 hazard mapping:
//   - HZ-001 (mode/target mismatch): UNIT-203 BeamController 結線時に「電子銃停止確認」
//     経路で計測値 = 0 mA を確認することで起動時不整合を構造的拒否 (Inc.1 後半完成).
//   - HZ-002 (race condition): 三重 atomic (commanded / noise_enabled / rng_state) +
//     UT 並行 TSan で機械検証.
//   - HZ-007 (legacy preconditions): static_assert(is_always_lock_free) 三重表明を
//     10 ユニット目に拡大 (UNIT-200/401/201/202/203/204/205/206/208/301).

#pragma once

#include "th25_ctrl/common_types.hpp"

#include <atomic>
#include <cstdint>

namespace th25_sim {

// ============================================================================
// SRS-D-008 範囲: ElectronGunCurrent_mA 0.0〜10.0 mA (SDD §4.12).
// ============================================================================
inline constexpr double kElectronGunMinMA{0.0};
inline constexpr double kElectronGunMaxMA{10.0};

// ============================================================================
// SDD §4.12 設定値 ± 1% ノイズ模擬の許容率.
// (1.0% = 0.01)
// ============================================================================
inline constexpr double kElectronGunNoiseFraction{0.01};

// ============================================================================
// ElectronGunSim (SDD §4.12 UNIT-301、Hardware Simulator 層の最初の具体実装).
// ============================================================================
class ElectronGunSim {
public:
    ElectronGunSim() noexcept = default;

    // 所有権を一意に保つためコピー/ムーブ禁止 (内部 atomic を含むため).
    ElectronGunSim(const ElectronGunSim&) = delete;
    ElectronGunSim(ElectronGunSim&&) = delete;
    auto operator=(const ElectronGunSim&) -> ElectronGunSim& = delete;
    auto operator=(ElectronGunSim&&) -> ElectronGunSim& = delete;
    ~ElectronGunSim() = default;

    // ----- SDD §4.12 公開 API (SRS-IF-001 と完全一致) -----

    // 電子銃ビーム電流指令.
    // 事前条件: なし (任意の値を受け取り、SRS-D-008 範囲外は物理的飽和を模擬).
    // 事後条件: clamp_to_range で 0.0〜10.0 mA に飽和した値を内部状態に保存
    //   (release store).
    auto set_current(th25_ctrl::ElectronGunCurrent_mA commanded) noexcept -> void;

    // 計測値取得.
    // 事後条件:
    //   - noise_enabled = false (default): commanded 値そのまま返却
    //   - noise_enabled = true: commanded ± 1% の疑似乱数ノイズを乗せて返却
    //     (rng_state_ を fetch_add で次状態に進める、簡易 LCG ベース)
    //   - 結果は clamp_to_range で 0.0〜10.0 mA に飽和 (SRS-I-008 準拠).
    [[nodiscard]] auto read_actual_current() const noexcept
        -> th25_ctrl::ElectronGunCurrent_mA;

    // ----- 補助 API (UT / シミュレーション用) -----

    // ノイズ有効化 (default OFF). seed で簡易 LCG の初期状態を設定.
    auto enable_noise(std::uint32_t seed) noexcept -> void;

    // ノイズ無効化 (read_actual_current が commanded 値そのまま返却するモードに戻す).
    auto disable_noise() noexcept -> void;

    // ノイズ有効/無効状態取得 (acquire load).
    [[nodiscard]] auto is_noise_enabled() const noexcept -> bool;

    // 設定済 commanded 値取得 (UT / debug 用).
    [[nodiscard]] auto current_commanded() const noexcept
        -> th25_ctrl::ElectronGunCurrent_mA;

    // ----- 静的純粋関数 (UT 用) -----

    // SRS-D-008 範囲チェック.
    [[nodiscard]] static constexpr auto is_current_in_range(
        th25_ctrl::ElectronGunCurrent_mA current) noexcept -> bool {
        const double v = current.value();
        return v >= kElectronGunMinMA && v <= kElectronGunMaxMA;
    }

    // SRS-D-008 範囲飽和 (実物理デバイスの飽和挙動を模擬).
    [[nodiscard]] static constexpr auto clamp_to_range(
        th25_ctrl::ElectronGunCurrent_mA current) noexcept
        -> th25_ctrl::ElectronGunCurrent_mA {
        const double v = current.value();
        if (v < kElectronGunMinMA) {
            return th25_ctrl::ElectronGunCurrent_mA{kElectronGunMinMA};
        }
        if (v > kElectronGunMaxMA) {
            return th25_ctrl::ElectronGunCurrent_mA{kElectronGunMaxMA};
        }
        return current;
    }

private:
    // 内部状態 (release-acquire ordering).
    std::atomic<double> commanded_current_ma_{0.0};
    std::atomic<bool> noise_enabled_{false};

    // 簡易 LCG (Linear Congruential Generator) 状態.
    // read_actual_current() 内で fetch_add(増分定数) で次状態に進めることで、
    // const メソッドかつ thread-safe な疑似乱数生成を実現.
    // mutable: read_actual_current() は論理的 const (commanded を変更しない) だが
    // ノイズ計算の rng 内部状態は更新する必要があるため、std::atomic + mutable で
    // 標準的なパターンを採用 (Hennessy & Patterson 推奨、C++ FAQ 推奨).
    mutable std::atomic<std::uint32_t> rng_state_{0U};

    // ----- HZ-007 構造的予防 (SDD §7 SOUP-003/004 機能要求) -----
    // UNIT-200/401/201/202/203/204/205/206/208 と同パターンを 10 ユニット目に拡大.
    static_assert(std::atomic<double>::is_always_lock_free,
        "std::atomic<double> must be always lock-free for ElectronGunSim "
        "commanded_current_ma_ "
        "(SDD §4.12, RCM-002, HZ-002/HZ-007 structural prevention).");
    static_assert(std::atomic<bool>::is_always_lock_free,
        "std::atomic<bool> must be always lock-free for ElectronGunSim noise_enabled_ "
        "(SDD §4.12, RCM-002, HZ-002/HZ-007 structural prevention).");
    static_assert(std::atomic<std::uint32_t>::is_always_lock_free,
        "std::atomic<std::uint32_t> must be always lock-free for ElectronGunSim rng_state_ "
        "(SDD §4.12, RCM-002, HZ-002/HZ-007 structural prevention).");
};

}  // namespace th25_sim
