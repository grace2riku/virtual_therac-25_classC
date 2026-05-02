// SPDX-License-Identifier: TBD
// TH25-SIM: UNIT-302 BendingMagnetSim ユニット試験 (UTPR-TH25-001 v0.11 §7.2 と整合).
//
// 試験対象: src/th25_sim/include/th25_sim/bending_magnet_sim.hpp /
//            src/th25_sim/src/bending_magnet_sim.cpp (UNIT-302).
// 適用範囲: SDD-TH25-001 v0.1.1 §4.13 で確定された Hardware Simulator 層の
//            2 ユニット目 (UNIT-301 ElectronGunSim と同パターン).
//
// IEC 62304 §5.5.4 クラス C 追加受入基準:
//   - 正常系 / 境界値 / 異常系 / 並行処理 / 資源 / データフロー
//
// Therac-25 主要因類型 (SPRP §4.3.1):
//   - A (race condition): 三重 atomic + UT TSan 並行試験で機械検証.
//   - F (legacy preconditions): is_always_lock_free static_assert 三重表明
//     (build 時検証、11 ユニット目に拡大).
//
// 注: UNIT-302 自体は ErrorCode を返さない (Sim は物理応答模擬のみ). 関連 ErrorCode
// `MagnetCurrentDeviation` は UNIT-206 BendingMagnetManager が UNIT-302 から値取得後
// に判定 (Inc.1 後半結線時).

#include "th25_sim/bending_magnet_sim.hpp"

#include "th25_ctrl/common_types.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <cmath>
#include <thread>
#include <type_traits>
#include <vector>

