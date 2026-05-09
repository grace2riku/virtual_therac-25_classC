// SPDX-License-Identifier: TBD (See LICENSE in repo root once defined)
// TH25-UI: UNIT-103 AlarmDisplay (Inc.4 で本格化、本 v0.1 では空殻 + IF のみ)
//
// IEC 62304 Class C, C++20.
// SDD-TH25-001 v0.1.1 §3.2 で「v0.1 では空殻ユニットとして記述」と予告された
// UNIT-103 の空殻 IF 骨格. SDD §4 内に独立セクションは未定義であり、本ヘッダの
// 公開 API は Inc.4 SDD 改訂(SDD v0.4)§4.17 UNIT-103 として正式追記される予定.
//
// 役割: アラーム/エラー表示 UI (Safety Core (UNIT-201 SafetyCoreOrchestrator) +
//       UNIT-207 SafetyMonitor から配信されるアラーム情報を操作者に表示する
//       出力系 UI、SRS-O-004 + SRS-UX-001/002/003 担当).
// **HZ-006 (暗号的エラーメッセージ + 操作者バイパス) 中核対応 + RCM-009
//   (暗号的エラーメッセージ廃止・人間可読化) 中核 (Inc.4 で本格化)**.
//
// SDD §3.2 公開 API (本 v0.1 範囲、Inc.4 SDD 改訂で確定予定):
//   - display_alarm(Severity, ErrorCode)
//                                 : アラーム表示要求. 本 v0.1 では常時
//                                   ErrorCode::InternalUnexpectedState を返却
//                                   (UI 層は Inc.4 で本格化).
//
// 補助 API (UT 駆動 + Inc.4 拡張点プレースホルダ):
//   - display_count()   : display_alarm() 呼出累積回数 (acquire load).
//   - reset_for_test()  : display_count_ を初期化 (UT 専用、Inc.4 で削除候補).
//
// SDD §3.2 設計判断 (Step 36 範囲):
//   - 本 v0.1 では同期 API として SDD §3.2 予告に従う. std::thread /
//     run_io_thread() / stop() 等は実装しない (Inc.4 で IPC 経由非同期化 +
//     アラーム配信受信時に追加予定).
//   - display_alarm() は常時 ErrorCode::InternalUnexpectedState を返却
//     (Inc.4 で実 UI 表示 + 人間可読メッセージ生成 + 操作者承認待ち管理 +
//     Severity::Critical 時の Halted 連携 + AuditLogger 転送に発展).
//   - UI フレームワーク選定 (Qt / wxWidgets / 独自 minimal terminal UI 等) は
//     Inc.4 で別 CR で確定予定. 本 v0.1 では UI フレームワーク依存を持たない.
//   - display_count_ は複数 UI スレッド (UI 描画スレッド + アラーム配信受信
//     ハンドラ等) からの並行 display_alarm() 呼出を集計するため atomic 化
//     (UT 並行試験で tsan 機械検証可能化).
//
// Step 36 範囲制約:
//   SDD §3.2 ユニット表で予告された「アラーム/エラー表示 UI」+ SRS-O-004
//   (構造化エラーコード + 人間可読メッセージ) + SRS-UX-001/002/003
//   を参照する形で空殻 API を設計.
//   実 UI 表示 + 人間可読メッセージ生成 + 操作者承認待ち管理 + IPC 経由
//   アラーム配信受信は Inc.4 で本格化. 本 Step では display_alarm() の
//   常時拒否プレースホルダ + display_count_ 集計のみ.
//   UNIT-201 SafetyCoreOrchestrator + UNIT-207 SafetyMonitor から本ユニット
//   への IPC 経路 (操作者 UI プロセス側 RPC) は Inc.1 後半 / Inc.2 で完成、
//   本格 UI 実装は Inc.4 で完成.
//
// Therac-25 hazard mapping (Inc.4 で本格化、本 v0.1 では構造のみ):
//   - HZ-002 (race condition): std::atomic<std::uint64_t> display_count_ +
//     UT 並行 TSan で機械検証. 空殻段階から並行設計の構造を確立.
//   - HZ-006 (cryptic error messages, 操作者バイパス中核対応): アラーム/
//     エラー表示 UI として、Therac-25 East Texas 事故型「MALFUNCTION 54
//     のような暗号的コード単独表示」を構造的に廃止し、人間可読メッセージ
//     (現象 + 原因 + 推奨対処) を表示する RCM-009 中核. 本 v0.1 では IF
//     骨格 + 常時拒否プレースホルダのみ. Inc.4 で実 UI + 人間可読メッセージ
//     生成 (ErrorCode → ローカライズされたメッセージ) + 操作者承認待ち管理
//     + Severity::Critical 時の LifecycleState::Halted 連携 + AuditLogger
//     転送により RCM-009 中核を完成 (Therac-25 East Texas 事故型「MALFUNCTION
//     54 単独表示 + バイパス常態化」の構造的不可能化).
//   - HZ-007 (legacy preconditions): static_assert(std::atomic<std::uint64_t>::is_always_lock_free)
//     を 19 ユニット目に拡大 (Inc.1 範囲全 14 ユニット + 空殻ユニット第 6 例).

#pragma once

