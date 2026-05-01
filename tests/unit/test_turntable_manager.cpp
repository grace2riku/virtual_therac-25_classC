// SPDX-License-Identifier: TBD
// TH25-CTRL: UNIT-205 TurntableManager ユニット試験 (UTPR-TH25-001 v0.7 §7.2 と整合).
//
// 試験対象: src/th25_ctrl/include/th25_ctrl/turntable_manager.hpp /
//            src/th25_ctrl/src/turntable_manager.cpp (UNIT-205).
// 適用範囲: SDD-TH25-001 v0.1.1 §4.6 で確定された HZ-001 直接対応 + RCM-005 中核.
//
// IEC 62304 §5.5.4 クラス C 追加受入基準:
//   - 正常系 / 境界値 / 異常系 / 並行処理 / 資源 / データフロー
//
// Therac-25 主要因類型 (SPRP §4.3.1):
//   - A (race condition): 4 atomic + UT TSan 並行試験で機械検証.
//   - D (interlock missing): 3 系統センサ集約 med-of-3 + 偏差判定で「センサ単一故障」
//     「3 系統不一致」を構造的検出 (Therac-25 Tyler 事故型「ターンテーブル位置不整合」).
//   - F (legacy preconditions): is_always_lock_free static_assert (build 時検証).

#include "th25_ctrl/turntable_manager.hpp"

#include "th25_ctrl/common_types.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <type_traits>
#include <vector>

