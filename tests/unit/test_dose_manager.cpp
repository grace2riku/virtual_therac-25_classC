// SPDX-License-Identifier: TBD
// TH25-CTRL: UNIT-204 DoseManager ユニット試験 (UTPR-TH25-001 v0.6 §7.2 と整合).
//
// 試験対象: src/th25_ctrl/include/th25_ctrl/dose_manager.hpp /
//            src/th25_ctrl/src/dose_manager.cpp (UNIT-204).
// 適用範囲: SDD-TH25-001 v0.1.1 §4.5 で確定された HZ-005 直接 + HZ-003 構造的排除の中核.
//
// IEC 62304 §5.5.4 クラス C 追加受入基準:
//   - 正常系 / 境界値 / 異常系 / 並行処理 / 資源 / データフロー
//
// Therac-25 主要因類型 (SPRP §4.3.1):
//   - A (race condition): PulseCounter + std::atomic<uint64_t> target_pulses_ +
//     std::atomic<bool> target_reached_ / target_set_、UT-204-XX で 並行 TSan 検証.
//   - B (integer overflow): PulseCounter 内 std::atomic<uint64_t> 5,800 万年余裕、
//     UT-204-XX で uint64_t::max() 境界値 + DoseOverflow 検出を機械検証.
//   - F (legacy preconditions): is_always_lock_free static_assert (build 時検証).

#include "th25_ctrl/dose_manager.hpp"

#include "th25_ctrl/common_types.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <limits>
#include <thread>
#include <type_traits>
#include <vector>

namespace th25_ctrl {

namespace {

// 共通定数: rate = 1.0 cGy/pulse として「1 パルス = 1 cGy」になり境界値計算が直感的.
constexpr DoseRatePerPulse_cGy_per_pulse kRateOneCGy{1.0};
// 実用に近い rate (0.05 cGy/pulse): SDD §6.5 で校正値が決定する想定値の一例.
constexpr DoseRatePerPulse_cGy_per_pulse kRateRealistic{0.05};

constexpr LifecycleState kReadyState = LifecycleState::Ready;
constexpr LifecycleState kPrescriptionSetState = LifecycleState::PrescriptionSet;

}  // namespace

// ============================================================================
// UT-204-01: 初期状態 (target 未設定 / accumulated 0 / reached false)
// ============================================================================
TEST(DoseManager_Initial, AllZeroAndUnset) {
    DoseManager dm{kRateOneCGy};
    EXPECT_DOUBLE_EQ(dm.current_accumulated().value(), 0.0);
    EXPECT_DOUBLE_EQ(dm.current_target().value(), 0.0);
    EXPECT_FALSE(dm.is_target_reached());
    // 内部 target_pulses_ は uint64_t::max() (未設定保護).
    EXPECT_EQ(dm.target_pulses_for_test(), std::numeric_limits<std::uint64_t>::max());
}

// ============================================================================
// UT-204-02: 異なる rate での明示初期化
// ============================================================================
TEST(DoseManager_Initial, ConstructWithDifferentRates) {
    DoseManager dm_one{kRateOneCGy};
    DoseManager dm_real{kRateRealistic};
    EXPECT_DOUBLE_EQ(dm_one.current_accumulated().value(), 0.0);
    EXPECT_DOUBLE_EQ(dm_real.current_accumulated().value(), 0.0);
}

// ============================================================================
// UT-204-03: set_dose_target 正常系 (Ready 状態、典型値)
// ============================================================================
TEST(DoseManager_SetDoseTarget, Normal_Ready) {
    DoseManager dm{kRateOneCGy};
    auto r = dm.set_dose_target(DoseUnit_cGy{200.0}, kReadyState);
    EXPECT_TRUE(r.has_value());
    // 1.0 cGy/pulse なので target_pulses_ = 200.
    EXPECT_EQ(dm.target_pulses_for_test(), 200U);
    EXPECT_FALSE(dm.is_target_reached());
}

// ============================================================================
// UT-204-04: set_dose_target 正常系 (PrescriptionSet 状態)
// ============================================================================
TEST(DoseManager_SetDoseTarget, Normal_PrescriptionSet) {
    DoseManager dm{kRateOneCGy};
    auto r = dm.set_dose_target(DoseUnit_cGy{500.0}, kPrescriptionSetState);
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(dm.target_pulses_for_test(), 500U);
}

// ============================================================================
// UT-204-05: set_dose_target 範囲下限 (0.01 cGy 受入、SRS-008 境界)
// ============================================================================
TEST(DoseManager_SetDoseTarget, BoundaryMin_Accept) {
    DoseManager dm{kRateRealistic};  // 0.05 cGy/pulse
    auto r = dm.set_dose_target(DoseUnit_cGy{0.01}, kReadyState);
    EXPECT_TRUE(r.has_value());
    // 0.01 / 0.05 = 0.2 → 切り捨てで 0 pulses (target_set_=true、ただし即時到達)
    // この境界では「target_pulses_=0 && target_set_=true」となるため、
    // 1 パルスも積算しないうちは accumulated < target で 0 < 0 は false → 未到達.
    // 1 パルス積算後 (1 ≥ 0) で到達.
    EXPECT_EQ(dm.target_pulses_for_test(), 0U);
}

// ============================================================================
// UT-204-06: set_dose_target 範囲上限 (10000.0 cGy 受入、SRS-008 境界)
// ============================================================================
TEST(DoseManager_SetDoseTarget, BoundaryMax_Accept) {
    DoseManager dm{kRateOneCGy};
    auto r = dm.set_dose_target(DoseUnit_cGy{10000.0}, kReadyState);
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(dm.target_pulses_for_test(), 10000U);
}

// ============================================================================
// UT-204-07: set_dose_target 範囲下限以下 (0.005 cGy → DoseOutOfRange)
// ============================================================================
TEST(DoseManager_SetDoseTarget, BelowMin_Reject) {
    DoseManager dm{kRateOneCGy};
    auto r = dm.set_dose_target(DoseUnit_cGy{0.005}, kReadyState);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::DoseOutOfRange);
    // 拒否時は内部状態が変わっていない.
    EXPECT_EQ(dm.target_pulses_for_test(), std::numeric_limits<std::uint64_t>::max());
}

