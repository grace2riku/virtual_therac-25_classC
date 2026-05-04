// SPDX-License-Identifier: TBD
// TH25-SIM: UNIT-303 TurntableSim ユニット試験 (UTPR-TH25-001 v0.12 §7.2 と整合).
//
// 試験対象: src/th25_sim/include/th25_sim/turntable_sim.hpp /
//            src/th25_sim/src/turntable_sim.cpp (UNIT-303).
// 適用範囲: SDD-TH25-001 v0.1.1 §4.12 で確定された Hardware Simulator 層の
//            3 ユニット目 (UNIT-301/302 と同パターン + 故障注入機能).
//
// IEC 62304 §5.5.4 クラス C 追加受入基準:
//   - 正常系 / 境界値 / 異常系 (故障注入) / 並行処理 / 資源 / データフロー
//
// Therac-25 主要因類型 (SPRP §4.3.1):
//   - A (race condition): 三重 atomic + std::atomic<FaultMode> + UT TSan 並行試験で機械検証.
//   - D (mode/target mismatch hardening): 故障注入 9 通り (3 sensor × 3 fault mode) で
//     UNIT-205 TurntableManager の med-of-3 + has_discrepancy 判定の試験ハーネス基盤を提供.
//   - F (legacy preconditions): is_always_lock_free static_assert 四重表明
//     (build 時検証、12 ユニット目に拡大).
//
// 注: UNIT-303 自体は ErrorCode を返さない (Sim は物理応答模擬 + 故障注入のみ).
// 関連 ErrorCode `TurntableSensorDiscrepancy` 等は UNIT-205 TurntableManager が UNIT-303
// から値取得後に判定 (Inc.1 後半結線時).

#include "th25_sim/turntable_sim.hpp"

#include "th25_ctrl/common_types.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <cmath>
#include <cstdint>
#include <thread>
#include <type_traits>
#include <vector>

namespace th25_sim {

using th25_ctrl::Position_mm;

// ============================================================================
// UT-303-01: 初期状態 (commanded=0、3 系統センサ全て None=正常で 0.0 を返却、noise OFF)
// ============================================================================
TEST(TurntableSim_Initial, AllZero) {
    TurntableSim sim;
    EXPECT_DOUBLE_EQ(sim.current_commanded().value(), 0.0);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor0).value(), 0.0);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor1).value(), 0.0);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor2).value(), 0.0);
    EXPECT_FALSE(sim.is_sensor_noise_enabled());
    EXPECT_EQ(sim.current_fault_mode(SensorId::Sensor0), FaultMode::None);
    EXPECT_EQ(sim.current_fault_mode(SensorId::Sensor1), FaultMode::None);
    EXPECT_EQ(sim.current_fault_mode(SensorId::Sensor2), FaultMode::None);
}

// ============================================================================
// UT-303-02: SRS-D-007 範囲定数 + ノイズ許容率定数 + 3 系統センサ数
// ============================================================================
TEST(TurntableSim_Constants, SrsValues) {
    EXPECT_DOUBLE_EQ(kTurntableMinMm, -100.0);
    EXPECT_DOUBLE_EQ(kTurntableMaxMm, 100.0);
    EXPECT_DOUBLE_EQ(kTurntableSensorNoiseAbsMm, 0.1);
    EXPECT_EQ(kSensorCount, 3U);
}

// ============================================================================
// UT-303-03: command_position 正常系 (SRS-006 所定 3 位置: Electron=0.0)
// ノイズ無効状態で 3 系統センサ全てが commanded を返す.
// ============================================================================
TEST(TurntableSim_Command, NormalElectron) {
    TurntableSim sim;
    sim.command_position(Position_mm{0.0});
    EXPECT_DOUBLE_EQ(sim.current_commanded().value(), 0.0);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor0).value(), 0.0);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor1).value(), 0.0);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor2).value(), 0.0);
}

