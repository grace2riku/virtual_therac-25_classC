// SPDX-License-Identifier: TBD (See LICENSE in repo root once defined)
// TH25-CTRL: UNIT-206 BendingMagnetManager
//
// IEC 62304 Class C, C++20.
// Implementation of SDD-TH25-001 v0.1.1 §4.7 (UNIT-206).
//
// 役割: ベンディング磁石電流指令(エネルギー対応表ベース) + 計測値偏差監視.
// **HZ-005(ドーズ計算誤り)部分対応 + RCM-008(強い型による単位安全)中核**.
//
// SDD §4.7 公開 API (3 種; 電子線/X 線の Energy_MeV / Energy_MV を分離):
//   - set_current_for_energy(TreatmentMode, Energy_MeV) : 電子線用 (1.0〜25.0 MeV)
//   - set_current_for_energy(TreatmentMode, Energy_MV)  : X 線用    (5.0〜25.0 MV)
//   - current_actual()                                  : 計測値取得
//   - is_within_tolerance()                             : 偏差 ±5% 以内か
//
// 補助 API (UT / シミュレーション用):
//   - inject_actual_current(MagnetCurrent_A) : 計測値を強制設定 (将来 UNIT-302/402
//                                              結線時はセンサ実読取に置換、Step 24 では
//                                              UT 駆動可能)
//   - current_target()                       : 設定済 target 取得
//
// 静的純粋関数 (UT 用):
//   - EnergyMagnetMap::compute_target_current_electron(Energy_MeV) : 電子線対応表 + 線形補間
//   - EnergyMagnetMap::compute_target_current_xray(Energy_MV)      : X 線対応表 + 線形補間
//   - is_current_within_tolerance(target, actual, pct)             : 偏差判定
//   - is_current_in_range(MagnetCurrent_A)                         : SRS-D-006 範囲判定
//
// SDD §4.7 設計判断 (Step 24 範囲):
//   - target_current_ / actual_current_ を std::atomic<double> で保持 (TurntableManager
//     と同パターン、release-acquire ordering)
//   - target_set_ を std::atomic<bool> で「target が一度でも設定されたか」フラグ管理
//     (DoseManager の target_set_ と同パターン、is_within_tolerance の事前条件チェック用)
//   - EnergyMagnetMap は線形補間係数 (slope, intercept) を内部 constexpr で保持し、
//     SDP §6.1 で確定予定の校正データに対応する単純な対応表 (Step 24 ではプレースホルダ
//     値を採用、SDP §6.1 確定後に置換予定。これは SDD §6.5「具体値はコンストラクタで
//     注入、コードに直接埋め込まない」と整合的な暫定値)
//   - SRS-004 範囲 (Electron 1.0〜25.0 MeV / XRay 5.0〜25.0 MV) を範囲定数で外部化
//   - SRS-D-006 範囲 (0.0〜500.0 A) を範囲定数で外部化
//   - SRS-ALM-004 偏差閾値 (±5% = 0.05) を kMagnetToleranceFraction で外部化
//
// Step 24 範囲制約:
//   SDD §4.7「BendingMagnetSim に MagnetCurrent_A 指令送信、±5% 以内に到達するまで
//   待機(タイムアウト 200 ms)」は UNIT-302 BendingMagnetSim + UNIT-402 InterProcessChannel
//   が未実装のため、Step 24 では target_current_ への保持 + inject_actual_current で
//   UT 駆動可能な構造を成立させるまで. 実 IPC 送信 + 200 ms タイムアウト + ±5% 到達待機
//   は Step 25+ で UNIT-302/402 結線時. SRS-ALM-004「連続 100 ms 以上」の継続検出も同様
//   に Step 25+.
//
// Therac-25 hazard mapping:
//   - HZ-005 (dose calculation): 強い型 Energy_MeV / Energy_MV / MagnetCurrent_A による
//     コンパイル時の単位安全 (RCM-008). 暗黙変換は common_types.hpp で構造的に禁止済.
//   - HZ-001 (mode/target mismatch): TreatmentMode 判定で電子線/X 線の対応表を切替.
//     不正な (Light, energy) 等の組合せは ModeInvalidTransition で拒否 (UNIT-202 と整合).
//   - HZ-002 (race condition): 3 atomic + UT 並行 TSan で機械検証.
//   - HZ-007 (legacy preconditions): static_assert(std::atomic<double>::is_always_lock_free)
//     + std::atomic<bool>::is_always_lock_free を 8 ユニット目に拡大 (UNIT-200/401/201/
//     202/203/204/205/206、コード実装範囲内全 8 ユニットに展開完了).

