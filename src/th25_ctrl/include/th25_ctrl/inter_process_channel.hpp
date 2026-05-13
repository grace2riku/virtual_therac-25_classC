// SPDX-License-Identifier: TBD (See LICENSE in repo root once defined)
// TH25-CTRL: UNIT-402 InterProcessChannel (Inc.4 で本格化、本 v0.1 では空殻 + IF のみ)
//
// IEC 62304 Class C, C++20.
// Implementation of SDD-TH25-001 v0.1.1 §4.14 (UNIT-402).
//
// 役割: プロセス間通信 (gRPC over Unix Domain Socket) のラッパ.
//        Protocol Buffers シリアライズ/デシリアライズを内包.
//        SAD §3 ARCH-004.2 配下、SRS-IF-005 + SAD §9 SEP-001/SEP-002 の実装層.
// **RCM-018 (SEP-001/SEP-002 の実装層) 中核 (Inc.4 で本格化)**.
//
// SDD §4.14 公開 API (本 v0.1 範囲):
//   - connect(socket_path) : UDS 接続確立. 本 v0.1 では常時
//                             ErrorCode::IpcChannelClosed を返却.
//   - send(SafetyCoreMessage) : メッセージ送信. 本 v0.1 では常時
//                                ErrorCode::IpcChannelClosed を返却.
//   - recv()             : メッセージ受信. 本 v0.1 では常時
//                            ErrorCode::IpcChannelClosed を返却.
//   - close()            : 接続クローズ. 本 v0.1 では noexcept で何もしない.
//
// 補助 API (UT 駆動 + Inc.4 拡張点プレースホルダ):
//   - operation_count() : 全 API (connect/send/recv) 呼出累積回数 (acquire load).
//                         空殻範囲では connect/send/recv の 3 種類を区別せず集計
//                         (Inc.4 で本格化時に個別 atomic に分割予定).
//   - reset_for_test()  : operation_count_ を初期化 (UT 専用、Inc.4 で削除候補).
//
// SDD §4.14 設計判断 (Step 40 範囲):
//   - 本 v0.1 では同期 API として SDD §4.14 表に厳密に従う. std::thread /
//     run_io_thread() / stop() 等は実装しない (Inc.4 で gRPC + Protocol Buffers
//     ベースの非同期 I/O 実装時に追加予定、別 CR で SOUP 追加要).
//   - connect/send/recv は常時 ErrorCode::IpcChannelClosed を返却
//     (Inc.4 で実 gRPC 接続 + proto3 シリアライズ + 1 MB サイズチェック +
//     IpcDeserializationFailure 検出に発展).
//   - close() は noexcept で何もしない (Inc.4 で gRPC channel.Shutdown() 呼出).
//   - SafetyCoreMessage 型は本 v0.1 範囲では未定義 (Inc.4 で IF-E-001 proto3
//     定義から code-gen される std::variant<...> 等として本格化予定、本ヘッダ
//     では型エイリアス SafetyCoreMessage = std::string プレースホルダで代用).
//   - operation_count_ は複数 UNIT スレッド (UI Process ↔ Safety Core Process
//     間の async I/O 経路) からの並行呼出を集計するため atomic 化
//     (UT 並行試験で tsan 機械検証可能化).
//
// Step 40 範囲制約:
//   SDD §4.14 サンプルの実 gRPC over UDS + Protocol Buffers シリアライズ +
//   1 MB サイズ制限 + close() 時の channel.Shutdown() は Inc.4 で本格化.
//   本 Step では connect/send/recv の常時拒否プレースホルダ + operation_count_
//   集計 + close() noexcept のみ. SafetyCoreMessage 型 (IF-E-001) は Inc.4 で
//   proto3 + code-gen で本格化する空殻 IF として本ヘッダにプレースホルダで
//   先行定義 (std::string).
//   UNIT-201 SafetyCoreOrchestrator から InterProcessChannel への
//   send/recv dispatch は Inc.1 後半 (observer pattern + Manager 結線) で
//   完成、本格 gRPC 実装は Inc.4 で完成 (gRPC + Protocol Buffers SOUP 追加
//   CR 別途必要).
//
// テンプレ流用元: UNIT-210 CoreAuthenticationGateway (`core_authentication_gateway.hpp`).
//   - 同一の空殻パターン (常時拒否 API + operation_count + reset_for_test).
//   - **Severity マッピング自己セルフチェック (CLAUDE.md 新運用ルール、CR-0026 / Step 39 制定) 適用済**:
//     [1] ErrorCode カテゴリ: IpcChannelClosed = 0x0601 → IPC 系 (0x06)
//     [2] SDD §6.2 マッピング表: IPC 系 (0x06) → Severity::High
//         (Internal 系 / Mode / Beam / Dose 系の Critical とは異なる)
//     [3] UNIT-200 test_common_types.cpp UT-200-XX で
//         severity_of(IpcChannelClosed) == Severity::High が既に網羅試験済
//         (`tests/unit/test_common_types.cpp:141` で確認)
//     [4] テンプレ流用時の変更項目: テスト名 (XxxIsHigh) / 期待 Severity (High) /
//         コメント表記 (Critical fail-stop → High)
//     [5] 本ヘッダ内コメントで明示
//
// Therac-25 hazard mapping (Inc.4 で本格化、本 v0.1 では構造のみ):
//   - HZ-002 (race condition): std::atomic<std::uint64_t> operation_count_ +
//     UT 並行 TSan で機械検証. 空殻段階から並行設計の構造を確立.
//   - HZ-006 (cryptic error messages): connect/send/recv が同期 API として常時
//     ErrorCode::IpcChannelClosed を返す構造により「IPC 未確立時はメッセージ
//     送受信できない」インタフェース契約を IF 骨格段階で確立 (SEP-001 構造的
//     実体、UI 層と安全コア間のメッセージパッシング前提を Inc.1 で確立).
//     Inc.4 で実 gRPC + 人間可読エラー表示で完成.
//   - HZ-007 (legacy preconditions): static_assert(std::atomic<std::uint64_t>::is_always_lock_free)
//     を 21 ユニット目に拡大 (Inc.1 範囲全 14 ユニット + 空殻ユニット第 8 例).