// ============================================================================
// UT-303-04: command_position 正常系 (SRS-006 XRay=50.0、Light=-50.0)
// ============================================================================
TEST(TurntableSim_Command, NormalXRayAndLight) {
    TurntableSim sim;
    sim.command_position(Position_mm{50.0});
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor0).value(), 50.0);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor1).value(), 50.0);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor2).value(), 50.0);

    sim.command_position(Position_mm{-50.0});
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor0).value(), -50.0);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor1).value(), -50.0);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor2).value(), -50.0);
}

// ============================================================================
// UT-303-05: SRS-D-007 上下限境界 (+100.0 / -100.0)
// ============================================================================
TEST(TurntableSim_Command, BoundaryMaxMin) {
    TurntableSim sim;
    sim.command_position(Position_mm{100.0});
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor0).value(), 100.0);

    sim.command_position(Position_mm{-100.0});
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor0).value(), -100.0);
}

// ============================================================================
// UT-303-06: 範囲外正方向 → clamp 上限 (150.0 → 100.0、物理的飽和模擬)
// ============================================================================
TEST(TurntableSim_Command, OutOfRangePositive) {
    TurntableSim sim;
    sim.command_position(Position_mm{150.0});
    EXPECT_DOUBLE_EQ(sim.current_commanded().value(), 100.0);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor0).value(), 100.0);
}

// ============================================================================
// UT-303-07: 範囲外負方向 → clamp 下限 (-200.0 → -100.0、物理的飽和模擬)
// ============================================================================
TEST(TurntableSim_Command, OutOfRangeNegative) {
    TurntableSim sim;
    sim.command_position(Position_mm{-200.0});
    EXPECT_DOUBLE_EQ(sim.current_commanded().value(), -100.0);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor0).value(), -100.0);
}

// ============================================================================
// UT-303-08: ノイズ無効デフォルト挙動 (read_sensor が commanded をそのまま返す)
// ============================================================================
TEST(TurntableSim_Noise, DefaultDisabled) {
    TurntableSim sim;
    sim.command_position(Position_mm{50.0});
    // 100 回連続読取で全て 50.0 (ノイズ無効).
    for (int i = 0; i < 100; ++i) {
        EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor0).value(), 50.0);
    }
}

// ============================================================================
// UT-303-09: noise enable / disable 状態遷移 (補助 API 動作)
// ============================================================================
TEST(TurntableSim_Noise, EnableDisableCycle) {
    TurntableSim sim;
    EXPECT_FALSE(sim.is_sensor_noise_enabled());
    sim.enable_sensor_noise(42U);
    EXPECT_TRUE(sim.is_sensor_noise_enabled());
    sim.disable_sensor_noise();
    EXPECT_FALSE(sim.is_sensor_noise_enabled());
}

// ============================================================================
// UT-303-10: ノイズ ±0.1 mm 許容範囲機械検証
// commanded=50.0 で 100 回読取り全て 49.9〜50.1 範囲内 (絶対値 ±0.1 mm).
// ============================================================================
TEST(TurntableSim_Noise, AbsTolerance) {
    TurntableSim sim;
    sim.command_position(Position_mm{50.0});
    sim.enable_sensor_noise(123U);

    for (int i = 0; i < 100; ++i) {
        const double v = sim.read_sensor(SensorId::Sensor0).value();
        EXPECT_GE(v, 50.0 - kTurntableSensorNoiseAbsMm)
            << "iter=" << i << " v=" << v;
        EXPECT_LE(v, 50.0 + kTurntableSensorNoiseAbsMm)
            << "iter=" << i << " v=" << v;
    }
}

