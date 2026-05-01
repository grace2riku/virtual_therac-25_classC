// SPDX-License-Identifier: TBD
// TH25-CTRL: UNIT-208 StartupSelfCheck ユニット試験 (UTPR-TH25-001 v0.9 §7.2 と整合).
//
// 試験対象: src/th25_ctrl/include/th25_ctrl/startup_self_check.hpp /
//            src/th25_ctrl/src/startup_self_check.cpp (UNIT-208).
// 適用範囲: SDD-TH25-001 v0.1.1 §4.9 で確定された RCM-013 中核 + HZ-009 部分対応.
//
// IEC 62304 §5.5.4 クラス C 追加受入基準:
//   - 正常系 / 境界値 / 異常系 / 並行処理 / 資源 / データフロー
//
// Therac-25 主要因類型 (SPRP §4.3.1):
//   - A (race condition): std::atomic<double> + UT TSan 並行試験で機械検証.
//   - D (interlock missing): 4 項目自己診断による初期化ガードで「起動時状態が
//     不定なまま治療開始」類型を構造的に拒否 (UT-208-XX で機械検証).
//   - F (legacy preconditions): is_always_lock_free static_assert (build 時検証).
//
// 注: SRS-012「StartupSelfCheckFailed」エラーは意味的名称、実装では 4 項目それぞれ
// 既存 ErrorCode 階層で分類 (ElectronGunCurrentOutOfRange / TurntableOutOfPosition /
// MagnetCurrentDeviation / InternalUnexpectedState).

#include "th25_ctrl/startup_self_check.hpp"

#include "th25_ctrl/bending_magnet_manager.hpp"
#include "th25_ctrl/common_types.hpp"
#include "th25_ctrl/dose_manager.hpp"
#include "th25_ctrl/turntable_manager.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <type_traits>
#include <vector>

namespace th25_ctrl {

namespace {

// 既定 rate (UNIT-204 DoseManager コンストラクタ用、SRS-009 校正値).
constexpr DoseRatePerPulse_cGy_per_pulse kDefaultRate{0.05};

// テスト用ヘルパ: Manager 群を初期化済 (Light 位置、磁石 0、ドーズ 0、電子銃 0) で
// 起動状態にし、StartupSelfCheck を構築. 4 項目全 Pass を返すべき初期構成.
struct StartupSelfCheckFixture {
    TurntableManager turntable;
    BendingMagnetManager bending_magnet;
    DoseManager dose_manager{kDefaultRate};
    StartupSelfCheck checker{turntable, bending_magnet, dose_manager};

    StartupSelfCheckFixture() {
        // ターンテーブルを Light 位置 (-50.0 mm) で 3 系統一致させる.
        turntable.inject_sensor_readings(kLightPositionMm, kLightPositionMm, kLightPositionMm);
        // 磁石は inject 0 (default は 0、明示的に再設定).
        bending_magnet.inject_actual_current(MagnetCurrent_A{0.0});
        // 電子銃は inject 0 (default は 0、明示的に再設定).
        checker.inject_electron_gun_current(ElectronGunCurrent_mA{0.0});
        // DoseManager は default で accumulated_pulses_ = 0 / target_set_ = false.
    }
};

}  // namespace

// ============================================================================
// UT-208-01: 初期状態 (4 項目すべて起動初期値) → Pass
// ============================================================================
TEST(StartupSelfCheck_Normal, AllInitial) {
    StartupSelfCheckFixture f;
    auto r = f.checker.perform_self_check();
    EXPECT_TRUE(r.has_value());
}

// ============================================================================
// UT-208-02: SRS 定数 (許容差) の値検証
// ============================================================================
TEST(StartupSelfCheck_Constants, SrsValues) {
    EXPECT_DOUBLE_EQ(kElectronGunZeroToleranceMA, 0.01);
    EXPECT_DOUBLE_EQ(kBendingMagnetZeroToleranceA, 0.01);
}

// ============================================================================
// UT-208-03: 電子銃電流 0.005 mA (許容差内) → Pass
// ============================================================================
TEST(StartupSelfCheck_ElectronGun, BoundaryAcceptInside) {
    StartupSelfCheckFixture f;
    f.checker.inject_electron_gun_current(ElectronGunCurrent_mA{0.005});
    auto r = f.checker.perform_self_check();
    EXPECT_TRUE(r.has_value());
}

// ============================================================================
// UT-208-04: 電子銃電流 0.011 mA (許容差超過) → ElectronGunCurrentOutOfRange
// ============================================================================
TEST(StartupSelfCheck_ElectronGun, BoundaryRejectPositive) {
    StartupSelfCheckFixture f;
    f.checker.inject_electron_gun_current(ElectronGunCurrent_mA{0.011});
    auto r = f.checker.perform_self_check();
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::ElectronGunCurrentOutOfRange);
}