#pragma once

#include "th25_ctrl/common_types.hpp"

#include <atomic>

namespace th25_ctrl {

// ============================================================================
// SRS-D-006 範囲: MagnetCurrent_A 0.0〜500.0 A (SDD §4.7).
// ============================================================================
inline constexpr double kMagnetCurrentMinA{0.0};
inline constexpr double kMagnetCurrentMaxA{500.0};

// ============================================================================
// SRS-ALM-004 偏差許容値: ±5% (= 0.05).
// SDD §4.7「±5% 以内に到達するまで待機」と整合.
// ============================================================================
inline constexpr double kMagnetToleranceFraction{0.05};

// ============================================================================
// EnergyMagnetMap: エネルギー → 磁石電流の対応表 (SDD §4.7 / §6.5).
//
// SDD §4.7 で「電子線時 1〜25 MeV / X 線時 5〜25 MV の対応表を線形補間で実装する
// ことのみ確定」と定義. 具体値は SDP §6.1 で確定予定の校正データに対応する.
//
// Step 24 範囲では、線形補間係数 (slope, intercept) を constexpr で内部保持する.
// プレースホルダ値:
//   - Electron: 1 MeV → 2.5 A, 25 MeV → 62.5 A (slope=2.5 A/MeV, intercept=0)
//   - XRay:     5 MV  → 50 A,  25 MV → 250 A   (slope=10 A/MV,  intercept=0)
// 実校正データは SDP §6.1 確定後に CR を起票して置換 (HZ-007 構造的予防).
// ============================================================================
class EnergyMagnetMap {
public:
    // 電子線エネルギー → 磁石電流 (線形補間).
    // 範囲外時は MagnetCurrent_A{NaN} を返さず、呼出側 (set_current_for_energy)
    // で範囲チェックが先行するため、本関数は値域内で正の値を返す前提.
    [[nodiscard]] static constexpr auto compute_target_current_electron(
        Energy_MeV energy) noexcept -> MagnetCurrent_A {
        // I_A = slope * E_MeV + intercept
        constexpr double kSlopeElectronAPerMeV{2.5};
        constexpr double kInterceptElectronA{0.0};
        return MagnetCurrent_A{kSlopeElectronAPerMeV * energy.value() + kInterceptElectronA};
    }

    // X 線エネルギー → 磁石電流 (線形補間).
    [[nodiscard]] static constexpr auto compute_target_current_xray(
        Energy_MV energy) noexcept -> MagnetCurrent_A {
        constexpr double kSlopeXrayAPerMV{10.0};
        constexpr double kInterceptXrayA{0.0};
        return MagnetCurrent_A{kSlopeXrayAPerMV * energy.value() + kInterceptXrayA};
    }
};

// ============================================================================
// BendingMagnetManager (SDD §4.7 UNIT-206、HZ-005 部分対応 + RCM-008 中核).
// ============================================================================
class BendingMagnetManager {
public:
    BendingMagnetManager() noexcept = default;

    // 所有権を一意に保つためコピー/ムーブ禁止 (3 atomic を含むため).
    BendingMagnetManager(const BendingMagnetManager&) = delete;
    BendingMagnetManager(BendingMagnetManager&&) = delete;
    auto operator=(const BendingMagnetManager&) -> BendingMagnetManager& = delete;
    auto operator=(BendingMagnetManager&&) -> BendingMagnetManager& = delete;
    ~BendingMagnetManager() = default;

    // ----- SDD §4.7 公開 API -----

    // 電子線エネルギー対応 磁石電流指令.
    // 事前条件: mode == Electron, energy ∈ [1.0, 25.0] MeV (SRS-004).
    // 事後条件 (Step 24 範囲): target_current_ に対応表計算結果を保存. 実 IPC 送信 +
    // ±5% 到達待機 (タイムアウト 200 ms) は Step 25+ で UNIT-302/402 結線時に完成.
    // エラー時:
    //   - mode != Electron: ModeInvalidTransition (UNIT-202 と整合)
    //   - energy 範囲外:    ModeInvalidTransition (Mode 系該当、SRS-004)
    //   - 計算結果が範囲外: MagnetCurrentDeviation (Magnet 系 0x05、安全側拒否)
    [[nodiscard]] auto set_current_for_energy(
        TreatmentMode mode, Energy_MeV electron_energy) noexcept -> Result<void, ErrorCode>;