// ============================================================================
// UT-303-11: noise seed 再現性機械検証
// 同 seed → 同シーケンス、異 seed → 別値 (簡易 LCG 決定論性).
// ============================================================================
TEST(TurntableSim_Noise, SeedReproducibility) {
    constexpr std::uint32_t kSeed1{777U};
    constexpr std::uint32_t kSeed2{888U};
    constexpr int kReadCount{20};

    // seed=777 で 20 回読取 (Sensor0).
    std::vector<double> seq1;
    {
        TurntableSim sim;
        sim.command_position(Position_mm{30.0});
        sim.enable_sensor_noise(kSeed1);
        for (int i = 0; i < kReadCount; ++i) {
            seq1.push_back(sim.read_sensor(SensorId::Sensor0).value());
        }
    }

    // seed=777 で再度 20 回読取 → 完全一致.
    std::vector<double> seq1b;
    {
        TurntableSim sim;
        sim.command_position(Position_mm{30.0});
        sim.enable_sensor_noise(kSeed1);
        for (int i = 0; i < kReadCount; ++i) {
            seq1b.push_back(sim.read_sensor(SensorId::Sensor0).value());
        }
    }

    EXPECT_EQ(seq1, seq1b);  // 同 seed → 同シーケンス

    // seed=888 で 20 回読取 → seq1 と少なくとも 1 つ異なる (異 seed → 別値).
    std::vector<double> seq2;
    {
        TurntableSim sim;
        sim.command_position(Position_mm{30.0});
        sim.enable_sensor_noise(kSeed2);
        for (int i = 0; i < kReadCount; ++i) {
            seq2.push_back(sim.read_sensor(SensorId::Sensor0).value());
        }
    }

    EXPECT_NE(seq1, seq2);
}

// ============================================================================
// UT-303-12: inject_fault StuckAt — 凍結値返却 (3 系統 Sensor0/1/2 各 1 通り)
// inject_stuck_at_value 事前設定 → inject_fault(StuckAt) 後の read_sensor 検証.
// ============================================================================
TEST(TurntableSim_Fault, StuckAtAllSensors) {
    TurntableSim sim;
    sim.command_position(Position_mm{50.0});  // 通常位置

    // 各センサに別の凍結値を設定.
    sim.inject_stuck_at_value(SensorId::Sensor0, Position_mm{10.0});
    sim.inject_stuck_at_value(SensorId::Sensor1, Position_mm{20.0});
    sim.inject_stuck_at_value(SensorId::Sensor2, Position_mm{-30.0});

    // 故障注入前は通常応答 (commanded 50.0 を返す).
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor0).value(), 50.0);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor1).value(), 50.0);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor2).value(), 50.0);

    // 全センサに StuckAt 注入.
    sim.inject_fault(SensorId::Sensor0, FaultMode::StuckAt);
    sim.inject_fault(SensorId::Sensor1, FaultMode::StuckAt);
    sim.inject_fault(SensorId::Sensor2, FaultMode::StuckAt);

    // 故障注入後は凍結値返却.
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor0).value(), 10.0);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor1).value(), 20.0);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor2).value(), -30.0);

    // 故障モードが正しく記録されている.
    EXPECT_EQ(sim.current_fault_mode(SensorId::Sensor0), FaultMode::StuckAt);
    EXPECT_EQ(sim.current_fault_mode(SensorId::Sensor1), FaultMode::StuckAt);
    EXPECT_EQ(sim.current_fault_mode(SensorId::Sensor2), FaultMode::StuckAt);
}

// ============================================================================
// UT-303-13: inject_fault Delay — 前回 commanded_position 返却 (3 系統各 1 通り)
// command_position(A) → command_position(B) → Delay 注入 → read_sensor は A を返す.
// ============================================================================
TEST(TurntableSim_Fault, DelayAllSensors) {
    TurntableSim sim;
    sim.command_position(Position_mm{50.0});   // 1 回目
    sim.command_position(Position_mm{-50.0});  // 2 回目 (50.0 → previous_, -50.0 → commanded_)

    // Delay 注入前は通常応答 (現在の commanded -50.0 を返す).
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor0).value(), -50.0);

    // 全センサに Delay 注入.
    sim.inject_fault(SensorId::Sensor0, FaultMode::Delay);
    sim.inject_fault(SensorId::Sensor1, FaultMode::Delay);
    sim.inject_fault(SensorId::Sensor2, FaultMode::Delay);

    // 全センサ前回 commanded (50.0) を返す.
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor0).value(), 50.0);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor1).value(), 50.0);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor2).value(), 50.0);
}

