// SPDX-License-Identifier: TBD
// TH25-CTRL: UNIT-202 TreatmentModeManager ユニット試験 (UTPR-TH25-001 v0.4 §7.2 と整合).
//
// 試験対象: src/th25_ctrl/include/th25_ctrl/treatment_mode_manager.hpp /
//            src/th25_ctrl/src/treatment_mode_manager.cpp (UNIT-202).
// 適用範囲: SDD-TH25-001 v0.1.1 §4.3 で確定された TreatmentModeManager
//            (HZ-001 直接対応の中核ユニット).
//
// IEC 62304 §5.5.4 クラス C 追加受入基準:
//   - 正常系 / 境界値 / 異常系 / 並行処理 / 資源 / データフロー
//
// Therac-25 主要因類型 (SPRP §4.3.1):
//   - A (race condition): std::atomic<TreatmentMode> + concurrent UT (UT-202-21, tsan).
//   - D (interlock missing): BeamState != Off / エネルギー範囲外 / API 不整合は
//     ErrorCode で構造的拒否. Therac-25 Tyler 事故の発現経路を阻止.
//   - F (legacy preconditions): is_always_lock_free static_assert (build 時検証).

#include "th25_ctrl/treatment_mode_manager.hpp"

#include "th25_ctrl/common_types.hpp"

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <thread>
#include <type_traits>
#include <vector>

namespace th25_ctrl {

// ============================================================================
// UT-202-01: 既定の初期状態 = Light (SDD §4.3, Therac-25 標準起動状態)
// ============================================================================
TEST(TreatmentModeManager_Initial, DefaultIsLight) {
    TreatmentModeManager mgr;
    EXPECT_EQ(mgr.current_mode(), TreatmentMode::Light);
}

// ============================================================================
// UT-202-02: 初期モードを明示指定可能 (Electron)
// ============================================================================
TEST(TreatmentModeManager_Initial, ExplicitInitialMode) {
    TreatmentModeManager mgr_e{TreatmentMode::Electron};
    EXPECT_EQ(mgr_e.current_mode(), TreatmentMode::Electron);

    TreatmentModeManager mgr_x{TreatmentMode::XRay};
    EXPECT_EQ(mgr_x.current_mode(), TreatmentMode::XRay);
}

// ============================================================================
// UT-202-03: Light → Electron 正常遷移
// ============================================================================
TEST(TreatmentModeManager_Transition, LightToElectron) {
    TreatmentModeManager mgr;  // 初期 Light
    auto r = mgr.request_mode_change(
        TreatmentMode::Electron, Energy_MeV{10.0}, BeamState::Off);
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(mgr.current_mode(), TreatmentMode::Electron);
}

// ============================================================================
// UT-202-04: Light → XRay 正常遷移
// ============================================================================
TEST(TreatmentModeManager_Transition, LightToXRay) {
    TreatmentModeManager mgr;
    auto r = mgr.request_mode_change(
        TreatmentMode::XRay, Energy_MV{18.0}, BeamState::Off);
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(mgr.current_mode(), TreatmentMode::XRay);
}

// ============================================================================
// UT-202-05: Electron → XRay 正常遷移 (SDD §4.3 表)
// ============================================================================
TEST(TreatmentModeManager_Transition, ElectronToXRay) {
    TreatmentModeManager mgr{TreatmentMode::Electron};
    auto r = mgr.request_mode_change(
        TreatmentMode::XRay, Energy_MV{15.0}, BeamState::Off);
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(mgr.current_mode(), TreatmentMode::XRay);
}

// ============================================================================
// UT-202-06: XRay → Electron 正常遷移
// ============================================================================
TEST(TreatmentModeManager_Transition, XRayToElectron) {
    TreatmentModeManager mgr{TreatmentMode::XRay};
    auto r = mgr.request_mode_change(
        TreatmentMode::Electron, Energy_MeV{20.0}, BeamState::Off);
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(mgr.current_mode(), TreatmentMode::Electron);
}

// ============================================================================
// UT-202-07: 任意モード → Light (治療終了時)
// ============================================================================
TEST(TreatmentModeManager_Transition, AnyToLight) {
    TreatmentModeManager mgr_e{TreatmentMode::Electron};
    EXPECT_TRUE(mgr_e.request_light_mode(BeamState::Off).has_value());
    EXPECT_EQ(mgr_e.current_mode(), TreatmentMode::Light);

    TreatmentModeManager mgr_x{TreatmentMode::XRay};
    EXPECT_TRUE(mgr_x.request_light_mode(BeamState::Off).has_value());
    EXPECT_EQ(mgr_x.current_mode(), TreatmentMode::Light);
}

// ============================================================================
// UT-202-08: 同モード再要求 (no-op として成功扱い、SDD §4.3 表)
// ============================================================================
TEST(TreatmentModeManager_Transition, SameModeIsNoOpSuccess) {
    TreatmentModeManager mgr{TreatmentMode::Electron};
    auto r = mgr.request_mode_change(
        TreatmentMode::Electron, Energy_MeV{12.0}, BeamState::Off);
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(mgr.current_mode(), TreatmentMode::Electron);
}

// ============================================================================
// UT-202-09: BeamState != Off で Electron 要求 → ModeBeamOnNotAllowed
// (Therac-25 Tyler 事故型の構造的拒否、RCM-001 中核)
// ============================================================================
TEST(TreatmentModeManager_BeamStateGuard, RejectsElectronWhenBeamOn) {
    TreatmentModeManager mgr;
    constexpr std::array<BeamState, 3> kNonOff{
        BeamState::Arming, BeamState::On, BeamState::Stopping};
    for (auto s : kNonOff) {
        auto r = mgr.request_mode_change(
            TreatmentMode::Electron, Energy_MeV{10.0}, s);
        ASSERT_FALSE(r.has_value());
        EXPECT_EQ(r.error_code(), ErrorCode::ModeBeamOnNotAllowed);
        // 状態は変わらない (Light のまま).
        EXPECT_EQ(mgr.current_mode(), TreatmentMode::Light);
    }
}

// ============================================================================
// UT-202-10: BeamState != Off で XRay 要求 → ModeBeamOnNotAllowed
// ============================================================================
TEST(TreatmentModeManager_BeamStateGuard, RejectsXRayWhenBeamOn) {
    TreatmentModeManager mgr;
    auto r = mgr.request_mode_change(
        TreatmentMode::XRay, Energy_MV{18.0}, BeamState::On);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::ModeBeamOnNotAllowed);
}

