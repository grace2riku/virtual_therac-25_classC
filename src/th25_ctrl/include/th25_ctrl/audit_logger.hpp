// SPDX-License-Identifier: TBD (See LICENSE in repo root once defined)
// TH25-CTRL: UNIT-209 AuditLogger (Inc.4 で本格化、本 v0.1 では空殻 + IF のみ)
//
// IEC 62304 Class C, C++20.
// Implementation of SDD-TH25-001 v0.1.1 §4.10 (UNIT-209).
//
// 役割: 全 UNIT から発信される安全関連イベントを集約し、append-only ログとして
//       永続化する独立 IO スレッドユニット (Inc.4 で本格化).
// **HZ-006 (暗号的エラーメッセージ) 構造的位置づけ + RCM-011 (append-only 完全性) /
//   RCM-016 (processVariableLogger) 中核 (Inc.4 で本格化)**.
//
// SDD §4.10 公開 API (本 v0.1 範囲):
//   - publish(AuditLogEntry) : ログエントリを受領. 本 v0.1 では std::cerr へ
//                              JSON Lines 1 行直接出力 (永続化は Inc.4).
//                              複数 UNIT スレッドからの並行呼出に対応
//                              (std::mutex で出力アトミック性のみ保証).
//   - run_io_thread()        : 独立 IO スレッドのループ. 本 v0.1 では
//                              stop_requested_ を acquire load し false の間
//                              ループ継続 + 10 ms sleep (Inc.4 で MessageBus
//                              consume + fsync 付き書込追加).
//
// 補助 API (UT 駆動 + Inc.4 拡張点プレースホルダ):
//   - is_stop_requested()    : stop_requested_ の acquire load.
//   - pending_count()        : 未処理エントリ数 (空殻範囲では publish() で 0
//                              維持、Inc.4 で MessageBus 受信時に増加).
//   - processed_count()      : 処理済エントリ数 (publish() 呼出毎に 1 増加、
//                              UT 駆動 + Inc.4 拡張点).
//   - stop()                 : stop_requested_ を release store で true に設定.
//   - reset_for_test()       : stop_requested_ + processed_count_ を初期化
//                              (UT 専用、Inc.4 で削除候補).
//   - try_consume_for_test() : 直近の processed_count_ を返す UT 補助 (Inc.4
//                              で削除候補).
//
// SDD §4.10 設計判断 (Step 31 範囲):
//   - 本 v0.1 では MessageBus / Filesystem 依存を持たない (両者とも Inc.4 で
//     本格化、SDD §4.10 サンプルはあくまで Inc.4 の予告).
//   - publish() は std::cerr へ JSON Lines 1 行を直接出力 (std::mutex で
//     複数スレッドからの並行呼出時の出力アトミック性のみ保証、std::atomic では
//     stream 出力のアトミック性が確保できないため別途必要).
//   - run_io_thread() は呼出側 (UT または Inc.1 後半 UNIT-201) が別スレッド
//     で起動する設計. AuditLogger 自身は std::thread を所有しない (lifetime
//     管理を呼出側に委ねる、UNIT-207 と同方針、Inc.4 で所有設計を再検討予定).
//   - ループ間隔 10 ms は SDD §4.10 の予告と整合. Inc.4 で永続化レイテンシ要求
//     との整合確認 + 調整予定.
//
// Step 31 範囲制約:
//   SDD §4.10 サンプルの MessageBus consume + fsync 付きファイル書込は
//   Inc.4 で本格化. 本 Step では publish() の std::cerr プレースホルダ出力 +
//   run_io_thread() 空ループ + 10 ms sleep のみ.
//   AuditLogEntry の `std::map<std::string, std::variant<...>> fields` は
//   Inc.4 で本格化. 本 v0.1 では timestamp + event_type + 単純 message
//   (std::string) で空殻範囲を充足.
//   UNIT-201 SafetyCoreOrchestrator から AuditLogger のスレッド起動 dispatch
//   は Inc.1 後半 (observer pattern + Manager 結線) で完成.
//
// Therac-25 hazard mapping (Inc.4 で本格化、本 v0.1 では構造のみ):
//   - HZ-002 (race condition): std::atomic<bool> stop_requested_ +
//     std::atomic<std::uint64_t> processed_count_ + std::mutex (cerr 出力) +
//     UT 並行 TSan で機械検証. 空殻段階から並行設計の構造を確立.
//   - HZ-006 (cryptic error messages, 操作者バイパス防御層補助): 全 UNIT から
//     publish() で監査ログを集約することで、操作者バイパス時の事後監査経路を
//     確立. Inc.4 で永続化により事後追跡可能性を完成.
//   - HZ-007 (legacy preconditions): static_assert(std::atomic<bool>::is_always_lock_free)
//     + static_assert(std::atomic<std::uint64_t>::is_always_lock_free) を
//     15 ユニット目に拡大 (Inc.1 範囲全 14 ユニット + 空殻ユニット第 2 例).

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>