// ============================================================================
// UT-303-14: inject_fault NoResponse — 0.0 mm 固定返却 (3 系統各 1 通り)
// ============================================================================
TEST(TurntableSim_Fault, NoResponseAllSensors) {
    TurntableSim sim;
    sim.command_position(Position_mm{50.0});  // 通常位置

    // NoResponse 注入前は通常応答.
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor0).value(), 50.0);

    // 全センサに NoResponse 注入.
    sim.inject_fault(SensorId::Sensor0, FaultMode::NoResponse);
    sim.inject_fault(SensorId::Sensor1, FaultMode::NoResponse);
    sim.inject_fault(SensorId::Sensor2, FaultMode::NoResponse);

    // 全センサ 0.0 mm 固定返却.
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor0).value(), 0.0);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor1).value(), 0.0);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor2).value(), 0.0);
}

// ============================================================================
// UT-303-15: 部分的故障注入 — Sensor0 のみ StuckAt、他 2 系統は正常
// Therac-25 Tyler 事故型「単一センサ故障」を構造的検出可能化 (UNIT-205 結線時).
// ============================================================================
TEST(TurntableSim_Fault, PartialStuckAtSensor0Only) {
    TurntableSim sim;
    sim.command_position(Position_mm{50.0});

    sim.inject_stuck_at_value(SensorId::Sensor0, Position_mm{0.0});
    sim.inject_fault(SensorId::Sensor0, FaultMode::StuckAt);

    // Sensor0 のみ凍結値、他 2 系統は通常応答 → UNIT-205 has_discrepancy 検出条件成立.
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor0).value(), 0.0);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor1).value(), 50.0);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor2).value(), 50.0);
}

// ============================================================================
// UT-303-16: 故障モードの混在 — Sensor0=StuckAt、Sensor1=Delay、Sensor2=NoResponse
// 3 系統故障注入 9 通りの組合せのうち代表的混在パターンを網羅.
// ============================================================================
TEST(TurntableSim_Fault, MixedFaultModes) {
    TurntableSim sim;
    sim.command_position(Position_mm{30.0});   // 1 回目 (previous 用)
    sim.command_position(Position_mm{60.0});   // 2 回目 (現在 commanded)

    sim.inject_stuck_at_value(SensorId::Sensor0, Position_mm{99.0});
    sim.inject_fault(SensorId::Sensor0, FaultMode::StuckAt);
    sim.inject_fault(SensorId::Sensor1, FaultMode::Delay);
    sim.inject_fault(SensorId::Sensor2, FaultMode::NoResponse);

    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor0).value(), 99.0);  // StuckAt
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor1).value(), 30.0);  // Delay (前回)
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor2).value(), 0.0);   // NoResponse
}

// ============================================================================
// UT-303-17: 故障モード解除 — None で再度通常応答に戻る
// ============================================================================
TEST(TurntableSim_Fault, RecoverToNone) {
    TurntableSim sim;
    sim.command_position(Position_mm{50.0});
    sim.inject_stuck_at_value(SensorId::Sensor0, Position_mm{10.0});
    sim.inject_fault(SensorId::Sensor0, FaultMode::StuckAt);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor0).value(), 10.0);

    sim.inject_fault(SensorId::Sensor0, FaultMode::None);
    EXPECT_EQ(sim.current_fault_mode(SensorId::Sensor0), FaultMode::None);
    EXPECT_DOUBLE_EQ(sim.read_sensor(SensorId::Sensor0).value(), 50.0);
}