namespace th25_sim {

using th25_ctrl::MagnetCurrent_A;

// ============================================================================
// UT-302-01: 初期状態 (commanded=0、actual=0、noise disabled)
// ============================================================================
TEST(BendingMagnetSim_Initial, AllZero) {
    BendingMagnetSim sim;
    EXPECT_DOUBLE_EQ(sim.current_commanded().value(), 0.0);
    EXPECT_DOUBLE_EQ(sim.read_actual_current().value(), 0.0);
    EXPECT_FALSE(sim.is_noise_enabled());
}

// ============================================================================
// UT-302-02: SRS-D-006 範囲定数 + ノイズ許容率定数
// ============================================================================
TEST(BendingMagnetSim_Constants, SrsValues) {
    EXPECT_DOUBLE_EQ(kBendingMagnetMinA, 0.0);
    EXPECT_DOUBLE_EQ(kBendingMagnetMaxA, 500.0);
    EXPECT_DOUBLE_EQ(kBendingMagnetNoiseFraction, 0.02);
}

// ============================================================================
// UT-302-03: set_current 正常系 (250.0 A、ノイズ無効でそのまま反映)
// ============================================================================
TEST(BendingMagnetSim_SetCurrent, Normal) {
    BendingMagnetSim sim;
    sim.set_current(MagnetCurrent_A{250.0});
    EXPECT_DOUBLE_EQ(sim.current_commanded().value(), 250.0);
    EXPECT_DOUBLE_EQ(sim.read_actual_current().value(), 250.0);
}

// ============================================================================
// UT-302-04: set_current 上限境界 (500.0 A、SRS-D-006 上限受入)
// ============================================================================
TEST(BendingMagnetSim_SetCurrent, BoundaryMax) {
    BendingMagnetSim sim;
    sim.set_current(MagnetCurrent_A{500.0});
    EXPECT_DOUBLE_EQ(sim.current_commanded().value(), 500.0);
}

// ============================================================================
// UT-302-05: set_current 下限境界 (0.0 A、SRS-D-006 下限受入)
// ============================================================================
TEST(BendingMagnetSim_SetCurrent, BoundaryMin) {
    BendingMagnetSim sim;
    sim.set_current(MagnetCurrent_A{0.0});
    EXPECT_DOUBLE_EQ(sim.current_commanded().value(), 0.0);
}

// ============================================================================
// UT-302-06: set_current 範囲外 正方向 (750.0 A → clamp 上限 500.0 A、物理的飽和)
// ============================================================================
TEST(BendingMagnetSim_SetCurrent, ClampOverMax) {
    BendingMagnetSim sim;
    sim.set_current(MagnetCurrent_A{750.0});
    EXPECT_DOUBLE_EQ(sim.current_commanded().value(), 500.0);
}

// ============================================================================
// UT-302-07: set_current 範囲外 負方向 (-100.0 A → clamp 下限 0.0 A、物理的飽和)
// ============================================================================
TEST(BendingMagnetSim_SetCurrent, ClampUnderMin) {
    BendingMagnetSim sim;
    sim.set_current(MagnetCurrent_A{-100.0});
    EXPECT_DOUBLE_EQ(sim.current_commanded().value(), 0.0);
}

// ============================================================================
// UT-302-08: read_actual_current ノイズ無効 (commanded と一致)
// ============================================================================
TEST(BendingMagnetSim_ReadActual, NoiseDisabled) {
    BendingMagnetSim sim;
    sim.set_current(MagnetCurrent_A{125.0});
    // ノイズ無効 (default) なら何度呼んでも commanded と一致.
    for (int i = 0; i < 100; ++i) {
        EXPECT_DOUBLE_EQ(sim.read_actual_current().value(), 125.0);
    }
}

// ============================================================================
// UT-302-09: enable_noise / disable_noise / is_noise_enabled の状態遷移
// ============================================================================
TEST(BendingMagnetSim_Noise, EnableDisableState) {
    BendingMagnetSim sim;
    EXPECT_FALSE(sim.is_noise_enabled());
    sim.enable_noise(42U);
    EXPECT_TRUE(sim.is_noise_enabled());
    sim.disable_noise();
    EXPECT_FALSE(sim.is_noise_enabled());
}

// ============================================================================
// UT-302-10: ノイズ有効時、actual が commanded ±2% 以内
// (UNIT-301 ±1% に対し UNIT-302 は ±2%、SDD §4.13 表)
// ============================================================================
TEST(BendingMagnetSim_Noise, ActualWithinTolerance) {
    BendingMagnetSim sim;
    sim.set_current(MagnetCurrent_A{200.0});
    sim.enable_noise(123U);

    // 100 回読み取り、すべて 200.0 ± 2% (= 196.0〜204.0) に収まることを確認.
    for (int i = 0; i < 100; ++i) {
        const double v = sim.read_actual_current().value();
        EXPECT_GE(v, 200.0 * (1.0 - kBendingMagnetNoiseFraction));
        EXPECT_LE(v, 200.0 * (1.0 + kBendingMagnetNoiseFraction));
    }
}

// ============================================================================
// UT-302-11: ノイズ seed 再現性 (同 seed で同値、異 seed で別値)
// ============================================================================
TEST(BendingMagnetSim_Noise, SeedReproducibility) {
    constexpr std::uint32_t kSeedA{42U};
    constexpr std::uint32_t kSeedB{1729U};

    // 同 seed の 2 つの Sim から同シーケンスが取れる.
    BendingMagnetSim sim_a1;
    BendingMagnetSim sim_a2;
    sim_a1.set_current(MagnetCurrent_A{200.0});
    sim_a2.set_current(MagnetCurrent_A{200.0});
    sim_a1.enable_noise(kSeedA);
    sim_a2.enable_noise(kSeedA);

    std::vector<double> seq_a1;
    std::vector<double> seq_a2;
    seq_a1.reserve(20);
    seq_a2.reserve(20);
    for (int i = 0; i < 20; ++i) {
        seq_a1.push_back(sim_a1.read_actual_current().value());
        seq_a2.push_back(sim_a2.read_actual_current().value());
    }
    EXPECT_EQ(seq_a1, seq_a2);

    // 異 seed なら少なくとも 1 つは値が異なる.
    BendingMagnetSim sim_b;
    sim_b.set_current(MagnetCurrent_A{200.0});
    sim_b.enable_noise(kSeedB);
    bool found_diff = false;
    for (int i = 0; i < 20; ++i) {
        const double v_b = sim_b.read_actual_current().value();
        if (v_b != seq_a1[static_cast<std::size_t>(i)]) {
            found_diff = true;
            break;
        }
    }
    EXPECT_TRUE(found_diff);
}

// ============================================================================
// UT-302-12: ノイズ有効でも commanded=0 なら actual=0 (0 × noise = 0)
// ============================================================================
TEST(BendingMagnetSim_Noise, ZeroCommandedZeroActual) {
    BendingMagnetSim sim;
    sim.set_current(MagnetCurrent_A{0.0});
    sim.enable_noise(7U);
    for (int i = 0; i < 50; ++i) {
        EXPECT_DOUBLE_EQ(sim.read_actual_current().value(), 0.0);
    }
}

// ============================================================================
// UT-302-13: 静的純粋関数 is_current_in_range
// ============================================================================
TEST(BendingMagnetSim_StaticPure, IsCurrentInRange) {
    EXPECT_TRUE(BendingMagnetSim::is_current_in_range(MagnetCurrent_A{0.0}));
    EXPECT_TRUE(BendingMagnetSim::is_current_in_range(MagnetCurrent_A{250.0}));
    EXPECT_TRUE(BendingMagnetSim::is_current_in_range(MagnetCurrent_A{500.0}));
    EXPECT_FALSE(BendingMagnetSim::is_current_in_range(MagnetCurrent_A{-0.001}));
    EXPECT_FALSE(BendingMagnetSim::is_current_in_range(MagnetCurrent_A{500.001}));
    EXPECT_FALSE(BendingMagnetSim::is_current_in_range(MagnetCurrent_A{1000.0}));

    // constexpr 性確認.
    static_assert(BendingMagnetSim::is_current_in_range(MagnetCurrent_A{250.0}));
    static_assert(!BendingMagnetSim::is_current_in_range(MagnetCurrent_A{600.0}));
}

// ============================================================================
// UT-302-14: 静的純粋関数 clamp_to_range
// ============================================================================
TEST(BendingMagnetSim_StaticPure, ClampToRange) {
    // 範囲内はそのまま.
    EXPECT_DOUBLE_EQ(
        BendingMagnetSim::clamp_to_range(MagnetCurrent_A{250.0}).value(), 250.0);
    EXPECT_DOUBLE_EQ(
        BendingMagnetSim::clamp_to_range(MagnetCurrent_A{0.0}).value(), 0.0);
    EXPECT_DOUBLE_EQ(
        BendingMagnetSim::clamp_to_range(MagnetCurrent_A{500.0}).value(), 500.0);
    // 上限超過は 500.0 に飽和.
    EXPECT_DOUBLE_EQ(
        BendingMagnetSim::clamp_to_range(MagnetCurrent_A{750.0}).value(), 500.0);
    EXPECT_DOUBLE_EQ(
        BendingMagnetSim::clamp_to_range(MagnetCurrent_A{10000.0}).value(), 500.0);
    // 下限割れは 0.0 に飽和.
    EXPECT_DOUBLE_EQ(
        BendingMagnetSim::clamp_to_range(MagnetCurrent_A{-1.0}).value(), 0.0);
    EXPECT_DOUBLE_EQ(
        BendingMagnetSim::clamp_to_range(MagnetCurrent_A{-1000.0}).value(), 0.0);

    // constexpr 性確認.
    static_assert(
        BendingMagnetSim::clamp_to_range(MagnetCurrent_A{250.0}) ==
        MagnetCurrent_A{250.0});
    static_assert(
        BendingMagnetSim::clamp_to_range(MagnetCurrent_A{750.0}) ==
        MagnetCurrent_A{500.0});
}

// ============================================================================
// UT-302-15: 強い型 compile-time 隔離 (MagnetCurrent_A は double から暗黙変換不可)
// ============================================================================
TEST(BendingMagnetSim_StrongTypes, CompileTimeIsolation) {
    static_assert(!std::is_convertible_v<double, MagnetCurrent_A>);
    SUCCEED();
}

// ============================================================================
// UT-302-16: コピー/ムーブ禁止 (compile-time)
// ============================================================================
TEST(BendingMagnetSim_Ownership, NoCopyNoMove) {
    static_assert(!std::is_copy_constructible_v<BendingMagnetSim>);
    static_assert(!std::is_copy_assignable_v<BendingMagnetSim>);
    static_assert(!std::is_move_constructible_v<BendingMagnetSim>);
    static_assert(!std::is_move_assignable_v<BendingMagnetSim>);
    SUCCEED();
}

// ============================================================================
// UT-302-17: HZ-007 lock-free 表明 (build 時 static_assert、三重表明)
// ============================================================================
TEST(BendingMagnetSim_HZ007, AtomicIsLockFree) {
    static_assert(std::atomic<double>::is_always_lock_free);
    static_assert(std::atomic<bool>::is_always_lock_free);
    static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
    SUCCEED();
}

// ============================================================================
// UT-302-18: SDD §4.13 表との網羅検証 (5 ケースの commanded 値 + 各 actual)
// ノイズ無効状態で commanded → actual の往復が SRS-D-006 範囲全域で正しく動作.
// ============================================================================
TEST(BendingMagnetSim_Matrix, SddTableCoverage) {
    struct Case {
        double commanded;
        double expected_actual;
    };
    constexpr Case kCases[] = {
        {0.0, 0.0},        // 下限境界
        {125.0, 125.0},    // 1/4 点
        {250.0, 250.0},    // 中央
        {375.0, 375.0},    // 3/4 点
        {500.0, 500.0},    // 上限境界
        {-50.0, 0.0},      // 範囲外負方向 → clamp 下限
        {750.0, 500.0},    // 範囲外正方向 → clamp 上限
    };
    for (const auto& c : kCases) {
        BendingMagnetSim sim;
        sim.set_current(MagnetCurrent_A{c.commanded});
        EXPECT_DOUBLE_EQ(sim.read_actual_current().value(), c.expected_actual)
            << "commanded=" << c.commanded;
    }
}

// ============================================================================
// UT-302-19: 並行処理 — 1 setter (set_current) + 多 reader (read_actual_current)
// `tsan` プリセットで race-free 検証 (HZ-002 直接対応、11 ユニット目に拡大).
// ============================================================================
TEST(BendingMagnetSim_Concurrency, SetterMultiReader) {
    BendingMagnetSim sim;
    sim.set_current(MagnetCurrent_A{125.0});

    constexpr int kReaders = 4;
    constexpr int kIterations = 5000;
    std::atomic<bool> setter_stop{false};
    std::atomic<std::uint64_t> reader_iters{0};

    // setter: 0.0 と 500.0 を交互に set_current (SRS-D-006 範囲内).
    std::thread setter([&]() {
        for (int i = 0; i < kIterations; ++i) {
            const double v = (i % 2 == 0) ? 0.0 : 500.0;
            sim.set_current(MagnetCurrent_A{v});
        }
        setter_stop.store(true, std::memory_order_release);
    });

    // 4 reader: read_actual_current + current_commanded を並行に呼ぶ.
    std::vector<std::thread> readers;
    readers.reserve(kReaders);
    for (int i = 0; i < kReaders; ++i) {
        readers.emplace_back([&]() {
            while (!setter_stop.load(std::memory_order_acquire)) {
                (void)sim.read_actual_current();
                (void)sim.current_commanded();
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

}  // namespace th25_sim