// ============================================================================
// UT-202-11: BeamState != Off で Light 要求 → ModeBeamOnNotAllowed
// ============================================================================
TEST(TreatmentModeManager_BeamStateGuard, RejectsLightWhenBeamOn) {
    TreatmentModeManager mgr{TreatmentMode::Electron};
    auto r = mgr.request_light_mode(BeamState::Arming);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::ModeBeamOnNotAllowed);
    EXPECT_EQ(mgr.current_mode(), TreatmentMode::Electron);
}

// ============================================================================
// UT-202-12: Electron エネルギー範囲外 (下限未満) → ModeInvalidTransition
// ============================================================================
TEST(TreatmentModeManager_EnergyRange, ElectronBelowMinIsRejected) {
    TreatmentModeManager mgr;
    auto r = mgr.request_mode_change(
        TreatmentMode::Electron, Energy_MeV{0.99}, BeamState::Off);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::ModeInvalidTransition);
    EXPECT_EQ(mgr.current_mode(), TreatmentMode::Light);
}

// ============================================================================
// UT-202-13: Electron エネルギー範囲外 (上限超) → ModeInvalidTransition
// ============================================================================
TEST(TreatmentModeManager_EnergyRange, ElectronAboveMaxIsRejected) {
    TreatmentModeManager mgr;
    auto r = mgr.request_mode_change(
        TreatmentMode::Electron, Energy_MeV{25.01}, BeamState::Off);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::ModeInvalidTransition);
}

// ============================================================================
// UT-202-14: XRay エネルギー範囲外 (下限未満) → ModeInvalidTransition
// ============================================================================
TEST(TreatmentModeManager_EnergyRange, XRayBelowMinIsRejected) {
    TreatmentModeManager mgr;
    auto r = mgr.request_mode_change(
        TreatmentMode::XRay, Energy_MV{4.99}, BeamState::Off);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::ModeInvalidTransition);
}

// ============================================================================
// UT-202-15: XRay エネルギー範囲外 (上限超) → ModeInvalidTransition
// ============================================================================
TEST(TreatmentModeManager_EnergyRange, XRayAboveMaxIsRejected) {
    TreatmentModeManager mgr;
    auto r = mgr.request_mode_change(
        TreatmentMode::XRay, Energy_MV{25.01}, BeamState::Off);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::ModeInvalidTransition);
}