// ============================================================================
// UT-303-18: 静的純粋関数 is_position_in_range / clamp_to_range / sensor_index
// constexpr 性確認も合わせて static_assert.
// ============================================================================
TEST(TurntableSim_StaticFns, RangeAndIndex) {
    EXPECT_TRUE(TurntableSim::is_position_in_range(Position_mm{0.0}));
    EXPECT_TRUE(TurntableSim::is_position_in_range(Position_mm{100.0}));
    EXPECT_TRUE(TurntableSim::is_position_in_range(Position_mm{-100.0}));
    EXPECT_FALSE(TurntableSim::is_position_in_range(Position_mm{100.01}));
    EXPECT_FALSE(TurntableSim::is_position_in_range(Position_mm{-100.01}));

    EXPECT_DOUBLE_EQ(
        TurntableSim::clamp_to_range(Position_mm{50.0}).value(), 50.0);
    EXPECT_DOUBLE_EQ(
        TurntableSim::clamp_to_range(Position_mm{200.0}).value(), 100.0);
    EXPECT_DOUBLE_EQ(
        TurntableSim::clamp_to_range(Position_mm{-200.0}).value(), -100.0);

    static_assert(
        TurntableSim::clamp_to_range(Position_mm{50.0}) == Position_mm{50.0});
    static_assert(
        TurntableSim::clamp_to_range(Position_mm{200.0}) == Position_mm{100.0});

    static_assert(TurntableSim::sensor_index(SensorId::Sensor0) == 0U);
    static_assert(TurntableSim::sensor_index(SensorId::Sensor1) == 1U);
    static_assert(TurntableSim::sensor_index(SensorId::Sensor2) == 2U);
}

// ============================================================================
// UT-303-19: 強い型 + enum compile-time 隔離
// Position_mm は double から暗黙変換不可 + SensorId/FaultMode は uint8_t/int から暗黙変換不可.
// ============================================================================
TEST(TurntableSim_StrongTypes, CompileTimeIsolation) {
    static_assert(!std::is_convertible_v<double, Position_mm>);
    static_assert(!std::is_convertible_v<int, SensorId>);
    static_assert(!std::is_convertible_v<int, FaultMode>);
    static_assert(!std::is_convertible_v<std::uint8_t, SensorId>);
    static_assert(!std::is_convertible_v<std::uint8_t, FaultMode>);
    SUCCEED();
}

// ============================================================================
// UT-303-20: コピー/ムーブ禁止 (compile-time)
// ============================================================================
TEST(TurntableSim_Ownership, NoCopyNoMove) {
    static_assert(!std::is_copy_constructible_v<TurntableSim>);
    static_assert(!std::is_copy_assignable_v<TurntableSim>);
    static_assert(!std::is_move_constructible_v<TurntableSim>);
    static_assert(!std::is_move_assignable_v<TurntableSim>);
    SUCCEED();
}

// ============================================================================
// UT-303-21: HZ-007 lock-free 表明 (build 時 static_assert、四重表明)
// ============================================================================
TEST(TurntableSim_HZ007, AtomicIsLockFree) {
    static_assert(std::atomic<double>::is_always_lock_free);
    static_assert(std::atomic<bool>::is_always_lock_free);
    static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
    static_assert(std::atomic<FaultMode>::is_always_lock_free);
    SUCCEED();
}

// ============================================================================
// UT-303-22: SDD §4.12 表との網羅検証
// FaultMode 4 種 × 代表的 commanded 値で commanded → read_sensor 往復が正しく動作.
// ============================================================================
TEST(TurntableSim_Matrix, SddTableCoverage) {
    struct Case {
        double previous_cmd;
        double current_cmd;
        FaultMode mode;
        double stuck_at;
        double expected_sensor;
        const char* description;
    };
    constexpr Case kCases[] = {
        // FaultMode::None: commanded をそのまま返す
        {0.0, 0.0, FaultMode::None, 0.0, 0.0, "None@0"},
        {0.0, 50.0, FaultMode::None, 0.0, 50.0, "None@50 (XRay)"},
        {0.0, -50.0, FaultMode::None, 0.0, -50.0, "None@-50 (Light)"},
        {0.0, 100.0, FaultMode::None, 0.0, 100.0, "None@100 (boundary)"},
        // FaultMode::StuckAt: stuck_at を返す
        {0.0, 50.0, FaultMode::StuckAt, 25.0, 25.0, "StuckAt@25 with cmd=50"},
        // FaultMode::Delay: previous_cmd を返す
        {30.0, 60.0, FaultMode::Delay, 0.0, 30.0, "Delay (prev=30, cur=60)"},
        // FaultMode::NoResponse: 0.0 を返す
        {0.0, 50.0, FaultMode::NoResponse, 0.0, 0.0, "NoResponse@cmd=50"},
    };
    for (const auto& c : kCases) {
        TurntableSim sim;
        if (c.previous_cmd != c.current_cmd) {
            sim.command_position(Position_mm{c.previous_cmd});
        }
        sim.command_position(Position_mm{c.current_cmd});
        sim.inject_stuck_at_value(SensorId::Sensor0, Position_mm{c.stuck_at});
        sim.inject_fault(SensorId::Sensor0, c.mode);
        EXPECT_DOUBLE_EQ(
            sim.read_sensor(SensorId::Sensor0).value(), c.expected_sensor)
            << c.description;
    }
}

