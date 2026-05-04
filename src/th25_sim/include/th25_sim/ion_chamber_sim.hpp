// SPDX-License-Identifier: TBD (See LICENSE in repo root once defined)
// TH25-SIM: UNIT-304 IonChamberSim (故障注入機能あり)
//
// IEC 62304 Class C, C++20.
// Implementation of SDD-TH25-001 v0.1.1 §4.12 (UNIT-304 IonChamberSim 表).
//
// 役割: 仮想イオンチェンバ (2 組 = 2 系統独立) の応答模擬 +
//       故障注入機能 (Saturation / ChannelFailure) (SAD §3 ARCH-003.4 Hardware Simulator 層).
// **Hardware Simulator 層の 4 ユニット目 (本層最終ユニット)**
// (UNIT-301 ElectronGunSim / UNIT-302 BendingMagnetSim / UNIT-303 TurntableSim に続く).
// 本ユニット完成により Hardware Simulator 層 (UNIT-301〜304) が完成.
//
// SDD §4.12 公開 API (3 種、SRS-IF-004 と完全一致):
//   - read_dose(ChannelId) const -> DoseUnit_cGy : 累積ドーズ取得 (2 系統独立)
//   - inject_saturation(ChannelId)                : サチュレーション模擬 (最大値返却)
//   - inject_channel_failure(ChannelId)           : 片系失陥模擬 (0.0 cGy 返却)
//
// 補助 API (UT / シミュレーション用):
//   - inject_dose_increment(ChannelId, DoseUnit_cGy) : UT 駆動用、累積値に加算 (clamp 後)
//   - reset_channel(ChannelId)                       : 累積リセット (テスト用)
//   - clear_fault(ChannelId)                         : 故障モード解除 (FaultMode::None に戻す)
//   - current_fault_mode(ChannelId) const            : 現在の故障モード読取 (debug 用)
//   - current_accumulated(ChannelId) const           : accumulated_dose 直接読取 (debug 用)
//
// 静的純粋関数 (UT 用):
//   - is_dose_in_range(DoseUnit_cGy)  : SRS-I-006 範囲 (0.0〜10000.0 cGy)
//   - clamp_to_range(DoseUnit_cGy)    : 範囲飽和
//   - channel_index(ChannelId)        : enum → 配列 index 変換
//
// SDD §4.12 設計判断 (Step 29 範囲):
//   - 内部状態 accumulated_dose_cgy_[2] を std::atomic<double> × 2 で保持 (release-acquire).
//   - SRS-I-006 範囲外指令 (累積) は clamp_to_range で物理的飽和を模擬 (実物理デバイス挙動、
//     0.0〜10000.0 cGy).
//   - 1 kHz でビーム電流 × 校正係数を積算は Step 29 範囲では構造のみ実装
//     (実時間 1 kHz サンプリング・積算は実装しない、inject_dose_increment(ChannelId,
//     DoseUnit_cGy) 補助 API で UT 駆動可能化). 実時間積算挙動は Inc.1 完了時の
//     IT-101 (SRS-010 「目標到達 → BeamOff < 1 ms」連鎖) + IT-105 (故障注入下のドーズ
//     整合性検証) で検証.
//   - SRS-IF-004 の `IIonChamber` 抽象 IF は Step 29 では未定義 (Inc.1 では UNIT-304 を
//     具体実装として直接利用). Inc.2/3 で複数 Sim 並行実装時に UNIT-301〜303 とまとめて
//     共通 IF として抽象化を検討.
//
// 故障モード設計 (SDD §4.12 表「Saturation / ChannelFailure」+ None):
//   - FaultMode::None           : 正常 (内部累積 accumulated_dose_cgy_ を返却)
//   - FaultMode::Saturation     : kSaturationValueCgy (10000.0 cGy = SRS-I-006 上限) 固定返却
//                                 (Therac-25 ドーズ計測異常型「上限張り付き」模擬、IT/ST 用)
//   - FaultMode::ChannelFailure : 0.0 cGy 固定返却 (片系失陥模擬、Therac-25 ドーズ計測異常型
//                                 「片系応答消失」)
//
// Step 29 範囲制約:
//   SDD §4.12「1 kHz でビーム電流 × 校正係数を積算」は Step 29 では構造のみ. UNIT-204
//   DoseManager との結線は Inc.1 後半 SafetyCoreOrchestrator dispatch 機構整備時に完成
//   (UNIT-204 の inject 系 API を本 Sim の read_dose 呼出に置き換え、API 不変).
//   UNIT-207 SafetyMonitor (Inc.2 で本格化) からの 2 系統ドーズ整合性監視結線も同 Step で完成.
//
// Therac-25 hazard mapping:
//   - HZ-005 (dose calculation): 2 系統独立イオンチェンバ + 故障注入で UNIT-204 DoseManager
//     + UNIT-207 SafetyMonitor 結線時のドーズ計測冗長性検証経路を IT/ST で機械検証可能化
//     (Therac-25 ドーズ計測異常型「片系故障時の検出経路」を構造的試験ハーネスとして提供).
//   - HZ-002 (race condition): 二重 atomic + std::atomic<FaultMode> + UT 並行 TSan で機械検証.
//   - HZ-007 (legacy preconditions): static_assert(is_always_lock_free) 二重表明
//     (double + FaultMode) を 13 ユニット目に拡大 (UNIT-200/401/201/202/203/204/205/206/208/
//     301/302/303/304、コード実装範囲内全 13 ユニットに展開完了).

