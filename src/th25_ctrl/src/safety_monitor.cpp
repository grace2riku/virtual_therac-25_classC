// SPDX-License-Identifier: TBD
// TH25-CTRL: UNIT-207 SafetyMonitor implementation (Inc.2 で本格化、本 v0.1 では空殻).
//
// SDD-TH25-001 v0.1.1 §4.8 と整合する独立並行タスクのインターロック監視ユニット.
// 本 v0.1 範囲では IF 骨格 + 空ループ + stop シグナルのみ.
// Inc.2 で MessageBus consume + AuditLogger 転送 + 検知ロジックを追加予定.

#include "th25_ctrl/safety_monitor.hpp"

#include <thread>

namespace th25_ctrl {

// ============================================================================
// SDD §4.8 公開 API.
// ============================================================================

auto SafetyMonitor::run_monitor_thread() noexcept -> void {
    // 空殻ループ: stop_requested_ が true になるまで kLoopIntervalMs 間隔で反復.
    // 各反復で tick_count_ を 1 増加 (UT 駆動 + Inc.2 拡張点プレースホルダ).
    //
    // memory ordering:
    //   - stop_requested_.load(acquire): stop() の release store と pair で
    //     同期 (RCM-002 release-acquire ordering).
    //   - tick_count_.fetch_add(acq_rel): tick_count() reader との同期 +
    //     read-modify-write の atomicity.
    //
    // Inc.2 で追加予定:
    //   - bus_.try_consume<SafetyAlarm>() が成功したら audit_.publish(...).
    //   - センサ 3 系統不一致検知 (RCM-006、UNIT-205 has_discrepancy 経由).
    //   - 2 組ドーズ比較 (RCM-007、UNIT-304 read_dose 経由).
    //   - 起動時自己診断失敗検知 (UNIT-208 perform_self_check 結果監視).
    while (!stop_requested_.load(std::memory_order_acquire)) {
        tick_count_.fetch_add(1U, std::memory_order_acq_rel);
        std::this_thread::sleep_for(kLoopIntervalMs);
    }
}

auto SafetyMonitor::stop() noexcept -> void {
    stop_requested_.store(true, std::memory_order_release);
}

// ============================================================================
// 補助 API (UT / Inc.2 拡張点).
// ============================================================================

auto SafetyMonitor::is_stop_requested() const noexcept -> bool {
    return stop_requested_.load(std::memory_order_acquire);
}

auto SafetyMonitor::tick_count() const noexcept -> std::uint64_t {
    return tick_count_.load(std::memory_order_acquire);
}

auto SafetyMonitor::reset_for_test() noexcept -> void {
    stop_requested_.store(false, std::memory_order_release);
    tick_count_.store(0U, std::memory_order_release);
}

}  // namespace th25_ctrl
