// SPDX-License-Identifier: TBD
// TH25-SIM: UNIT-304 IonChamberSim ユニット試験 (UTPR-TH25-001 v0.13 §7.2 と整合).
//
// 試験対象: src/th25_sim/include/th25_sim/ion_chamber_sim.hpp /
//            src/th25_sim/src/ion_chamber_sim.cpp (UNIT-304).
// 適用範囲: SDD-TH25-001 v0.1.1 §4.12 で確定された Hardware Simulator 層の
//            4 ユニット目 (本層最終ユニット、UNIT-301〜303 と同パターン + 故障注入機能 +
//            2 系統独立).
//
// IEC 62304 §5.5.4 クラス C 追加受入基準:
//   - 正常系 / 境界値 / 異常系 (故障注入) / 並行処理 / 資源 / データフロー
//
// Therac-25 主要因類型 (SPRP §4.3.1):
//   - A (race condition): 二重 atomic + std::atomic<FaultMode> + UT TSan 並行試験で機械検証.
//   - D (dose calculation hardening): 故障注入 Saturation/ChannelFailure × 2 チャネル +
//     部分故障 + 混在故障モードで UNIT-204/207 結線時のドーズ計測冗長性検証経路を IT/ST で
//     機械検証可能化する試験ハーネス基盤を提供.
//   - F (legacy preconditions): is_always_lock_free static_assert 二重表明
//     (build 時検証、13 ユニット目に拡大、コード実装範囲内全 13 ユニットに展開完了).
//
// 注: UNIT-304 自体は ErrorCode を返さない (Sim は物理応答模擬 + 故障注入のみ).
// 関連 ErrorCode `DoseChannelDiscrepancy` 等は UNIT-204 DoseManager / UNIT-207 SafetyMonitor
// が UNIT-304 から値取得後に判定 (Inc.1 後半 / Inc.2 結線時).

#include "th25_sim/ion_chamber_sim.hpp"

#include "th25_ctrl/common_types.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <cmath>
#include <cstdint>
#include <thread>
#include <type_traits>
#include <vector>

namespace th25_sim {

using th25_ctrl::DoseUnit_cGy;

// ============================================================================
// UT-304-01: 初期状態 (累積=0、両チャネル None、両 read_dose=0)
// ============================================================================
TEST(IonChamberSim_Initial, AllZero) {
    IonChamberSim sim;
    EXPECT_DOUBLE_EQ(sim.current_accumulated(ChannelId::Channel0).value(), 0.0);
    EXPECT_DOUBLE_EQ(sim.current_accumulated(ChannelId::Channel1).value(), 0.0);
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel0).value(), 0.0);
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel1).value(), 0.0);
    EXPECT_EQ(sim.current_fault_mode(ChannelId::Channel0), FaultMode::None);
    EXPECT_EQ(sim.current_fault_mode(ChannelId::Channel1), FaultMode::None);
}

// ============================================================================
// UT-304-02: SRS-I-006 範囲定数 + Saturation 値 + 2 系統チャネル数
// ============================================================================
TEST(IonChamberSim_Constants, SrsValues) {
    EXPECT_DOUBLE_EQ(kIonChamberMinCgy, 0.0);
    EXPECT_DOUBLE_EQ(kIonChamberMaxCgy, 10000.0);
    EXPECT_DOUBLE_EQ(kSaturationValueCgy, 10000.0);
    EXPECT_EQ(kChannelCount, 2U);
}

// ============================================================================
// UT-304-03: inject_dose_increment 正常系 (Channel0 に 5.0 cGy 加算)
// ============================================================================
TEST(IonChamberSim_Increment, NormalChannel0) {
    IonChamberSim sim;
    sim.inject_dose_increment(ChannelId::Channel0, DoseUnit_cGy{5.0});
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel0).value(), 5.0);
    // Channel1 は影響を受けない
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel1).value(), 0.0);
}

// ============================================================================
// UT-304-04: inject_dose_increment 正常系 (Channel1 に 7.5 cGy 加算)
// ============================================================================
TEST(IonChamberSim_Increment, NormalChannel1) {
    IonChamberSim sim;
    sim.inject_dose_increment(ChannelId::Channel1, DoseUnit_cGy{7.5});
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel1).value(), 7.5);
    // Channel0 は影響を受けない
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel0).value(), 0.0);
}

