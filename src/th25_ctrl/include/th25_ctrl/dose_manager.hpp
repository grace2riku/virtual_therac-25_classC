// SPDX-License-Identifier: TBD (See LICENSE in repo root once defined)
// TH25-CTRL: UNIT-204 DoseManager
//
// IEC 62304 Class C, C++20.
// Implementation of SDD-TH25-001 v0.1.1 §4.5 (UNIT-204).
//
// 役割: ドーズ目標管理 + ドーズ積算 + 目標到達検知 + 即時ビームオフ指令フラグ.
// **HZ-005(ドーズ計算誤り)直接対応 + HZ-003(整数オーバフロー)構造的排除の中核**.
//
// SDD §4.5 公開 API (3 種):
//   - set_dose_target(DoseUnit_cGy, LifecycleState) : ドーズ目標設定
//   - on_dose_pulse(PulseCount)                     : 1 パルス分のドーズ積算 + 到達検知
//   - current_accumulated()                         : 累積ドーズ取得
//
// 補助 API (UT / observer 用):
//   - current_target()                              : 設定済目標ドーズ取得
//   - is_target_reached()                           : 到達フラグ取得 (UNIT-201 / UNIT-203 が後続 Step で観測)
//   - pulse_count_to_dose(PulseCount)               : 単位変換 (rate_ 経由)
//   - reset()                                       : 累積カウンタ + 到達フラグをクリア
//   - is_dose_target_in_range(DoseUnit_cGy)         : 静的純粋関数 (UT 用、SRS-008 範囲)
//
// SDD §4.5 設計判断 (Step 22 範囲):
//   - PulseCounter (UNIT-200 既存) を内包し std::atomic<uint64_t> で HZ-003 構造的排除
//     (uint64_t::max() ≒ 1.8e19 パルスは 1 kHz サンプリングで 5,800 万年連続照射相当)
//   - 目標値を内部で target_pulses_ (std::atomic<uint64_t>) に変換し整数比較化
//     (浮動小数 precision 問題回避、is_always_lock_free 保証)
//   - target_reached_ を std::atomic<bool> で立て、後続 Step で UNIT-201 が
//     observer pattern で BeamOff 指令を発行する設計 (SDD §4.5 サンプルの
//     MessageBus 経由 BeamOff publish は dispatch 機構整備時に完成、Step 22
//     範囲では「目標到達フラグ」までを実装する漸進策)
//
// Step 22 範囲制約:
//   SDD §4.5「IF-U-002 経由で BeamOff 要求送信」の MessageBus 結線は、UNIT-401
//   InProcessQueue は実装済みだが BeamOffRequest メッセージ型と SafetyCoreOrchestrator
//   からの dispatch 機構が未確定のため、Step 22 ではフラグ立てまで. UNIT-203
//   BeamController との結線は Step 23+ で実施.
//
// Therac-25 hazard mapping:
//   - HZ-002 (race condition): 全 atomic 共有変数 + UT TSan 並行試験で機械検証.
//   - HZ-003 (integer overflow): PulseCounter 内 std::atomic<uint64_t> で構造的排除.
//     5,800 万年連続照射相当の余裕は実用上オーバフロー不可能.
//   - HZ-005 (dose calculation): 範囲検証 + 強い型 + 目標到達検知 + 即時 BeamOff
//     指令フラグの 4 段階で構造的予防. UT 境界値で機械検証.
//   - HZ-007 (legacy preconditions): static_assert(is_always_lock_free) を
//     UNIT-200/401/201/202/203 の 5 ユニットに続き 6 ユニット目に拡大.

#pragma once

#include "th25_ctrl/common_types.hpp"

#include <atomic>
#include <cstdint>
#include <limits>

namespace th25_ctrl {

// ============================================================================
// SDD §6.5 で予告された強い型: 1 パルスあたりのドーズ単位 (cGy/pulse).
// 校正値として外部から注入し、コンパイル時に PulseCount → DoseUnit_cGy 変換に使用.
// 共通パターン (UNIT-200 強い型と同じ): explicit ctor + value() + 比較演算子のみ.
// 算術演算子は提供しない (DoseUnit_cGy / PulseCount との変換は専用関数経由).
// ============================================================================
class DoseRatePerPulse_cGy_per_pulse {
public:
    explicit constexpr DoseRatePerPulse_cGy_per_pulse(double value) noexcept : value_(value) {}
    [[nodiscard]] constexpr auto value() const noexcept -> double { return value_; }
    constexpr auto operator<=>(const DoseRatePerPulse_cGy_per_pulse&) const noexcept = default;
    constexpr auto operator==(const DoseRatePerPulse_cGy_per_pulse&) const noexcept -> bool = default;

private:
    double value_;
};

// ============================================================================
// SRS-008 範囲定数 (DoseManager の set_dose_target 入力検証で使用).
// ============================================================================
inline constexpr DoseUnit_cGy kDoseTargetMinCGy{0.01};
inline constexpr DoseUnit_cGy kDoseTargetMaxCGy{10000.0};

// ============================================================================
// DoseManager (SDD §4.5 UNIT-204、HZ-005 直接 + HZ-003 構造的排除の中核).
// ============================================================================
class DoseManager {
public:
    // 校正値 rate を constructor 引数で注入する (SDD §6.5 構成定義の C++ 側受口).
    explicit DoseManager(DoseRatePerPulse_cGy_per_pulse rate) noexcept;