// ============================================================================
// UT-202-16: Electron エネルギー境界値 (1.0 / 25.0) → 受入
// ============================================================================
TEST(TreatmentModeManager_EnergyRange, ElectronBoundaryIsAccepted) {
    EXPECT_TRUE(TreatmentModeManager::is_electron_energy_in_range(
        Energy_MeV{kElectronEnergyMinMeV}));
    EXPECT_TRUE(TreatmentModeManager::is_electron_energy_in_range(
        Energy_MeV{kElectronEnergyMaxMeV}));
    EXPECT_FALSE(TreatmentModeManager::is_electron_energy_in_range(
        Energy_MeV{kElectronEnergyMinMeV - 0.01}));
    EXPECT_FALSE(TreatmentModeManager::is_electron_energy_in_range(
        Energy_MeV{kElectronEnergyMaxMeV + 0.01}));

    // 実際の API 呼出でも境界値で受入.
    TreatmentModeManager mgr;
    EXPECT_TRUE(
        mgr.request_mode_change(TreatmentMode::Electron,
                                Energy_MeV{kElectronEnergyMinMeV},
                                BeamState::Off)
            .has_value());
    EXPECT_TRUE(
        mgr.request_mode_change(TreatmentMode::Electron,
                                Energy_MeV{kElectronEnergyMaxMeV},
                                BeamState::Off)
            .has_value());
}

// ============================================================================
// UT-202-17: XRay エネルギー境界値 (5.0 / 25.0) → 受入
// ============================================================================
TEST(TreatmentModeManager_EnergyRange, XRayBoundaryIsAccepted) {
    EXPECT_TRUE(TreatmentModeManager::is_xray_energy_in_range(
        Energy_MV{kXrayEnergyMinMV}));
    EXPECT_TRUE(TreatmentModeManager::is_xray_energy_in_range(
        Energy_MV{kXrayEnergyMaxMV}));
    EXPECT_FALSE(TreatmentModeManager::is_xray_energy_in_range(
        Energy_MV{kXrayEnergyMinMV - 0.01}));
    EXPECT_FALSE(TreatmentModeManager::is_xray_energy_in_range(
        Energy_MV{kXrayEnergyMaxMV + 0.01}));
}

// ============================================================================
// UT-202-18: Electron API に new_mode != Electron を指定 → ModeInvalidTransition
// (Therac-25 Tyler 事故の「Electron モードで X 線エネルギーを設定」型を構造的拒否)
// ============================================================================
TEST(TreatmentModeManager_ApiGuard, ElectronApiRejectsXRayMode) {
    TreatmentModeManager mgr;
    // Electron 用 API に XRay を渡した場合.
    auto r = mgr.request_mode_change(
        TreatmentMode::XRay, Energy_MeV{10.0}, BeamState::Off);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::ModeInvalidTransition);
    EXPECT_EQ(mgr.current_mode(), TreatmentMode::Light);
}

// ============================================================================
// UT-202-19: XRay API に new_mode != XRay を指定 → ModeInvalidTransition
// ============================================================================
TEST(TreatmentModeManager_ApiGuard, XRayApiRejectsElectronMode) {
    TreatmentModeManager mgr;
    auto r = mgr.request_mode_change(
        TreatmentMode::Electron, Energy_MV{10.0}, BeamState::Off);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::ModeInvalidTransition);
}

// ============================================================================
// UT-202-20: 強い型 compile-time 試験 (Energy_MeV と Energy_MV は別型)
// ============================================================================
TEST(TreatmentModeManager_StrongTypes, EnergyMeVAndEnergyMVAreDistinct) {
    static_assert(!std::is_convertible_v<Energy_MeV, Energy_MV>);
    static_assert(!std::is_convertible_v<Energy_MV, Energy_MeV>);
    static_assert(!std::is_convertible_v<double, Energy_MeV>);
    static_assert(!std::is_convertible_v<double, Energy_MV>);
    SUCCEED() << "Compile-time strong-type checks passed.";
}