#include <atomic>
#include <cstdint>

#include "th25_ctrl/common_types.hpp"

namespace th25_ui {

// ============================================================================
// AlarmDisplay (SDD §3.2 UNIT-103、Inc.4 で本格化、本 v0.1 では空殻).
// ============================================================================
//
// 設計方針 (空殻):
//   - display_alarm() は同期 API として常時 ErrorCode::InternalUnexpectedState
//     を返却.
//   - display_count_ で呼出累積回数を集計 (UT 駆動 + Inc.4 拡張点).
//   - Inc.4 で実 UI 表示 (UI フレームワーク + アラーム表示ウィジェット) +
//     人間可読メッセージ生成 (ErrorCode → SRS-UX-001「現象 + 原因 + 推奨対処」
//     ローカライズ) + 操作者承認待ち管理 + Severity::Critical 時の
//     LifecycleState::Halted 連携 + AuditLogger 転送を本格化 (本 v0.1 範囲では
//     常時拒否のみ).
class AlarmDisplay {
public:
    // 空殻のため依存を持たない. Inc.4 で UiRenderer& / IpcReceiver& /
    // AuditLogger& 引数追加予定 (別 CR、SDD v0.4 改訂と同時).
    AlarmDisplay() noexcept = default;

    // 所有権を一意に保つためコピー/ムーブ禁止 (内部 atomic 保持、別スレッドから
    // 参照される設計).
    AlarmDisplay(const AlarmDisplay&) = delete;
    AlarmDisplay(AlarmDisplay&&) = delete;
    auto operator=(const AlarmDisplay&) -> AlarmDisplay& = delete;
    auto operator=(AlarmDisplay&&) -> AlarmDisplay& = delete;
    ~AlarmDisplay() = default;

    // ----- SDD §3.2 公開 API (本 v0.1 範囲、Inc.4 SDD 改訂で確定予定) -----

    // アラーム表示要求. 本 v0.1 では常時 ErrorCode::InternalUnexpectedState
    // を返却 (実 UI 表示 + 人間可読メッセージ生成 + 操作者承認待ち管理 +
    // AuditLogger 転送は Inc.4).
    //
    // 事前条件: なし (本関数は thread-safe).
    // 事後条件: display_count_ が 1 増加. 常に Result::error(InternalUnexpectedState)
    //           返却.
    // エラー処理: 例外送出なし (noexcept). 失敗は Result::error(...) で返却.
    //
    // Inc.4 で追加予定:
    //   - UI 表示 (アラーム表示ウィジェット、Severity 別の視覚的区別).
    //   - 人間可読メッセージ生成 (ErrorCode → SRS-UX-001「現象 + 原因 +
    //     推奨対処」のローカライズメッセージ、"MALFUNCTION 54" のような
    //     暗号的コード単独表示は構造的に禁止).
    //   - 操作者承認待ち管理 (Severity::Critical は単一キー操作によるバイパス
    //     不可、物理的再起動が必要、SRS-UX-002 / RCM-010 連携).
    //   - Severity::Critical 時の LifecycleState::Halted 連携 (UNIT-201
    //     SafetyCoreOrchestrator 経由).
    //   - IPC 経由アラーム配信受信 (UNIT-201 / UNIT-207 SafetyMonitor から
    //     IF-U-006 SafetyAlarm メッセージ受信、UNIT-402 InterProcessChannel
    //     経由).
    //   - AuditLogger 転送 (アラーム表示履歴を Warning/Error/SafetyAlarm
    //     EventType で publish).
    [[nodiscard]] auto display_alarm(th25_ctrl::Severity severity,
                                     th25_ctrl::ErrorCode code) noexcept
        -> th25_ctrl::Result<void, th25_ctrl::ErrorCode>;

    // ----- 補助 API (UT 駆動 + Inc.4 拡張点プレースホルダ) -----

    // display_alarm() 呼出累積回数 (acquire load).
    // 空殻範囲: display_alarm() 呼出毎に 1 増加.
    // Inc.4 拡張点: Severity 別の集計、操作者承認受信回数等に拡張可能.
    [[nodiscard]] auto display_count() const noexcept -> std::uint64_t;

    // UT 専用: display_count_ を初期化 (Inc.4 で削除候補).
    // 同一 AlarmDisplay インスタンスで複数回試験する場合に使用.
    auto reset_for_test() noexcept -> void;

private:
    // display_alarm() 呼出累積回数 (UT 駆動 + Inc.4 拡張点プレースホルダ).
    // release-acquire memory ordering で複数スレッド間の可視性を保証 (RCM-002).
    std::atomic<std::uint64_t> display_count_{0};

    // ----- HZ-007 構造的予防 (SDD §7 SOUP-003/004 機能要求) -----
    // UNIT-200/401/201/202/203/204/205/206/208/301/302/303/304/207/209/210/101/102
    // と同パターンを 19 ユニット目に拡大 (Inc.1 範囲全 14 ユニット + 空殻ユニット
    // 第 6 例).
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
        "std::atomic<std::uint64_t> must be always lock-free for "
        "AlarmDisplay display_count_ "
        "(SDD §3.2, RCM-002, HZ-002/HZ-007 structural prevention).");
};

}  // namespace th25_ui