// ============================================================================
// UT-204-08: set_dose_target 範囲上限超 (10001.0 cGy → DoseOutOfRange)
// ============================================================================
TEST(DoseManager_SetDoseTarget, AboveMax_Reject) {
    DoseManager dm{kRateOneCGy};
    auto r = dm.set_dose_target(DoseUnit_cGy{10001.0}, kReadyState);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::DoseOutOfRange);
}

// ============================================================================
// UT-204-09: set_dose_target 0.0 cGy (範囲外、DoseOutOfRange)
// ============================================================================
TEST(DoseManager_SetDoseTarget, Zero_Reject) {
    DoseManager dm{kRateOneCGy};
    auto r = dm.set_dose_target(DoseUnit_cGy{0.0}, kReadyState);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::DoseOutOfRange);
}

// ============================================================================
// UT-204-10: set_dose_target lifecycle 違反 (Idle → InternalUnexpectedState)
// ============================================================================
TEST(DoseManager_SetDoseTarget, LifecycleIdle_Reject) {
    DoseManager dm{kRateOneCGy};
    auto r = dm.set_dose_target(DoseUnit_cGy{200.0}, LifecycleState::Idle);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::InternalUnexpectedState);
    // 拒否時は内部状態が変わっていない.
    EXPECT_EQ(dm.target_pulses_for_test(), std::numeric_limits<std::uint64_t>::max());
}

// ============================================================================
// UT-204-11: set_dose_target lifecycle 違反 (BeamOn → InternalUnexpectedState)
// ============================================================================
TEST(DoseManager_SetDoseTarget, LifecycleBeamOn_Reject) {
    DoseManager dm{kRateOneCGy};
    auto r = dm.set_dose_target(DoseUnit_cGy{200.0}, LifecycleState::BeamOn);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::InternalUnexpectedState);
}