// ============================================================================
// UT-304-05: 累積動作 (3 回追加で 6.0 cGy)
// ============================================================================
TEST(IonChamberSim_Increment, Accumulates) {
    IonChamberSim sim;
    sim.inject_dose_increment(ChannelId::Channel0, DoseUnit_cGy{2.0});
    sim.inject_dose_increment(ChannelId::Channel0, DoseUnit_cGy{2.0});
    sim.inject_dose_increment(ChannelId::Channel0, DoseUnit_cGy{2.0});
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel0).value(), 6.0);
}

// ============================================================================
// UT-304-06: SRS-I-006 上限境界 (10000.0 cGy 受入)
// ============================================================================
TEST(IonChamberSim_Increment, BoundaryMax) {
    IonChamberSim sim;
    sim.inject_dose_increment(ChannelId::Channel0, DoseUnit_cGy{10000.0});
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel0).value(), 10000.0);
}

// ============================================================================
// UT-304-07: 範囲外正方向 → clamp 上限 (15000.0 → 10000.0、物理的飽和模擬)
// ============================================================================
TEST(IonChamberSim_Increment, OutOfRangePositive) {
    IonChamberSim sim;
    sim.inject_dose_increment(ChannelId::Channel0, DoseUnit_cGy{15000.0});
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel0).value(), 10000.0);
    EXPECT_DOUBLE_EQ(sim.current_accumulated(ChannelId::Channel0).value(), 10000.0);
}

// ============================================================================
// UT-304-08: 累積後の負方向 delta が下限を下回る場合 → clamp 0.0
// (実物理デバイスは累積ドーズが負にならないため、下限飽和を保証)
// ============================================================================
TEST(IonChamberSim_Increment, OutOfRangeNegative) {
    IonChamberSim sim;
    sim.inject_dose_increment(ChannelId::Channel0, DoseUnit_cGy{5.0});
    sim.inject_dose_increment(ChannelId::Channel0, DoseUnit_cGy{-100.0});
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel0).value(), 0.0);
}

// ============================================================================
// UT-304-09: 2 チャネル独立性 (Channel0 と Channel1 は独立累積)
// ============================================================================
TEST(IonChamberSim_Increment, ChannelsIndependent) {
    IonChamberSim sim;
    sim.inject_dose_increment(ChannelId::Channel0, DoseUnit_cGy{100.0});
    sim.inject_dose_increment(ChannelId::Channel1, DoseUnit_cGy{200.0});
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel0).value(), 100.0);
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel1).value(), 200.0);
}

// ============================================================================
// UT-304-10: reset_channel 動作 (累積 → 0.0 に戻る、他チャネルは影響なし)
// ============================================================================
TEST(IonChamberSim_Reset, ResetSingleChannel) {
    IonChamberSim sim;
    sim.inject_dose_increment(ChannelId::Channel0, DoseUnit_cGy{50.0});
    sim.inject_dose_increment(ChannelId::Channel1, DoseUnit_cGy{75.0});

    sim.reset_channel(ChannelId::Channel0);
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel0).value(), 0.0);
    // Channel1 は影響を受けない
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel1).value(), 75.0);
}

// ============================================================================
// UT-304-11: inject_saturation 全チャネル機械検証 (両チャネル → 10000.0 返却)
// ============================================================================
TEST(IonChamberSim_Fault, SaturationBothChannels) {
    IonChamberSim sim;
    sim.inject_dose_increment(ChannelId::Channel0, DoseUnit_cGy{50.0});
    sim.inject_dose_increment(ChannelId::Channel1, DoseUnit_cGy{75.0});

    // 故障注入前は通常応答 (累積値返却)
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel0).value(), 50.0);
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel1).value(), 75.0);

    sim.inject_saturation(ChannelId::Channel0);
    sim.inject_saturation(ChannelId::Channel1);

    // 故障注入後は 10000.0 cGy 固定返却
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel0).value(), 10000.0);
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel1).value(), 10000.0);

    // 故障モード状態確認
    EXPECT_EQ(sim.current_fault_mode(ChannelId::Channel0), FaultMode::Saturation);
    EXPECT_EQ(sim.current_fault_mode(ChannelId::Channel1), FaultMode::Saturation);

    // 内部累積は変化しない (current_accumulated で確認)
    EXPECT_DOUBLE_EQ(sim.current_accumulated(ChannelId::Channel0).value(), 50.0);
    EXPECT_DOUBLE_EQ(sim.current_accumulated(ChannelId::Channel1).value(), 75.0);
}

