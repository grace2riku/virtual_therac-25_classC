// SPDX-License-Identifier: TBD
// TH25-CTRL: UNIT-203 BeamController ユニット試験 (UTPR-TH25-001 v0.5 §7.2 と整合).
//
// 試験対象: src/th25_ctrl/include/th25_ctrl/beam_controller.hpp /
//            src/th25_ctrl/src/beam_controller.cpp (UNIT-203).
// 適用範囲: SDD-TH25-001 v0.1.1 §4.4 で確定された RCM-017 中核ユニット.
//
// IEC 62304 §5.5.4 クラス C 追加受入基準:
//   - 正常系 / 境界値 / 異常系 / 並行処理 / 資源 / データフロー
//
// Therac-25 主要因類型 (SPRP §4.3.1):
//   - A (race condition): std::atomic<BeamState> + std::atomic<bool> +
//     compare_exchange_strong による消費型設計、UT-203-XX で 並行 TSan 検証.
//   - D (interlock missing): 許可フラグ 1 回限り消費で二重ビームオン要求を構造的拒否、
//     UT-203-XX で機械検証.
//   - F (legacy preconditions): is_always_lock_free static_assert (build 時検証).

#include "th25_ctrl/beam_controller.hpp"

#include "th25_ctrl/common_types.hpp"

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <thread>
#include <type_traits>
#include <vector>

namespace th25_ctrl {

// ============================================================================
// UT-203-01: 初期状態 = Off
// ============================================================================
TEST(BeamController_Initial, IsOff) {
    BeamController bc;
    EXPECT_EQ(bc.current_state(), BeamState::Off);
}

// ============================================================================
// UT-203-02: 初期 permission = false (安全側既定値)
// ============================================================================
TEST(BeamController_Initial, PermissionDefaultFalse) {
    BeamController bc;
    EXPECT_FALSE(bc.is_beam_on_permitted());
}

// ============================================================================
// UT-203-03: set_beam_on_permission で許可フラグ操作
// ============================================================================
TEST(BeamController_Permission, SetAndRead) {
    BeamController bc;
    bc.set_beam_on_permission(true);
    EXPECT_TRUE(bc.is_beam_on_permitted());

    bc.set_beam_on_permission(false);
    EXPECT_FALSE(bc.is_beam_on_permitted());
}

// ============================================================================
// UT-203-04: Lifecycle != Ready で beam_on → BeamOnNotPermitted
// (RCM-017 中核、Therac-25 Tyler 事故型「Lifecycle 状態確認漏れ」の構造的拒否)
// ============================================================================
TEST(BeamController_LifecycleGuard, RejectsBeamOnWhenNotReady) {
    BeamController bc;
    bc.set_beam_on_permission(true);

    constexpr std::array<LifecycleState, 7> kNonReady{
        LifecycleState::Init,
        LifecycleState::SelfCheck,
        LifecycleState::Idle,
        LifecycleState::PrescriptionSet,
        LifecycleState::BeamOn,
        LifecycleState::Halted,
        LifecycleState::Error,
    };
    for (auto s : kNonReady) {
        auto r = bc.request_beam_on(s);
        ASSERT_FALSE(r.has_value());
        EXPECT_EQ(r.error_code(), ErrorCode::BeamOnNotPermitted);
        // 状態は Off のまま、permission も維持 (消費されていない).
        EXPECT_EQ(bc.current_state(), BeamState::Off);
        EXPECT_TRUE(bc.is_beam_on_permitted());
    }
}

// ============================================================================
// UT-203-05: permission = false で beam_on → BeamOnNotPermitted
// ============================================================================
TEST(BeamController_PermissionGuard, RejectsBeamOnWhenPermissionFalse) {
    BeamController bc;
    // permission = false (既定) のまま Ready で要求.
    auto r = bc.request_beam_on(LifecycleState::Ready);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::BeamOnNotPermitted);
    EXPECT_EQ(bc.current_state(), BeamState::Off);
}

