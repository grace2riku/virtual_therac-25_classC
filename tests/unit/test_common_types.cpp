// SPDX-License-Identifier: TBD
// TH25-CTRL: UNIT-200 CommonTypes ユニット試験 (UTPR-TH25-001 §7.2 と整合).
//
// 試験対象: src/th25_ctrl/include/th25_ctrl/common_types.hpp (UNIT-200).
// 適用範囲: SDD-TH25-001 v0.1 §4.1 / §6.1〜§6.4 で定義された全型.
//
// IEC 62304 §5.5.4 クラス C 追加受入基準:
//   - 正常系 / 境界値 / 異常系 / 資源 / 分岐網羅 / データフロー / 並行処理
//
// Therac-25 主要因類型 (SPRP §4.3.1):
//   - A (race condition): PulseCounter atomic 動作
//   - B (整数オーバフロー): PulseCounter uint64_t 境界値
//   - F (レガシー前提喪失): is_always_lock_free static_assert (build 時検証)

#include "th25_ctrl/common_types.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <limits>
#include <thread>
#include <type_traits>
#include <vector>

namespace th25_ctrl {

// ============================================================================
// 検出用 concept (UT-200-09 で算術演算子の非提供を SFINAE-friendly に検証).
// 関数本体内に直接 requires 式を書くと clang-17 / gcc-13 では SFINAE 文脈外と
// なりコンパイルエラーになるため、concept にラップして SFINAE 文脈に持ち込む.
// ============================================================================
namespace detail {
template <typename T>
concept HasPlusOp = requires(T a, T b) { a + b; };
template <typename T>
concept HasMinusOp = requires(T a, T b) { a - b; };
template <typename T>
concept HasMulOp = requires(T a, T b) { a * b; };
}  // namespace detail

// ============================================================================
// UT-200-01: enum class TreatmentMode 値・型安全
// ============================================================================
TEST(CommonTypes_TreatmentMode, EnumValuesAreStable) {
    EXPECT_EQ(static_cast<std::uint8_t>(TreatmentMode::Electron), 1U);
    EXPECT_EQ(static_cast<std::uint8_t>(TreatmentMode::XRay), 2U);
    EXPECT_EQ(static_cast<std::uint8_t>(TreatmentMode::Light), 3U);
}

TEST(CommonTypes_TreatmentMode, IsStronglyTypedEnumeration) {
    // Compile-time check: TreatmentMode must NOT be implicitly convertible to int.
    static_assert(!std::is_convertible_v<TreatmentMode, int>,
        "TreatmentMode must be enum class (RCM-001 / HZ-001).");
    static_assert(std::is_same_v<std::underlying_type_t<TreatmentMode>, std::uint8_t>);
}

// ============================================================================
// UT-200-02: enum class LifecycleState 8 状態
// ============================================================================
TEST(CommonTypes_LifecycleState, AllEightStatesArePresent) {
    EXPECT_EQ(static_cast<std::uint8_t>(LifecycleState::Init), 0U);
    EXPECT_EQ(static_cast<std::uint8_t>(LifecycleState::SelfCheck), 1U);
    EXPECT_EQ(static_cast<std::uint8_t>(LifecycleState::Idle), 2U);
    EXPECT_EQ(static_cast<std::uint8_t>(LifecycleState::PrescriptionSet), 3U);
    EXPECT_EQ(static_cast<std::uint8_t>(LifecycleState::Ready), 4U);
    EXPECT_EQ(static_cast<std::uint8_t>(LifecycleState::BeamOn), 5U);
    EXPECT_EQ(static_cast<std::uint8_t>(LifecycleState::Halted), 6U);
    EXPECT_EQ(static_cast<std::uint8_t>(LifecycleState::Error), 7U);
}

// ============================================================================
// UT-200-03: enum class BeamState 4 状態
// ============================================================================
TEST(CommonTypes_BeamState, AllFourStatesArePresent) {
    EXPECT_EQ(static_cast<std::uint8_t>(BeamState::Off), 0U);
    EXPECT_EQ(static_cast<std::uint8_t>(BeamState::Arming), 1U);
    EXPECT_EQ(static_cast<std::uint8_t>(BeamState::On), 2U);
    EXPECT_EQ(static_cast<std::uint8_t>(BeamState::Stopping), 3U);
}

// ============================================================================
// UT-200-04: ErrorCode 8 系統階層 (上位 8 bit による系統判定)
// ============================================================================
TEST(CommonTypes_ErrorCode, CategoryMaskCoversEightCategories) {
    // Mode 系
    EXPECT_EQ(error_category(ErrorCode::ModeInvalidTransition), 0x01U);
    EXPECT_EQ(error_category(ErrorCode::ModePositionMismatch), 0x01U);
    EXPECT_EQ(error_category(ErrorCode::ModeBeamOnNotAllowed), 0x01U);

    // Beam 系
    EXPECT_EQ(error_category(ErrorCode::BeamOnNotPermitted), 0x02U);
    EXPECT_EQ(error_category(ErrorCode::BeamOffTimeout), 0x02U);
    EXPECT_EQ(error_category(ErrorCode::ElectronGunCurrentOutOfRange), 0x02U);

    // Dose 系
    EXPECT_EQ(error_category(ErrorCode::DoseOutOfRange), 0x03U);
    EXPECT_EQ(error_category(ErrorCode::DoseTargetExceeded), 0x03U);
    EXPECT_EQ(error_category(ErrorCode::DoseSensorFailure), 0x03U);
    EXPECT_EQ(error_category(ErrorCode::DoseOverflow), 0x03U);

    // Turntable 系
    EXPECT_EQ(error_category(ErrorCode::TurntableOutOfPosition), 0x04U);
    EXPECT_EQ(error_category(ErrorCode::TurntableSensorDiscrepancy), 0x04U);
    EXPECT_EQ(error_category(ErrorCode::TurntableMoveTimeout), 0x04U);

    // Magnet 系
    EXPECT_EQ(error_category(ErrorCode::MagnetCurrentDeviation), 0x05U);
    EXPECT_EQ(error_category(ErrorCode::MagnetCurrentSensorFailure), 0x05U);

    // IPC 系
    EXPECT_EQ(error_category(ErrorCode::IpcChannelClosed), 0x06U);
    EXPECT_EQ(error_category(ErrorCode::IpcMessageTooLarge), 0x06U);
    EXPECT_EQ(error_category(ErrorCode::IpcDeserializationFailure), 0x06U);
    EXPECT_EQ(error_category(ErrorCode::IpcQueueOverflow), 0x06U);

    // Auth 系
    EXPECT_EQ(error_category(ErrorCode::AuthRequired), 0x07U);
    EXPECT_EQ(error_category(ErrorCode::AuthInvalid), 0x07U);
    EXPECT_EQ(error_category(ErrorCode::AuthExpired), 0x07U);

    // Internal 系
    EXPECT_EQ(error_category(ErrorCode::InternalAssertion), 0xFFU);
    EXPECT_EQ(error_category(ErrorCode::InternalQueueFull), 0xFFU);
    EXPECT_EQ(error_category(ErrorCode::InternalUnexpectedState), 0xFFU);
}

// ============================================================================
// UT-200-05: severity_of() — Critical/High/Medium/Low の判定規則
// ============================================================================
TEST(CommonTypes_Severity, SeverityOfFollowsSpecRules) {
    // Critical: Mode / Beam / Dose / Internal
    EXPECT_EQ(severity_of(ErrorCode::ModeInvalidTransition), Severity::Critical);
    EXPECT_EQ(severity_of(ErrorCode::BeamOnNotPermitted), Severity::Critical);
    EXPECT_EQ(severity_of(ErrorCode::DoseTargetExceeded), Severity::Critical);
    EXPECT_EQ(severity_of(ErrorCode::InternalAssertion), Severity::Critical);

    // High: Turntable / IPC
    EXPECT_EQ(severity_of(ErrorCode::TurntableOutOfPosition), Severity::High);
    EXPECT_EQ(severity_of(ErrorCode::IpcChannelClosed), Severity::High);

    // Medium: Magnet / Auth
    EXPECT_EQ(severity_of(ErrorCode::MagnetCurrentDeviation), Severity::Medium);
    EXPECT_EQ(severity_of(ErrorCode::AuthRequired), Severity::Medium);
}

// ============================================================================
// UT-200-06〜10: 強い型 — explicit / 比較 / 算術非提供 / 単位混在禁止
// ============================================================================

TEST(CommonTypes_StrongType, EnergyMeVRequiresExplicitConstruction) {
    // explicit constructor: implicit conversion from double must NOT compile.
    static_assert(!std::is_convertible_v<double, Energy_MeV>,
        "Energy_MeV constructor must be explicit (RCM-008).");

    constexpr Energy_MeV e10{10.0};
    EXPECT_DOUBLE_EQ(e10.value(), 10.0);
}

TEST(CommonTypes_StrongType, EnergyMeVAndEnergyMVAreDistinctTypes) {
    // SDD §4.1 / SRS-D-002 / SRS-D-003: Energy_MeV と Energy_MV は別型 (暗黙変換禁止).
    static_assert(!std::is_convertible_v<Energy_MeV, Energy_MV>);
    static_assert(!std::is_convertible_v<Energy_MV, Energy_MeV>);
}

TEST(CommonTypes_StrongType, ComparisonOperatorsAreProvided) {
    constexpr Energy_MeV a{5.0};
    constexpr Energy_MeV b{10.0};
    constexpr Energy_MeV c{5.0};

    EXPECT_LT(a, b);
    EXPECT_GT(b, a);
    EXPECT_EQ(a, c);
    EXPECT_NE(a, b);
    EXPECT_LE(a, c);
    EXPECT_GE(b, a);
}

TEST(CommonTypes_StrongType, ArithmeticOperatorsAreNotProvided) {
    // SDD §6.3: 算術演算子 (+, -, *, /) は提供しない (単位混在の危険).
    // Compile-time SFINAE check via detail:: concept (RCM-008).
    static_assert(!detail::HasPlusOp<Energy_MeV>,
        "Energy_MeV must NOT support operator+ (RCM-008).");
    static_assert(!detail::HasMinusOp<Energy_MeV>,
        "Energy_MeV must NOT support operator- (RCM-008).");
    static_assert(!detail::HasMulOp<Energy_MeV>,
        "Energy_MeV must NOT support operator* (RCM-008).");
    SUCCEED() << "Compile-time arithmetic-non-availability checks passed.";
}

TEST(CommonTypes_StrongType, DoseUnitAddDoseAccumulatesOnly) {
    // SDD §4.1: 「減算非提供 (放射線は累積のみ)」.
    constexpr DoseUnit_cGy initial{100.0};
    constexpr DoseUnit_cGy delta{50.0};
    constexpr DoseUnit_cGy total = initial.add_dose(delta);
    EXPECT_DOUBLE_EQ(total.value(), 150.0);
    // 元のインスタンスは不変であること.
    EXPECT_DOUBLE_EQ(initial.value(), 100.0);
}

TEST(CommonTypes_StrongType, PositionAbsDiffIsNonNegative) {
    constexpr Position_mm a{10.0};
    constexpr Position_mm b{15.5};
    EXPECT_DOUBLE_EQ(a.abs_diff(b).value(), 5.5);
    EXPECT_DOUBLE_EQ(b.abs_diff(a).value(), 5.5);

    // 同一値での差分は 0.
    EXPECT_DOUBLE_EQ(a.abs_diff(a).value(), 0.0);
}

// ============================================================================
// UT-200-11: PulseCount 値型
// ============================================================================
TEST(CommonTypes_PulseCount, ConstructionAndComparison) {
    constexpr PulseCount p100{100U};
    constexpr PulseCount p200{200U};
    EXPECT_EQ(p100.value(), 100U);
    EXPECT_LT(p100, p200);
}

// ============================================================================
// UT-200-12: PulseCounter — lock-free 表明 (SDD §7 SOUP-003/004 / HZ-007 構造的)
// ============================================================================
TEST(CommonTypes_PulseCounter, AtomicUint64IsAlwaysLockFree) {
    // is_always_lock_free は constexpr static. ヘッダ内で static_assert 済だが,
    // 明示的に runtime チェックも追加して fail-stop 性を補強.
    EXPECT_TRUE(std::atomic<std::uint64_t>::is_always_lock_free);
}

// ============================================================================
// UT-200-13: PulseCounter 正常系 — 初期値 / fetch_add / load
// ============================================================================
TEST(CommonTypes_PulseCounter, InitialValueIsZero) {
    PulseCounter counter;
    EXPECT_EQ(counter.load().value(), 0U);
}

TEST(CommonTypes_PulseCounter, FetchAddReturnsPreviousValue) {
    PulseCounter counter;
    const PulseCount before = counter.fetch_add(PulseCount{10U});
    EXPECT_EQ(before.value(), 0U);
    EXPECT_EQ(counter.load().value(), 10U);

    const PulseCount before2 = counter.fetch_add(PulseCount{5U});
    EXPECT_EQ(before2.value(), 10U);
    EXPECT_EQ(counter.load().value(), 15U);
}

// ============================================================================
// UT-200-14: PulseCounter reset() で 0 に戻る
// ============================================================================
TEST(CommonTypes_PulseCounter, ResetReturnsToZero) {
    PulseCounter counter;
    (void)counter.fetch_add(PulseCount{12345U});
    EXPECT_EQ(counter.load().value(), 12345U);
    counter.reset();
    EXPECT_EQ(counter.load().value(), 0U);
}

// ============================================================================
// UT-200-15: PulseCounter 境界値 — uint64_t::max() オーバフロー直前
// HZ-003 (整数オーバフロー) 構造的予防の boundary 検証.
// ============================================================================
TEST(CommonTypes_PulseCounter, NumericLimitsBoundary) {
    PulseCounter counter;
    // 直接 uint64_t::max() - 1 まで進める.
    constexpr std::uint64_t kBigJump = std::numeric_limits<std::uint64_t>::max() - 1U;
    (void)counter.fetch_add(PulseCount{kBigJump});
    EXPECT_EQ(counter.load().value(), kBigJump);

    // さらに +1 で uint64_t::max() に到達.
    (void)counter.fetch_add(PulseCount{1U});
    EXPECT_EQ(counter.load().value(), std::numeric_limits<std::uint64_t>::max());

    // 1 kHz で 5,800 万年連続照射相当. 実用上ここに到達しないことを SDD §4.5 で保証.
    // ここでは「構造的に到達可能」かつ「到達時の挙動が定義されている」ことを確認.
}

// ============================================================================
// UT-200-16: PulseCounter 並行処理 — TSan で race condition が検出されないこと.
// SDD §4.13 / RCM-002 / RCM-019 の動作検証.
// ============================================================================
TEST(CommonTypes_PulseCounter, ConcurrentFetchAddIsRaceFree) {
    PulseCounter counter;
    constexpr int kThreads = 4;
    constexpr std::uint64_t kIncrementsPerThread = 10000U;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&counter] {
            for (std::uint64_t k = 0U; k < kIncrementsPerThread; ++k) {
                (void)counter.fetch_add(PulseCount{1U});
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }

    const std::uint64_t expected = static_cast<std::uint64_t>(kThreads) * kIncrementsPerThread;
    EXPECT_EQ(counter.load().value(), expected);
}

// ============================================================================
// UT-200-17: PulseCounter コピー / ムーブ禁止 (Compile-time 検証)
// ============================================================================
TEST(CommonTypes_PulseCounter, IsNotCopyableNorMovable) {
    static_assert(!std::is_copy_constructible_v<PulseCounter>);
    static_assert(!std::is_copy_assignable_v<PulseCounter>);
    static_assert(!std::is_move_constructible_v<PulseCounter>);
    static_assert(!std::is_move_assignable_v<PulseCounter>);
}

// ============================================================================
// UT-200-18〜20: Result<T, E> — 値 / 失敗 / void 特殊化
// ============================================================================

TEST(CommonTypes_Result, ValueConstructionAndAccess) {
    Result<DoseUnit_cGy, ErrorCode> r{DoseUnit_cGy{42.5}};
    ASSERT_TRUE(r.has_value());
    ASSERT_TRUE(static_cast<bool>(r));
    EXPECT_DOUBLE_EQ(r.value().value(), 42.5);
}

TEST(CommonTypes_Result, ErrorConstructionAndAccess) {
    auto r = Result<DoseUnit_cGy, ErrorCode>::error(ErrorCode::DoseOutOfRange);
    ASSERT_FALSE(r.has_value());
    ASSERT_FALSE(static_cast<bool>(r));
    EXPECT_EQ(r.error_code(), ErrorCode::DoseOutOfRange);
}

TEST(CommonTypes_Result, VoidSpecializationOk) {
    auto r = Result<void, ErrorCode>::ok();
    EXPECT_TRUE(r.has_value());
    EXPECT_TRUE(static_cast<bool>(r));
}

TEST(CommonTypes_Result, VoidSpecializationError) {
    auto r = Result<void, ErrorCode>::error(ErrorCode::InternalAssertion);
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::InternalAssertion);
}

}  // namespace th25_ctrl