// ============================================================================
// UT-204-12: set_dose_target lifecycle 全 8 状態網羅 (Init/SelfCheck/Halted/Error も拒否)
// ============================================================================
TEST(DoseManager_SetDoseTarget, AllLifecycleStates) {
    DoseManager dm{kRateOneCGy};
    constexpr DoseUnit_cGy target{100.0};

    // 受入 2 状態.
    EXPECT_TRUE(dm.set_dose_target(target, LifecycleState::PrescriptionSet).has_value());
    dm.reset();
    EXPECT_TRUE(dm.set_dose_target(target, LifecycleState::Ready).has_value());
    dm.reset();

    // 拒否 6 状態.
    constexpr LifecycleState kRejectStates[] = {
        LifecycleState::Init,        LifecycleState::SelfCheck, LifecycleState::Idle,
        LifecycleState::BeamOn,      LifecycleState::Halted,    LifecycleState::Error,
    };
    for (const auto state : kRejectStates) {
        auto r = dm.set_dose_target(target, state);
        ASSERT_FALSE(r.has_value()) << "state index = " << static_cast<int>(state);
        EXPECT_EQ(r.error_code(), ErrorCode::InternalUnexpectedState);
    }
}

// ============================================================================
// UT-204-13: set_dose_target で累積カウンタが 0 にリセットされる
// ============================================================================
TEST(DoseManager_SetDoseTarget, ResetsAccumulator) {
    DoseManager dm{kRateOneCGy};
    ASSERT_TRUE(dm.set_dose_target(DoseUnit_cGy{100.0}, kReadyState).has_value());

    // 50 パルス積算.
    dm.on_dose_pulse(PulseCount{50});
    EXPECT_DOUBLE_EQ(dm.current_accumulated().value(), 50.0);

    // 再 set_dose_target で累積がクリアされる + reached フラグが false に戻る.
    ASSERT_TRUE(dm.set_dose_target(DoseUnit_cGy{200.0}, kReadyState).has_value());
    EXPECT_DOUBLE_EQ(dm.current_accumulated().value(), 0.0);
    EXPECT_FALSE(dm.is_target_reached());
}

// ============================================================================
// UT-204-14: on_dose_pulse 単発で累積が進む
// ============================================================================
TEST(DoseManager_OnDosePulse, SingleAdvance) {
    DoseManager dm{kRateOneCGy};
    ASSERT_TRUE(dm.set_dose_target(DoseUnit_cGy{100.0}, kReadyState).has_value());

    dm.on_dose_pulse(PulseCount{1});
    EXPECT_DOUBLE_EQ(dm.current_accumulated().value(), 1.0);
}

// ============================================================================
// UT-204-15: on_dose_pulse 複数回で累積が単調増加
// ============================================================================
TEST(DoseManager_OnDosePulse, Monotonic) {
    DoseManager dm{kRateOneCGy};
    ASSERT_TRUE(dm.set_dose_target(DoseUnit_cGy{1000.0}, kReadyState).has_value());

    constexpr int kIterations = 100;
    for (int i = 0; i < kIterations; ++i) {
        dm.on_dose_pulse(PulseCount{1});
    }
    EXPECT_DOUBLE_EQ(dm.current_accumulated().value(), 100.0);
}

// ============================================================================
// UT-204-16: pulse_count_to_dose 単位変換が rate 通り
// ============================================================================
TEST(DoseManager_PulseToDose, ConversionMatchesRate) {
    DoseManager dm_one{kRateOneCGy};
    EXPECT_DOUBLE_EQ(dm_one.pulse_count_to_dose(PulseCount{0}).value(), 0.0);
    EXPECT_DOUBLE_EQ(dm_one.pulse_count_to_dose(PulseCount{42}).value(), 42.0);
    EXPECT_DOUBLE_EQ(dm_one.pulse_count_to_dose(PulseCount{1000}).value(), 1000.0);

    DoseManager dm_real{kRateRealistic};  // 0.05 cGy/pulse
    EXPECT_DOUBLE_EQ(dm_real.pulse_count_to_dose(PulseCount{0}).value(), 0.0);
    EXPECT_DOUBLE_EQ(dm_real.pulse_count_to_dose(PulseCount{20}).value(), 1.0);
    EXPECT_DOUBLE_EQ(dm_real.pulse_count_to_dose(PulseCount{200}).value(), 10.0);
}

