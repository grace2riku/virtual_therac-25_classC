// SPDX-License-Identifier: TBD
// TH25-SIM: UNIT-301 ElectronGunSim ユニット試験 (UTPR-TH25-001 v0.10 §7.2 と整合).
//
// 試験対象: src/th25_sim/include/th25_sim/electron_gun_sim.hpp /
//            src/th25_sim/src/electron_gun_sim.cpp (UNIT-301).
// 適用範囲: SDD-TH25-001 v0.1.1 §4.12 で確定された Hardware Simulator 層の最初の
//            具体実装.
//
// IEC 62304 §5.5.4 クラス C 追加受入基準:
//   - 正常系 / 境界値 / 異常系 / 並行処理 / 資源 / データフロー
//
// Therac-25 主要因類型 (SPRP §4.3.1):
//   - A (race condition): 三重 atomic + UT TSan 並行試験で機械検証.
//   - F (legacy preconditions): is_always_lock_free static_assert 三重表明
//     (build 時検証、10 ユニット目に拡大).
//
// 注: UNIT-301 自体は ErrorCode を返さない (Sim は物理応答模擬のみ). 関連 ErrorCode
// `ElectronGunCurrentOutOfRange` は UNIT-203 BeamController が UNIT-301 から値取得後
// に判定 (Inc.1 後半結線時).

#include "th25_sim/electron_gun_sim.hpp"

#include "th25_ctrl/common_types.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <cmath>
#include <thread>
#include <type_traits>
#include <vector>