    // X 線エネルギー対応 磁石電流指令.
    // 事前条件: mode == XRay, energy ∈ [5.0, 25.0] MV (SRS-004).
    // 事後条件・エラー処理は Electron 版と同じ.
    [[nodiscard]] auto set_current_for_energy(
        TreatmentMode mode, Energy_MV xray_energy) noexcept -> Result<void, ErrorCode>;

    // 計測値取得 (read-only, atomic acquire load).
    // Step 24 範囲: inject_actual_current で設定された値を返す. 将来 UNIT-302 結線時は
    // BendingMagnetSim の最後の有効値を返す (SDD §4.7 「センサ失敗時例外なし、最後の
    // 有効値を返す」).
    [[nodiscard]] auto current_actual() const noexcept -> MagnetCurrent_A;

    // 偏差 ±5% 以内か判定 (SDD §4.7 / SRS-ALM-004).
    // 事前条件: target が set_current_for_energy で設定済 (target_set_ = true).
    //   target 未設定時は false (in-tolerance と認めない、安全側).
    // 事後条件: |actual - target| / target ≤ 0.05 で true.
    //
    // Step 24 範囲: 一回限りの判定. SRS-ALM-004「連続 100 ms 以上」の継続検出は
    // Step 25+ で UNIT-302/402 結線時の連続監視ループに組み込む.
    [[nodiscard]] auto is_within_tolerance() const noexcept -> bool;

    // ----- 補助 API (UT / シミュレーション用) -----

    // 計測値を強制設定 (UT 駆動 / シミュレーション用).
    // 将来 (Step 25+) UNIT-302 BendingMagnetSim 経由のセンサ読取に置換予定.
    auto inject_actual_current(MagnetCurrent_A actual) noexcept -> void;

    // 設定済 target 電流取得.
    [[nodiscard]] auto current_target() const noexcept -> MagnetCurrent_A;

    // target が一度でも設定済か (DoseManager::is_target_set と同パターン).
    [[nodiscard]] auto is_target_set() const noexcept -> bool;

    // ----- 静的純粋関数 (UT 用) -----

    // 偏差判定: |actual - target| / |target| ≤ tolerance_fraction か.
    // target == 0.0 の場合は「actual も 0.0 のとき true」とする (起動時/Light 時).
    [[nodiscard]] static constexpr auto is_current_within_tolerance(
        MagnetCurrent_A target, MagnetCurrent_A actual,
        double tolerance_fraction = kMagnetToleranceFraction) noexcept -> bool {
        const double t = target.value();
        const double a = actual.value();
        if (t == 0.0) {
            // ゼロ目標時は actual も完全 0 のときのみ in-tolerance.
            return a == 0.0;
        }
        const double diff = a - t;
        const double abs_diff = (diff < 0.0) ? -diff : diff;
        const double abs_t = (t < 0.0) ? -t : t;
        return (abs_diff / abs_t) <= tolerance_fraction;
    }

    // SRS-D-006 範囲チェック (UT 用 + set_current_for_energy 内部使用).
    [[nodiscard]] static constexpr auto is_current_in_range(MagnetCurrent_A current) noexcept
        -> bool {
        return current.value() >= kMagnetCurrentMinA && current.value() <= kMagnetCurrentMaxA;
    }

private:
    // target_current_ / actual_current_ を std::atomic<double> で保持
    // (TurntableManager と同パターン、SAD §9 SEP-003 / RCM-002 / RCM-019).
    std::atomic<double> target_current_{0.0};
    std::atomic<double> actual_current_{0.0};
    std::atomic<bool> target_set_{false};

    // ----- HZ-007 構造的予防 (SDD §7 SOUP-003/004 機能要求) -----
    // UNIT-200/401/201/202/203/204/205 と同パターンを 8 ユニット目に拡大.
    static_assert(std::atomic<double>::is_always_lock_free,
        "std::atomic<double> must be always lock-free for BendingMagnetManager target/actual "
        "(SDD §4.7, RCM-002, HZ-002/HZ-007 structural prevention).");
    static_assert(std::atomic<bool>::is_always_lock_free,
        "std::atomic<bool> must be always lock-free for BendingMagnetManager target_set_ "
        "(SDD §4.7, RCM-002, HZ-002/HZ-007 structural prevention).");
};

}  // namespace th25_ctrl