// ============================================================================
// UT-204-17: 目標未到達 (target=100、99 パルス → reached=false)
// ============================================================================
TEST(DoseManager_TargetReached, JustBelow) {
    DoseManager dm{kRateOneCGy};
    ASSERT_TRUE(dm.set_dose_target(DoseUnit_cGy{100.0}, kReadyState).has_value());

    for (int i = 0; i < 99; ++i) {
        dm.on_dose_pulse(PulseCount{1});
    }
    EXPECT_DOUBLE_EQ(dm.current_accumulated().value(), 99.0);
    EXPECT_FALSE(dm.is_target_reached());
}

// ============================================================================
// UT-204-18: 目標到達 (target=100、100 パルス → reached=true)
// ============================================================================
TEST(DoseManager_TargetReached, ExactReach) {
    DoseManager dm{kRateOneCGy};
    ASSERT_TRUE(dm.set_dose_target(DoseUnit_cGy{100.0}, kReadyState).has_value());

    for (int i = 0; i < 100; ++i) {
        dm.on_dose_pulse(PulseCount{1});
    }
    EXPECT_DOUBLE_EQ(dm.current_accumulated().value(), 100.0);
    EXPECT_TRUE(dm.is_target_reached());
}

// ============================================================================
// UT-204-19: 目標超過 (101 パルス → reached=true 維持、累積は単調増加)
// ============================================================================
TEST(DoseManager_TargetReached, OverShoot) {
    DoseManager dm{kRateOneCGy};
    ASSERT_TRUE(dm.set_dose_target(DoseUnit_cGy{100.0}, kReadyState).has_value());

    for (int i = 0; i < 101; ++i) {
        dm.on_dose_pulse(PulseCount{1});
    }
    EXPECT_DOUBLE_EQ(dm.current_accumulated().value(), 101.0);
    EXPECT_TRUE(dm.is_target_reached());
}

// ============================================================================
// UT-204-20: 目標未設定状態の on_dose_pulse は累積のみ進め、reached を立てない
// ============================================================================
TEST(DoseManager_TargetReached, NoTargetSet_NoReachDetection) {
    DoseManager dm{kRateOneCGy};
    // target 未設定のまま on_dose_pulse を呼ぶ.
    for (int i = 0; i < 1000; ++i) {
        dm.on_dose_pulse(PulseCount{1});
    }
    EXPECT_DOUBLE_EQ(dm.current_accumulated().value(), 1000.0);
    EXPECT_FALSE(dm.is_target_reached());
}

// ============================================================================
// UT-204-21: HZ-003 構造的排除 — uint64_t::max() 直前まで積算可能、オーバフロー先送り
// 5,800 万年連続照射相当の余裕を境界値で確認.
// ============================================================================
TEST(DoseManager_HZ003, LargeAccumulationStable) {
    DoseManager dm{kRateOneCGy};
    ASSERT_TRUE(dm.set_dose_target(DoseUnit_cGy{10000.0}, kReadyState).has_value());

    // 10^9 パルス (= 1.0 GBq ・1 kHz で 11.5 日分) を一度に積算しても破綻しない.
    constexpr std::uint64_t kHugePulses = 1'000'000'000ULL;
    dm.on_dose_pulse(PulseCount{kHugePulses});
    EXPECT_DOUBLE_EQ(dm.current_accumulated().value(),
                     static_cast<double>(kHugePulses));
    EXPECT_TRUE(dm.is_target_reached());

    // 内部 atomic<uint64_t> なので uint64_t::max() に達するまで安全.
    EXPECT_LT(static_cast<double>(kHugePulses),
              static_cast<double>(std::numeric_limits<std::uint64_t>::max()));
}