// ============================================================================
// UT-203-06: Lifecycle = Ready + permission = true で beam_on 成功 → BeamState::On
// ============================================================================
TEST(BeamController_BeamOn, SuccessTransitionsToOn) {
    BeamController bc;
    bc.set_beam_on_permission(true);

    auto r = bc.request_beam_on(LifecycleState::Ready);
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(bc.current_state(), BeamState::On);
    // permission は消費されている (1 回限り).
    EXPECT_FALSE(bc.is_beam_on_permitted());
}

// ============================================================================
// UT-203-07: 1 回限り消費 — 連続 beam_on は 2 回目で BeamOnNotPermitted
// (Therac-25 Tyler 事故型「ビーム二重照射」の構造的拒否、RCM-017 中核)
// ============================================================================
TEST(BeamController_PermissionGuard, IsConsumedOnceAndRejectsSecond) {
    BeamController bc;
    bc.set_beam_on_permission(true);

    // 1 回目: 成功.
    auto r1 = bc.request_beam_on(LifecycleState::Ready);
    EXPECT_TRUE(r1.has_value());
    EXPECT_EQ(bc.current_state(), BeamState::On);

    // 2 回目: permission が消費されているため拒否.
    // (実際は state == On なので state チェックでも拒否される)
    auto r2 = bc.request_beam_on(LifecycleState::Ready);
    ASSERT_FALSE(r2.has_value());
    EXPECT_EQ(r2.error_code(), ErrorCode::BeamOnNotPermitted);
}

// ============================================================================
// UT-203-08: On → request_beam_off で Stopping → Off へ遷移
// ============================================================================
TEST(BeamController_BeamOff, OnToOffViaStopping) {
    BeamController bc;
    bc.set_beam_on_permission(true);
    ASSERT_TRUE(bc.request_beam_on(LifecycleState::Ready).has_value());
    ASSERT_EQ(bc.current_state(), BeamState::On);

    auto r = bc.request_beam_off();
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(bc.current_state(), BeamState::Off);
    // beam_off 後は permission も false に倒される (次回は再設定が必要).
    EXPECT_FALSE(bc.is_beam_on_permitted());
}

// ============================================================================
// UT-203-09: Off で request_beam_off は no-op (許容、フェイルセーフ)
// ============================================================================
TEST(BeamController_BeamOff, OffIsNoOp) {
    BeamController bc;
    EXPECT_EQ(bc.current_state(), BeamState::Off);

    auto r = bc.request_beam_off();
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(bc.current_state(), BeamState::Off);
}

// ============================================================================
// UT-203-10: beam_off 後の再 beam_on は permission 再設定が必要
// (RCM-017 二重照射防止の継続)
// ============================================================================
TEST(BeamController_PermissionLifecycle, ReBeamOnRequiresPermissionReset) {
    BeamController bc;
    bc.set_beam_on_permission(true);
    ASSERT_TRUE(bc.request_beam_on(LifecycleState::Ready).has_value());
    ASSERT_TRUE(bc.request_beam_off().has_value());
    ASSERT_EQ(bc.current_state(), BeamState::Off);

    // permission 再設定なしで beam_on → 拒否.
    auto r1 = bc.request_beam_on(LifecycleState::Ready);
    ASSERT_FALSE(r1.has_value());
    EXPECT_EQ(r1.error_code(), ErrorCode::BeamOnNotPermitted);

    // permission 再設定後は受入.
    bc.set_beam_on_permission(true);
    auto r2 = bc.request_beam_on(LifecycleState::Ready);
    EXPECT_TRUE(r2.has_value());
    EXPECT_EQ(bc.current_state(), BeamState::On);
}