namespace th25_ctrl {

// ============================================================================
// EventType (空殻範囲の最小列挙、Inc.2/4 で拡張予定).
// ============================================================================
//
// 安全関連イベントの種別を識別する列挙型. SDD §5 IF-U-007 と整合.
// 本 v0.1 では最小限の値を定義し、Inc.2 (SafetyAlarm 連携) / Inc.4 (操作者
// 操作・状態遷移・自己診断結果等) で値を追加する.
enum class EventType : std::uint8_t {
    // 一般情報 (system startup / shutdown 等). 本 v0.1 範囲の主用途.
    Info = 0,

    // 警告 (recoverable error 検出). Inc.2 で SafetyMonitor からの一部警報に使用.
    Warning = 1,

    // エラー (重大な状態遷移、ErrorCode 階層に対応). Inc.2/4 で拡張.
    Error = 2,

    // 安全関連アラーム (SafetyAlarm 受信時). Inc.2 で SafetyMonitor 連携時に
    // 主用途化.
    SafetyAlarm = 3,
};

// ============================================================================
// AuditLogEntry (空殻範囲、SDD §5 IF-U-007 と整合).
// ============================================================================
//
// 全 UNIT が AuditLogger に publish する安全関連イベントの構造体.
// 本 v0.1 範囲では timestamp + event_type + 単純 message (std::string) で
// 空殻範囲を充足. SDD §5 IF-U-007 の `std::map<std::string, std::variant<...>>
// fields` は Inc.4 で本格化.
struct AuditLogEntry {
    std::chrono::system_clock::time_point timestamp{};
    EventType event_type{EventType::Info};
    std::string message{};
};

// ============================================================================
// AuditLogger (SDD §4.10 UNIT-209、Inc.4 で本格化、本 v0.1 では空殻).
// ============================================================================
//
// 設計方針 (空殻):
//   - publish() で std::cerr に JSON Lines 1 行を直接出力 (std::mutex で
//     並行呼出時の出力アトミック性のみ保証、Inc.4 で MessageBus 経由非同期化).
//   - 別スレッドで run_io_thread() を実行 (呼出側責任).
//   - stop() で stop_requested_ を true に設定 → ループ離脱 → スレッド合流可能.
//   - Inc.4 で MessageBus consume + ファイル永続化 + fsync を内部で実装
//     (本 v0.1 範囲では空ループ + 10 ms sleep のみ).
class AuditLogger {
public:
    // IO スレッドのループ間隔 (空殻、Inc.4 で永続化レイテンシ要求と整合確認後
    // に調整予定).
    static constexpr std::chrono::milliseconds kIoLoopIntervalMs{10};

    // 空殻のため依存を持たない. Inc.4 で MessageBus& / FilePath& 引数追加予定
    // (別 CR、SDD v0.4 改訂と同時).
    AuditLogger() noexcept = default;

    // 所有権を一意に保つためコピー/ムーブ禁止 (内部 atomic + mutex 保持、
    // 別スレッドから参照される設計).
    AuditLogger(const AuditLogger&) = delete;
    AuditLogger(AuditLogger&&) = delete;
    auto operator=(const AuditLogger&) -> AuditLogger& = delete;
    auto operator=(AuditLogger&&) -> AuditLogger& = delete;
    ~AuditLogger() = default;

    // ----- SDD §4.10 公開 API (本 v0.1 範囲) -----