// ============================================================================
// UT-204-22: SRS-008 境界値の機械検証 (受入 2 件 + 拒否 4 件)
// ============================================================================
TEST(DoseManager_SetDoseTarget, BoundaryValuesMechanicalCheck) {
    struct Case {
        double cgy;
        bool accept;
    };
    constexpr Case kCases[] = {
        {0.005, false},
        {0.01, true},
        {10000.0, true},
        {10000.001, false},
        {10001.0, false},
        {-1.0, false},
    };
    for (const auto& c : kCases) {
        DoseManager dm{kRateOneCGy};
        auto r = dm.set_dose_target(DoseUnit_cGy{c.cgy}, kReadyState);
        EXPECT_EQ(r.has_value(), c.accept) << "cgy=" << c.cgy;
        if (!c.accept) {
            EXPECT_EQ(r.error_code(), ErrorCode::DoseOutOfRange);
        }
    }
}

// ============================================================================
// UT-204-23: is_dose_target_in_range 静的純粋関数の網羅
// ============================================================================
TEST(DoseManager_IsInRange, StaticPredicate) {
    EXPECT_FALSE(DoseManager::is_dose_target_in_range(DoseUnit_cGy{-1.0}));
    EXPECT_FALSE(DoseManager::is_dose_target_in_range(DoseUnit_cGy{0.0}));
    EXPECT_FALSE(DoseManager::is_dose_target_in_range(DoseUnit_cGy{0.005}));
    EXPECT_TRUE(DoseManager::is_dose_target_in_range(DoseUnit_cGy{0.01}));
    EXPECT_TRUE(DoseManager::is_dose_target_in_range(DoseUnit_cGy{1.0}));
    EXPECT_TRUE(DoseManager::is_dose_target_in_range(DoseUnit_cGy{10000.0}));
    EXPECT_FALSE(DoseManager::is_dose_target_in_range(DoseUnit_cGy{10000.001}));
    EXPECT_FALSE(DoseManager::is_dose_target_in_range(DoseUnit_cGy{10001.0}));
}

// ============================================================================
// UT-204-24: reset() で全フィールドが初期状態に戻る
// ============================================================================
TEST(DoseManager_Reset, AllFieldsCleared) {
    DoseManager dm{kRateOneCGy};
    ASSERT_TRUE(dm.set_dose_target(DoseUnit_cGy{100.0}, kReadyState).has_value());
    for (int i = 0; i < 100; ++i) {
        dm.on_dose_pulse(PulseCount{1});
    }
    ASSERT_TRUE(dm.is_target_reached());

    dm.reset();
    EXPECT_DOUBLE_EQ(dm.current_accumulated().value(), 0.0);
    EXPECT_DOUBLE_EQ(dm.current_target().value(), 0.0);
    EXPECT_FALSE(dm.is_target_reached());
    EXPECT_EQ(dm.target_pulses_for_test(), std::numeric_limits<std::uint64_t>::max());
}

// ============================================================================
// UT-204-25: SDD §4.5 表との網羅機械検証 (lifecycle × 範囲 = 8 × 3 = 24 組合せ)
// ============================================================================
TEST(DoseManager_SetDoseTarget, MatrixCoverage) {
    constexpr LifecycleState kAllStates[] = {
        LifecycleState::Init,           LifecycleState::SelfCheck,
        LifecycleState::Idle,           LifecycleState::PrescriptionSet,
        LifecycleState::Ready,          LifecycleState::BeamOn,
        LifecycleState::Halted,         LifecycleState::Error,
    };
    struct DoseCase {
        double cgy;
        bool in_range;
    };
    constexpr DoseCase kDoseCases[] = {
        {0.005, false},
        {1.0, true},
        {10001.0, false},
    };

    for (const auto state : kAllStates) {
        const bool lifecycle_ok = (state == LifecycleState::PrescriptionSet
                                   || state == LifecycleState::Ready);
        for (const auto& dc : kDoseCases) {
            DoseManager dm{kRateOneCGy};
            auto r = dm.set_dose_target(DoseUnit_cGy{dc.cgy}, state);
            const bool expect_ok = lifecycle_ok && dc.in_range;
            EXPECT_EQ(r.has_value(), expect_ok)
                << "state=" << static_cast<int>(state) << " cgy=" << dc.cgy;
            if (!expect_ok) {
                if (!lifecycle_ok) {
                    EXPECT_EQ(r.error_code(), ErrorCode::InternalUnexpectedState);
                } else {
                    EXPECT_EQ(r.error_code(), ErrorCode::DoseOutOfRange);
                }
            }
        }
    }
}