// ============================================================================
// UT-203-11: コピー/ムーブ禁止 (compile-time)
// ============================================================================
TEST(BeamController_Ownership, IsNotCopyableNorMovable) {
    using B = BeamController;
    static_assert(!std::is_copy_constructible_v<B>);
    static_assert(!std::is_copy_assignable_v<B>);
    static_assert(!std::is_move_constructible_v<B>);
    static_assert(!std::is_move_assignable_v<B>);
}

// ============================================================================
// UT-203-12: lock-free 表明 (HZ-007 構造的予防)
// ============================================================================
TEST(BeamController_LockFree, AtomicsAreAlwaysLockFree) {
    EXPECT_TRUE(std::atomic<BeamState>::is_always_lock_free);
    EXPECT_TRUE(std::atomic<bool>::is_always_lock_free);
}

// ============================================================================
// UT-203-13: 並行処理 — 多数 consumer が同時に beam_on を要求した場合、
// permission の compare_exchange により **唯一 1 件のみ成功** することを機械検証.
// (Therac-25 Tyler 事故型「許可確認の race condition」の構造的予防、HZ-002/HZ-004)
// tsan プリセットで race detection 0 を期待.
// ============================================================================
TEST(BeamController_Concurrency, BeamOnIsConsumedExactlyOnceUnderRace) {
    constexpr int kThreads = 8;
    constexpr int kIterationsPerSet = 1000;

    BeamController bc;
    std::atomic<int> total_success{0};

    // permission を都度 true に再設定し、N スレッドで beam_on を競合させる.
    // 各セットで唯一 1 スレッドのみが成功するはず.
    for (int set = 0; set < kIterationsPerSet; ++set) {
        // 前回の On を Off に戻す.
        ASSERT_TRUE(bc.request_beam_off().has_value());
        ASSERT_EQ(bc.current_state(), BeamState::Off);

        bc.set_beam_on_permission(true);

        std::atomic<int> set_success{0};
        std::vector<std::thread> threads;
        threads.reserve(kThreads);
        for (int t = 0; t < kThreads; ++t) {
            threads.emplace_back([&]() {
                auto r = bc.request_beam_on(LifecycleState::Ready);
                if (r.has_value()) {
                    set_success.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
        for (auto& th : threads) {
            th.join();
        }

        // 各セットで成功は唯一 1 回 (compare_exchange による消費型動作).
        ASSERT_EQ(set_success.load(), 1)
            << "Set " << set << ": expected exactly 1 success but got "
            << set_success.load();
        total_success.fetch_add(set_success.load(), std::memory_order_relaxed);
    }

    EXPECT_EQ(total_success.load(), kIterationsPerSet);
}

// ============================================================================
// UT-203-14: 並行処理 — 多数 thread が並列に beam_off を発行しても race-free
// (フェイルセーフ系、HZ-002 機械検証)
// ============================================================================
TEST(BeamController_Concurrency, ParallelBeamOffIsRaceFree) {
    constexpr int kThreads = 8;
    BeamController bc;
    bc.set_beam_on_permission(true);
    ASSERT_TRUE(bc.request_beam_on(LifecycleState::Ready).has_value());
    ASSERT_EQ(bc.current_state(), BeamState::On);

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&]() {
            // 各 thread が beam_off を呼出.
            EXPECT_TRUE(bc.request_beam_off().has_value());
        });
    }
    for (auto& th : threads) {
        th.join();
    }

    // 最終状態は Off で安定.
    EXPECT_EQ(bc.current_state(), BeamState::Off);
}

// ============================================================================
// UT-203-15: BeamState 値の安定性 (UNIT-200 enum と整合、SDD §4.1)
// ============================================================================
TEST(BeamController_BeamStateValues, AllFourStatesAreStable) {
    static_assert(static_cast<int>(BeamState::Off) == 0);
    static_assert(static_cast<int>(BeamState::Arming) == 1);
    static_assert(static_cast<int>(BeamState::On) == 2);
    static_assert(static_cast<int>(BeamState::Stopping) == 3);
    SUCCEED() << "Compile-time BeamState enum value checks passed.";
}

// ============================================================================
// UT-203-16: 防御的 — Lifecycle 不正値 (キャスト経由) を beam_on で拒否
// ============================================================================
TEST(BeamController_LifecycleGuard, RejectsInvalidLifecycleEnumValue) {
    BeamController bc;
    bc.set_beam_on_permission(true);

    auto bogus = static_cast<LifecycleState>(99);
    auto r = bc.request_beam_on(bogus);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::BeamOnNotPermitted);
}