// ============================================================================
// UT-202-21: 並行処理 — N writer + N reader での race-free 検証
// SDD §4.3 / RCM-001 / RCM-002 / HZ-002 直接対応.
// tsan プリセットで race condition 検出 0 を期待.
// ============================================================================
TEST(TreatmentModeManager_Concurrency, ConcurrentRequestsAreRaceFree) {
    TreatmentModeManager mgr;
    constexpr int kIterations = 1000;

    std::atomic<int> successful_changes{0};

    // Writer 1: Electron 要求.
    std::thread writer_e([&]() {
        for (int i = 0; i < kIterations; ++i) {
            auto r = mgr.request_mode_change(
                TreatmentMode::Electron, Energy_MeV{10.0}, BeamState::Off);
            if (r.has_value()) {
                successful_changes.fetch_add(1, std::memory_order_relaxed);
            }
        }
    });

    // Writer 2: XRay 要求.
    std::thread writer_x([&]() {
        for (int i = 0; i < kIterations; ++i) {
            auto r = mgr.request_mode_change(
                TreatmentMode::XRay, Energy_MV{18.0}, BeamState::Off);
            if (r.has_value()) {
                successful_changes.fetch_add(1, std::memory_order_relaxed);
            }
        }
    });

    // Reader: current_mode() を頻繁に呼ぶ. 値が 3 つの enum のいずれかに
    // なっていることを確認 (race による未定義値の検出).
    std::thread reader([&]() {
        for (int i = 0; i < kIterations * 4; ++i) {
            const TreatmentMode m = mgr.current_mode();
            ASSERT_TRUE(m == TreatmentMode::Electron ||
                        m == TreatmentMode::XRay ||
                        m == TreatmentMode::Light);
        }
    });

    writer_e.join();
    writer_x.join();
    reader.join();

    // 最終モードは Electron か XRay のいずれか (Light は明示変更しない限り
    // 残らない).
    const TreatmentMode final_m = mgr.current_mode();
    EXPECT_TRUE(final_m == TreatmentMode::Electron ||
                final_m == TreatmentMode::XRay);

    // 全 writer 要求が成功 (BeamState::Off + 範囲内エネルギーのため).
    EXPECT_EQ(successful_changes.load(), kIterations * 2);
}

// ============================================================================
// UT-202-22: コピー/ムーブ禁止 (compile-time)
// ============================================================================
TEST(TreatmentModeManager_Ownership, IsNotCopyableNorMovable) {
    using M = TreatmentModeManager;
    static_assert(!std::is_copy_constructible_v<M>);
    static_assert(!std::is_copy_assignable_v<M>);
    static_assert(!std::is_move_constructible_v<M>);
    static_assert(!std::is_move_assignable_v<M>);
}

// ============================================================================
// UT-202-23: lock-free 表明 (HZ-007 構造的予防)
// ============================================================================
TEST(TreatmentModeManager_LockFree, AtomicTreatmentModeIsAlwaysLockFree) {
    EXPECT_TRUE(std::atomic<TreatmentMode>::is_always_lock_free);
}

// ============================================================================
// UT-202-24: verify_mode_consistency — 一致 / 不一致
// ============================================================================
TEST(TreatmentModeManager_VerifyConsistency, MatchAndMismatch) {
    TreatmentModeManager mgr{TreatmentMode::Electron};

    // 一致時は Ok.
    auto ok = mgr.verify_mode_consistency(TreatmentMode::Electron);
    EXPECT_TRUE(ok.has_value());

    // 不一致時は ModePositionMismatch.
    auto err = mgr.verify_mode_consistency(TreatmentMode::XRay);
    ASSERT_FALSE(err.has_value());
    EXPECT_EQ(err.error_code(), ErrorCode::ModePositionMismatch);
}

// ============================================================================
// UT-202-25: モード遷移許可表の網羅試験 (3 状態 × 3 要求モード = 9 通り)
// SDD §4.3 表との完全一致を機械検証.
// ============================================================================
TEST(TreatmentModeManager_TransitionTable, ExhaustiveAllPairsAllowed) {
    constexpr std::array<TreatmentMode, 3> kAll{
        TreatmentMode::Electron, TreatmentMode::XRay, TreatmentMode::Light};

    int allowed_count = 0;
    for (auto from : kAll) {
        for (auto to : kAll) {
            if (TreatmentModeManager::is_mode_transition_allowed(from, to)) {
                ++allowed_count;
            }
        }
    }
    // SDD §4.3 表: 全 9 通り許可.
    EXPECT_EQ(allowed_count, 9);
}

// ============================================================================
// UT-202-26: 不正な enum 値 (キャスト経由) は遷移許可表で拒否
// (列挙値の妥当性チェックの防御的検証)
// ============================================================================
TEST(TreatmentModeManager_TransitionTable, RejectsInvalidEnumValues) {
    // SDD §4.1 で TreatmentMode は 3 値固定 (Electron=1, XRay=2, Light=3).
    // 0 や 4 はキャスト経由でしか発生しない不正値.
    auto bogus = static_cast<TreatmentMode>(0);
    EXPECT_FALSE(TreatmentModeManager::is_mode_transition_allowed(
        bogus, TreatmentMode::Electron));
    EXPECT_FALSE(TreatmentModeManager::is_mode_transition_allowed(
        TreatmentMode::Electron, bogus));
}

}  // namespace th25_ctrl
