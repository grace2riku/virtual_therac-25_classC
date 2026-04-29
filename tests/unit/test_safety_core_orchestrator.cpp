// SPDX-License-Identifier: TBD
// TH25-CTRL: UNIT-201 SafetyCoreOrchestrator ユニット試験 (UTPR-TH25-001 v0.3 §7.2 と整合).
//
// 試験対象: src/th25_ctrl/include/th25_ctrl/safety_core_orchestrator.hpp /
//            src/th25_ctrl/src/safety_core_orchestrator.cpp (UNIT-201).
// 適用範囲: SDD-TH25-001 v0.1.1 §4.2 / §6.1 で確定された LifecycleState 状態機械.
//
// IEC 62304 §5.5.4 クラス C 追加受入基準:
//   - 正常系 / 境界値 / 異常系 / 並行処理 / 資源 / データフロー
//
// Therac-25 主要因類型 (SPRP §4.3.1):
//   - A (race condition): SPSC イベント配送 concurrent 試験 (UT-201-23, tsan).
//   - D (interlock missing): 不正遷移は Halted + shutdown_requested_ で構造的に拒否.
//   - F (legacy preconditions): is_always_lock_free static_assert (build 時検証).

#include "th25_ctrl/safety_core_orchestrator.hpp"

#include "th25_ctrl/common_types.hpp"
#include "th25_ctrl/in_process_queue.hpp"

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <chrono>
#include <thread>
#include <type_traits>