// ============================================================================
// UT-203-17: 並行処理 — 1 thread が permission set / N thread が beam_on で
// 「唯一 1 件成功」を確認 (set/consume の atomic 整合性、HZ-002 機械検証)
// ============================================================================
TEST(BeamController_Concurrency, SetPermissionRacesWithBeamOnButConsumesOnce) {
    BeamController bc;
    constexpr int kIterations = 500;
    constexpr int kReaders = 4;

    for (int i = 0; i < kIterations; ++i) {
        // 前回をリセット.
        ASSERT_TRUE(bc.request_beam_off().has_value());

        std::atomic<int> success_count{0};
        std::vector<std::thread> readers;
        readers.reserve(kReaders);

        // permission を立てる thread.
        std::thread setter([&]() {
            bc.set_beam_on_permission(true);
        });

        // 並列に beam_on を試みる reader thread 群.
        for (int t = 0; t < kReaders; ++t) {
            readers.emplace_back([&]() {
                // setter が設定する前に呼ばれる可能性もある (busy-wait).
                for (int retry = 0; retry < 1000; ++retry) {
                    auto r = bc.request_beam_on(LifecycleState::Ready);
                    if (r.has_value()) {
                        success_count.fetch_add(1, std::memory_order_relaxed);
                        return;
                    }
                    std::this_thread::yield();
                }
            });
        }

        setter.join();
        for (auto& th : readers) {
            th.join();
        }

        // 唯一 1 thread が成功 (compare_exchange による消費).
        EXPECT_EQ(success_count.load(), 1)
            << "Iteration " << i << ": expected exactly 1 success but got "
            << success_count.load();
    }
}

// ============================================================================
// UT-203-18: 既に On 状態で beam_on を再要求 → BeamOnNotPermitted
// (state ガード、再エントラント防止)
// ============================================================================
TEST(BeamController_StateGuard, RejectsBeamOnWhenAlreadyOn) {
    BeamController bc;
    bc.set_beam_on_permission(true);
    ASSERT_TRUE(bc.request_beam_on(LifecycleState::Ready).has_value());
    ASSERT_EQ(bc.current_state(), BeamState::On);

    // 既に On 状態. permission を再設定しても state ガードで拒否.
    bc.set_beam_on_permission(true);
    auto r = bc.request_beam_on(LifecycleState::Ready);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error_code(), ErrorCode::BeamOnNotPermitted);
    EXPECT_EQ(bc.current_state(), BeamState::On);
}

// ============================================================================
// UT-203-19: beam_off は ErrorCode を返さない (任意状態で常に成功)
// (SDD §4.4 「事前条件: 任意」のフェイルセーフ性質)
// ============================================================================
TEST(BeamController_BeamOff, AlwaysSucceedsRegardlessOfCurrentState) {
    BeamController bc;
    // Off で beam_off.
    EXPECT_TRUE(bc.request_beam_off().has_value());
    EXPECT_EQ(bc.current_state(), BeamState::Off);

    // On で beam_off.
    bc.set_beam_on_permission(true);
    ASSERT_TRUE(bc.request_beam_on(LifecycleState::Ready).has_value());
    EXPECT_TRUE(bc.request_beam_off().has_value());
    EXPECT_EQ(bc.current_state(), BeamState::Off);
}

}  // namespace th25_ctrl
