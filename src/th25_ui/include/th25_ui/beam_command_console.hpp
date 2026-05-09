// SPDX-License-Identifier: TBD (See LICENSE in repo root once defined)
// TH25-UI: UNIT-102 BeamCommandConsole (Inc.4 で本格化、本 v0.1 では空殻 + IF のみ)
//
// IEC 62304 Class C, C++20.
// SDD-TH25-001 v0.1.1 §3.2 で「v0.1 では空殻ユニットとして記述」と予告された
// UNIT-102 の空殻 IF 骨格. SDD §4 内に独立セクションは未定義であり、本ヘッダの
// 公開 API は Inc.4 SDD 改訂(SDD v0.4)§4.16 UNIT-102 として正式追記される予定.
//
// 役割: ビームオン/オフ操作 UI (操作者が BeamCommand { On, Off } を発行し、
//       Safety Core (UNIT-201 SafetyCoreOrchestrator) + UNIT-203 BeamController
//       に IPC 経由で BeamCommand メッセージを送信する操作系 UI).
// **HZ-006 (暗号的エラーメッセージ + 操作者バイパス) 中核対応 + RCM-010
//   (致死的エラーバイパス禁止) 中核 (Inc.4 で本格化)**.
//
// SDD §3.2 公開 API (本 v0.1 範囲、Inc.4 SDD 改訂で確定予定):
//   - submit_beam_command(BeamCommand)
//                                 : ビームオン/オフ要求送信. 本 v0.1 では常時
//                                   ErrorCode::InternalUnexpectedState を返却
//                                   (UI 層は Inc.4 で本格化).
//
// 補助 API (UT 駆動 + Inc.4 拡張点プレースホルダ):
//   - submit_count()    : submit_beam_command() 呼出累積回数 (acquire load).
//   - reset_for_test()  : submit_count_ を初期化 (UT 専用、Inc.4 で削除候補).
//
// SDD §3.2 設計判断 (Step 35 範囲):
//   - 本 v0.1 では同期 API として SDD §3.2 予告に従う. std::thread /
//     run_io_thread() / stop() 等は実装しない (Inc.4 で IPC 経由非同期化 +
//     BeamCommand メッセージ送信時に追加予定).
//   - submit_beam_command() は常時 ErrorCode::InternalUnexpectedState を
//     返却 (Inc.4 で実 UI 表示 + 致死的エラーバイパス試行拒否 + IPC 経由送信
//     に発展).
//   - UI フレームワーク選定 (Qt / wxWidgets / 独自 minimal terminal UI 等) は
//     Inc.4 で別 CR で確定予定. 本 v0.1 では UI フレームワーク依存を持たない.
//   - submit_count_ は複数 UI スレッド (UI 描画スレッド + 入力イベントハンドラ +
//     物理ボタン GPIO ハンドラ等) からの並行 submit_beam_command() 呼出を集計
//     するため atomic 化 (UT 並行試験で tsan 機械検証可能化).
//
// Step 35 範囲制約:
//   SDD §3.2 ユニット表で予告された「ビームオン/オフ操作 UI」+ SRS-I-004
//   (BeamCommand 列挙体 { On, Off }) を参照する形で空殻 API を設計.
//   実 UI 表示 + 致死的エラーバイパス試行拒否 + IPC 経由 BeamCommand メッセージ
//   送信は Inc.4 で本格化. 本 Step では submit_beam_command() の常時拒否
//   プレースホルダ + submit_count_ 集計のみ.
//   UNIT-201 SafetyCoreOrchestrator + UNIT-203 BeamController から本ユニット
//   への IPC 経路 (操作者 UI プロセス側 RPC) は Inc.1 後半 (observer pattern +
//   Manager 結線) で完成、本格 UI 実装は Inc.4 で完成.
//
// Therac-25 hazard mapping (Inc.4 で本格化、本 v0.1 では構造のみ):
//   - HZ-002 (race condition): std::atomic<std::uint64_t> submit_count_ +
//     UT 並行 TSan で機械検証. 空殻段階から並行設計の構造を確立.
//   - HZ-006 (cryptic error messages, 操作者バイパス中核対応): ビームオン/オフ
//     操作 UI として、致死的エラー発生時の単一キー操作によるバイパスを構造的に
//     拒否する RCM-010 中核. 本 v0.1 では IF 骨格 + 常時拒否プレースホルダのみ.
//     Inc.4 で実 UI + 致死的エラー判定 + バイパス試行拒否 + IPC 経由送信により
//     RCM-010 を完成 (Therac-25 East Texas 事故型 "P キー押下で照射継続" の
//     構造的不可能化).
//   - HZ-007 (legacy preconditions): static_assert(std::atomic<std::uint64_t>::is_always_lock_free)
//     を 18 ユニット目に拡大 (Inc.1 範囲全 14 ユニット + 空殻ユニット第 5 例).

#pragma once

#include <atomic>
#include <cstdint>

#include "th25_ctrl/common_types.hpp"

