// SPDX-License-Identifier: TBD
// TH25-CTRL: UNIT-402 InterProcessChannel implementation
//            (Inc.4 で本格化、本 v0.1 では空殻).
//
// SDD-TH25-001 v0.1.1 §4.14 で予告された UNIT-402 InterProcessChannel の
// 空殻実装. 本 v0.1 範囲では IF 骨格 + 常時拒否プレースホルダ
// (connect/send/recv が常に ErrorCode::IpcChannelClosed を返却) +
// operation_count_ 集計 + close() noexcept のみ.
// Inc.4 で gRPC + Protocol Buffers SOUP 追加と同時に本格 IPC 実装を行う予定
// (別 CR、SDD v0.4 改訂と同時).
//
// テンプレ流用元: UNIT-210 CoreAuthenticationGateway (`core_authentication_gateway.cpp`).
//   Severity マッピング自己セルフチェック (CLAUDE.md 新運用ルール、CR-0026 / Step 39 制定)
//   適用済: IpcChannelClosed = 0x0601 → IPC 系 (0x06) → Severity::High
//   (Internal/Mode/Beam/Dose 系の Critical とは異なる).

#include "th25_ctrl/inter_process_channel.hpp"

namespace th25_ctrl {

// ============================================================================
// SDD §4.14 公開 API (本 v0.1 範囲).
// ============================================================================

auto InterProcessChannel::connect(std::string_view socket_path) noexcept
    -> Result<void, ErrorCode> {
    // 空殻実装: 引数を参照せず常時 ErrorCode::IpcChannelClosed を返却.
    // Inc.4 で実 gRPC 接続 (grpc::CreateChannel + InsecureChannelCredentials +
    // タイムアウト管理 + grpc::ChannelState 監視) に発展.
    //
    // memory ordering / synchronization:
    //   - operation_count_.fetch_add(release): operation_count() reader との
    //     happens-before 関係確立 + read-modify-write のアトミック性.
    static_cast<void>(socket_path);

    operation_count_.fetch_add(1U, std::memory_order_release);

    // 本 v0.1 範囲は gRPC 接続が未実装のため、接続要求を常時拒否する.
    // ErrorCode::IpcChannelClosed は SDD §6.1 ErrorCode 階層の IPC 系 (0x06) で、
    // SDD §6.2 で Severity::High にマップされている (Internal/Mode/Beam/Dose 系
    // の Critical とは異なる). Inc.4 で本格 gRPC 接続実装時に「接続失敗時 =
    // IpcChannelClosed」という本来挙動と空殻挙動が同一 ErrorCode で表現可能、
    // 後方互換性.
    return Result<void, ErrorCode>::error(ErrorCode::IpcChannelClosed);
}

auto InterProcessChannel::send(const SafetyCoreMessage& msg) noexcept
    -> Result<void, ErrorCode> {
    // 空殻実装: 引数を参照せず常時 ErrorCode::IpcChannelClosed を返却.
    // Inc.4 で proto3 シリアライズ + grpc::Stream::Write + 1 MB サイズチェック
    // (IpcMessageTooLarge 検出) に発展.
    static_cast<void>(msg);

    operation_count_.fetch_add(1U, std::memory_order_release);

    return Result<void, ErrorCode>::error(ErrorCode::IpcChannelClosed);
}

auto InterProcessChannel::recv() noexcept
    -> Result<SafetyCoreMessage, ErrorCode> {
    // 空殻実装: 常時 ErrorCode::IpcChannelClosed を返却.
    // Inc.4 で grpc::Stream::Read + proto3 デシリアライズ
    // (IpcDeserializationFailure 検出) + キューイング (IpcQueueOverflow 検出)
    // に発展.
    operation_count_.fetch_add(1U, std::memory_order_release);

    return Result<SafetyCoreMessage, ErrorCode>::error(
        ErrorCode::IpcChannelClosed);
}

auto InterProcessChannel::close() noexcept -> void {
    // 空殻実装: 何もしない. operation_count_ もインクリメントしない
    // (close() は接続終了処理であり、通常の I/O 操作とは別扱い).
    // Inc.4 で gRPC channel.Shutdown() 呼出 + リソース解放に発展.
}

// ============================================================================
// 補助 API (UT / Inc.4 拡張点).
// ============================================================================

auto InterProcessChannel::operation_count() const noexcept -> std::uint64_t {
    return operation_count_.load(std::memory_order_acquire);
}

auto InterProcessChannel::reset_for_test() noexcept -> void {
    operation_count_.store(0U, std::memory_order_release);
}

}  // namespace th25_ctrl
