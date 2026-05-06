// SPDX-License-Identifier: TBD (See LICENSE in repo root once defined)
// TH25-CTRL: UNIT-207 SafetyMonitor (Inc.2 で本格化、本 v0.1 では空殻 + IF のみ)
//
// IEC 62304 Class C, C++20.
// Implementation of SDD-TH25-001 v0.1.1 §4.8 (UNIT-207).
//
// 役割: 独立並行タスクのインターロック監視 (Inc.2 で本格化).
// **RCM-006(センサ多数決)/ RCM-007(2 組ドーズ比較)中核(Inc.2 で本格化)**.
//
// SDD §4.8 公開 API (本 v0.1 範囲):
//   - run_monitor_thread() : スレッド本体ループ. 本 v0.1 では stop_requested_ を
//                            acquire load し、false の間ループ継続 + 10 ms sleep.
//                            Inc.2 で MessageBus consume + 検知ロジック追加.
//   - stop()               : stop_requested_ を release store で true に設定.
//                            スレッド合流は呼出側責任 (SafetyMonitor 自身は
//                            std::thread を所有しない設計、UNIT-201 が所有).
//
// 補助 API (UT 駆動 + Inc.2 拡張点プレースホルダ):
//   - is_stop_requested()  : stop_requested_ の acquire load.
//   - tick_count()         : ループ反復回数 (空殻でも UT 駆動可能化、Inc.2 で
//                            MessageBus 受信回数等に拡張可能).
//   - reset_for_test()     : stop_requested_ + tick_count_ を初期化 (UT 専用、
//                            Inc.2 で削除候補).
//
// SDD §4.8 設計判断 (Step 30 範囲):
//   - 本 v0.1 では MessageBus / AuditLogger 依存を持たない (両者とも Inc.1 範囲外
//     の本格実装、SDD §4.8 サンプル擬似コードはあくまで Inc.2 の予告).
//   - run_monitor_thread() は呼出側 (UT または Inc.1 後半 UNIT-201) が別スレッド
//     で起動する設計. SafetyMonitor 自身は std::thread を所有しない (lifetime
//     管理を呼出側に委ねる、Inc.2 で所有設計を再検討予定).
//   - ループ間隔 10 ms は SDD §4.8 サンプル擬似コードに合わせた仮値. Inc.2 で
//     SRS-ALM-004 (連続 100 ms 検出) との整合確認 + 調整予定.
//
// Step 30 範囲制約:
//   SDD §4.8 サンプル擬似コードの `bus_.try_consume<SafetyAlarm>()` +
//   `audit_.publish(...)` は Inc.2 で本格化. 本 Step では空ループ + 10 ms sleep
//   のみ. SafetyAlarm 構造体 (IF-U-006) も Inc.2 で確定予定.
//   UNIT-201 SafetyCoreOrchestrator から SafetyMonitor のスレッド起動 dispatch
//   は Inc.1 後半 (observer pattern + Manager 結線) で完成.
//
// Therac-25 hazard mapping (Inc.2 で本格化、本 v0.1 では構造のみ):
//   - HZ-002 (race condition): std::atomic<bool> stop_requested_ + UT 並行 TSan
//     で機械検証. 空殻段階から並行設計の構造を確立.
//   - HZ-007 (legacy preconditions): static_assert(std::atomic<bool>::is_always_lock_free)
//     + static_assert(std::atomic<std::uint64_t>::is_always_lock_free) を
//     14 ユニット目に拡大 (UNIT-200/401/201/202/203/204/205/206/208/301/302/303/304/207).
//   - D 主要因 (interlock missing): Inc.2 で本格化. 本 v0.1 では構造的位置づけのみ
//     (SAD §3 ARCH-002.7 独立並行監視タスクとしての placeholder).

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

namespace th25_ctrl {

// ============================================================================
// SafetyMonitor (SDD §4.8 UNIT-207、Inc.2 で本格化、本 v0.1 では空殻).
// ============================================================================
//
// 設計方針 (空殻):
//   - 別スレッドで run_monitor_thread() を実行 (呼出側責任).
//   - stop() で stop_requested_ を true に設定 → ループ離脱 → スレッド合流可能.
//   - Inc.2 で MessageBus consume + AuditLogger 転送 + 検知ロジックを内部で実装
//     (本 v0.1 範囲では空ループ + 10 ms sleep のみ).
class SafetyMonitor {
public:
    // ループ間隔 (空殻、Inc.2 で SRS-ALM-004 整合確認後に調整予定).
    static constexpr std::chrono::milliseconds kLoopIntervalMs{10};

