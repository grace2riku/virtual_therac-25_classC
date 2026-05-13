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

#include "th25_ctrl/beam_controller.hpp"
#include "th25_ctrl/common_types.hpp"
#include "th25_ctrl/dose_manager.hpp"
#include "th25_ctrl/in_process_queue.hpp"
#include "th25_ctrl/safety_event_observer.hpp"

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <chrono>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace th25_ctrl {

using EventQueue = SafetyCoreOrchestrator::EventQueue;

// ============================================================================
// UT-201-01: 初期状態 = Init
// ============================================================================
TEST(SafetyCoreOrchestrator_Initial, IsInit) {
    EventQueue q;
    BeamController bc;
    SafetyCoreOrchestrator orch{q, bc};
    EXPECT_EQ(orch.current_state(), LifecycleState::Init);
}

// ============================================================================
// UT-201-02: init_subsystems() で Init → SelfCheck
// ============================================================================
TEST(SafetyCoreOrchestrator_InitSubsystems, TransitionsToSelfCheck) {
    EventQueue q;
    BeamController bc;
    SafetyCoreOrchestrator orch{q, bc};

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
    BeamController bc;
    SafetyCoreOrchestrator orch{q, bc};

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
    BeamController bc;
    SafetyCoreOrchestrator orch{q, bc};

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
    BeamController bc;
    SafetyCoreOrchestrator orch{q, bc};

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
    BeamController bc;
    SafetyCoreOrchestrator orch{q, bc};

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
    BeamController bc;
    SafetyCoreOrchestrator orch{q, bc};

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
    BeamController bc;
    SafetyCoreOrchestrator orch{q, bc};

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

// ============================================================================
// UT-201-25: on_safety_event(DoseTargetReached) で BeamController.request_beam_off()
//             が同期直接呼出される (Step 44 / CR-0030、SDD §4.5 サンプル UT 粒度).
// ============================================================================
//
// BeamController を Off → Arming → On に遷移させた状態で on_safety_event を呼出し、
// Stopping/Off に遷移することを確認. SafetyEventObserver 呼出契約「< 10 ms 以内」を
// 実装が満たすことを構造的に検証 (BeamController.request_beam_off() は atomic store
// のみで ns 単位で完了).
TEST(SafetyCoreOrchestrator_Observer, DoseTargetReachedTriggersBeamOff) {
    EventQueue q;
    BeamController bc;
    SafetyCoreOrchestrator orch{q, bc};

    // BeamController を On 状態に遷移させる前準備: 許可フラグ設定 + Ready で request_beam_on.
    bc.set_beam_on_permission(true);
    ASSERT_TRUE(bc.request_beam_on(LifecycleState::Ready).has_value());
    ASSERT_EQ(bc.current_state(), BeamState::On);

    // on_safety_event(DoseTargetReached) を直接呼出.
    // SafetyEventObserver* として呼出して virtual dispatch の整合性も確認.
    SafetyEventObserver* const observer = &orch;
    observer->on_safety_event(SafetyEvent::DoseTargetReached);

    // BeamController が Stopping または Off に遷移したことを確認 (SDD §4.4 状態機械).
    const BeamState after = bc.current_state();
    EXPECT_TRUE(after == BeamState::Stopping || after == BeamState::Off)
        << "BeamState after on_safety_event = " << static_cast<int>(after);
}

// ============================================================================
// UT-201-26: DoseManager → SafetyCoreOrchestrator → BeamController 連鎖試験
//             (Step 44 / CR-0030、SDD §4.5 サンプル「目標到達 → BeamOff < 1 ms 連鎖」
//              UT 粒度実証).
// ============================================================================
//
// attach + on_dose_pulse target 到達 → SafetyCoreOrchestrator の on_safety_event が
// 呼ばれ → BeamController.request_beam_off() が呼ばれ → BeamState が変化する経路を
// end-to-end で検証. IT-101 < 10 ms 実時間実測は Inc.1 完了 Step で実施.
TEST(SafetyCoreOrchestrator_Observer, EndToEndDoseTargetToBeamOff) {
    EventQueue q;
    BeamController bc;
    SafetyCoreOrchestrator orch{q, bc};

    // BeamController を On 状態に遷移.
    bc.set_beam_on_permission(true);
    ASSERT_TRUE(bc.request_beam_on(LifecycleState::Ready).has_value());
    ASSERT_EQ(bc.current_state(), BeamState::On);

    // DoseManager に SafetyCoreOrchestrator を observer として attach.
    DoseManager dm{DoseRatePerPulse_cGy_per_pulse{1.0}};  // 1 pulse = 1 cGy.
    dm.attach_observer(&orch);

    // 目標 3 cGy 設定 (Ready 状態) + 3 pulse 投入で target 到達.
    ASSERT_TRUE(dm.set_dose_target(DoseUnit_cGy{3.0}, LifecycleState::Ready).has_value());
    dm.on_dose_pulse(PulseCount{1});  // accumulated=1
    dm.on_dose_pulse(PulseCount{1});  // accumulated=2
    EXPECT_EQ(bc.current_state(), BeamState::On);  // 未到達 → BeamState 不変
    dm.on_dose_pulse(PulseCount{1});  // accumulated=3 = target → 到達 → on_safety_event 発火

    // BeamController が Stopping または Off に遷移したことを確認.
    const BeamState after = bc.current_state();
    EXPECT_TRUE(after == BeamState::Stopping || after == BeamState::Off)
        << "BeamState after target reach = " << static_cast<int>(after);
    EXPECT_TRUE(dm.is_target_reached());

    // Cleanup: detach (必須ではないが寿命の明確化のため).
    dm.detach_observer();
}

// ============================================================================
// UT-201-27: 並行 producer pulse + 多 attacher/detacher + on_safety_event race-free
//             (Step 44 / CR-0030、`tsan` プリセット必須、HZ-002 機械的予防が
//              dispatch 機構結線にも展開).
// ============================================================================
//
// 1 producer (on_dose_pulse 5000 回) + 4 attacher/detacher 並行で
// `observer_` atomic + `on_safety_event` 経路の race-free を TSan で機械検証.
// CR-0021 教訓水平展開: attacher/detacher は並行多重なので do-while パターン適用.
// target は SRS-008 範囲内 10000 cGy (CR-0029 制定の SRS 範囲内セルフチェック適用、
// UT-204-37 同パターンを参照、producer 5000 pulse では到達しない設計).
TEST(SafetyCoreOrchestrator_Observer, ConcurrentAttachDetachIsRaceFree) {
    EventQueue q;
    BeamController bc;
    SafetyCoreOrchestrator orch{q, bc};

    DoseManager dm{DoseRatePerPulse_cGy_per_pulse{1.0}};
    // target を SRS-008 範囲内 10000 cGy (= 10000 pulses) に設定し、producer の 5000 pulse
    // では到達しないようにする (PRB-0008 / CR-0029 教訓: SRS 範囲内セルフチェック適用、
    // UT-204-37 同パターン).
    ASSERT_TRUE(dm.set_dose_target(DoseUnit_cGy{10000.0}, LifecycleState::Ready).has_value());

    constexpr int kIterations = 5000;
    std::atomic<bool> stop{false};

    // producer: 1 kHz 想定の連続 pulse (到達しないため observer notify は発火しない).
    std::thread producer([&]() {
        for (int i = 0; i < kIterations; ++i) {
            dm.on_dose_pulse(PulseCount{1});
        }
        stop.store(true, std::memory_order_release);
    });

    // 4 thread が並行に attach/detach を交互実行.
    // do-while パターンで最低 1 回 body 実行を構造的に保証
    // (CR-0021 教訓水平展開、PRB-0005 / PRB-0006 同根本原因対策).
    constexpr int kAttachers = 4;
    std::vector<std::thread> attachers;
    attachers.reserve(kAttachers);
    for (int i = 0; i < kAttachers; ++i) {
        attachers.emplace_back([&, i]() {
            bool attach_phase = (i % 2 == 0);
            do {
                if (attach_phase) {
                    dm.attach_observer(&orch);
                } else {
                    dm.detach_observer();
                }
                attach_phase = !attach_phase;
            } while (!stop.load(std::memory_order_acquire));
        });
    }

    producer.join();
    for (auto& t : attachers) {
        t.join();
    }

    // TSan が race を検出しなければ SUCCEED. observer 通知は target 未到達のため発火せず、
    // `observer_` atomic 自体の attach/detach 並行 race-free のみを検証.
    SUCCEED();
}

// ============================================================================
// UT-201-28: SafetyCoreOrchestrator は SafetyEventObserver を継承 + on_safety_event
//             は noexcept (compile-time 表明、Step 44 / CR-0030).
// ============================================================================
TEST(SafetyCoreOrchestrator_Observer, IsSafetyEventObserverAndNoexcept) {
    // 継承関係の compile-time 表明.
    static_assert(std::is_base_of_v<SafetyEventObserver, SafetyCoreOrchestrator>,
        "SafetyCoreOrchestrator must inherit from SafetyEventObserver "
        "(Step 44 / CR-0030).");

    // on_safety_event は noexcept (SafetyEventObserver 呼出契約).
    static_assert(noexcept(std::declval<SafetyCoreOrchestrator&>().on_safety_event(
        SafetyEvent::DoseTargetReached)),
        "SafetyCoreOrchestrator::on_safety_event must be noexcept "
        "(SafetyEventObserver contract).");
}

}  // namespace th25_ctrl