// ============================================================================
// UT-208-05: 電子銃電流 -0.011 mA (負方向 許容差超過) → ElectronGunCurrentOutOfRange
// ============================================================================
TEST(StartupSelfCheck_ElectronGun, BoundaryRejectNegative) {
    StartupSelfCheckFixture f;
    f.checker.inject_electron_gun_current(ElectronGunCurrent_mA{-0.011});
    auto r = f.checker.perform_self_check();
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::ElectronGunCurrentOutOfRange);
}

// ============================================================================
// UT-208-06: ターンテーブル Light 位置 (3 系統一致) → Pass
// (UT-208-01 で確認済だが、明示的に確認)
// ============================================================================
TEST(StartupSelfCheck_Turntable, LightPositionPass) {
    StartupSelfCheckFixture f;
    // 既に setUp で Light 位置設定済.
    auto r = f.checker.perform_self_check();
    EXPECT_TRUE(r.has_value());
}

// ============================================================================
// UT-208-07: ターンテーブル Light 位置 ±0.5 mm 境界 (受入)
// ============================================================================
TEST(StartupSelfCheck_Turntable, LightBoundaryAccept) {
    StartupSelfCheckFixture f;
    // -50.0 + 0.5 = -49.5 mm (3 系統一致)
    f.turntable.inject_sensor_readings(
        Position_mm{-49.5}, Position_mm{-49.5}, Position_mm{-49.5});
    auto r = f.checker.perform_self_check();
    EXPECT_TRUE(r.has_value());

    // -50.0 - 0.5 = -50.5 mm (3 系統一致)
    f.turntable.inject_sensor_readings(
        Position_mm{-50.5}, Position_mm{-50.5}, Position_mm{-50.5});
    auto r2 = f.checker.perform_self_check();
    EXPECT_TRUE(r2.has_value());
}

// ============================================================================
// UT-208-08: ターンテーブル Light 位置 ±0.51 mm (拒否) → TurntableOutOfPosition
// ============================================================================
TEST(StartupSelfCheck_Turntable, LightBoundaryReject) {
    StartupSelfCheckFixture f;
    // 偏差 0.51 mm (3 系統一致だが Light から 0.51 mm ずれ)
    f.turntable.inject_sensor_readings(
        Position_mm{-49.49}, Position_mm{-49.49}, Position_mm{-49.49});
    auto r = f.checker.perform_self_check();
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::TurntableOutOfPosition);
}

// ============================================================================
// UT-208-09: ターンテーブル 3 系統不一致 (max-min > 1.0 mm) → TurntableOutOfPosition
// ============================================================================
TEST(StartupSelfCheck_Turntable, ThreeSensorDiscrepancy) {
    StartupSelfCheckFixture f;
    // sensor_0/1 = Light、sensor_2 = 0 mm (故障で原点貼り付き)
    f.turntable.inject_sensor_readings(kLightPositionMm, kLightPositionMm, Position_mm{0.0});
    auto r = f.checker.perform_self_check();
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::TurntableOutOfPosition);
}

// ============================================================================
// UT-208-10: ターンテーブル Electron 位置 (Light でない) → TurntableOutOfPosition
// ============================================================================
TEST(StartupSelfCheck_Turntable, NotAtLight) {
    StartupSelfCheckFixture f;
    f.turntable.inject_sensor_readings(
        kElectronPositionMm, kElectronPositionMm, kElectronPositionMm);
    auto r = f.checker.perform_self_check();
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::TurntableOutOfPosition);
}