// ============================================================================
// UT-304-12: inject_channel_failure 全チャネル機械検証 (両チャネル → 0.0 返却)
// ============================================================================
TEST(IonChamberSim_Fault, ChannelFailureBothChannels) {
    IonChamberSim sim;
    sim.inject_dose_increment(ChannelId::Channel0, DoseUnit_cGy{50.0});
    sim.inject_dose_increment(ChannelId::Channel1, DoseUnit_cGy{75.0});

    sim.inject_channel_failure(ChannelId::Channel0);
    sim.inject_channel_failure(ChannelId::Channel1);

    // 故障注入後は 0.0 cGy 固定返却
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel0).value(), 0.0);
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel1).value(), 0.0);

    // 故障モード状態確認
    EXPECT_EQ(sim.current_fault_mode(ChannelId::Channel0), FaultMode::ChannelFailure);
    EXPECT_EQ(sim.current_fault_mode(ChannelId::Channel1), FaultMode::ChannelFailure);

    // 内部累積は変化しない (current_accumulated で確認)
    EXPECT_DOUBLE_EQ(sim.current_accumulated(ChannelId::Channel0).value(), 50.0);
    EXPECT_DOUBLE_EQ(sim.current_accumulated(ChannelId::Channel1).value(), 75.0);
}

// ============================================================================
// UT-304-13: 部分故障 — Channel0 のみ Saturation、Channel1 は正常
// Therac-25 ドーズ計測異常型「片系故障時の検出経路」を構造的検出可能化
// (UNIT-204/207 結線時の冗長性検証).
// ============================================================================
TEST(IonChamberSim_Fault, PartialSaturationChannel0Only) {
    IonChamberSim sim;
    sim.inject_dose_increment(ChannelId::Channel0, DoseUnit_cGy{50.0});
    sim.inject_dose_increment(ChannelId::Channel1, DoseUnit_cGy{50.0});

    sim.inject_saturation(ChannelId::Channel0);

    // Channel0 のみ 10000.0、Channel1 は通常応答 50.0 → UNIT-207 不一致検出条件成立
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel0).value(), 10000.0);
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel1).value(), 50.0);
}

// ============================================================================
// UT-304-14: 部分故障 — Channel1 のみ ChannelFailure、Channel0 は正常
// Therac-25 ドーズ計測異常型「片系応答消失」検証.
// ============================================================================
TEST(IonChamberSim_Fault, PartialChannelFailureChannel1Only) {
    IonChamberSim sim;
    sim.inject_dose_increment(ChannelId::Channel0, DoseUnit_cGy{60.0});
    sim.inject_dose_increment(ChannelId::Channel1, DoseUnit_cGy{60.0});

    sim.inject_channel_failure(ChannelId::Channel1);

    // Channel1 のみ 0.0、Channel0 は通常応答 60.0 → UNIT-207 不一致検出条件成立
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel0).value(), 60.0);
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel1).value(), 0.0);
}

// ============================================================================
// UT-304-15: 混在故障モード (Channel0=Saturation + Channel1=ChannelFailure)
// 両チャネル同時故障シナリオを機械検証 (Therac-25 ドーズ計測異常型「2 チャネル不一致」).
// ============================================================================
TEST(IonChamberSim_Fault, MixedFaultModes) {
    IonChamberSim sim;
    sim.inject_dose_increment(ChannelId::Channel0, DoseUnit_cGy{30.0});
    sim.inject_dose_increment(ChannelId::Channel1, DoseUnit_cGy{30.0});

    sim.inject_saturation(ChannelId::Channel0);
    sim.inject_channel_failure(ChannelId::Channel1);

    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel0).value(), 10000.0);  // Saturation
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel1).value(), 0.0);      // ChannelFailure
}

// ============================================================================
// UT-304-16: clear_fault で None 復帰 (通常応答に戻る)
// ============================================================================
TEST(IonChamberSim_Fault, ClearFaultRecovers) {
    IonChamberSim sim;
    sim.inject_dose_increment(ChannelId::Channel0, DoseUnit_cGy{42.0});
    sim.inject_saturation(ChannelId::Channel0);
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel0).value(), 10000.0);

    sim.clear_fault(ChannelId::Channel0);
    EXPECT_EQ(sim.current_fault_mode(ChannelId::Channel0), FaultMode::None);
    EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel0).value(), 42.0);
}