// ============================================================================
// UT-303-23: 並行処理 — 1 commander + 4 reader race-free
// `tsan` プリセットで race-free 検証 (HZ-002 直接対応、12 ユニット目に拡大).
// ============================================================================
TEST(TurntableSim_Concurrency, CommanderMultiReader) {
    TurntableSim sim;
    sim.command_position(Position_mm{0.0});

    constexpr int kReaders = 4;
    constexpr int kIterations = 5000;
    std::atomic<bool> commander_stop{false};
    std::atomic<std::uint64_t> reader_iters{0};

    // commander: -50.0 と 50.0 を交互に command_position.
    std::thread commander([&]() {
        for (int i = 0; i < kIterations; ++i) {
            const double v = (i % 2 == 0) ? -50.0 : 50.0;
            sim.command_position(Position_mm{v});
        }
        commander_stop.store(true, std::memory_order_release);
    });

    // 4 reader: 3 系統センサを並行に読取.
    std::vector<std::thread> readers;
    readers.reserve(kReaders);
    for (int i = 0; i < kReaders; ++i) {
        readers.emplace_back([&]() {
            while (!commander_stop.load(std::memory_order_acquire)) {
                (void)sim.read_sensor(SensorId::Sensor0);
                (void)sim.read_sensor(SensorId::Sensor1);
                (void)sim.read_sensor(SensorId::Sensor2);
                (void)sim.current_commanded();
                reader_iters.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    commander.join();
    for (auto& r : readers) {
        r.join();
    }

    EXPECT_GT(reader_iters.load(), 0U);
}

// ============================================================================
// UT-303-24: 並行処理 — fault injector + multi reader race-free
// 故障モード切替と 3 系統センサ読取の並行処理を tsan で機械検証.
// ============================================================================
TEST(TurntableSim_Concurrency, FaultInjectorMultiReader) {
    TurntableSim sim;
    sim.command_position(Position_mm{50.0});
    sim.inject_stuck_at_value(SensorId::Sensor0, Position_mm{10.0});

    constexpr int kReaders = 3;
    constexpr int kIterations = 5000;
    std::atomic<bool> injector_stop{false};
    std::atomic<std::uint64_t> reader_iters{0};

    // injector: 故障モードを循環切替.
    std::thread injector([&]() {
        constexpr FaultMode kCycle[] = {
            FaultMode::None, FaultMode::StuckAt, FaultMode::Delay,
            FaultMode::NoResponse,
        };
        for (int i = 0; i < kIterations; ++i) {
            sim.inject_fault(SensorId::Sensor0, kCycle[i % 4]);
        }
        injector_stop.store(true, std::memory_order_release);
    });

    // 3 reader: Sensor0 を並行に読取.
    std::vector<std::thread> readers;
    readers.reserve(kReaders);
    for (int i = 0; i < kReaders; ++i) {
        readers.emplace_back([&]() {
            while (!injector_stop.load(std::memory_order_acquire)) {
                (void)sim.read_sensor(SensorId::Sensor0);
                (void)sim.current_fault_mode(SensorId::Sensor0);
                reader_iters.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    injector.join();
    for (auto& r : readers) {
        r.join();
    }

    EXPECT_GT(reader_iters.load(), 0U);
}

}  // namespace th25_sim