namespace th25_ui {

// ============================================================================
// BeamCommand 列挙体 (SRS-I-004 と整合).
// ============================================================================
//
// 空殻範囲では本ヘッダ内に定義. Inc.4 SDD 改訂 (SDD v0.4) で SRS-I-004 IF 仕様
// 確定時に common_types.hpp に移動し、UNIT-203 BeamController + IF-E-001 等の
// 関連 IF と統一する候補.
//
// 設計方針:
//   - On = 0 / Off = 1 (uint8_t fits). 値域は固定 (Inc.4 で UNIT-203 BeamController
//     + IPC 経由送信時に proto3 enum と整合させる予定).
enum class BeamCommand : std::uint8_t {
    On = 0,
    Off = 1,
};

// ============================================================================
// BeamCommandConsole (SDD §3.2 UNIT-102、Inc.4 で本格化、本 v0.1 では空殻).
// ============================================================================
//
// 設計方針 (空殻):
//   - submit_beam_command() は同期 API として常時 ErrorCode::InternalUnexpectedState
//     を返却.
//   - submit_count_ で呼出累積回数を集計 (UT 駆動 + Inc.4 拡張点).
//   - Inc.4 で実 UI 表示 (UI フレームワーク + ビームオン/オフ物理ボタン or
//     ソフトボタン) + 致死的エラー判定 + バイパス試行拒否 (RCM-010) + IPC
//     経由 BeamCommand メッセージ送信を本格化 (本 v0.1 範囲では常時拒否のみ).
class BeamCommandConsole {
public:
    // 空殻のため依存を持たない. Inc.4 で UiRenderer& / IpcSender& /
    // CurrentErrorState& 引数追加予定 (別 CR、SDD v0.4 改訂と同時).
    BeamCommandConsole() noexcept = default;

    // 所有権を一意に保つためコピー/ムーブ禁止 (内部 atomic 保持、別スレッドから
    // 参照される設計).
    BeamCommandConsole(const BeamCommandConsole&) = delete;
    BeamCommandConsole(BeamCommandConsole&&) = delete;
    auto operator=(const BeamCommandConsole&) -> BeamCommandConsole& = delete;
    auto operator=(BeamCommandConsole&&) -> BeamCommandConsole& = delete;
    ~BeamCommandConsole() = default;

    // ----- SDD §3.2 公開 API (本 v0.1 範囲、Inc.4 SDD 改訂で確定予定) -----

    // ビームオン/オフ要求送信. 本 v0.1 では常時 ErrorCode::InternalUnexpectedState
    // を返却 (実 UI 実装 + 致死的エラーバイパス試行拒否 + IPC 経由 BeamCommand
    // 送信は Inc.4).
    //
    // 事前条件: なし (本関数は thread-safe).
    // 事後条件: submit_count_ が 1 増加. 常に Result::error(InternalUnexpectedState)
    //           返却.
    // エラー処理: 例外送出なし (noexcept). 失敗は Result::error(...) で返却.
    //
    // Inc.4 で追加予定:
    //   - UI 表示 (ビームオン/オフ物理ボタン or ソフトボタン).
    //   - 致死的エラー状態の参照 (SafetyCoreOrchestrator から CurrentErrorState
    //     購読).
    //   - 致死的エラーバイパス試行拒否 (RCM-010、Therac-25 East Texas 事故型
    //     "P キー押下で照射継続" の構造的不可能化).
    //   - IPC 経由 BeamCommand メッセージ生成 + 送信
    //     (UNIT-402 InterProcessChannel 経由).
    //   - Safety Core (UNIT-201) + UNIT-203 BeamController からの応答受信 +
    //     UI フィードバック.
    //   - AuditLogger 転送 (操作者ビーム指令履歴を Info/Warning EventType で
    //     publish).
    [[nodiscard]] auto submit_beam_command(BeamCommand command) noexcept
        -> th25_ctrl::Result<void, th25_ctrl::ErrorCode>;

    // ----- 補助 API (UT 駆動 + Inc.4 拡張点プレースホルダ) -----

    // submit_beam_command() 呼出累積回数 (acquire load).
    // 空殻範囲: submit_beam_command() 呼出毎に 1 増加.
    // Inc.4 拡張点: On/Off 別の集計、致死的エラーバイパス試行回数等に拡張可能.
    [[nodiscard]] auto submit_count() const noexcept -> std::uint64_t;

    // UT 専用: submit_count_ を初期化 (Inc.4 で削除候補).
    // 同一 BeamCommandConsole インスタンスで複数回試験する場合に使用.
    auto reset_for_test() noexcept -> void;

private:
    // submit_beam_command() 呼出累積回数 (UT 駆動 + Inc.4 拡張点プレースホルダ).
    // release-acquire memory ordering で複数スレッド間の可視性を保証 (RCM-002).
    std::atomic<std::uint64_t> submit_count_{0};

    // ----- HZ-007 構造的予防 (SDD §7 SOUP-003/004 機能要求) -----
    // UNIT-200/401/201/202/203/204/205/206/208/301/302/303/304/207/209/210/101
    // と同パターンを 18 ユニット目に拡大 (Inc.1 範囲全 14 ユニット + 空殻ユニット
    // 第 5 例).
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
        "std::atomic<std::uint64_t> must be always lock-free for "
        "BeamCommandConsole submit_count_ "
        "(SDD §3.2, RCM-002, HZ-002/HZ-007 structural prevention).");
};

}  // namespace th25_ui
