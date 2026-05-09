// SPDX-License-Identifier: TBD (See LICENSE in repo root once defined)
// TH25-UI: UNIT-101 PrescriptionEditor (Inc.4 で本格化、本 v0.1 では空殻 + IF のみ)
//
// IEC 62304 Class C, C++20.
// SDD-TH25-001 v0.1.1 §3.2 で「v0.1 では空殻ユニットとして記述」と予告された
// UNIT-101 の空殻 IF 骨格. SDD §4 内に独立セクションは未定義であり、本ヘッダの
// 公開 API は Inc.4 SDD 改訂(SDD v0.4)§4.15 UNIT-101 として正式追記される予定.
//
// 役割: 治療計画入力 UI (操作者が TreatmentMode / Energy / DoseUnit_cGy を
//       入力し、IF-E-001 PrescriptionSet メッセージとして Safety Core
//       (UNIT-201 SafetyCoreOrchestrator) に送信する前段の入力検証ゲートウェイ).
// **HZ-006 (暗号的エラーメッセージ + 操作者バイパス) 防御層補助 + RCM-009/010
//   中核 (Inc.4 で本格化)**.
//
// SDD §3.2 公開 API (本 v0.1 範囲、Inc.4 SDD 改訂で確定予定):
//   - submit_prescription(TreatmentMode, std::variant<Energy_MeV, Energy_MV>,
//                         DoseUnit_cGy)
//                                 : 治療計画送信. 本 v0.1 では常時
//                                   ErrorCode::InternalUnexpectedState を返却
//                                   (UI 層は Inc.4 で本格化).
//
// 補助 API (UT 駆動 + Inc.4 拡張点プレースホルダ):
//   - submit_count()    : submit_prescription() 呼出累積回数 (acquire load).
//   - reset_for_test()  : submit_count_ を初期化 (UT 専用、Inc.4 で削除候補).
//
// SDD §3.2 設計判断 (Step 34 範囲):
//   - 本 v0.1 では同期 API として SDD §3.2 予告に従う. std::thread /
//     run_io_thread() / stop() 等は実装しない (Inc.4 で IPC 経由非同期化 +
//     PrescriptionSet メッセージ送信時に追加予定).
//   - submit_prescription() は常時 ErrorCode::InternalUnexpectedState を
//     返却 (Inc.4 で実 UI 表示 + 入力バリデーション + IPC 経由送信に発展).
//   - UI フレームワーク選定 (Qt / wxWidgets / 独自 minimal terminal UI 等) は
//     Inc.4 で別 CR で確定予定. 本 v0.1 では UI フレームワーク依存を持たない.
//   - submit_count_ は複数 UI スレッド (UI 描画スレッド + 入力イベントハンドラ)
//     からの並行 submit_prescription() 呼出を集計するため atomic 化
//     (UT 並行試験で tsan 機械検証可能化).
//
// Step 34 範囲制約:
//   SDD §3.2 ユニット表で予告された「治療計画入力 UI」+ IF-E-001 PrescriptionSet
//   proto3 スキーマ (SDD §5.2 で確定済) を参照する形で空殻 API を設計.
//   実 UI 表示 + 入力バリデーション + IPC 経由 PrescriptionSet メッセージ送信は
//   Inc.4 で本格化. 本 Step では submit_prescription() の常時拒否プレースホルダ
//   + submit_count_ 集計のみ.
//   UNIT-201 SafetyCoreOrchestrator から本ユニットへの IPC 経路 (操作者 UI
//   プロセス側 RPC) は Inc.1 後半 (observer pattern + Manager 結線) で完成、
//   本格 UI 実装は Inc.4 で完成.
//
// Therac-25 hazard mapping (Inc.4 で本格化、本 v0.1 では構造のみ):
//   - HZ-002 (race condition): std::atomic<std::uint64_t> submit_count_ +
//     UT 並行 TSan で機械検証. 空殻段階から並行設計の構造を確立.
//   - HZ-006 (cryptic error messages, 操作者バイパス防御層補助): 治療計画入力
//     UI として、IF-E-001 PrescriptionSet を Safety Core に送信する前段の
//     入力検証ゲートウェイの構造的位置づけを確立. 本 v0.1 では IF 骨格 +
//     常時拒否プレースホルダのみ. Inc.4 で実 UI + 入力バリデーション + IPC
//     経由送信により事前監査経路を完成.
//   - HZ-007 (legacy preconditions): static_assert(std::atomic<std::uint64_t>::is_always_lock_free)
//     を 17 ユニット目に拡大 (Inc.1 範囲全 14 ユニット + 空殻ユニット第 4 例).

#pragma once

#include <atomic>
#include <cstdint>
#include <variant>

#include "th25_ctrl/common_types.hpp"