#pragma once

#include "th25_ctrl/common_types.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace th25_sim {

// ============================================================================
// SRS-I-006 範囲: DoseUnit_cGy 0.0〜10000.0 cGy (SDD §4.12 / SRS-I-006).
// 注: SRS-D-004 の 0.01〜10000.0 cGy は「目標値・制御指令側」の範囲、
// SRS-I-006 の 0.0〜10000.0 cGy は「計測値・センサ読み値側」の範囲
// (計測ゼロは正常、累積開始時は 0.0 cGy). 本ユニットは Sim 計測側のため
// SRS-I-006 範囲を採用.
// ============================================================================
inline constexpr double kIonChamberMinCgy{0.0};
inline constexpr double kIonChamberMaxCgy{10000.0};

// ============================================================================
// FaultMode::Saturation 時の固定返却値 (SRS-I-006 上限 = 物理上限).
// 実物理イオンチェンバはサチュレーション時に上限値で頭打ちになる挙動を模擬.
// ============================================================================
inline constexpr double kSaturationValueCgy{kIonChamberMaxCgy};

// ============================================================================
// 2 組イオンチェンバ数 (SRS-IF-004: ChannelId ∈ {0, 1}).
// ============================================================================
inline constexpr std::size_t kChannelCount{2};

// ============================================================================
// ChannelId 列挙 (SRS-IF-004).
// std::uint8_t 基底で std::atomic<ChannelId> の lock-free 性を保証 (本 UNIT 内では
// std::atomic<ChannelId> は使わないが、Inc.2/3 抽象 IF 検討時の互換性のため明示).
// ============================================================================
enum class ChannelId : std::uint8_t {
    Channel0 = 0,
    Channel1 = 1,
};

// ============================================================================
// FaultMode 列挙 (SDD §4.12「Saturation / ChannelFailure」+ None).
// std::uint8_t 基底で std::atomic<FaultMode> の lock-free 性を保証.
//
// - None           : 正常 (累積ドーズ accumulated_dose_cgy_ を返却)
// - Saturation     : kSaturationValueCgy (10000.0 cGy) 固定返却
//                    (Therac-25 ドーズ計測異常型「上限張り付き」)
// - ChannelFailure : 0.0 cGy 固定返却 (片系失陥模擬)
//
// 注: UNIT-303 TurntableSim にも FaultMode enum があるが、ユニット間でモードの意味と種類
// が異なる (UNIT-303: None/StuckAt/Delay/NoResponse 4 種、UNIT-304: None/Saturation/
// ChannelFailure 3 種) ため、各ユニットスコープで独立して定義する (意味的に異なる enum を
// 共有することを避ける、Inc.2/3 の抽象 IF 検討時に統合判断).
// ============================================================================
enum class FaultMode : std::uint8_t {
    None = 0,
    Saturation = 1,
    ChannelFailure = 2,
};

// ============================================================================
// IonChamberSim (SDD §4.12 UNIT-304、Hardware Simulator 層の 4 ユニット目 / 本層最終).
// ============================================================================
class IonChamberSim {
public:
    IonChamberSim() noexcept = default;

    // 所有権を一意に保つためコピー/ムーブ禁止 (内部 atomic を含むため).
    IonChamberSim(const IonChamberSim&) = delete;
    IonChamberSim(IonChamberSim&&) = delete;
    auto operator=(const IonChamberSim&) -> IonChamberSim& = delete;
    auto operator=(IonChamberSim&&) -> IonChamberSim& = delete;
    ~IonChamberSim() = default;

    // ----- SDD §4.12 公開 API (SRS-IF-004 と完全一致) -----

    // 指定チャネルの累積ドーズ取得 (SRS-IF-004: ChannelId ∈ {0, 1}).
    // 事後条件 (故障モード別):
    //   - None           : accumulated_dose_cgy_[ch] を返却
    //   - Saturation     : kSaturationValueCgy (10000.0 cGy) を返却
    //   - ChannelFailure : 0.0 cGy を返却
    [[nodiscard]] auto read_dose(ChannelId channel) const noexcept
        -> th25_ctrl::DoseUnit_cGy;