namespace th25_ctrl {

namespace {

constexpr Position_mm kElectron = kElectronPositionMm;  // 0.0
constexpr Position_mm kXRay = kXRayPositionMm;          // 50.0
constexpr Position_mm kLight = kLightPositionMm;        // -50.0

}  // namespace

// ============================================================================
// UT-205-01: 初期状態 (target=0、全センサ=0)
// ============================================================================
TEST(TurntableManager_Initial, AllZero) {
    TurntableManager tm;
    EXPECT_DOUBLE_EQ(tm.current_target().value(), 0.0);

    auto r = tm.read_position();
    ASSERT_TRUE(r.has_value());
    EXPECT_DOUBLE_EQ(r.value().sensor_0.value(), 0.0);
    EXPECT_DOUBLE_EQ(r.value().sensor_1.value(), 0.0);
    EXPECT_DOUBLE_EQ(r.value().sensor_2.value(), 0.0);
}

// ============================================================================
// UT-205-02: SRS-006 所定 3 位置定数の値
// ============================================================================
TEST(TurntableManager_Constants, SrsPositions) {
    EXPECT_DOUBLE_EQ(kElectronPositionMm.value(), 0.0);
    EXPECT_DOUBLE_EQ(kXRayPositionMm.value(), 50.0);
    EXPECT_DOUBLE_EQ(kLightPositionMm.value(), -50.0);
    EXPECT_DOUBLE_EQ(kInPositionToleranceMm, 0.5);
    EXPECT_DOUBLE_EQ(kSensorDiscrepancyThresholdMm, 1.0);
}

// ============================================================================
// UT-205-03: move_to 正常系 (Electron 位置)
// ============================================================================
TEST(TurntableManager_MoveTo, NormalElectron) {
    TurntableManager tm;
    auto r = tm.move_to(kElectron);
    EXPECT_TRUE(r.has_value());
    EXPECT_DOUBLE_EQ(tm.current_target().value(), 0.0);
}

// ============================================================================
// UT-205-04: move_to 正常系 (XRay 位置)
// ============================================================================
TEST(TurntableManager_MoveTo, NormalXRay) {
    TurntableManager tm;
    auto r = tm.move_to(kXRay);
    EXPECT_TRUE(r.has_value());
    EXPECT_DOUBLE_EQ(tm.current_target().value(), 50.0);
}

// ============================================================================
// UT-205-05: move_to 正常系 (Light 位置)
// ============================================================================
TEST(TurntableManager_MoveTo, NormalLight) {
    TurntableManager tm;
    auto r = tm.move_to(kLight);
    EXPECT_TRUE(r.has_value());
    EXPECT_DOUBLE_EQ(tm.current_target().value(), -50.0);
}

// ============================================================================
// UT-205-06: move_to 範囲境界受入 (-100.0/+100.0 mm、SRS-D-007 境界)
// ============================================================================
TEST(TurntableManager_MoveTo, BoundaryAccept) {
    TurntableManager tm;
    EXPECT_TRUE(tm.move_to(Position_mm{-100.0}).has_value());
    EXPECT_TRUE(tm.move_to(Position_mm{100.0}).has_value());
}

// ============================================================================
// UT-205-07: move_to 範囲外拒否 (-100.001 / +100.001 mm)
// ============================================================================
TEST(TurntableManager_MoveTo, BoundaryReject) {
    TurntableManager tm;
    {
        auto r = tm.move_to(Position_mm{-100.001});
        ASSERT_FALSE(r.has_value());
        EXPECT_EQ(r.error_code(), ErrorCode::TurntableOutOfPosition);
    }
    {
        auto r = tm.move_to(Position_mm{100.001});
        ASSERT_FALSE(r.has_value());
        EXPECT_EQ(r.error_code(), ErrorCode::TurntableOutOfPosition);
    }
}

// ============================================================================
// UT-205-08: inject_sensor_readings + read_position
// ============================================================================
TEST(TurntableManager_Sensors, InjectAndRead) {
    TurntableManager tm;
    tm.inject_sensor_readings(Position_mm{50.0}, Position_mm{50.1}, Position_mm{49.9});
    auto r = tm.read_position();
    ASSERT_TRUE(r.has_value());
    EXPECT_DOUBLE_EQ(r.value().sensor_0.value(), 50.0);
    EXPECT_DOUBLE_EQ(r.value().sensor_1.value(), 50.1);
    EXPECT_DOUBLE_EQ(r.value().sensor_2.value(), 49.9);
}

// ============================================================================
// UT-205-09: compute_median 静的純粋関数 (3 値中央値)
// ============================================================================
TEST(TurntableManager_StaticPure, ComputeMedian) {
    // 既ソート / 逆順 / 中央が小 / 中央が大 のパターンを網羅.
    EXPECT_DOUBLE_EQ(TurntableManager::compute_median(1.0, 2.0, 3.0), 2.0);
    EXPECT_DOUBLE_EQ(TurntableManager::compute_median(3.0, 2.0, 1.0), 2.0);
    EXPECT_DOUBLE_EQ(TurntableManager::compute_median(2.0, 1.0, 3.0), 2.0);
    EXPECT_DOUBLE_EQ(TurntableManager::compute_median(2.0, 3.0, 1.0), 2.0);
    EXPECT_DOUBLE_EQ(TurntableManager::compute_median(1.0, 3.0, 2.0), 2.0);
    EXPECT_DOUBLE_EQ(TurntableManager::compute_median(3.0, 1.0, 2.0), 2.0);
    // 同値ケース.
    EXPECT_DOUBLE_EQ(TurntableManager::compute_median(5.0, 5.0, 5.0), 5.0);
    EXPECT_DOUBLE_EQ(TurntableManager::compute_median(5.0, 5.0, 1.0), 5.0);
    EXPECT_DOUBLE_EQ(TurntableManager::compute_median(1.0, 5.0, 5.0), 5.0);
}

// ============================================================================
// UT-205-10: has_discrepancy_values 静的純粋関数
// ============================================================================
TEST(TurntableManager_StaticPure, HasDiscrepancyValues) {
    // 全一致 → false.
    EXPECT_FALSE(TurntableManager::has_discrepancy_values(50.0, 50.0, 50.0, 1.0));
    // 微小ばらつき (0.5 mm) → false.
    EXPECT_FALSE(TurntableManager::has_discrepancy_values(50.0, 50.5, 50.0, 1.0));
    // 1.0 mm 境界 → false (max - min = 1.0 は超 *ない*、has_discrepancy は >).
    EXPECT_FALSE(TurntableManager::has_discrepancy_values(49.5, 50.5, 50.0, 1.0));
    // 1.001 mm 超 → true.
    EXPECT_TRUE(TurntableManager::has_discrepancy_values(49.5, 50.501, 50.0, 1.0));
    // 1 系統故障 (sensor_2 が 0 mm に貼り付き) → true.
    EXPECT_TRUE(TurntableManager::has_discrepancy_values(50.0, 50.0, 0.0, 1.0));
}

// ============================================================================
// UT-205-11: TurntablePosition::median メンバ関数
// ============================================================================
TEST(TurntablePosition_Median, ReturnsMedOf3) {
    TurntablePosition pos{Position_mm{49.5}, Position_mm{50.0}, Position_mm{50.5}};
    EXPECT_DOUBLE_EQ(pos.median().value(), 50.0);

    TurntablePosition pos2{Position_mm{0.0}, Position_mm{50.0}, Position_mm{-50.0}};
    EXPECT_DOUBLE_EQ(pos2.median().value(), 0.0);
}

// ============================================================================
// UT-205-12: TurntablePosition::has_discrepancy メンバ関数
// ============================================================================
TEST(TurntablePosition_HasDiscrepancy, DefaultThreshold) {
    TurntablePosition tight{Position_mm{50.0}, Position_mm{50.3}, Position_mm{49.7}};
    EXPECT_FALSE(tight.has_discrepancy());  // 0.6 mm < 1.0 mm

    TurntablePosition loose{Position_mm{50.0}, Position_mm{52.0}, Position_mm{48.0}};
    EXPECT_TRUE(loose.has_discrepancy());  // 4.0 mm > 1.0 mm

    // カスタム閾値.
    EXPECT_TRUE(tight.has_discrepancy(0.5));  // 0.6 mm > 0.5 mm
    EXPECT_FALSE(loose.has_discrepancy(5.0));  // 4.0 mm < 5.0 mm
}

// ============================================================================
// UT-205-13: is_in_position 正常系 (3 系統一致 + median と expected の差 ≤ 0.5 mm)
// ============================================================================
TEST(TurntableManager_IsInPosition, NormalAccept) {
    TurntableManager tm;
    // sensor_0/1/2 = 50.0 (XRay 位置一致), expected = XRay (50.0)
    tm.inject_sensor_readings(kXRay, kXRay, kXRay);
    EXPECT_TRUE(tm.is_in_position(kXRay));
}

// ============================================================================
// UT-205-14: is_in_position 境界 (median と expected の差が 0.5 mm 境界)
// ============================================================================
TEST(TurntableManager_IsInPosition, BoundaryDeviation) {
    TurntableManager tm;
    // 3 系統一致 + median - expected = 0.5 mm → in-position 受入.
    tm.inject_sensor_readings(Position_mm{50.5}, Position_mm{50.5}, Position_mm{50.5});
    EXPECT_TRUE(tm.is_in_position(kXRay));

    // 3 系統一致 + median - expected = 0.51 mm → 拒否.
    tm.inject_sensor_readings(Position_mm{50.51}, Position_mm{50.51}, Position_mm{50.51});
    EXPECT_FALSE(tm.is_in_position(kXRay));
}

// ============================================================================
// UT-205-15: is_in_position 3 系統不一致 → false (Therac-25 Tyler 事故型対応)
// ============================================================================
TEST(TurntableManager_IsInPosition, DiscrepancyRejected) {
    TurntableManager tm;
    // 3 系統最大-最小差 = 1.5 mm > 1.0 mm threshold → in-position 拒否.
    tm.inject_sensor_readings(
        Position_mm{50.0}, Position_mm{51.5}, Position_mm{50.0});
    EXPECT_FALSE(tm.is_in_position(kXRay));

    // たとえ median が expected と一致していても、3 系統不一致なら拒否.
    tm.inject_sensor_readings(
        Position_mm{49.0}, Position_mm{50.0}, Position_mm{51.0});
    // max - min = 2.0 mm > 1.0 mm → 拒否 (median = 50.0 だが).
    EXPECT_FALSE(tm.is_in_position(kXRay));
}

// ============================================================================
// UT-205-16: is_in_position センサ単一故障 (Therac-25 Tyler 事故型: D 主要因)
// ============================================================================
TEST(TurntableManager_IsInPosition, SingleSensorFault) {
    TurntableManager tm;
    // sensor_0/1 = XRay 位置、sensor_2 = 0 (故障で原点貼り付き) → 3 系統不一致で拒否.
    tm.inject_sensor_readings(kXRay, kXRay, kElectron);
    EXPECT_FALSE(tm.is_in_position(kXRay));
    // RCM-005 が単一故障を構造的に検出 (median のみでは判別不能だが has_discrepancy で防ぐ).
}

// ============================================================================
// UT-205-17: 全 SRS-006 所定 3 位置で in-position 確認
// ============================================================================
TEST(TurntableManager_IsInPosition, AllThreeSrsPositions) {
    TurntableManager tm;

    // Electron (0.0)
    tm.inject_sensor_readings(kElectron, kElectron, kElectron);
    EXPECT_TRUE(tm.is_in_position(kElectron));
    EXPECT_FALSE(tm.is_in_position(kXRay));  // 50.0 mm 離れているので拒否

    // XRay (50.0)
    tm.inject_sensor_readings(kXRay, kXRay, kXRay);
    EXPECT_TRUE(tm.is_in_position(kXRay));
    EXPECT_FALSE(tm.is_in_position(kElectron));

    // Light (-50.0)
    tm.inject_sensor_readings(kLight, kLight, kLight);
    EXPECT_TRUE(tm.is_in_position(kLight));
    EXPECT_FALSE(tm.is_in_position(kXRay));
}

// ============================================================================
// UT-205-18: is_position_in_range 静的純粋関数の網羅
// ============================================================================
TEST(TurntableManager_IsInRange, StaticPredicate) {
    EXPECT_TRUE(TurntableManager::is_position_in_range(Position_mm{-100.0}));
    EXPECT_TRUE(TurntableManager::is_position_in_range(Position_mm{0.0}));
    EXPECT_TRUE(TurntableManager::is_position_in_range(Position_mm{100.0}));
    EXPECT_FALSE(TurntableManager::is_position_in_range(Position_mm{-100.001}));
    EXPECT_FALSE(TurntableManager::is_position_in_range(Position_mm{100.001}));
    EXPECT_FALSE(TurntableManager::is_position_in_range(Position_mm{-200.0}));
    EXPECT_FALSE(TurntableManager::is_position_in_range(Position_mm{200.0}));
}

// ============================================================================
// UT-205-19: コピー/ムーブ禁止 (compile-time)
// ============================================================================
TEST(TurntableManager_Ownership, NoCopyNoMove) {
    static_assert(!std::is_copy_constructible_v<TurntableManager>);
    static_assert(!std::is_copy_assignable_v<TurntableManager>);
    static_assert(!std::is_move_constructible_v<TurntableManager>);
    static_assert(!std::is_move_assignable_v<TurntableManager>);
    SUCCEED();
}

// ============================================================================
// UT-205-20: HZ-007 lock-free 表明 (build 時 static_assert、UNIT-204 教訓に従い
// runtime is_lock_free() メンバは呼ばない)
// ============================================================================
TEST(TurntableManager_HZ007, AtomicIsLockFree) {
    static_assert(std::atomic<double>::is_always_lock_free);
    SUCCEED();
}

// ============================================================================
// UT-205-21: 強い型 compile-time (Position_mm は double から暗黙変換不可)
// ============================================================================
TEST(TurntableManager_StrongTypes, CompileTimeIsolation) {
    static_assert(!std::is_convertible_v<double, Position_mm>);
    SUCCEED();
}

// ============================================================================
// UT-205-22: SDD §4.6 表 (RCM-005) との網羅機械検証
// 3 系統センサ × {一致 / 微小ばらつき / 不一致} × expected 位置 = 9 ケース
// ============================================================================
TEST(TurntableManager_IsInPosition, MatrixCoverage) {
    struct Case {
        double s0, s1, s2;
        Position_mm expected;
        bool in_position;
    };
    // ※全て expected = XRay (50.0) を基準に評価
    constexpr Case kCases[] = {
        // 全一致 + 偏差 0 → in-position
        {50.0, 50.0, 50.0, kXRay, true},
        // 微小ばらつき (max-min = 0.6 mm < 1.0 mm) + median = 50.0 → in-position
        {49.7, 50.0, 50.3, kXRay, true},
        // 偏差 0.5 mm 境界 + 一致 → in-position
        {50.5, 50.5, 50.5, kXRay, true},
        // 偏差 0.51 mm + 一致 → 拒否
        {50.51, 50.51, 50.51, kXRay, false},
        // max - min = 1.5 mm 不一致 → 拒否
        {49.0, 50.5, 49.5, kXRay, false},
        // 1 系統 0 mm 故障 → 拒否
        {50.0, 50.0, 0.0, kXRay, false},
        // 期待位置から大きく外れ → 拒否
        {0.0, 0.0, 0.0, kXRay, false},
        // expected = Electron で 50.0 にあれば拒否
        {50.0, 50.0, 50.0, kElectron, false},
        // expected = Light で -50.0 にあれば in-position
        {-50.0, -50.0, -50.0, kLight, true},
    };
    for (const auto& c : kCases) {
        TurntableManager tm;
        tm.inject_sensor_readings(Position_mm{c.s0}, Position_mm{c.s1}, Position_mm{c.s2});
        EXPECT_EQ(tm.is_in_position(c.expected), c.in_position)
            << "s=" << c.s0 << "," << c.s1 << "," << c.s2
            << " expected=" << c.expected.value();
    }
}

// ============================================================================
// UT-205-23: 並行処理 — 1 writer (inject_sensor_readings) + 多 reader (read_position) を
// `tsan` プリセットで race-free 検証 (HZ-002 直接対応).
// ============================================================================
TEST(TurntableManager_Concurrency, WriterMultiReader) {
    TurntableManager tm;
    tm.inject_sensor_readings(kXRay, kXRay, kXRay);

    constexpr int kReaders = 4;
    constexpr int kIterations = 5000;
    std::atomic<bool> writer_stop{false};
    std::atomic<std::uint64_t> reader_iters{0};

    // writer: 50.0 と 50.5 を交互に inject (in-position 範囲内、3 系統一致).
    std::thread writer([&]() {
        for (int i = 0; i < kIterations; ++i) {
            const double v = (i % 2 == 0) ? 50.0 : 50.5;
            tm.inject_sensor_readings(Position_mm{v}, Position_mm{v}, Position_mm{v});
        }
        writer_stop.store(true, std::memory_order_release);
    });

    // 4 reader: read_position + is_in_position + current_target を並行に呼ぶ.
    std::vector<std::thread> readers;
    readers.reserve(kReaders);
    for (int i = 0; i < kReaders; ++i) {
        readers.emplace_back([&]() {
            while (!writer_stop.load(std::memory_order_acquire)) {
                (void)tm.read_position();
                (void)tm.is_in_position(kXRay);
                (void)tm.current_target();
                reader_iters.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    writer.join();
    for (auto& r : readers) {
        r.join();
    }

    // race-free に reader が走ったことを確認 (TSan で race detection 0 が本試験の本旨).
    EXPECT_GT(reader_iters.load(), 0U);
}

// ============================================================================
// UT-205-24: 並行処理 — move_to と read_position の race-free 検証
// ============================================================================
TEST(TurntableManager_Concurrency, MoveToAndRead) {
    TurntableManager tm;
    tm.inject_sensor_readings(kElectron, kElectron, kElectron);

    constexpr int kIterations = 5000;
    std::atomic<bool> mover_stop{false};

    std::thread mover([&]() {
        for (int i = 0; i < kIterations; ++i) {
            // 所定 3 位置 (範囲内) を交互に指令.
            const auto target = (i % 3 == 0) ? kElectron : ((i % 3 == 1) ? kXRay : kLight);
            (void)tm.move_to(target);
        }
        mover_stop.store(true, std::memory_order_release);
    });

    std::thread reader([&]() {
        while (!mover_stop.load(std::memory_order_acquire)) {
            (void)tm.current_target();
            (void)tm.read_position();
        }
    });

    mover.join();
    reader.join();
    SUCCEED();  // race-free 検証は TSan で行う.
}

// ============================================================================
// UT-205-25: 防御的 ErrorCode 系統 (TurntableOutOfPosition は Turntable 系 0x04)
// 注: common_types.hpp §6.2 Severity 判定規則で Turntable 系 (0x04) → High。
// (Critical は Mode/Beam/Dose/Internal 系のみ)
// ============================================================================
TEST(TurntableManager_ErrorCodes, CategoryConsistency) {
    EXPECT_EQ(error_category(ErrorCode::TurntableOutOfPosition), 0x04U);
    EXPECT_EQ(error_category(ErrorCode::TurntableSensorDiscrepancy), 0x04U);
    EXPECT_EQ(error_category(ErrorCode::TurntableMoveTimeout), 0x04U);
    EXPECT_EQ(severity_of(ErrorCode::TurntableOutOfPosition), Severity::High);
    EXPECT_EQ(severity_of(ErrorCode::TurntableSensorDiscrepancy), Severity::High);
    EXPECT_EQ(severity_of(ErrorCode::TurntableMoveTimeout), Severity::High);
}

}  // namespace th25_ctrl