namespace th25_sim {

using th25_ctrl::ElectronGunCurrent_mA;

// ============================================================================
// UT-301-01: 初期状態 (commanded=0、actual=0、noise disabled)
// ============================================================================
TEST(ElectronGunSim_Initial, AllZero) {
    ElectronGunSim sim;
    EXPECT_DOUBLE_EQ(sim.current_commanded().value(), 0.0);
    EXPECT_DOUBLE_EQ(sim.read_actual_current().value(), 0.0);
    EXPECT_FALSE(sim.is_noise_enabled());
}

// ============================================================================
// UT-301-02: SRS-D-008 範囲定数 + ノイズ許容率定数
// ============================================================================
TEST(ElectronGunSim_Constants, SrsValues) {
    EXPECT_DOUBLE_EQ(kElectronGunMinMA, 0.0);
    EXPECT_DOUBLE_EQ(kElectronGunMaxMA, 10.0);
    EXPECT_DOUBLE_EQ(kElectronGunNoiseFraction, 0.01);
}

// ============================================================================
// UT-301-03: set_current 正常系 (5.0 mA、ノイズ無効でそのまま反映)
// ============================================================================
TEST(ElectronGunSim_SetCurrent, Normal) {
    ElectronGunSim sim;
    sim.set_current(ElectronGunCurrent_mA{5.0});
    EXPECT_DOUBLE_EQ(sim.current_commanded().value(), 5.0);
    EXPECT_DOUBLE_EQ(sim.read_actual_current().value(), 5.0);
}

// ============================================================================
// UT-301-04: set_current 上限境界 (10.0 mA、SRS-D-008 上限受入)
// ============================================================================
TEST(ElectronGunSim_SetCurrent, BoundaryMax) {
    ElectronGunSim sim;
    sim.set_current(ElectronGunCurrent_mA{10.0});
    EXPECT_DOUBLE_EQ(sim.current_commanded().value(), 10.0);
}

// ============================================================================
// UT-301-05: set_current 下限境界 (0.0 mA、SRS-D-008 下限受入)
// ============================================================================
TEST(ElectronGunSim_SetCurrent, BoundaryMin) {
    ElectronGunSim sim;
    sim.set_current(ElectronGunCurrent_mA{0.0});
    EXPECT_DOUBLE_EQ(sim.current_commanded().value(), 0.0);
}

// ============================================================================
// UT-301-06: set_current 範囲外 正方向 (15.0 mA → clamp 上限 10.0 mA、物理的飽和)
// ============================================================================
TEST(ElectronGunSim_SetCurrent, ClampOverMax) {
    ElectronGunSim sim;
    sim.set_current(ElectronGunCurrent_mA{15.0});
    EXPECT_DOUBLE_EQ(sim.current_commanded().value(), 10.0);
}

// ============================================================================
// UT-301-07: set_current 範囲外 負方向 (-5.0 mA → clamp 下限 0.0 mA、物理的飽和)
// ============================================================================
TEST(ElectronGunSim_SetCurrent, ClampUnderMin) {
    ElectronGunSim sim;
    sim.set_current(ElectronGunCurrent_mA{-5.0});
    EXPECT_DOUBLE_EQ(sim.current_commanded().value(), 0.0);
}

// ============================================================================
// UT-301-08: read_actual_current ノイズ無効 (commanded と一致)
// ============================================================================
TEST(ElectronGunSim_ReadActual, NoiseDisabled) {
    ElectronGunSim sim;
    sim.set_current(ElectronGunCurrent_mA{3.0});
    // ノイズ無効 (default) なら何度呼んでも commanded と一致.
    for (int i = 0; i < 100; ++i) {
        EXPECT_DOUBLE_EQ(sim.read_actual_current().value(), 3.0);
    }
}

// ============================================================================
// UT-301-09: enable_noise / disable_noise / is_noise_enabled の状態遷移
// ============================================================================
TEST(ElectronGunSim_Noise, EnableDisableState) {
    ElectronGunSim sim;
    EXPECT_FALSE(sim.is_noise_enabled());
    sim.enable_noise(42U);
    EXPECT_TRUE(sim.is_noise_enabled());
    sim.disable_noise();
    EXPECT_FALSE(sim.is_noise_enabled());
}

// ============================================================================
// UT-301-10: ノイズ有効時、actual が commanded ±1% 以内
// ============================================================================
TEST(ElectronGunSim_Noise, ActualWithinTolerance) {
    ElectronGunSim sim;
    sim.set_current(ElectronGunCurrent_mA{5.0});
    sim.enable_noise(123U);

    // 100 回読み取り、すべて 5.0 ± 1% (= 4.95〜5.05) に収まることを確認.
    for (int i = 0; i < 100; ++i) {
        const double v = sim.read_actual_current().value();
        EXPECT_GE(v, 5.0 * (1.0 - kElectronGunNoiseFraction));
        EXPECT_LE(v, 5.0 * (1.0 + kElectronGunNoiseFraction));
    }
}

// ============================================================================
// UT-301-11: ノイズ seed 再現性 (同 seed で同値、異 seed で別値)
// ============================================================================
TEST(ElectronGunSim_Noise, SeedReproducibility) {
    constexpr std::uint32_t kSeedA{42U};
    constexpr std::uint32_t kSeedB{1729U};

    // 同 seed の 2 つの Sim から同シーケンスが取れる.
    ElectronGunSim sim_a1;
    ElectronGunSim sim_a2;
    sim_a1.set_current(ElectronGunCurrent_mA{5.0});
    sim_a2.set_current(ElectronGunCurrent_mA{5.0});
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
    ElectronGunSim sim_b;
    sim_b.set_current(ElectronGunCurrent_mA{5.0});
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
// UT-301-12: ノイズ有効でも commanded=0 なら actual=0 (0 × noise = 0)
// ============================================================================
TEST(ElectronGunSim_Noise, ZeroCommandedZeroActual) {
    ElectronGunSim sim;
    sim.set_current(ElectronGunCurrent_mA{0.0});
    sim.enable_noise(7U);
    for (int i = 0; i < 50; ++i) {
        EXPECT_DOUBLE_EQ(sim.read_actual_current().value(), 0.0);
    }
}

// ============================================================================
// UT-301-13: 静的純粋関数 is_current_in_range
// ============================================================================
TEST(ElectronGunSim_StaticPure, IsCurrentInRange) {
    EXPECT_TRUE(ElectronGunSim::is_current_in_range(ElectronGunCurrent_mA{0.0}));
    EXPECT_TRUE(ElectronGunSim::is_current_in_range(ElectronGunCurrent_mA{5.0}));
    EXPECT_TRUE(ElectronGunSim::is_current_in_range(ElectronGunCurrent_mA{10.0}));
    EXPECT_FALSE(ElectronGunSim::is_current_in_range(ElectronGunCurrent_mA{-0.001}));
    EXPECT_FALSE(ElectronGunSim::is_current_in_range(ElectronGunCurrent_mA{10.001}));
    EXPECT_FALSE(ElectronGunSim::is_current_in_range(ElectronGunCurrent_mA{100.0}));

    // constexpr 性確認.
    static_assert(ElectronGunSim::is_current_in_range(ElectronGunCurrent_mA{5.0}));
    static_assert(!ElectronGunSim::is_current_in_range(ElectronGunCurrent_mA{20.0}));
}

// ============================================================================
// UT-301-14: 静的純粋関数 clamp_to_range
// ============================================================================
TEST(ElectronGunSim_StaticPure, ClampToRange) {
    // 範囲内はそのまま.
    EXPECT_DOUBLE_EQ(
        ElectronGunSim::clamp_to_range(ElectronGunCurrent_mA{5.0}).value(), 5.0);
    EXPECT_DOUBLE_EQ(
        ElectronGunSim::clamp_to_range(ElectronGunCurrent_mA{0.0}).value(), 0.0);
    EXPECT_DOUBLE_EQ(
        ElectronGunSim::clamp_to_range(ElectronGunCurrent_mA{10.0}).value(), 10.0);
    // 上限超過は 10.0 に飽和.
    EXPECT_DOUBLE_EQ(
        ElectronGunSim::clamp_to_range(ElectronGunCurrent_mA{15.0}).value(), 10.0);
    EXPECT_DOUBLE_EQ(
        ElectronGunSim::clamp_to_range(ElectronGunCurrent_mA{1000.0}).value(), 10.0);
    // 下限割れは 0.0 に飽和.
    EXPECT_DOUBLE_EQ(
        ElectronGunSim::clamp_to_range(ElectronGunCurrent_mA{-1.0}).value(), 0.0);
    EXPECT_DOUBLE_EQ(
        ElectronGunSim::clamp_to_range(ElectronGunCurrent_mA{-100.0}).value(), 0.0);

    // constexpr 性確認.
    static_assert(
        ElectronGunSim::clamp_to_range(ElectronGunCurrent_mA{5.0}) ==
        ElectronGunCurrent_mA{5.0});
    static_assert(
        ElectronGunSim::clamp_to_range(ElectronGunCurrent_mA{15.0}) ==
        ElectronGunCurrent_mA{10.0});
}

// ============================================================================
// UT-301-15: 強い型 compile-time 隔離 (ElectronGunCurrent_mA は double から暗黙変換不可)
// ============================================================================
TEST(ElectronGunSim_StrongTypes, CompileTimeIsolation) {
    static_assert(!std::is_convertible_v<double, ElectronGunCurrent_mA>);
    SUCCEED();
}

// ============================================================================
// UT-301-16: コピー/ムーブ禁止 (compile-time)
// ============================================================================
TEST(ElectronGunSim_Ownership, NoCopyNoMove) {
    static_assert(!std::is_copy_constructible_v<ElectronGunSim>);
    static_assert(!std::is_copy_assignable_v<ElectronGunSim>);
    static_assert(!std::is_move_constructible_v<ElectronGunSim>);
    static_assert(!std::is_move_assignable_v<ElectronGunSim>);
    SUCCEED();
}

// ============================================================================
// UT-301-17: HZ-007 lock-free 表明 (build 時 static_assert、三重表明)
// ============================================================================
TEST(ElectronGunSim_HZ007, AtomicIsLockFree) {
    static_assert(std::atomic<double>::is_always_lock_free);
    static_assert(std::atomic<bool>::is_always_lock_free);
    static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
    SUCCEED();
}

// ============================================================================
// UT-301-18: SDD §4.12 表との網羅検証 (5 ケースの commanded 値 + 各 actual)
// ノイズ無効状態で commanded → actual の往復が SRS-D-008 範囲全域で正しく動作.
// ============================================================================
TEST(ElectronGunSim_Matrix, SddTableCoverage) {
    struct Case {
        double commanded;
        double expected_actual;
    };
    constexpr Case kCases[] = {
        {0.0, 0.0},     // 下限境界
        {2.5, 2.5},     // 1/4 点
        {5.0, 5.0},     // 中央
        {7.5, 7.5},     // 3/4 点
        {10.0, 10.0},   // 上限境界
        {-1.0, 0.0},    // 範囲外負方向 → clamp 下限
        {15.0, 10.0},   // 範囲外正方向 → clamp 上限
    };
    for (const auto& c : kCases) {
        ElectronGunSim sim;
        sim.set_current(ElectronGunCurrent_mA{c.commanded});
        EXPECT_DOUBLE_EQ(sim.read_actual_current().value(), c.expected_actual)
            << "commanded=" << c.commanded;
    }
}

// ============================================================================
// UT-301-19: 並行処理 — 1 setter (set_current) + 多 reader (read_actual_current)
// `tsan` プリセットで race-free 検証 (HZ-002 直接対応、10 ユニット目に拡大).
// ============================================================================
TEST(ElectronGunSim_Concurrency, SetterMultiReader) {
    ElectronGunSim sim;
    sim.set_current(ElectronGunCurrent_mA{2.5});

    constexpr int kReaders = 4;
    constexpr int kIterations = 5000;
    std::atomic<bool> setter_stop{false};
    std::atomic<std::uint64_t> reader_iters{0};

    // setter: 0.0 と 10.0 を交互に set_current (SRS-D-008 範囲内).
    std::thread setter([&]() {
        for (int i = 0; i < kIterations; ++i) {
            const double v = (i % 2 == 0) ? 0.0 : 10.0;
            sim.set_current(ElectronGunCurrent_mA{v});
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