    // サチュレーション模擬 (試験用、SDD §4.12 表).
    // 事後条件: 指定チャネルの故障モードを Saturation に更新 (release store).
    auto inject_saturation(ChannelId channel) noexcept -> void;

    // 片系失陥模擬 (試験用、SDD §4.12 表).
    // 事後条件: 指定チャネルの故障モードを ChannelFailure に更新 (release store).
    auto inject_channel_failure(ChannelId channel) noexcept -> void;

    // ----- 補助 API (UT / シミュレーション用) -----

    // ドーズ累積加算 (UT 駆動用、Inc.1 後半でビーム電流 × 校正係数の 1 kHz 積算に置換予定).
    // 事後条件: 指定チャネルの accumulated_dose_cgy_ に delta を加算後、
    //   clamp_to_range で 0.0〜10000.0 cGy に飽和 (物理的飽和模擬、SRS-I-006 準拠).
    auto inject_dose_increment(ChannelId channel,
                               th25_ctrl::DoseUnit_cGy delta) noexcept -> void;

    // 累積リセット (テスト用、accumulated_dose_cgy_[ch] = 0.0).
    auto reset_channel(ChannelId channel) noexcept -> void;

    // 故障モード解除 (FaultMode::None に戻す、テスト用).
    auto clear_fault(ChannelId channel) noexcept -> void;

    // 指定チャネルの故障モード読取 (UT / debug 用、acquire load).
    [[nodiscard]] auto current_fault_mode(ChannelId channel) const noexcept
        -> FaultMode;

    // 指定チャネルの累積ドーズ直接読取 (UT / debug 用、故障モードに関わらず内部値を返す).
    [[nodiscard]] auto current_accumulated(ChannelId channel) const noexcept
        -> th25_ctrl::DoseUnit_cGy;

    // ----- 静的純粋関数 (UT 用) -----

    // SRS-I-006 範囲チェック.
    [[nodiscard]] static constexpr auto is_dose_in_range(
        th25_ctrl::DoseUnit_cGy dose) noexcept -> bool {
        const double v = dose.value();
        return v >= kIonChamberMinCgy && v <= kIonChamberMaxCgy;
    }

    // SRS-I-006 範囲飽和 (実物理デバイスの飽和挙動を模擬).
    [[nodiscard]] static constexpr auto clamp_to_range(
        th25_ctrl::DoseUnit_cGy dose) noexcept -> th25_ctrl::DoseUnit_cGy {
        const double v = dose.value();
        if (v < kIonChamberMinCgy) {
            return th25_ctrl::DoseUnit_cGy{kIonChamberMinCgy};
        }
        if (v > kIonChamberMaxCgy) {
            return th25_ctrl::DoseUnit_cGy{kIonChamberMaxCgy};
        }
        return dose;
    }

    // ChannelId enum → 配列 index 変換 (UT / 内部用、constexpr).
    [[nodiscard]] static constexpr auto channel_index(ChannelId channel) noexcept
        -> std::size_t {
        return static_cast<std::size_t>(channel);
    }

private:
    // 内部状態 (release-acquire ordering).
    // accumulated_dose_cgy_[ch]: 各チャネルの累積ドーズ.
    std::atomic<double> accumulated_dose_cgy_[kChannelCount]{
        std::atomic<double>{0.0},
        std::atomic<double>{0.0},
    };

    // 各チャネルの故障モード (2 系統独立).
    std::atomic<FaultMode> fault_mode_[kChannelCount]{
        std::atomic<FaultMode>{FaultMode::None},
        std::atomic<FaultMode>{FaultMode::None},
    };

    // ----- HZ-007 構造的予防 (SDD §7 SOUP-003/004 機能要求) -----
    // UNIT-200/401/201/202/203/204/205/206/208/301/302/303 と同パターンを 13 ユニット目に拡大.
    // 本ユニットは RNG 不要のため、二重表明 (double + FaultMode) で構成 (UNIT-303 四重表明
    // から本ユニットに必要な atomic 種に絞り込み、シンプルさを優先).
    static_assert(std::atomic<double>::is_always_lock_free,
        "std::atomic<double> must be always lock-free for IonChamberSim "
        "accumulated_dose_cgy_ "
        "(SDD §4.12, RCM-002, HZ-002/HZ-007 structural prevention).");
    static_assert(std::atomic<FaultMode>::is_always_lock_free,
        "std::atomic<FaultMode> must be always lock-free for IonChamberSim fault_mode_ "
        "(SDD §4.12, RCM-002, HZ-002/HZ-007 structural prevention).");
};

}  // namespace th25_sim
