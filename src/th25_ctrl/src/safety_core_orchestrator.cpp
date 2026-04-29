// SPDX-License-Identifier: TBD
// TH25-CTRL: UNIT-201 SafetyCoreOrchestrator implementation.
//
// SDD-TH25-001 v0.1.1 §4.2 / §6.1 と完全一致する状態機械実装.

#include "th25_ctrl/safety_core_orchestrator.hpp"

#include <thread>
#include <utility>

namespace th25_ctrl {

namespace {

// SDD §6.1 状態遷移許可表をハードコード. compute_next_state は副作用なしの
// 純粋関数として実装し、is_transition_allowed / next_state / handle_event の
// 共通実装として参照される.
//
// 戻り値:
//   - has_value(): 許可された遷移. 値は次状態.
//   - nullopt: 許可されない遷移 (handle_event は Halted へ強制遷移する).
//
// 特例:
//   - ShutdownRequested は任意状態 (Halted/Error 含む) で Halted へ遷移する
//     緊急停止イベント.
//   - Halted / Error からは ShutdownRequested 以外の遷移は不可.
[[nodiscard]] auto compute_next_state(
    LifecycleState from, LifecycleEventKind event) noexcept
    -> std::optional<LifecycleState> {
    using L = LifecycleState;
    using E = LifecycleEventKind;

    if (event == E::ShutdownRequested) {
        return L::Halted;
    }

    switch (from) {
        case L::Init:
            if (event == E::InitRequested) { return L::SelfCheck; }
            return std::nullopt;
        case L::SelfCheck:
            if (event == E::SelfCheckPassed) { return L::Idle; }
            if (event == E::SelfCheckFailed) { return L::Error; }
            return std::nullopt;
        case L::Idle:
            if (event == E::PrescriptionReceived) { return L::PrescriptionSet; }
            return std::nullopt;
        case L::PrescriptionSet:
            if (event == E::PrescriptionValidated) { return L::Ready; }
            if (event == E::PrescriptionReset) { return L::Idle; }
            return std::nullopt;
        case L::Ready:
            if (event == E::BeamOnRequested) { return L::BeamOn; }
            if (event == E::PrescriptionReset) { return L::Idle; }
            return std::nullopt;
        case L::BeamOn:
            if (event == E::BeamOffCompleted) { return L::Idle; }
            if (event == E::DoseTargetReached) { return L::Idle; }
            if (event == E::CriticalAlarmRaised) { return L::Halted; }
            return std::nullopt;
        case L::Halted:
        case L::Error:
        default:
            // ShutdownRequested 以外の遷移は不可 (上で早期 return 済み).
            return std::nullopt;
    }
}

}  // namespace

SafetyCoreOrchestrator::SafetyCoreOrchestrator(EventQueue& events) noexcept
    : events_(events) {}

auto SafetyCoreOrchestrator::next_state(
    LifecycleState from, LifecycleEventKind event) noexcept
    -> std::optional<LifecycleState> {
    return compute_next_state(from, event);
}

auto SafetyCoreOrchestrator::is_transition_allowed(
    LifecycleState from, LifecycleEventKind event) noexcept -> bool {
    return compute_next_state(from, event).has_value();
}

auto SafetyCoreOrchestrator::current_state() const noexcept -> LifecycleState {
    return state_.load(std::memory_order_acquire);
}

auto SafetyCoreOrchestrator::shutdown() noexcept -> void {
    // signal-safe: lock-free atomic release store のみ.
    // state は変更しない (現在状態のまま loop 終了させる). BeamState の
    // ビームオフ強制等は Step 20+ で BeamController と結線して実装する.
    shutdown_requested_.store(true, std::memory_order_release);
}

auto SafetyCoreOrchestrator::init_subsystems() noexcept
    -> Result<void, ErrorCode> {
    // SDD §4.2 事前条件: プロセス起動直後に main から 1 回のみ呼出.
    // 二重呼出 / 不正タイミングは compare_exchange の失敗で検出.
    LifecycleState expected = LifecycleState::Init;
    if (!state_.compare_exchange_strong(
            expected, LifecycleState::SelfCheck,
            std::memory_order_acq_rel, std::memory_order_acquire)) {
        return Result<void, ErrorCode>::error(
            ErrorCode::InternalUnexpectedState);
    }
    return Result<void, ErrorCode>::ok();
}

auto SafetyCoreOrchestrator::handle_event(const LifecycleEvent& event) noexcept
    -> void {
    const LifecycleState cur = state_.load(std::memory_order_acquire);
    const auto next = compute_next_state(cur, event.kind);

    LifecycleState resolved{};
    if (next.has_value()) {
        resolved = *next;
    } else {
        // SDD §4.2: 不正状態遷移試行 → Halted 遷移 + ModeInvalidTransition.
        // 本 Step では ErrorCode のディスパッチ先 (AuditLogger / AlarmDisplay)
        // が未結線のため、状態遷移のみで対応する. AuditLogger 結線は Step 20+.
        resolved = LifecycleState::Halted;
    }

    state_.store(resolved, std::memory_order_release);

    // 致死的状態に到達したらイベントループの終了をスケジュール.
    if (resolved == LifecycleState::Halted ||
        resolved == LifecycleState::Error) {
        shutdown_requested_.store(true, std::memory_order_release);
    }

    // 本 Step ではイベント詳細 (error_code) のディスパッチは行わない.
    // Step 20+ で AuditLogger / AlarmDisplay と結線する.
    static_cast<void>(event.error_code);
}

auto SafetyCoreOrchestrator::run_event_loop() noexcept -> int {
    while (!shutdown_requested_.load(std::memory_order_acquire)) {
        auto event = events_.try_consume();
        if (event.has_value()) {
            handle_event(*event);
        } else {
            // queue empty: yield してビジーウェイトの CPU 消費を抑える.
            // condition variable 等の待機機構は Step 20+ で導入を検討
            // (本 Step は SDD §4.2 と整合する範囲で yield のみ).
            std::this_thread::yield();
        }
    }

    // 終了時の状態に応じて戻り値を決定 (SDD §4.2 「0=正常 / 非 0=異常」).
    const LifecycleState final_state = state_.load(std::memory_order_acquire);
    if (final_state == LifecycleState::Halted ||
        final_state == LifecycleState::Error) {
        return 1;
    }
    return 0;
}

}  // namespace th25_ctrl