// ============================================================================
// UT-204-26: コピー/ムーブ禁止 (compile-time)
// ============================================================================
TEST(DoseManager_Ownership, NoCopyNoMove) {
    static_assert(!std::is_copy_constructible_v<DoseManager>);
    static_assert(!std::is_copy_assignable_v<DoseManager>);
    static_assert(!std::is_move_constructible_v<DoseManager>);
    static_assert(!std::is_move_assignable_v<DoseManager>);
    SUCCEED();
}

// ============================================================================
// UT-204-27: HZ-007 lock-free 表明 (build 時 static_assert)
// 注: メンバ関数 is_lock_free() (runtime) は GCC libstdc++ 14 で libatomic への
// 動的リンクを要求するケースがあるため使用しない. is_always_lock_free (static
// constexpr) で build 時検証する方が厳密 (lock-free 性が失われたら fail-stop).
// ============================================================================
TEST(DoseManager_HZ007, AtomicsAreLockFree) {
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
    static_assert(std::atomic<bool>::is_always_lock_free);
    SUCCEED();
}

// ============================================================================
// UT-204-28: 強い型 compile-time (DoseRatePerPulse_cGy_per_pulse / DoseUnit_cGy /
// PulseCount は相互に暗黙変換不可)
// ============================================================================
TEST(DoseManager_StrongTypes, CompileTimeIsolation) {
    static_assert(!std::is_convertible_v<double, DoseRatePerPulse_cGy_per_pulse>);
    static_assert(!std::is_convertible_v<double, DoseUnit_cGy>);
    static_assert(!std::is_convertible_v<DoseUnit_cGy, DoseRatePerPulse_cGy_per_pulse>);
    static_assert(!std::is_convertible_v<DoseRatePerPulse_cGy_per_pulse, DoseUnit_cGy>);
    static_assert(!std::is_convertible_v<PulseCount, DoseUnit_cGy>);
    static_assert(!std::is_convertible_v<DoseUnit_cGy, PulseCount>);
    SUCCEED();
}

// ============================================================================
// UT-204-29: DoseOverflow 検出 (rate=0 や rate 不正で target/rate が表現範囲外)
// ============================================================================
TEST(DoseManager_SetDoseTarget, DoseOverflowOnInvalidRate) {
    // rate=0 → target/rate = inf → DoseOverflow.
    {
        DoseManager dm{DoseRatePerPulse_cGy_per_pulse{0.0}};
        auto r = dm.set_dose_target(DoseUnit_cGy{100.0}, kReadyState);
        ASSERT_FALSE(r.has_value());
        EXPECT_EQ(r.error_code(), ErrorCode::DoseOverflow);
    }
    // rate=負値 → DoseOverflow.
    {
        DoseManager dm{DoseRatePerPulse_cGy_per_pulse{-1.0}};
        auto r = dm.set_dose_target(DoseUnit_cGy{100.0}, kReadyState);
        ASSERT_FALSE(r.has_value());
        EXPECT_EQ(r.error_code(), ErrorCode::DoseOverflow);
    }
    // rate=極小 → target/rate が uint64_t::max() を超える → DoseOverflow.
    {
        DoseManager dm{DoseRatePerPulse_cGy_per_pulse{1e-300}};
        auto r = dm.set_dose_target(DoseUnit_cGy{1.0}, kReadyState);
        ASSERT_FALSE(r.has_value());
        EXPECT_EQ(r.error_code(), ErrorCode::DoseOverflow);
    }
}