// ============================================================================
// UT-208-11: ベンディング磁石 0 A → Pass
// ============================================================================
TEST(StartupSelfCheck_BendingMagnet, ZeroPass) {
    StartupSelfCheckFixture f;
    f.bending_magnet.inject_actual_current(MagnetCurrent_A{0.0});
    auto r = f.checker.perform_self_check();
    EXPECT_TRUE(r.has_value());
}

// ============================================================================
// UT-208-12: ベンディング磁石 0.005 A (許容差内) → Pass
// ============================================================================
TEST(StartupSelfCheck_BendingMagnet, BoundaryAcceptInside) {
    StartupSelfCheckFixture f;
    f.bending_magnet.inject_actual_current(MagnetCurrent_A{0.005});
    auto r = f.checker.perform_self_check();
    EXPECT_TRUE(r.has_value());
}

// ============================================================================
// UT-208-13: ベンディング磁石 0.011 A (許容差超過) → MagnetCurrentDeviation
// ============================================================================
TEST(StartupSelfCheck_BendingMagnet, BoundaryReject) {
    StartupSelfCheckFixture f;
    f.bending_magnet.inject_actual_current(MagnetCurrent_A{0.011});
    auto r = f.checker.perform_self_check();
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::MagnetCurrentDeviation);
}

// ============================================================================
// UT-208-14: ベンディング磁石 -0.011 A (負方向超過) → MagnetCurrentDeviation
// ============================================================================
TEST(StartupSelfCheck_BendingMagnet, BoundaryRejectNegative) {
    StartupSelfCheckFixture f;
    f.bending_magnet.inject_actual_current(MagnetCurrent_A{-0.011});
    auto r = f.checker.perform_self_check();
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::MagnetCurrentDeviation);
}

// ============================================================================
// UT-208-15: ドーズ積算カウンタ 0 cGy → Pass
// ============================================================================
TEST(StartupSelfCheck_Dose, ZeroPass) {
    StartupSelfCheckFixture f;
    // Default で accumulated_pulses_ = 0、current_accumulated() = 0.0 cGy.
    auto r = f.checker.perform_self_check();
    EXPECT_TRUE(r.has_value());
}

// ============================================================================
// UT-208-16: ドーズ積算カウンタ != 0 (積算後) → InternalUnexpectedState
// ============================================================================
TEST(StartupSelfCheck_Dose, NonZeroReject) {
    StartupSelfCheckFixture f;
    // パルス積算で累積を 0 でなくする.
    f.dose_manager.on_dose_pulse(PulseCount{100U});
    auto r = f.checker.perform_self_check();
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::InternalUnexpectedState);
}

// ============================================================================
// UT-208-17: 早期リターン順序 (電子銃 NG が最優先で返却される)
// 電子銃 NG + 磁石 NG + ドーズ NG が同時発生した場合、電子銃の ErrorCode が返る
// ことを機械検証 (上流 → 下流の物理順序).
// ============================================================================
TEST(StartupSelfCheck_EarlyReturn, ElectronGunFirst) {
    StartupSelfCheckFixture f;
    f.checker.inject_electron_gun_current(ElectronGunCurrent_mA{0.5});  // NG
    f.bending_magnet.inject_actual_current(MagnetCurrent_A{1.0});       // NG (もし呼ばれたら)
    f.dose_manager.on_dose_pulse(PulseCount{100U});                     // NG (もし呼ばれたら)
    auto r = f.checker.perform_self_check();
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::ElectronGunCurrentOutOfRange);
}

// ============================================================================
// UT-208-18: 早期リターン順序 (ターンテーブル NG が 2 番目で返却される)
// ============================================================================
TEST(StartupSelfCheck_EarlyReturn, TurntableSecond) {
    StartupSelfCheckFixture f;
    // 電子銃は OK、ターンテーブル NG、磁石 NG、ドーズ NG.
    f.turntable.inject_sensor_readings(
        kElectronPositionMm, kElectronPositionMm, kElectronPositionMm);
    f.bending_magnet.inject_actual_current(MagnetCurrent_A{1.0});
    f.dose_manager.on_dose_pulse(PulseCount{100U});
    auto r = f.checker.perform_self_check();
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::TurntableOutOfPosition);
}

