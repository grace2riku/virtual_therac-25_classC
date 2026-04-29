// SPDX-License-Identifier: TBD (See LICENSE in repo root once defined)
// TH25-CTRL: UNIT-201 SafetyCoreOrchestrator
//
// IEC 62304 Class C, C++20.
// Implementation of SDD-TH25-001 v0.1.1 §4.2 (UNIT-201) and §6.1
// (LifecycleState 状態機械).
//
// 役割: Safety Core Process のエントリポイント. 単一スレッドのイベントループで
// LifecycleState 8 状態の状態機械を駆動する. 入力は UNIT-401 InProcessQueue
// 経由の LifecycleEvent. 各 Manager (UNIT-202〜206) との結線は後続 Step で実装.
//
// SDD §4.2 公開 API (4 種):
//   - init_subsystems()  : Init → SelfCheck (起動時 1 回呼出)
//   - run_event_loop()   : メインループ (戻り値: 0=正常 / 非 0=異常)
//   - current_state()    : 現在状態の取得 (read-only, noexcept)
//   - shutdown()         : 緊急停止 (signal-safe, noexcept)
//
// Therac-25 hazard mapping:
//   - HZ-001 (mode/target mismatch): 状態機械による不正遷移の構造的拒否
//     (RCM-001). Idle→BeamOn 直接遷移 / BeamOn→Idle (BeamOff 通知なし) 等の
//     不正遷移は Halted + ModeInvalidTransition で物理的再起動を要求.
//   - HZ-002 (race condition): state_ を std::atomic<LifecycleState> で保持
//     (release-acquire ordering). イベントは UNIT-401 SPSC lock-free queue
//     経由で配送 (SDD §9 SEP-003 構造的予防).
//   - HZ-007 (legacy preconditions): static_assert(is_always_lock_free) で
//     コンパイラ・標準ライブラリ更新時にビルド時 fail-stop.

#pragma once

#include "th25_ctrl/common_types.hpp"
#include "th25_ctrl/in_process_queue.hpp"

#include <atomic>
#include <cstdint>
#include <optional>

namespace th25_ctrl {

// ============================================================================
// LifecycleEventKind (SDD §6.1 表のイベント列を列挙)
//
// 状態遷移を駆動する入力イベントの種類. 各 Manager / Operator UI / SafetyMonitor
// から SDD §5.1 IF-U-001〜007 / IF-E-001 経由で配送される論理イベントを集約する.
// 後続 Step (UNIT-202〜206 結線時) では各メッセージ型を本 enum に変換するアダプタ
// を提供する想定.
// ============================================================================
enum class LifecycleEventKind : std::uint8_t {
    InitRequested = 0,         // Init → SelfCheck (init_subsystems() 内部発行)
    SelfCheckPassed = 1,       // SelfCheck → Idle
    SelfCheckFailed = 2,       // SelfCheck → Error
    PrescriptionReceived = 3,  // Idle → PrescriptionSet
    PrescriptionValidated = 4, // PrescriptionSet → Ready
    PrescriptionReset = 5,     // PrescriptionSet/Ready → Idle
    BeamOnRequested = 6,       // Ready → BeamOn
    BeamOffCompleted = 7,      // BeamOn → Idle
    DoseTargetReached = 8,     // BeamOn → Idle
    CriticalAlarmRaised = 9,   // BeamOn → Halted (任意状態でも Halted へ)
    ShutdownRequested = 10,    // 任意状態 → Halted (緊急停止イベント)
};

// ============================================================================
// LifecycleEvent (状態機械の入力).
// ErrorCode は SelfCheckFailed / CriticalAlarmRaised / ShutdownRequested 等の
// 文脈で詳細を伝達するために利用する. 本 Step では state 遷移のみを担当し、
// ErrorCode の AuditLogger / AlarmDisplay へのディスパッチは Step 20+ で結線する.
// ============================================================================
struct LifecycleEvent {
    LifecycleEventKind kind;
    std::optional<ErrorCode> error_code;
};

// ============================================================================
// SafetyCoreOrchestrator (SDD §4.2 UNIT-201).
// ============================================================================
class SafetyCoreOrchestrator {
public:
    // SDD §6.5 MessageBusCapacity = 4096.
    using EventQueue = InProcessQueue<LifecycleEvent, kDefaultMessageBusCapacity>;

    explicit SafetyCoreOrchestrator(EventQueue& events) noexcept;

    // SPSC 同様、所有権を一意に保つためコピー/ムーブ禁止.
    SafetyCoreOrchestrator(const SafetyCoreOrchestrator&) = delete;
    SafetyCoreOrchestrator(SafetyCoreOrchestrator&&) = delete;
    auto operator=(const SafetyCoreOrchestrator&) -> SafetyCoreOrchestrator& = delete;
    auto operator=(SafetyCoreOrchestrator&&) -> SafetyCoreOrchestrator& = delete;
    ~SafetyCoreOrchestrator() = default;

    // ----- SDD §4.2 公開 API -----

    // Init → SelfCheck への遷移. プロセス起動直後に main から 1 回のみ呼出.
    // 既に Init 以外で呼ばれた場合は InternalUnexpectedState を返す
    // (compare_exchange による idempotency 保護).
    [[nodiscard]] auto init_subsystems() noexcept -> Result<void, ErrorCode>;

    // メインイベントループ. shutdown_requested_ が立つまでメッセージを処理する.
    // 戻り値: 0 = 正常終了 / 1 = Halted/Error で終了 (SDD §4.2 「非 0 = 異常」).
    [[nodiscard]] auto run_event_loop() noexcept -> int;

    // 現在状態を取得 (read-only). std::atomic acquire load.
    [[nodiscard]] auto current_state() const noexcept -> LifecycleState;

    // 緊急停止. signal-safe な実装 (lock-free atomic store のみ).
    // state は変更せず、shutdown_requested_ のみ立てる.
    // run_event_loop が次イテレーションで終了する.
    auto shutdown() noexcept -> void;

    // ----- 静的純粋関数 (UT / 設計レビュー用に公開) -----

    // SDD §6.1 状態遷移許可表に従い、(from, event) → 次状態 を計算する.
    // 許可されない場合は std::nullopt を返す.
    [[nodiscard]] static auto next_state(
        LifecycleState from, LifecycleEventKind event) noexcept
        -> std::optional<LifecycleState>;

    // (from, event) の組み合わせが許可された遷移か判定.
    [[nodiscard]] static auto is_transition_allowed(
        LifecycleState from, LifecycleEventKind event) noexcept -> bool;

private:
    // 単一イベントを処理する内部ハンドラ. 不正遷移時は Halted + shutdown_requested_.
    auto handle_event(const LifecycleEvent& event) noexcept -> void;

    EventQueue& events_;
    std::atomic<LifecycleState> state_{LifecycleState::Init};
    std::atomic<bool> shutdown_requested_{false};

    // ----- HZ-007 構造的予防 (SDD §7 SOUP-003/004 機能要求) -----
    static_assert(std::atomic<LifecycleState>::is_always_lock_free,
        "std::atomic<LifecycleState> must be always lock-free for "
        "SafetyCoreOrchestrator (SDD §4.2, RCM-001, HZ-002/HZ-007 structural prevention).");
    static_assert(std::atomic<bool>::is_always_lock_free,
        "std::atomic<bool> must be always lock-free for shutdown_requested_ "
        "(SDD §4.2 signal-safe shutdown semantics).");
};

}  // namespace th25_ctrl
