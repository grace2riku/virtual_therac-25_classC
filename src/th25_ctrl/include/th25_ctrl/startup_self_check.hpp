// SPDX-License-Identifier: TBD (See LICENSE in repo root once defined)
// TH25-CTRL: UNIT-208 StartupSelfCheck
//
// IEC 62304 Class C, C++20.
// Implementation of SDD-TH25-001 v0.1.1 §4.9 (UNIT-208).
//
// 役割: 起動時 4 項目自己診断 (SRS-012).
// **RCM-013(起動時状態検証)中核 + HZ-009(電源喪失再起動時不定動作)部分対応**.
//
// SDD §4.9 公開 API:
//   - perform_self_check() -> Result<void, ErrorCode> : 4 項目全 Pass で成功
//
// 自己診断 4 項目 (SRS-012):
//   1. 電子銃電流確認: 0.0 mA であること (abs < 0.01 mA)
//   2. ターンテーブル位置確認: Light 位置 (-50.0 mm) ±0.5 mm 以内、3 系統偏差 < 1.0 mm
//   3. ベンディング磁石電流確認: 0.0 A であること (abs < 0.01 A)
//   4. ドーズ積算カウンタ初期化確認: 累積ドーズ = 0.0 cGy
//
// SRS-012 「StartupSelfCheckFailed」エラーは意味的名称. 実装では 4 項目それぞれ
// 最も意味的に適切な既存 ErrorCode を返却することで、`inc1-design-frozen` を
// 維持しつつ SRS-UX-001(詳細エラーメッセージ + 推奨対処)に整合:
//   - 電子銃 NG       -> ElectronGunCurrentOutOfRange (Beam 系 0x02, Critical)
//   - ターンテーブル NG -> TurntableOutOfPosition       (Turntable 系 0x04, High)
//   - 磁石 NG          -> MagnetCurrentDeviation        (Magnet 系 0x05, Medium)
//   - ドーズ NG        -> InternalUnexpectedState       (Internal 系 0xFF, Critical)
//
// 補助 API (UT / シミュレーション用):
//   - inject_electron_gun_current(ElectronGunCurrent_mA) : 電子銃計測値を強制設定
//                                                          (将来 UNIT-301 ElectronGunSim
//                                                          結線時はセンサ実読取に置換、
//                                                          Step 25 では UT 駆動可能)
//   - current_electron_gun_current() : 設定済電子銃計測値取得
//
// 静的純粋関数 (UT 用):
//   - is_electron_gun_in_zero(ElectronGunCurrent_mA, tolerance) : abs < tolerance か
//   - is_bending_magnet_in_zero(MagnetCurrent_A, tolerance)     : 同上
//   - is_dose_zero(DoseUnit_cGy)                                : == 0.0 cGy か
//
// SDD §4.9 設計判断 (Step 25 範囲):
//   - 依存先 Manager 群 (UNIT-205 TurntableManager / UNIT-206 BendingMagnetManager /
//     UNIT-204 DoseManager) を参照保持し、perform_self_check 内部で各 Manager の
//     read-only API (read_position / current_actual / current_accumulated) を呼出.
//   - 電子銃計測値は UNIT-301 ElectronGunSim 未実装のため、内部 atomic に保持し
//     inject_electron_gun_current で UT 駆動可能 (将来 UNIT-301 経由のセンサ
//     実読取に置換、API 不変).
//   - ターンテーブル Light 位置判定は UNIT-205::is_in_position(kLightPositionMm) を
//     直接利用. 「±0.5 mm 以内」+「3 系統偏差 < 1.0 mm」の両方の判定を含む
//     (SDD §4.9 + UNIT-205 構造の重ね合わせで構造的に達成).
//
// Step 25 範囲制約:
//   SDD §4.9「ElectronGunSim から計測値読取」は UNIT-301 が未実装のため、
//   inject_electron_gun_current で UT 駆動可能な構造を成立させるまで. 実 IPC
//   経由センサ実読取は Step 26+ で UNIT-301/402 結線時.
//   UNIT-201::init_subsystems() からの呼出 dispatch は Inc.1 後半 (observer
//   pattern + Manager 結線) で完成.
//
// Therac-25 hazard mapping:
//   - HZ-009 (power-loss / restart undefined behavior): 起動時 4 項目自己診断で
//     不定状態のまま治療開始することを構造的に拒否 (Inc.4 で電源喪失再起動シナリオを
//     完全カバー予定).
//   - HZ-001 (mode/target mismatch): ターンテーブル Light 位置確認で「電子線モード
//     設定だがターンテーブルが X 線位置」の発現経路を起動時に拒否.
//   - HZ-002 (race condition): std::atomic<double> + UT 並行 TSan で機械検証.
//   - HZ-007 (legacy preconditions): static_assert(std::atomic<double>::is_always_lock_free)
//     を 9 ユニット目に拡大 (UNIT-200/401/201/202/203/204/205/206/208).

#pragma once

#include "th25_ctrl/bending_magnet_manager.hpp"
#include "th25_ctrl/common_types.hpp"
#include "th25_ctrl/dose_manager.hpp"
#include "th25_ctrl/turntable_manager.hpp"

#include <atomic>