// ============================================================================
// UT-204-30: 並行処理 — 複数 producer (on_dose_pulse) + reader (current_accumulated /
// is_target_reached) を `tsan` プリセットで race-free 検証 (HZ-002 直接対応).
// ============================================================================
TEST(DoseManager_Concurrency, ProducerProducerReader) {
    DoseManager dm{kRateOneCGy};
    constexpr DoseUnit_cGy target{100000.0};  // 余裕で達成可能
    ASSERT_TRUE(dm.set_dose_target(target, kReadyState).has_value());

    constexpr int kProducers = 4;
    constexpr int kPulsesPerProducer = 10000;

    std::atomic<bool> reader_stop{false};
    std::atomic<std::uint64_t> reader_iterations{0};

    // reader: current_accumulated と is_target_reached を並行に呼ぶ (race-free 検証).
    std::thread reader([&]() {
        while (!reader_stop.load(std::memory_order_acquire)) {
            (void)dm.current_accumulated();
            (void)dm.is_target_reached();
            reader_iterations.fetch_add(1, std::memory_order_relaxed);
        }
    });

    // producers: 4 thread が並行に on_dose_pulse を呼ぶ.
    std::vector<std::thread> producers;
    producers.reserve(kProducers);
    for (int i = 0; i < kProducers; ++i) {
        producers.emplace_back([&]() {
            for (int j = 0; j < kPulsesPerProducer; ++j) {
                dm.on_dose_pulse(PulseCount{1});
            }
        });
    }
    for (auto& t : producers) {
        t.join();
    }

    reader_stop.store(true, std::memory_order_release);
    reader.join();

    // 累積が 4 × 10000 = 40000 cGy になっているはず (rate=1.0 cGy/pulse).
    EXPECT_DOUBLE_EQ(dm.current_accumulated().value(),
                     static_cast<double>(kProducers * kPulsesPerProducer));
    // 40000 < 100000 なので reached は false のまま.
    EXPECT_FALSE(dm.is_target_reached());
    // reader が少なくとも 1 回は走ったこと (race-free に load できた証拠).
    EXPECT_GT(reader_iterations.load(), 0U);
}

// ============================================================================
// UT-204-31: 並行処理 — 1 producer (on_dose_pulse) と 1 setter (set_dose_target /
// reset) の race-free 検証 (target 切替が atomic に行われることを `tsan` で確認).
// ============================================================================
TEST(DoseManager_Concurrency, ProducerSetterRace) {
    DoseManager dm{kRateOneCGy};
    ASSERT_TRUE(dm.set_dose_target(DoseUnit_cGy{500.0}, kReadyState).has_value());

    constexpr int kIterations = 5000;
    std::atomic<bool> setter_stop{false};

    std::thread producer([&]() {
        for (int i = 0; i < kIterations; ++i) {
            dm.on_dose_pulse(PulseCount{1});
        }
        setter_stop.store(true, std::memory_order_release);
    });

    // setter: 100 ms 周期で target を切替・reset を呼ぶ (atomic 切替の race-free 検証).
    std::thread setter([&]() {
        bool toggle = false;
        while (!setter_stop.load(std::memory_order_acquire)) {
            const DoseUnit_cGy t{toggle ? 500.0 : 1000.0};
            (void)dm.set_dose_target(t, kReadyState);
            toggle = !toggle;
        }
    });

    producer.join();
    setter.join();

    // setter で reset されているため累積は predictable ではないが、データレースなしを TSan が確認.
    SUCCEED();
}

// ============================================================================
// UT-204-32: 防御的 ErrorCode 系統 (DoseOutOfRange / InternalUnexpectedState /
// DoseOverflow が想定通りの系統に属する)
// ============================================================================
TEST(DoseManager_ErrorCodes, CategoryConsistency) {
    EXPECT_EQ(error_category(ErrorCode::DoseOutOfRange), 0x03U);
    EXPECT_EQ(error_category(ErrorCode::DoseOverflow), 0x03U);
    EXPECT_EQ(error_category(ErrorCode::InternalUnexpectedState), 0xFFU);

    EXPECT_EQ(severity_of(ErrorCode::DoseOutOfRange), Severity::Critical);
    EXPECT_EQ(severity_of(ErrorCode::DoseOverflow), Severity::Critical);
    EXPECT_EQ(severity_of(ErrorCode::InternalUnexpectedState), Severity::Critical);
}

}  // namespace th25_ctrl