// ============================================================================
// UT-208-19: 早期リターン順序 (磁石 NG が 3 番目で返却される)
// ============================================================================
TEST(StartupSelfCheck_EarlyReturn, BendingMagnetThird) {
    StartupSelfCheckFixture f;
    // 電子銃 OK、ターンテーブル OK、磁石 NG、ドーズ NG.
    f.bending_magnet.inject_actual_current(MagnetCurrent_A{1.0});
    f.dose_manager.on_dose_pulse(PulseCount{100U});
    auto r = f.checker.perform_self_check();
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::MagnetCurrentDeviation);
}

// ============================================================================
// UT-208-20: 静的純粋関数 is_electron_gun_in_zero
// ============================================================================
TEST(StartupSelfCheck_StaticPure, IsElectronGunInZero) {
    EXPECT_TRUE(StartupSelfCheck::is_electron_gun_in_zero(ElectronGunCurrent_mA{0.0}));
    EXPECT_TRUE(StartupSelfCheck::is_electron_gun_in_zero(ElectronGunCurrent_mA{0.005}));
    EXPECT_TRUE(StartupSelfCheck::is_electron_gun_in_zero(ElectronGunCurrent_mA{-0.005}));
    EXPECT_TRUE(StartupSelfCheck::is_electron_gun_in_zero(ElectronGunCurrent_mA{0.009}));
    // 境界 0.01 はちょうど (< tolerance なので false).
    EXPECT_FALSE(StartupSelfCheck::is_electron_gun_in_zero(ElectronGunCurrent_mA{0.01}));
    EXPECT_FALSE(StartupSelfCheck::is_electron_gun_in_zero(ElectronGunCurrent_mA{0.02}));
    EXPECT_FALSE(StartupSelfCheck::is_electron_gun_in_zero(ElectronGunCurrent_mA{-0.02}));

    // カスタム閾値.
    EXPECT_TRUE(StartupSelfCheck::is_electron_gun_in_zero(ElectronGunCurrent_mA{0.05}, 0.1));
    EXPECT_FALSE(StartupSelfCheck::is_electron_gun_in_zero(ElectronGunCurrent_mA{0.005}, 0.001));
}

// ============================================================================
// UT-208-21: 静的純粋関数 is_bending_magnet_in_zero
// ============================================================================
TEST(StartupSelfCheck_StaticPure, IsBendingMagnetInZero) {
    EXPECT_TRUE(StartupSelfCheck::is_bending_magnet_in_zero(MagnetCurrent_A{0.0}));
    EXPECT_TRUE(StartupSelfCheck::is_bending_magnet_in_zero(MagnetCurrent_A{0.005}));
    EXPECT_TRUE(StartupSelfCheck::is_bending_magnet_in_zero(MagnetCurrent_A{-0.005}));
    EXPECT_FALSE(StartupSelfCheck::is_bending_magnet_in_zero(MagnetCurrent_A{0.01}));
    EXPECT_FALSE(StartupSelfCheck::is_bending_magnet_in_zero(MagnetCurrent_A{1.0}));
    EXPECT_FALSE(StartupSelfCheck::is_bending_magnet_in_zero(MagnetCurrent_A{-1.0}));

    // カスタム閾値.
    EXPECT_TRUE(StartupSelfCheck::is_bending_magnet_in_zero(MagnetCurrent_A{0.5}, 1.0));
    EXPECT_FALSE(StartupSelfCheck::is_bending_magnet_in_zero(MagnetCurrent_A{0.005}, 0.001));
}

// ============================================================================
// UT-208-22: 静的純粋関数 is_dose_zero
// ============================================================================
TEST(StartupSelfCheck_StaticPure, IsDoseZero) {
    EXPECT_TRUE(StartupSelfCheck::is_dose_zero(DoseUnit_cGy{0.0}));
    EXPECT_FALSE(StartupSelfCheck::is_dose_zero(DoseUnit_cGy{0.001}));
    EXPECT_FALSE(StartupSelfCheck::is_dose_zero(DoseUnit_cGy{1.0}));
    // constexpr 性確認.
    static_assert(StartupSelfCheck::is_dose_zero(DoseUnit_cGy{0.0}));
    static_assert(!StartupSelfCheck::is_dose_zero(DoseUnit_cGy{0.5}));
}