    // 所有権を一意に保つためコピー/ムーブ禁止 (PulseCounter 内 atomic を含むため).
    DoseManager(const DoseManager&) = delete;
    DoseManager(DoseManager&&) = delete;
    auto operator=(const DoseManager&) -> DoseManager& = delete;
    auto operator=(DoseManager&&) -> DoseManager& = delete;
    ~DoseManager() = default;

    // ----- SDD §4.5 公開 API -----

    // ドーズ目標設定 (引数: 目標ドーズ + 現在の LifecycleState).
    // 事前条件:
    //   - lifecycle_state ∈ {PrescriptionSet, Ready}
    //   - target ∈ [0.01, 10000.0] cGy (SRS-008)
    // 事後条件 (成功時):
    //   - target_pulses_ = target / rate_  (整数比較用に変換)
    //   - target_set_ = true
    //   - target_reached_ = false
    //   - accumulated_pulses_ = 0 にリセット (SDD §4.5「累積ドーズ = 0 にリセット」)
    // エラー時:
    //   - lifecycle 違反: InternalUnexpectedState (UNIT-201 init_subsystems と同パターン)
    //   - 範囲外: DoseOutOfRange (SDD §4.5)
    [[nodiscard]] auto set_dose_target(DoseUnit_cGy target,
                                       LifecycleState lifecycle_state) noexcept
        -> Result<void, ErrorCode>;

    // 1 パルス分のドーズ積算 + 目標到達検知.
    // 事前条件: なし (IonChamberSim から 1 kHz で呼出される、SDD §4.5).
    // 事後条件:
    //   - accumulated_pulses_.fetch_add(pulse_delta, acq_rel)
    //   - 目標到達時 (累積 ≥ target_pulses_ 且つ target_set_) は target_reached_ = true.
    //   - target_set_ = false の状態では到達検知を実施しない (未設定保護).
    // 注: SDD §4.5 サンプルでは MessageBus 経由 BeamOff publish するが、Step 22 では
    // 到達フラグまで実装. UNIT-203 BeamController への結線は Step 23+ (SafetyCore
    // Orchestrator dispatch 機構整備時) で実施.
    auto on_dose_pulse(PulseCount pulse_delta) noexcept -> void;

    // 現在の累積ドーズ取得 (read-only). atomic acquire load + 単位変換.
    [[nodiscard]] auto current_accumulated() const noexcept -> DoseUnit_cGy;

    // ----- 補助 API (UT / observer 用、SDD §4.5 サンプルの dispatch 機構整備時の準備) -----

    // 設定済目標ドーズ取得. 未設定時は DoseUnit_cGy{0.0} を返す.
    [[nodiscard]] auto current_target() const noexcept -> DoseUnit_cGy;

    // 目標到達フラグ取得.
    // UNIT-201 / UNIT-203 が後続 Step で observer pattern により BeamOff 指令発行に使用.
    [[nodiscard]] auto is_target_reached() const noexcept -> bool;

    // 単位変換 (PulseCount → DoseUnit_cGy). rate_ 経由.
    [[nodiscard]] auto pulse_count_to_dose(PulseCount count) const noexcept -> DoseUnit_cGy;

    // 累積カウンタ + 到達フラグ + target_set_ を初期状態にクリア.
    // 治療セッション開始前 / 緊急停止後の再初期化用.
    auto reset() noexcept -> void;

    // ----- 静的純粋関数 (UT 用) -----

    // SRS-008 範囲 [0.01, 10000.0] cGy 内か.
    [[nodiscard]] static constexpr auto is_dose_target_in_range(DoseUnit_cGy target) noexcept
        -> bool {
        return target >= kDoseTargetMinCGy && target <= kDoseTargetMaxCGy;
    }

    // テスト用: 内部 target_pulses_ 値を覗く (uint64_t 整数比較化の検証用).
    // 通常コードからは使わない.
    [[nodiscard]] auto target_pulses_for_test() const noexcept -> std::uint64_t;

private:
    // 累積パルス数 (RCM-003 構造的予防の中核、UNIT-200 PulseCounter 内 atomic<uint64_t>).
    PulseCounter accumulated_pulses_;

    // 目標到達判定用の整数値 (target / rate_ で算出. uint64_t::max() = 未設定保護).
    // 整数比較化により浮動小数 precision 問題を回避し is_always_lock_free を保証.
    std::atomic<std::uint64_t> target_pulses_{std::numeric_limits<std::uint64_t>::max()};

    // 目標到達フラグ (UNIT-201 / UNIT-203 が後続 Step で observer pattern に使用).
    std::atomic<bool> target_reached_{false};

    // 目標設定済フラグ (set_dose_target 呼出後 true, reset() で false).
    // 未設定時 (false) は on_dose_pulse が到達検知を実施せず、累積のみ進める.
    std::atomic<bool> target_set_{false};

    // 校正値 (1 パルスあたり cGy). constructor で固定、変更不可.
    DoseRatePerPulse_cGy_per_pulse rate_;

    // ----- HZ-007 構造的予防 (SDD §7 SOUP-003/004 機能要求) -----
    // UNIT-200/401/201/202/203 と同パターンを 6 ユニット目に拡大.
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
        "std::atomic<uint64_t> must be always lock-free for DoseManager target_pulses_ "
        "(SDD §4.5, RCM-003, HZ-002/HZ-003/HZ-005/HZ-007 structural prevention).");
    static_assert(std::atomic<bool>::is_always_lock_free,
        "std::atomic<bool> must be always lock-free for DoseManager target_reached_ / "
        "target_set_ (SDD §4.5, observer flag for UNIT-201/UNIT-203 dispatch).");
};

}  // namespace th25_ctrl
