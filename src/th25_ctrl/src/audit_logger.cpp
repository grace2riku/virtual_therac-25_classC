// SPDX-License-Identifier: TBD
// TH25-CTRL: UNIT-209 AuditLogger implementation (Inc.4 で本格化、本 v0.1 では空殻).
//
// SDD-TH25-001 v0.1.1 §4.10 と整合する安全関連イベントの集約・永続化ユニット.
// 本 v0.1 範囲では IF 骨格 + std::cerr プレースホルダ出力 + 空ループ + stop シグナルのみ.
// Inc.4 で MessageBus consume + ファイル永続化 + fsync を追加予定.

#include "th25_ctrl/audit_logger.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

namespace th25_ctrl {

namespace {

// EventType を JSON 文字列リテラルに変換 (空殻範囲、Inc.2/4 で拡張時に追従).
auto to_string(EventType type) noexcept -> const char* {
    switch (type) {
        case EventType::Info:        return "Info";
        case EventType::Warning:     return "Warning";
        case EventType::Error:       return "Error";
        case EventType::SafetyAlarm: return "SafetyAlarm";
    }
    // unreachable in practice (enum class with closed value set), but
    // defensive default for compiler analysis.
    return "Unknown";
}

// std::string を JSON 文字列リテラルとしてエスケープ (空殻範囲では
// バックスラッシュとダブルクォートのみ最小エスケープ. Inc.4 で本格化).
auto json_escape(const std::string& src) -> std::string {
    std::string out;
    out.reserve(src.size() + 2);
    for (const char c : src) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                out += c;
                break;
        }
    }
    return out;
}

}  // namespace

// ============================================================================
// SDD §4.10 公開 API.
// ============================================================================

auto AuditLogger::publish(const AuditLogEntry& entry) noexcept -> void {
    // 空殻実装: std::cerr に JSON Lines 1 行を出力 (永続化は Inc.4).
    //
    // memory ordering / synchronization:
    //   - std::mutex output_mutex_ で複数スレッドからの並行 publish() 呼出時の
    //     出力行混在を防ぐ (std::cerr は thread-safe だが行単位のアトミック性は
    //     保証されないため別途必要).
    //   - processed_count_.fetch_add(release): processed_count() reader との
    //     happens-before 関係確立 + read-modify-write のアトミック性.
    //
    // Inc.4 で追加予定:
    //   - MessageBus.try_publish(entry) で非同期化 (本実装の同期出力は廃止).
    //   - 構造化 fields の JSON 出力.
    //   - ファイル永続化 + fsync.
    try {
        const auto epoch_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                entry.timestamp.time_since_epoch()).count();

        std::ostringstream oss;
        oss << "{\"timestamp_ns\":" << epoch_ns
            << ",\"event_type\":\"" << to_string(entry.event_type) << "\""
            << ",\"message\":\"" << json_escape(entry.message) << "\"}\n";

        {
            const std::lock_guard<std::mutex> lock(output_mutex_);
            std::cerr << oss.str();
        }
    } catch (...) {
        // 空殻範囲は best-effort. 出力失敗時も例外送出なし (noexcept).
        // Inc.4 でファイル永続化失敗時のリトライ・エラーログ転送等を本格化.
    }

    processed_count_.fetch_add(1U, std::memory_order_release);
}

auto AuditLogger::run_io_thread() noexcept -> void {
    // 空殻ループ: stop_requested_ が true になるまで kIoLoopIntervalMs 間隔で反復.
    //
    // memory ordering:
    //   - stop_requested_.load(acquire): stop() の release store と pair で
    //     同期 (RCM-002 release-acquire ordering).
    //
    // Inc.4 で追加予定:
    //   - bus_.try_consume<AuditLogEntry>() が成功したらファイルに append-only
    //     書込 + fsync.
    //   - rotate / size 制限.
    //   - 終了時の pending エントリ flush.
    while (!stop_requested_.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(kIoLoopIntervalMs);
    }
}

auto AuditLogger::stop() noexcept -> void {
    stop_requested_.store(true, std::memory_order_release);
}

// ============================================================================
// 補助 API (UT / Inc.4 拡張点).
// ============================================================================

auto AuditLogger::is_stop_requested() const noexcept -> bool {
    return stop_requested_.load(std::memory_order_acquire);
}

auto AuditLogger::pending_count() const noexcept -> std::uint64_t {
    // 空殻範囲: publish() は同期出力のため常に 0.
    // Inc.4 拡張点: MessageBus 受信エントリ数 - 処理済エントリ数 に拡張.
    return 0U;
}

auto AuditLogger::processed_count() const noexcept -> std::uint64_t {
    return processed_count_.load(std::memory_order_acquire);
}

auto AuditLogger::reset_for_test() noexcept -> void {
    stop_requested_.store(false, std::memory_order_release);
    processed_count_.store(0U, std::memory_order_release);
}

}  // namespace th25_ctrl