namespace th25_ctrl {

using EventQueue = SafetyCoreOrchestrator::EventQueue;

// ============================================================================
// UT-201-01: 初期状態 = Init
// ============================================================================
TEST(SafetyCoreOrchestrator_Initial, IsInit) {
    EventQueue q;
    SafetyCoreOrchestrator orch{q};
    EXPECT_EQ(orch.current_state(), LifecycleState::Init);
}

// ============================================================================
// UT-201-02: init_subsystems() で Init → SelfCheck
// ============================================================================
TEST(SafetyCoreOrchestrator_InitSubsystems, TransitionsToSelfCheck) {
    EventQueue q;
    SafetyCoreOrchestrator orch{q};

    auto result = orch.init_subsystems();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(orch.current_state(), LifecycleState::SelfCheck);
}

// ============================================================================
// UT-201-03: init_subsystems() の二重呼出は InternalUnexpectedState を返す
// (SDD §4.2 事前条件「main から 1 回のみ呼出」の構造的保護).
// ============================================================================
TEST(SafetyCoreOrchestrator_InitSubsystems, SecondCallReturnsInternalUnexpectedState) {
    EventQueue q;
    SafetyCoreOrchestrator orch{q};

    auto first = orch.init_subsystems();
    ASSERT_TRUE(first.has_value());

    auto second = orch.init_subsystems();
    EXPECT_FALSE(second.has_value());
    EXPECT_EQ(second.error_code(), ErrorCode::InternalUnexpectedState);

    // 状態は SelfCheck のまま (Init には戻らない).
    EXPECT_EQ(orch.current_state(), LifecycleState::SelfCheck);
}

// ============================================================================
// UT-201-04: std::atomic<LifecycleState> は always lock-free (HZ-007 構造的予防)
// ============================================================================
TEST(SafetyCoreOrchestrator_LockFree, AtomicLifecycleStateIsAlwaysLockFree) {
    EXPECT_TRUE(std::atomic<LifecycleState>::is_always_lock_free);
    EXPECT_TRUE(std::atomic<bool>::is_always_lock_free);
}

// ============================================================================
// UT-201-05: コピー/ムーブ禁止 (compile-time)
// ============================================================================
TEST(SafetyCoreOrchestrator_Ownership, IsNotCopyableNorMovable) {
    using O = SafetyCoreOrchestrator;
    static_assert(!std::is_copy_constructible_v<O>);
    static_assert(!std::is_copy_assignable_v<O>);
    static_assert(!std::is_move_constructible_v<O>);
    static_assert(!std::is_move_assignable_v<O>);
}

// ============================================================================
// UT-201-06: shutdown() は signal-safe (noexcept) で shutdown_requested_ のみ立てる
// (state は呼出時のまま. SDD §4.2 「signal handler から呼出可」)
// ============================================================================
TEST(SafetyCoreOrchestrator_Shutdown, SetsShutdownFlagWithoutChangingState) {
    EventQueue q;
    SafetyCoreOrchestrator orch{q};

    static_assert(noexcept(orch.shutdown()));

    auto init = orch.init_subsystems();
    ASSERT_TRUE(init.has_value());
    EXPECT_EQ(orch.current_state(), LifecycleState::SelfCheck);

    orch.shutdown();
    // shutdown() は state を変更しない (BeamState 制御は Step 20+).
    EXPECT_EQ(orch.current_state(), LifecycleState::SelfCheck);
}

// ============================================================================
// UT-201-07: shutdown() を立てた状態で run_event_loop は即座に 0 を返す
// (Halted/Error 以外の状態では正常終了)
// ============================================================================
TEST(SafetyCoreOrchestrator_EventLoop, ReturnsZeroWhenShutdownFromIdle) {
    EventQueue q;
    SafetyCoreOrchestrator orch{q};

    // Init → SelfCheck → SelfCheckPassed → Idle.
    ASSERT_TRUE(orch.init_subsystems().has_value());
    ASSERT_TRUE(q.try_publish({LifecycleEventKind::SelfCheckPassed, std::nullopt}));

    std::thread loop([&]() { orch.shutdown(); });
    loop.join();

    // shutdown 後に loop を起動 (1 イテレーションで終了).
    // Idle 状態のままなので戻り値は 0.
    auto rc = orch.run_event_loop();
    EXPECT_EQ(rc, 0);
}

// ============================================================================
// UT-201-08〜17: 静的遷移許可表 (next_state) — SDD §6.1 表との完全一致
// ============================================================================
TEST(SafetyCoreOrchestrator_Transition, SelfCheckPassedToIdle) {
    auto n = SafetyCoreOrchestrator::next_state(
        LifecycleState::SelfCheck, LifecycleEventKind::SelfCheckPassed);
    ASSERT_TRUE(n.has_value());
    EXPECT_EQ(*n, LifecycleState::Idle);
}

TEST(SafetyCoreOrchestrator_Transition, SelfCheckFailedToError) {
    auto n = SafetyCoreOrchestrator::next_state(
        LifecycleState::SelfCheck, LifecycleEventKind::SelfCheckFailed);
    ASSERT_TRUE(n.has_value());
    EXPECT_EQ(*n, LifecycleState::Error);
}

TEST(SafetyCoreOrchestrator_Transition, IdleToPrescriptionSet) {
    auto n = SafetyCoreOrchestrator::next_state(
        LifecycleState::Idle, LifecycleEventKind::PrescriptionReceived);
    ASSERT_TRUE(n.has_value());
    EXPECT_EQ(*n, LifecycleState::PrescriptionSet);
}

TEST(SafetyCoreOrchestrator_Transition, PrescriptionSetToReady) {
    auto n = SafetyCoreOrchestrator::next_state(
        LifecycleState::PrescriptionSet, LifecycleEventKind::PrescriptionValidated);
    ASSERT_TRUE(n.has_value());
    EXPECT_EQ(*n, LifecycleState::Ready);
}

TEST(SafetyCoreOrchestrator_Transition, PrescriptionSetToIdleByReset) {
    auto n = SafetyCoreOrchestrator::next_state(
        LifecycleState::PrescriptionSet, LifecycleEventKind::PrescriptionReset);
    ASSERT_TRUE(n.has_value());
    EXPECT_EQ(*n, LifecycleState::Idle);
}

TEST(SafetyCoreOrchestrator_Transition, ReadyToBeamOn) {
    auto n = SafetyCoreOrchestrator::next_state(
        LifecycleState::Ready, LifecycleEventKind::BeamOnRequested);
    ASSERT_TRUE(n.has_value());
    EXPECT_EQ(*n, LifecycleState::BeamOn);
}

TEST(SafetyCoreOrchestrator_Transition, ReadyToIdleByReset) {
    auto n = SafetyCoreOrchestrator::next_state(
        LifecycleState::Ready, LifecycleEventKind::PrescriptionReset);
    ASSERT_TRUE(n.has_value());
    EXPECT_EQ(*n, LifecycleState::Idle);
}

TEST(SafetyCoreOrchestrator_Transition, BeamOnToIdleByBeamOff) {
    auto n = SafetyCoreOrchestrator::next_state(
        LifecycleState::BeamOn, LifecycleEventKind::BeamOffCompleted);
    ASSERT_TRUE(n.has_value());
    EXPECT_EQ(*n, LifecycleState::Idle);
}

TEST(SafetyCoreOrchestrator_Transition, BeamOnToIdleByDoseTarget) {
    auto n = SafetyCoreOrchestrator::next_state(
        LifecycleState::BeamOn, LifecycleEventKind::DoseTargetReached);
    ASSERT_TRUE(n.has_value());
    EXPECT_EQ(*n, LifecycleState::Idle);
}

TEST(SafetyCoreOrchestrator_Transition, BeamOnToHaltedByCriticalAlarm) {
    auto n = SafetyCoreOrchestrator::next_state(
        LifecycleState::BeamOn, LifecycleEventKind::CriticalAlarmRaised);
    ASSERT_TRUE(n.has_value());
    EXPECT_EQ(*n, LifecycleState::Halted);
}

// ============================================================================
// UT-201-18: 不正遷移 — Idle → BeamOn 直接は禁止 (RCM-001 中核)
// (Therac-25 事故主要因 D 「インターロック欠落」への構造的予防)
// ============================================================================
TEST(SafetyCoreOrchestrator_Transition, IdleToBeamOnIsForbidden) {
    auto n = SafetyCoreOrchestrator::next_state(
        LifecycleState::Idle, LifecycleEventKind::BeamOnRequested);
    EXPECT_FALSE(n.has_value());
}

// ============================================================================
// UT-201-19: Halted は終端状態 (ShutdownRequested 以外は遷移不可)
// ============================================================================
TEST(SafetyCoreOrchestrator_Transition, HaltedIsTerminal) {
    constexpr std::array<LifecycleEventKind, 10> kEvents{
        LifecycleEventKind::InitRequested,
        LifecycleEventKind::SelfCheckPassed,
        LifecycleEventKind::SelfCheckFailed,
        LifecycleEventKind::PrescriptionReceived,
        LifecycleEventKind::PrescriptionValidated,
        LifecycleEventKind::PrescriptionReset,
        LifecycleEventKind::BeamOnRequested,
        LifecycleEventKind::BeamOffCompleted,
        LifecycleEventKind::DoseTargetReached,
        LifecycleEventKind::CriticalAlarmRaised,
    };
    for (auto e : kEvents) {
        auto n = SafetyCoreOrchestrator::next_state(LifecycleState::Halted, e);
        EXPECT_FALSE(n.has_value())
            << "Halted should reject event " << static_cast<int>(e);
    }

    // ShutdownRequested は Halted で受け入れられ、Halted のまま.
    auto n = SafetyCoreOrchestrator::next_state(
        LifecycleState::Halted, LifecycleEventKind::ShutdownRequested);
    ASSERT_TRUE(n.has_value());
    EXPECT_EQ(*n, LifecycleState::Halted);
}

// ============================================================================
// UT-201-20: ShutdownRequested は任意状態から Halted へ (緊急停止)
// ============================================================================
TEST(SafetyCoreOrchestrator_Transition, ShutdownRequestedAlwaysGoesToHalted) {
    constexpr std::array<LifecycleState, 8> kAllStates{
        LifecycleState::Init,
        LifecycleState::SelfCheck,
        LifecycleState::Idle,
        LifecycleState::PrescriptionSet,
        LifecycleState::Ready,
        LifecycleState::BeamOn,
        LifecycleState::Halted,
        LifecycleState::Error,
    };
    for (auto s : kAllStates) {
        auto n = SafetyCoreOrchestrator::next_state(
            s, LifecycleEventKind::ShutdownRequested);
        ASSERT_TRUE(n.has_value())
            << "ShutdownRequested should be accepted from state "
            << static_cast<int>(s);
        EXPECT_EQ(*n, LifecycleState::Halted);
    }
}

// ============================================================================
// UT-201-21: 遷移許可表の網羅試験 — 8 状態 × 11 イベント = 88 組合せ
// 許可された 12 件 (SDD §6.1 表 + ShutdownRequested 8 件) と
// 拒否された 76 件 を全件確認. SDD §6.1 表との完全一致を機械検証.
// ============================================================================
TEST(SafetyCoreOrchestrator_Transition, ExhaustiveTableMatchesSdd) {
    constexpr std::array<LifecycleState, 8> kAllStates{
        LifecycleState::Init,
        LifecycleState::SelfCheck,
        LifecycleState::Idle,
        LifecycleState::PrescriptionSet,
        LifecycleState::Ready,
        LifecycleState::BeamOn,
        LifecycleState::Halted,
        LifecycleState::Error,
    };
    constexpr std::array<LifecycleEventKind, 11> kAllEvents{
        LifecycleEventKind::InitRequested,
        LifecycleEventKind::SelfCheckPassed,
        LifecycleEventKind::SelfCheckFailed,
        LifecycleEventKind::PrescriptionReceived,
        LifecycleEventKind::PrescriptionValidated,
        LifecycleEventKind::PrescriptionReset,
        LifecycleEventKind::BeamOnRequested,
        LifecycleEventKind::BeamOffCompleted,
        LifecycleEventKind::DoseTargetReached,
        LifecycleEventKind::CriticalAlarmRaised,
        LifecycleEventKind::ShutdownRequested,
    };

    int allowed_count = 0;
    int rejected_count = 0;
    for (auto s : kAllStates) {
        for (auto e : kAllEvents) {
            const bool allowed = SafetyCoreOrchestrator::is_transition_allowed(s, e);
            if (allowed) {
                ++allowed_count;
                // is_transition_allowed と next_state は整合.
                EXPECT_TRUE(SafetyCoreOrchestrator::next_state(s, e).has_value());
            } else {
                ++rejected_count;
                EXPECT_FALSE(SafetyCoreOrchestrator::next_state(s, e).has_value());
            }
        }
    }

    // 期待値 (SDD §6.1 表との一致):
    //   論理遷移 11 件:
    //     Init/InitRequested(1) + SelfCheck/{Pass,Fail}(2) + Idle/PrescriptionRecv(1)
    //     + PrescriptionSet/{Validated,Reset}(2) + Ready/{BeamOn,Reset}(2)
    //     + BeamOn/{BeamOff,DoseTarget,Critical}(3) = 11
    //   ShutdownRequested 8 件 (任意状態 → Halted, Halted/Error 含む)
    //   許可合計 = 11 + 8 = 19, 拒否 = 8 * 11 - 19 = 69
    EXPECT_EQ(allowed_count, 11 + 8);
    EXPECT_EQ(rejected_count, (8 * 11) - (11 + 8));
}

// ============================================================================
// UT-201-22: イベントループ — キューに投入したイベントを状態機械で処理
// (init_subsystems → SelfCheckPassed → PrescriptionReceived → ShutdownRequested)
// ============================================================================
TEST(SafetyCoreOrchestrator_EventLoop, DispatchesQueuedEventsThroughStateMachine) {
    EventQueue q;
    SafetyCoreOrchestrator orch{q};

    ASSERT_TRUE(orch.init_subsystems().has_value());
    EXPECT_EQ(orch.current_state(), LifecycleState::SelfCheck);

    ASSERT_TRUE(q.try_publish({LifecycleEventKind::SelfCheckPassed, std::nullopt}));
    ASSERT_TRUE(q.try_publish({LifecycleEventKind::PrescriptionReceived, std::nullopt}));
    ASSERT_TRUE(q.try_publish({LifecycleEventKind::ShutdownRequested, std::nullopt}));

    auto rc = orch.run_event_loop();
    // ShutdownRequested で Halted に遷移するため、戻り値は 1.
    EXPECT_EQ(rc, 1);
    EXPECT_EQ(orch.current_state(), LifecycleState::Halted);
}

// ============================================================================
// UT-201-23: 不正遷移は Halted + run_event_loop 終了 (戻り値 1)
// (Idle 状態で BeamOnRequested を投入 → Halted)
// ============================================================================
TEST(SafetyCoreOrchestrator_EventLoop, IllegalEventTransitionsToHaltedAndExits) {
    EventQueue q;
    SafetyCoreOrchestrator orch{q};

    // Init → SelfCheck → Idle.
    ASSERT_TRUE(orch.init_subsystems().has_value());
    ASSERT_TRUE(q.try_publish({LifecycleEventKind::SelfCheckPassed, std::nullopt}));

    // Idle で BeamOnRequested は不正 (PrescriptionSet → Ready を経由しないため).
    ASSERT_TRUE(q.try_publish({LifecycleEventKind::BeamOnRequested, std::nullopt}));

    auto rc = orch.run_event_loop();
    EXPECT_EQ(rc, 1);
    EXPECT_EQ(orch.current_state(), LifecycleState::Halted);
}

// ============================================================================
// UT-201-24: 並行処理 — 1 producer (event publisher) + 1 consumer (orchestrator)
// SDD §9 SEP-003 + RCM-002/019 / HZ-002 直接対応の中核試験.
// tsan プリセットで race condition 検出 0 を期待.
// ============================================================================
TEST(SafetyCoreOrchestrator_Concurrency, SpscEventDeliveryIsRaceFree) {
    EventQueue q;
    SafetyCoreOrchestrator orch{q};

    // Init → SelfCheck.
    ASSERT_TRUE(orch.init_subsystems().has_value());

    // Producer: 一連のイベント (SelfCheckPassed → PrescriptionRecv → Validated
    //           → BeamOnReq → DoseTargetReached → ShutdownReq) を順次投入.
    constexpr int kCycles = 200;
    std::thread producer([&]() {
        for (int i = 0; i < kCycles; ++i) {
            const std::array<LifecycleEventKind, 6> seq{
                LifecycleEventKind::SelfCheckPassed,        // 初回のみ有効
                LifecycleEventKind::PrescriptionReceived,
                LifecycleEventKind::PrescriptionValidated,
                LifecycleEventKind::BeamOnRequested,
                LifecycleEventKind::DoseTargetReached,
                LifecycleEventKind::PrescriptionReceived,   // (次サイクル開始)
            };
            for (auto kind : seq) {
                while (!q.try_publish({kind, std::nullopt})) {
                    std::this_thread::yield();
                }
            }
        }
        // 最後に ShutdownRequested を投入してループ終了.
        while (!q.try_publish({LifecycleEventKind::ShutdownRequested, std::nullopt})) {
            std::this_thread::yield();
        }
    });

    // Consumer: orchestrator の run_event_loop が単一 consumer として動作.
    auto rc = orch.run_event_loop();

    producer.join();

    // ShutdownRequested 受信で Halted 終了 → 戻り値 1.
    EXPECT_EQ(rc, 1);
    EXPECT_EQ(orch.current_state(), LifecycleState::Halted);
}

}  // namespace th25_ctrl