// ============================================================================
// UT-208-23: inject_electron_gun_current + current_electron_gun_current ラウンドトリップ
// ============================================================================
TEST(StartupSelfCheck_Inject, RoundTrip) {
    StartupSelfCheckFixture f;
    f.checker.inject_electron_gun_current(ElectronGunCurrent_mA{2.5});
    EXPECT_DOUBLE_EQ(f.checker.current_electron_gun_current().value(), 2.5);
    f.checker.inject_electron_gun_current(ElectronGunCurrent_mA{-1.5});
    EXPECT_DOUBLE_EQ(f.checker.current_electron_gun_current().value(), -1.5);
}

// ============================================================================
// UT-208-24: コピー/ムーブ禁止 (compile-time)
// ============================================================================
TEST(StartupSelfCheck_Ownership, NoCopyNoMove) {
    static_assert(!std::is_copy_constructible_v<StartupSelfCheck>);
    static_assert(!std::is_copy_assignable_v<StartupSelfCheck>);
    static_assert(!std::is_move_constructible_v<StartupSelfCheck>);
    static_assert(!std::is_move_assignable_v<StartupSelfCheck>);
    SUCCEED();
}

// ============================================================================
// UT-208-25: HZ-007 lock-free 表明 (build 時 static_assert)
// ============================================================================
TEST(StartupSelfCheck_HZ007, AtomicIsLockFree) {
    static_assert(std::atomic<double>::is_always_lock_free);
    SUCCEED();
}

// ============================================================================
// UT-208-26: 強い型 compile-time 隔離
// ============================================================================
TEST(StartupSelfCheck_StrongTypes, CompileTimeIsolation) {
    static_assert(!std::is_convertible_v<double, ElectronGunCurrent_mA>);
    static_assert(!std::is_convertible_v<double, MagnetCurrent_A>);
    static_assert(!std::is_convertible_v<double, DoseUnit_cGy>);
    static_assert(!std::is_convertible_v<ElectronGunCurrent_mA, MagnetCurrent_A>);
    static_assert(!std::is_convertible_v<MagnetCurrent_A, DoseUnit_cGy>);
    SUCCEED();
}

// ============================================================================
// UT-208-27: SDD §4.9 表との網羅機械検証
// 4 項目 × {OK, NG} = 16 ケース (ただし早期リターンを考慮し意味的に網羅).
// ここでは 4 項目それぞれが「単独 NG (他 3 項目 OK)」のパターンと、
// 「全 OK」「全 NG」を検証.
// ============================================================================
TEST(StartupSelfCheck_Matrix, SddTableCoverage) {
    // ケース 1: 全 OK → Pass
    {
        StartupSelfCheckFixture f;
        auto r = f.checker.perform_self_check();
        EXPECT_TRUE(r.has_value()) << "All OK should pass";
    }

    // ケース 2: 電子銃のみ NG → ElectronGunCurrentOutOfRange
    {
        StartupSelfCheckFixture f;
        f.checker.inject_electron_gun_current(ElectronGunCurrent_mA{0.5});
        auto r = f.checker.perform_self_check();
        ASSERT_FALSE(r.has_value());
        EXPECT_EQ(r.error_code(), ErrorCode::ElectronGunCurrentOutOfRange);
    }

    // ケース 3: ターンテーブルのみ NG → TurntableOutOfPosition
    {
        StartupSelfCheckFixture f;
        f.turntable.inject_sensor_readings(
            kElectronPositionMm, kElectronPositionMm, kElectronPositionMm);
        auto r = f.checker.perform_self_check();
        ASSERT_FALSE(r.has_value());
        EXPECT_EQ(r.error_code(), ErrorCode::TurntableOutOfPosition);
    }

    // ケース 4: 磁石のみ NG → MagnetCurrentDeviation
    {
        StartupSelfCheckFixture f;
        f.bending_magnet.inject_actual_current(MagnetCurrent_A{1.0});
        auto r = f.checker.perform_self_check();
        ASSERT_FALSE(r.has_value());
        EXPECT_EQ(r.error_code(), ErrorCode::MagnetCurrentDeviation);
    }

    // ケース 5: ドーズのみ NG → InternalUnexpectedState
    {
        StartupSelfCheckFixture f;
        f.dose_manager.on_dose_pulse(PulseCount{100U});
        auto r = f.checker.perform_self_check();
        ASSERT_FALSE(r.has_value());
        EXPECT_EQ(r.error_code(), ErrorCode::InternalUnexpectedState);
    }

    // ケース 6: 全 NG → 早期リターン (電子銃が最初に NG)
    {
        StartupSelfCheckFixture f;
        f.checker.inject_electron_gun_current(ElectronGunCurrent_mA{0.5});
        f.turntable.inject_sensor_readings(
            kElectronPositionMm, kElectronPositionMm, kElectronPositionMm);
        f.bending_magnet.inject_actual_current(MagnetCurrent_A{1.0});
        f.dose_manager.on_dose_pulse(PulseCount{100U});
        auto r = f.checker.perform_self_check();
        ASSERT_FALSE(r.has_value());
        EXPECT_EQ(r.error_code(), ErrorCode::ElectronGunCurrentOutOfRange);
    }
}