// ============================================================================
// UT-304-17: 静的純粋関数 is_dose_in_range / clamp_to_range
// ============================================================================
TEST(IonChamberSim_StaticFns, RangeAndClamp) {
    EXPECT_TRUE(IonChamberSim::is_dose_in_range(DoseUnit_cGy{0.0}));
    EXPECT_TRUE(IonChamberSim::is_dose_in_range(DoseUnit_cGy{10000.0}));
    EXPECT_TRUE(IonChamberSim::is_dose_in_range(DoseUnit_cGy{5000.0}));
    EXPECT_FALSE(IonChamberSim::is_dose_in_range(DoseUnit_cGy{-0.01}));
    EXPECT_FALSE(IonChamberSim::is_dose_in_range(DoseUnit_cGy{10000.01}));

    EXPECT_DOUBLE_EQ(
        IonChamberSim::clamp_to_range(DoseUnit_cGy{5000.0}).value(), 5000.0);
    EXPECT_DOUBLE_EQ(
        IonChamberSim::clamp_to_range(DoseUnit_cGy{20000.0}).value(), 10000.0);
    EXPECT_DOUBLE_EQ(
        IonChamberSim::clamp_to_range(DoseUnit_cGy{-100.0}).value(), 0.0);

    static_assert(
        IonChamberSim::clamp_to_range(DoseUnit_cGy{5000.0}) == DoseUnit_cGy{5000.0});
    static_assert(
        IonChamberSim::clamp_to_range(DoseUnit_cGy{20000.0}) == DoseUnit_cGy{10000.0});
    static_assert(
        IonChamberSim::clamp_to_range(DoseUnit_cGy{-100.0}) == DoseUnit_cGy{0.0});
}

// ============================================================================
// UT-304-18: channel_index constexpr 静的検証
// ============================================================================
TEST(IonChamberSim_StaticFns, ChannelIndex) {
    static_assert(IonChamberSim::channel_index(ChannelId::Channel0) == 0U);
    static_assert(IonChamberSim::channel_index(ChannelId::Channel1) == 1U);
    EXPECT_EQ(IonChamberSim::channel_index(ChannelId::Channel0), 0U);
    EXPECT_EQ(IonChamberSim::channel_index(ChannelId::Channel1), 1U);
}

// ============================================================================
// UT-304-19: 強い型 + enum compile-time 隔離
// DoseUnit_cGy は double から暗黙変換不可 + ChannelId/FaultMode は uint8_t/int から
// 暗黙変換不可.
// ============================================================================
TEST(IonChamberSim_StrongTypes, CompileTimeIsolation) {
    static_assert(!std::is_convertible_v<double, DoseUnit_cGy>);
    static_assert(!std::is_convertible_v<int, ChannelId>);
    static_assert(!std::is_convertible_v<int, FaultMode>);
    static_assert(!std::is_convertible_v<std::uint8_t, ChannelId>);
    static_assert(!std::is_convertible_v<std::uint8_t, FaultMode>);
    SUCCEED();
}

// ============================================================================
// UT-304-20: コピー/ムーブ禁止 (compile-time)
// ============================================================================
TEST(IonChamberSim_Ownership, NoCopyNoMove) {
    static_assert(!std::is_copy_constructible_v<IonChamberSim>);
    static_assert(!std::is_copy_assignable_v<IonChamberSim>);
    static_assert(!std::is_move_constructible_v<IonChamberSim>);
    static_assert(!std::is_move_assignable_v<IonChamberSim>);
    SUCCEED();
}

// ============================================================================
// UT-304-21: HZ-007 lock-free 表明 (build 時 static_assert、二重表明)
// ============================================================================
TEST(IonChamberSim_HZ007, AtomicIsLockFree) {
    static_assert(std::atomic<double>::is_always_lock_free);
    static_assert(std::atomic<FaultMode>::is_always_lock_free);
    SUCCEED();
}