    // ログエントリを受領. 本 v0.1 では std::cerr に JSON Lines 1 行を直接
    // 出力 (永続化は Inc.4). 複数 UNIT スレッドからの並行呼出に対応.
    //
    // 事前条件: なし (本関数は thread-safe).
    // 事後条件: std::cerr に 1 行出力. processed_count_ が 1 増加.
    // エラー処理: 出力失敗時も例外送出なし (空殻範囲は best-effort).
    //
    // Inc.4 で追加予定:
    //   - MessageBus への try_publish (非同期化).
    //   - 構造化 fields (std::map<std::string, std::variant<...>>) の出力.
    //   - ファイル永続化 + fsync.
    auto publish(const AuditLogEntry& entry) noexcept -> void;

    // 独立 IO スレッドのループ. 呼出側 (UT または Inc.1 後半 UNIT-201) が
    // 別スレッドで起動する. stop_requested_ が true になるまで
    // kIoLoopIntervalMs 間隔でループを継続.
    //
    // 事前条件: 別スレッドから呼出されること (本関数自身はブロッキング).
    // 事後条件: stop_requested_ = true 検出後に return.
    // エラー処理: 本 v0.1 範囲では例外送出なし (noexcept).
    //
    // Inc.4 で追加予定:
    //   - MessageBus から AuditLogEntry を consume.
    //   - ファイルへ append-only 書込 + fsync.
    //   - rotate / size 制限 (Inc.4 で本格化).
    auto run_io_thread() noexcept -> void;

    // ----- 補助 API (UT 駆動 + Inc.4 拡張点プレースホルダ) -----

    // IO スレッド停止要求. release store で stop_requested_ を true に設定.
    // 本関数は別スレッドからも安全に呼出可能 (atomic 経由).
    //
    // 事前条件: なし.
    // 事後条件: stop_requested_ = true. run_io_thread() のループは次反復で
    //           離脱する (最大 kIoLoopIntervalMs = 10 ms 遅延).
    auto stop() noexcept -> void;

    // stop_requested_ の現在値 (acquire load).
    [[nodiscard]] auto is_stop_requested() const noexcept -> bool;

    // 未処理エントリ数 (acquire load).
    // 空殻範囲: publish() は同期出力のため常に 0.
    // Inc.4 拡張点: MessageBus 受信エントリ数 - 処理済エントリ数 に拡張可能.
    [[nodiscard]] auto pending_count() const noexcept -> std::uint64_t;

    // 処理済エントリ数 (acquire load).
    // 空殻範囲: publish() 呼出毎に 1 増加.
    // Inc.4 拡張点: MessageBus consume + ファイル書込完了時に 1 増加.
    [[nodiscard]] auto processed_count() const noexcept -> std::uint64_t;

    // UT 専用: stop_requested_ + processed_count_ を初期化 (Inc.4 で削除候補).
    // 同一 AuditLogger インスタンスで複数回 run_io_thread() を試験する場合に使用.
    auto reset_for_test() noexcept -> void;

private:
    // IO スレッド停止フラグ. release-acquire memory ordering で複数スレッド間の
    // 可視性を保証 (RCM-002).
    std::atomic<bool> stop_requested_{false};

    // 処理済エントリ数 (UT 駆動 + Inc.4 拡張点プレースホルダ).
    std::atomic<std::uint64_t> processed_count_{0};

    // std::cerr 出力アトミック性確保用 mutex.
    // 複数スレッドから publish() が並行呼出された場合の出力行混在を防ぐ.
    // std::atomic では stream 出力のアトミック性が確保できないため別途必要.
    // Inc.4 で MessageBus 経由非同期化時に削除候補.
    mutable std::mutex output_mutex_{};

    // ----- HZ-007 構造的予防 (SDD §7 SOUP-003/004 機能要求) -----
    // UNIT-200/401/201/202/203/204/205/206/208/301/302/303/304/207 と同パターン
    // を 15 ユニット目に拡大 (Inc.1 範囲全 14 ユニット + 空殻ユニット第 2 例).
    static_assert(std::atomic<bool>::is_always_lock_free,
        "std::atomic<bool> must be always lock-free for AuditLogger "
        "stop_requested_ "
        "(SDD §4.10, RCM-002, HZ-002/HZ-007 structural prevention).");
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
        "std::atomic<std::uint64_t> must be always lock-free for AuditLogger "
        "processed_count_ "
        "(SDD §4.10, RCM-002, HZ-002/HZ-007 structural prevention).");
};

}  // namespace th25_ctrl