    // 空殻のため依存を持たない. Inc.2 で MessageBus& / AuditLogger& 引数追加予定
    // (別 CR、SDD v0.2 改訂と同時).
    SafetyMonitor() noexcept = default;

    // 所有権を一意に保つためコピー/ムーブ禁止 (内部 atomic 保持、別スレッドから
    // 参照される設計).
    SafetyMonitor(const SafetyMonitor&) = delete;
    SafetyMonitor(SafetyMonitor&&) = delete;
    auto operator=(const SafetyMonitor&) -> SafetyMonitor& = delete;
    auto operator=(SafetyMonitor&&) -> SafetyMonitor& = delete;
    ~SafetyMonitor() = default;

    // ----- SDD §4.8 公開 API (本 v0.1 範囲) -----

    // スレッド本体ループ. 呼出側 (UT または Inc.1 後半 UNIT-201) が別スレッドで
    // 起動する. stop_requested_ が true になるまで 10 ms 間隔でループを継続.
    //
    // 事前条件: 別スレッドから呼出されること (本関数自身はブロッキング).
    // 事後条件: stop_requested_ = true 検出後に return. tick_count_ は最終的な
    //           ループ反復回数を保持.
    // エラー処理: 本 v0.1 範囲では例外送出なし (noexcept).
    //
    // Inc.2 で追加予定:
    //   - MessageBus から SafetyAlarm を consume → AuditLogger へ転送.
    //   - センサ 3 系統不一致検知 (RCM-006).
    //   - 2 組ドーズ比較 (RCM-007).
    //   - 起動時自己診断失敗検知.
    auto run_monitor_thread() noexcept -> void;

    // スレッド停止要求. release store で stop_requested_ を true に設定.
    // 本関数は別スレッドからも安全に呼出可能 (atomic 経由).
    //
    // 事前条件: なし.
    // 事後条件: stop_requested_ = true. run_monitor_thread() のループは次反復で
    //           離脱する (最大 kLoopIntervalMs = 10 ms 遅延).
    auto stop() noexcept -> void;

    // ----- 補助 API (UT 駆動 + Inc.2 拡張点プレースホルダ) -----

    // stop_requested_ の現在値 (acquire load).
    [[nodiscard]] auto is_stop_requested() const noexcept -> bool;

    // run_monitor_thread() ループの反復回数 (acquire load).
    // 空殻段階: ループ反復ごとに 1 増加 (UT 駆動 + 並行試験用).
    // Inc.2 拡張点: MessageBus 受信回数 / 検知発生回数 等に意味を拡張可能.
    [[nodiscard]] auto tick_count() const noexcept -> std::uint64_t;

    // UT 専用: stop_requested_ + tick_count_ を初期化 (Inc.2 で削除候補).
    // 同一 SafetyMonitor インスタンスで複数回 run_monitor_thread() を試験する
    // 場合に使用.
    auto reset_for_test() noexcept -> void;

private:
    // スレッド停止フラグ. release-acquire memory ordering で複数スレッド間の
    // 可視性を保証 (RCM-002).
    std::atomic<bool> stop_requested_{false};

    // ループ反復カウンタ (UT 駆動 + Inc.2 拡張点プレースホルダ).
    std::atomic<std::uint64_t> tick_count_{0};

    // ----- HZ-007 構造的予防 (SDD §7 SOUP-003/004 機能要求) -----
    // UNIT-200/401/201/202/203/204/205/206/208/301/302/303/304 と同パターンを
    // 14 ユニット目に拡大 (Inc.1 範囲全 14 ユニット完成).
    static_assert(std::atomic<bool>::is_always_lock_free,
        "std::atomic<bool> must be always lock-free for SafetyMonitor "
        "stop_requested_ "
        "(SDD §4.8, RCM-002, HZ-002/HZ-007 structural prevention).");
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
        "std::atomic<std::uint64_t> must be always lock-free for SafetyMonitor "
        "tick_count_ "
        "(SDD §4.8, RCM-002, HZ-002/HZ-007 structural prevention).");
};

}  // namespace th25_ctrl