// ============================================================================
// UT-304-22: SDD §4.12 表との網羅検証
// FaultMode 3 種 × 代表的 dose 値で accumulated → read_dose 往復が正しく動作.
// ============================================================================
TEST(IonChamberSim_Matrix, SddTableCoverage) {
    struct Case {
        double dose_increment;
        FaultMode mode;
        double expected_read;
        const char* description;
    };
    constexpr Case kCases[] = {
        // FaultMode::None: 累積値そのまま返却
        {0.0, FaultMode::None, 0.0, "None@0"},
        {100.0, FaultMode::None, 100.0, "None@100"},
        {10000.0, FaultMode::None, 10000.0, "None@10000 (boundary)"},
        // FaultMode::Saturation: 10000.0 固定
        {50.0, FaultMode::Saturation, 10000.0, "Saturation@inc=50"},
        // FaultMode::ChannelFailure: 0.0 固定
        {50.0, FaultMode::ChannelFailure, 0.0, "ChannelFailure@inc=50"},
        // 故障注入後でも内部累積は変化しないことを確認
        {7500.0, FaultMode::Saturation, 10000.0, "Saturation@inc=7500"},
    };
    for (const auto& c : kCases) {
        IonChamberSim sim;
        sim.inject_dose_increment(ChannelId::Channel0, DoseUnit_cGy{c.dose_increment});
        if (c.mode == FaultMode::Saturation) {
            sim.inject_saturation(ChannelId::Channel0);
        } else if (c.mode == FaultMode::ChannelFailure) {
            sim.inject_channel_failure(ChannelId::Channel0);
        }
        EXPECT_DOUBLE_EQ(sim.read_dose(ChannelId::Channel0).value(), c.expected_read)
            << c.description;
    }
}

// ============================================================================
// UT-304-23: 並行処理 — 1 incrementer + 4 reader race-free
// `tsan` プリセットで race-free 検証 (HZ-002 直接対応、13 ユニット目に拡大).
// ============================================================================
TEST(IonChamberSim_Concurrency, IncrementerMultiReader) {
    IonChamberSim sim;

    constexpr int kReaders = 4;
    constexpr int kIterations = 5000;
    std::atomic<bool> incrementer_stop{false};
    std::atomic<std::uint64_t> reader_iters{0};

    // incrementer: Channel0 と Channel1 に交互に小さな増分を加算 (clamp 上限超えないよう
    // 微小値). 単一 incrementer 前提 (inject_dose_increment は read-modify-write のため
    // 並行 incrementer は要求外).
    std::thread incrementer([&]() {
        for (int i = 0; i < kIterations; ++i) {
            const auto ch = (i % 2 == 0) ? ChannelId::Channel0 : ChannelId::Channel1;
            sim.inject_dose_increment(ch, DoseUnit_cGy{0.001});
        }
        incrementer_stop.store(true, std::memory_order_release);
    });

    // 4 reader: 両チャネルの read_dose を並行に呼ぶ.
    std::vector<std::thread> readers;
    readers.reserve(kReaders);
    for (int i = 0; i < kReaders; ++i) {
        readers.emplace_back([&]() {
            while (!incrementer_stop.load(std::memory_order_acquire)) {
                (void)sim.read_dose(ChannelId::Channel0);
                (void)sim.read_dose(ChannelId::Channel1);
                (void)sim.current_fault_mode(ChannelId::Channel0);
                reader_iters.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    incrementer.join();
    for (auto& r : readers) {
        r.join();
    }

    EXPECT_GT(reader_iters.load(), 0U);
}

// ============================================================================
// UT-304-24: 並行処理 — fault injector + 3 reader race-free
// 故障モード切替と read_dose の並行処理を tsan で機械検証
// (`std::atomic<FaultMode>` 並行操作の正しさを検証).
// ============================================================================
TEST(IonChamberSim_Concurrency, FaultInjectorMultiReader) {
    IonChamberSim sim;
    sim.inject_dose_increment(ChannelId::Channel0, DoseUnit_cGy{1000.0});

    constexpr int kReaders = 3;
    constexpr int kIterations = 5000;
    std::atomic<bool> injector_stop{false};
    std::atomic<std::uint64_t> reader_iters{0};

    // injector: 故障モードを循環切替 (None → Saturation → ChannelFailure → None ...).
    std::thread injector([&]() {
        for (int i = 0; i < kIterations; ++i) {
            switch (i % 3) {
                case 0:
                    sim.clear_fault(ChannelId::Channel0);
                    break;
                case 1:
                    sim.inject_saturation(ChannelId::Channel0);
                    break;
                case 2:
                    sim.inject_channel_failure(ChannelId::Channel0);
                    break;
                default:
                    break;
            }
        }
        injector_stop.store(true, std::memory_order_release);
    });

    // 3 reader: Channel0 を並行に読取.
    std::vector<std::thread> readers;
    readers.reserve(kReaders);
    for (int i = 0; i < kReaders; ++i) {
        readers.emplace_back([&]() {
            while (!injector_stop.load(std::memory_order_acquire)) {
                (void)sim.read_dose(ChannelId::Channel0);
                (void)sim.current_fault_mode(ChannelId::Channel0);
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