#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <string_view>

#include "th25_ctrl/common_types.hpp"

namespace th25_ctrl {

// ============================================================================
// SafetyCoreMessage (空殻範囲、Inc.4 で IF-E-001 proto3 から code-gen 予定).
// ============================================================================
//
// SDD §5.2 IF-E-001 / IF-E-002 / IF-E-003 で proto3 として定義予定の
// メッセージ型のプレースホルダ. 本 v0.1 範囲では std::string ベースで代用.
// Inc.4 で gRPC + Protocol Buffers SOUP 追加時に std::variant<...> として
// 本格化予定 (PrescriptionSet / BeamCommand / SafetyAlarm / AuditLogEntry 等).
using SafetyCoreMessage = std::string;

// ============================================================================
// InterProcessChannel (SDD §4.14 UNIT-402、Inc.4 で本格化、本 v0.1 では空殻).
// ============================================================================
//
// 設計方針 (空殻):
//   - connect/send/recv は同期 API として常時 ErrorCode::IpcChannelClosed
//     を返却 (IPC 未確立を IF 骨格段階で構造的に表明).
//   - close() は noexcept で何もしない (Inc.4 で gRPC channel.Shutdown()).
//   - operation_count_ で全 API 呼出累積回数を集計 (UT 駆動 + Inc.4 拡張点).
//   - Inc.4 で実 gRPC over UDS + Protocol Buffers シリアライズ + 1 MB サイズ
//     制限 + IpcDeserializationFailure 検出を本格化
//     (本 v0.1 範囲では常時拒否のみ).
class InterProcessChannel {
public:
    // 空殻のため依存を持たない. Inc.4 で grpc::Channel / std::thread (recv loop) /
    // AuditLogger& 引数追加予定 (別 CR、SDD v0.4 改訂と同時、SOUP 追加要).
    InterProcessChannel() noexcept = default;

    // 所有権を一意に保つためコピー/ムーブ禁止 (内部 atomic 保持、別スレッドから
    // 参照される設計、grpc::Channel リソース所有).
    InterProcessChannel(const InterProcessChannel&) = delete;
    InterProcessChannel(InterProcessChannel&&) = delete;
    auto operator=(const InterProcessChannel&) -> InterProcessChannel& = delete;
    auto operator=(InterProcessChannel&&) -> InterProcessChannel& = delete;
    ~InterProcessChannel() = default;

    // ----- SDD §4.14 公開 API (本 v0.1 範囲) -----