// ============================================================================
// UT-208-28: 防御的 ErrorCode 系統 (4 項目それぞれ正しい系統 + Severity)
// 注: common_types.hpp §6.2 Severity 判定規則を確認:
//   - ElectronGunCurrentOutOfRange (Beam 系 0x02) → Critical
//   - TurntableOutOfPosition       (Turntable 系 0x04) → High
//   - MagnetCurrentDeviation       (Magnet 系 0x05) → Medium
//   - InternalUnexpectedState      (Internal 系 0xFF) → Critical
// ============================================================================
TEST(StartupSelfCheck_ErrorCodes, CategoryAndSeverity) {
    EXPECT_EQ(error_category(ErrorCode::ElectronGunCurrentOutOfRange), 0x02U);
    EXPECT_EQ(severity_of(ErrorCode::ElectronGunCurrentOutOfRange), Severity::Critical);
    EXPECT_EQ(error_category(ErrorCode::TurntableOutOfPosition), 0x04U);
    EXPECT_EQ(severity_of(ErrorCode::TurntableOutOfPosition), Severity::High);
    EXPECT_EQ(error_category(ErrorCode::MagnetCurrentDeviation), 0x05U);
    EXPECT_EQ(severity_of(ErrorCode::MagnetCurrentDeviation), Severity::Medium);
    EXPECT_EQ(error_category(ErrorCode::InternalUnexpectedState), 0xFFU);
    EXPECT_EQ(severity_of(ErrorCode::InternalUnexpectedState), Severity::Critical);
}

// ============================================================================
// UT-208-29: 並行処理 — 1 setter (inject_electron_gun_current) +
// 多 reader (perform_self_check + current_electron_gun_current) を `tsan`
// プリセットで race-free 検証 (HZ-002 直接対応).
// ============================================================================
TEST(StartupSelfCheck_Concurrency, SetterMultiReader) {
    StartupSelfCheckFixture f;

    constexpr int kReaders = 4;
    constexpr int kIterations = 5000;
    std::atomic<bool> setter_stop{false};
    std::atomic<std::uint64_t> reader_iters{0};

    // setter: 0.0 と 0.005 (許容差内) を交互に inject (Pass を維持).
    std::thread setter([&]() {
        for (int i = 0; i < kIterations; ++i) {
            const double v = (i % 2 == 0) ? 0.0 : 0.005;
            f.checker.inject_electron_gun_current(ElectronGunCurrent_mA{v});
        }
        setter_stop.store(true, std::memory_order_release);
    });

    // 4 reader: perform_self_check + current_electron_gun_current を並行に呼ぶ.
    std::vector<std::thread> readers;
    readers.reserve(kReaders);
    for (int i = 0; i < kReaders; ++i) {
        readers.emplace_back([&]() {
            while (!setter_stop.load(std::memory_order_acquire)) {
                (void)f.checker.perform_self_check();
                (void)f.checker.current_electron_gun_current();
                reader_iters.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    setter.join();
    for (auto& r : readers) {
        r.join();
    }

    EXPECT_GT(reader_iters.load(), 0U);
}

}  // namespace th25_ctrl