namespace th25_ui {

// ============================================================================
// PrescriptionEditor (SDD §3.2 UNIT-101、Inc.4 で本格化、本 v0.1 では空殻).
// ============================================================================
//
// 設計方針 (空殻):
//   - submit_prescription() は同期 API として常時 ErrorCode::InternalUnexpectedState
//     を返却.
//   - submit_count_ で呼出累積回数を集計 (UT 駆動 + Inc.4 拡張点).
//   - Inc.4 で実 UI 表示 (UI フレームワーク + 入力ウィジェット) + 入力バリデーション
//     (SRS-I-001 治療計画入力範囲) + IPC 経由 PrescriptionSet メッセージ送信
//     を本格化 (本 v0.1 範囲では常時拒否のみ).
class PrescriptionEditor {
public:
    // 空殻のため依存を持たない. Inc.4 で UiRenderer& / IpcSender& 引数追加予定
    // (別 CR、SDD v0.4 改訂と同時).
    PrescriptionEditor() noexcept = default;

    // 所有権を一意に保つためコピー/ムーブ禁止 (内部 atomic 保持、別スレッドから
    // 参照される設計).
    PrescriptionEditor(const PrescriptionEditor&) = delete;
    PrescriptionEditor(PrescriptionEditor&&) = delete;
    auto operator=(const PrescriptionEditor&) -> PrescriptionEditor& = delete;
    auto operator=(PrescriptionEditor&&) -> PrescriptionEditor& = delete;
    ~PrescriptionEditor() = default;

    // ----- SDD §3.2 公開 API (本 v0.1 範囲、Inc.4 SDD 改訂で確定予定) -----

    // 治療計画送信. 本 v0.1 では常時 ErrorCode::InternalUnexpectedState を
    // 返却 (実 UI 実装 + 入力バリデーション + IPC 経由 PrescriptionSet 送信は
    // Inc.4).
    //
    // 事前条件: なし (本関数は thread-safe).
    // 事後条件: submit_count_ が 1 増加. 常に Result::error(InternalUnexpectedState)
    //           返却.
    // エラー処理: 例外送出なし (noexcept). 失敗は Result::error(...) で返却.
    //
    // Inc.4 で追加予定:
    //   - UI 表示 (TreatmentMode / Energy / DoseUnit_cGy 入力ウィジェット).
    //   - 入力バリデーション (SRS-I-001 治療計画入力範囲、SRS-004 エネルギー
    //     範囲、SRS-008 線量範囲).
    //   - IF-E-001 PrescriptionSet proto3 メッセージ生成 + IPC 経由送信
    //     (UNIT-402 InterProcessChannel 経由).
    //   - Safety Core (UNIT-201) からの応答 (PrescriptionAck / PrescriptionReject)
    //     受信 + UI フィードバック.
    //   - AuditLogger 転送 (操作者入力履歴を Info/Warning EventType で publish).
    [[nodiscard]] auto submit_prescription(
        th25_ctrl::TreatmentMode mode,
        const std::variant<th25_ctrl::Energy_MeV, th25_ctrl::Energy_MV>& energy,
        th25_ctrl::DoseUnit_cGy target_dose) noexcept
        -> th25_ctrl::Result<void, th25_ctrl::ErrorCode>;

    // ----- 補助 API (UT 駆動 + Inc.4 拡張点プレースホルダ) -----

    // submit_prescription() 呼出累積回数 (acquire load).
    // 空殻範囲: submit_prescription() 呼出毎に 1 増加.
    // Inc.4 拡張点: 入力成功/失敗別の集計、AuditLogger 転送カウンタ等に拡張可能.
    [[nodiscard]] auto submit_count() const noexcept -> std::uint64_t;

    // UT 専用: submit_count_ を初期化 (Inc.4 で削除候補).
    // 同一 PrescriptionEditor インスタンスで複数回試験する場合に使用.
    auto reset_for_test() noexcept -> void;

private:
    // submit_prescription() 呼出累積回数 (UT 駆動 + Inc.4 拡張点プレースホルダ).
    // release-acquire memory ordering で複数スレッド間の可視性を保証 (RCM-002).
    std::atomic<std::uint64_t> submit_count_{0};

    // ----- HZ-007 構造的予防 (SDD §7 SOUP-003/004 機能要求) -----
    // UNIT-200/401/201/202/203/204/205/206/208/301/302/303/304/207/209/210
    // と同パターンを 17 ユニット目に拡大 (Inc.1 範囲全 14 ユニット + 空殻ユニット
    // 第 4 例).
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
        "std::atomic<std::uint64_t> must be always lock-free for "
        "PrescriptionEditor submit_count_ "
        "(SDD §3.2, RCM-002, HZ-002/HZ-007 structural prevention).");
};

}  // namespace th25_ui