    // UDS 接続確立. 本 v0.1 では常時 ErrorCode::IpcChannelClosed を返却
    // (実 gRPC 接続は Inc.4).
    //
    // 事前条件: なし (本関数は thread-safe).
    // 事後条件: operation_count_ が 1 増加. 常に Result::error(IpcChannelClosed)
    //           返却.
    // エラー処理: 例外送出なし (noexcept). 接続失敗は Result::error(...) で返却.
    //
    // Inc.4 で追加予定:
    //   - grpc::CreateChannel(socket_path, InsecureChannelCredentials()) 呼出.
    //   - 接続状態の管理 (`grpc::ChannelState`).
    //   - タイムアウト管理 (例: 5 秒).
    //   - 接続失敗時の IpcChannelClosed 詳細メッセージ生成.
    [[nodiscard]] auto connect(std::string_view socket_path) noexcept
        -> Result<void, ErrorCode>;

    // メッセージ送信. 本 v0.1 では常時 ErrorCode::IpcChannelClosed を返却
    // (実 gRPC 送信は Inc.4).
    //
    // 事前条件: connect() 成功済 (本 v0.1 では常時 false のため呼出側は受付不可).
    // 事後条件: operation_count_ が 1 増加. 常に Result::error(IpcChannelClosed)
    //           返却.
    // エラー処理: 例外送出なし (noexcept).
    //
    // Inc.4 で追加予定:
    //   - SafetyCoreMessage を proto3 シリアライズ (1 MB 超過時 IpcMessageTooLarge).
    //   - grpc::Stream::Write() 呼出.
    //   - 送信失敗時の IpcChannelClosed / IpcMessageTooLarge 詳細生成.
    [[nodiscard]] auto send(const SafetyCoreMessage& msg) noexcept
        -> Result<void, ErrorCode>;

    // メッセージ受信. 本 v0.1 では常時 ErrorCode::IpcChannelClosed を返却
    // (実 gRPC 受信は Inc.4).
    //
    // 事前条件: connect() 成功済 (本 v0.1 では常時 false).
    // 事後条件: operation_count_ が 1 増加. 常に Result::error(IpcChannelClosed)
    //           返却.
    // エラー処理: 例外送出なし (noexcept).
    //
    // Inc.4 で追加予定:
    //   - grpc::Stream::Read() 呼出 (blocking, タイムアウト管理).
    //   - proto3 デシリアライズ (失敗時 IpcDeserializationFailure).
    //   - 受信メッセージのキューイング (IpcQueueOverflow 検出).
    [[nodiscard]] auto recv() noexcept
        -> Result<SafetyCoreMessage, ErrorCode>;

    // 接続クローズ. 本 v0.1 では noexcept で何もしない
    // (Inc.4 で gRPC channel.Shutdown() 呼出).
    //
    // 事前条件: なし.
    // 事後条件: なし (本 v0.1 範囲では状態変更なし).
    // エラー処理: 例外送出なし (noexcept、Result 返却なし).
    auto close() noexcept -> void;

    // ----- 補助 API (UT 駆動 + Inc.4 拡張点プレースホルダ) -----

    // 全 API (connect/send/recv) 呼出累積回数 (acquire load).
    // 空殻範囲: 3 API 呼出毎に 1 増加 (close() は count しない).
    // Inc.4 拡張点: connect/send/recv 別の集計、接続状態別カウンタ等に拡張可能.
    [[nodiscard]] auto operation_count() const noexcept -> std::uint64_t;

    // UT 専用: operation_count_ を初期化 (Inc.4 で削除候補).
    auto reset_for_test() noexcept -> void;

private:
    // 全 API 呼出累積回数 (UT 駆動 + Inc.4 拡張点プレースホルダ).
    // release-acquire memory ordering で複数スレッド間の可視性を保証 (RCM-002).
    std::atomic<std::uint64_t> operation_count_{0};

    // ----- HZ-007 構造的予防 (SDD §7 SOUP-003/004 機能要求) -----
    // UNIT-200/401/201/202/203/204/205/206/208/301/302/303/304/207/209/210/101/102/103/104
    // と同パターンを 21 ユニット目に拡大 (Inc.1 範囲全 14 ユニット + 空殻ユニット
    // 第 8 例).
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
        "std::atomic<std::uint64_t> must be always lock-free for "
        "InterProcessChannel operation_count_ "
        "(SDD §4.14, RCM-002, HZ-002/HZ-007 structural prevention).");
};

}  // namespace th25_ctrl