namespace th25_ctrl {

// ============================================================================
// 自己診断 許容差定数 (SDD §4.9).
// ============================================================================

// SDD §4.9: 電子銃電流 abs_diff < 0.01 mA.
inline constexpr double kElectronGunZeroToleranceMA{0.01};

// SDD §4.9 で「0.0 A であること」のみ明記、許容差未記載. 実用的に同等の 0.01 A
// を採用 (浮動小数点誤差を吸収).
inline constexpr double kBendingMagnetZeroToleranceA{0.01};

// ============================================================================
// StartupSelfCheck (SDD §4.9 UNIT-208、RCM-013 中核 + HZ-009 部分対応).
// ============================================================================
class StartupSelfCheck {
public:
    // Manager 群への参照を保持. 各 Manager の lifetime は本クラスより長くなければ
    // ならない (UNIT-201 SafetyCoreOrchestrator が両者を所有する設計、Inc.1 後半).
    StartupSelfCheck(
        TurntableManager& turntable,
        BendingMagnetManager& bending_magnet,
        DoseManager& dose_manager) noexcept;

    // 所有権を一意に保つためコピー/ムーブ禁止 (内部 atomic + 参照保持).
    StartupSelfCheck(const StartupSelfCheck&) = delete;
    StartupSelfCheck(StartupSelfCheck&&) = delete;
    auto operator=(const StartupSelfCheck&) -> StartupSelfCheck& = delete;
    auto operator=(StartupSelfCheck&&) -> StartupSelfCheck& = delete;
    ~StartupSelfCheck() = default;

    // ----- SDD §4.9 公開 API -----

    // 起動時 4 項目自己診断を順に実施.
    // 事前条件: プロセス起動直後 (UNIT-201::init_subsystems() から呼出予定、
    //   Step 25 範囲では呼出側責任).
    // 事後条件: 4 項目全 Pass で Result::ok(). 1 項目でも NG なら **早期リターン**
    //   で該当 ErrorCode を返却 (残りはスキップ).
    // 早期リターン順:
    //   1. 電子銃        -> ElectronGunCurrentOutOfRange (Critical)
    //   2. ターンテーブル -> TurntableOutOfPosition       (High)
    //   3. ベンディング磁石 -> MagnetCurrentDeviation        (Medium)
    //   4. ドーズカウンタ  -> InternalUnexpectedState       (Critical)
    //
    // 設計判断: Severity が高い順 (Critical -> High -> Medium -> Critical) ではなく、
    // 物理的なビーム経路順 (上流 -> 下流) で確認する. 上流が NG なら下流確認は
    // 無意味なため早期リターンは順序的に正当.
    [[nodiscard]] auto perform_self_check() const noexcept -> Result<void, ErrorCode>;

    // ----- 補助 API (UT / シミュレーション用) -----

    // 電子銃計測値を強制設定 (UT 駆動 / シミュレーション用).
    // 将来 (Step 26+) UNIT-301 ElectronGunSim 経由のセンサ読取に置換予定.
    auto inject_electron_gun_current(ElectronGunCurrent_mA actual) noexcept -> void;

    // 設定済電子銃計測値取得 (read-only, atomic acquire load).
    [[nodiscard]] auto current_electron_gun_current() const noexcept -> ElectronGunCurrent_mA;

    // ----- 静的純粋関数 (UT 用) -----

    // 電子銃電流が 0 (許容差内) か.
    [[nodiscard]] static constexpr auto is_electron_gun_in_zero(
        ElectronGunCurrent_mA current,
        double tolerance_ma = kElectronGunZeroToleranceMA) noexcept -> bool {
        const double v = current.value();
        const double abs_v = (v < 0.0) ? -v : v;
        return abs_v < tolerance_ma;
    }

    // ベンディング磁石電流が 0 (許容差内) か.
    [[nodiscard]] static constexpr auto is_bending_magnet_in_zero(
        MagnetCurrent_A current,
        double tolerance_a = kBendingMagnetZeroToleranceA) noexcept -> bool {
        const double v = current.value();
        const double abs_v = (v < 0.0) ? -v : v;
        return abs_v < tolerance_a;
    }

    // 累積ドーズが 0 か (浮動小数点誤差なし、PulseCounter * rate_ で 0 は正確).
    [[nodiscard]] static constexpr auto is_dose_zero(DoseUnit_cGy dose) noexcept -> bool {
        return dose.value() == 0.0;
    }

private:
    // Manager 群への参照. lifetime 保証は呼出側 (UNIT-201) の責任.
    TurntableManager& turntable_;
    BendingMagnetManager& bending_magnet_;
    DoseManager& dose_manager_;

    // 電子銃計測値 (UNIT-301 ElectronGunSim 未実装のため inject API で設定).
    // 並行アクセス保護: std::atomic<double> 1 個 (release-acquire).
    std::atomic<double> electron_gun_current_ma_{0.0};

    // ----- HZ-007 構造的予防 (SDD §7 SOUP-003/004 機能要求) -----
    // UNIT-200/401/201/202/203/204/205/206 と同パターンを 9 ユニット目に拡大.
    static_assert(std::atomic<double>::is_always_lock_free,
        "std::atomic<double> must be always lock-free for StartupSelfCheck "
        "electron_gun_current_ma_ "
        "(SDD §4.9, RCM-002, HZ-002/HZ-007 structural prevention).");
};

}  // namespace th25_ctrl
