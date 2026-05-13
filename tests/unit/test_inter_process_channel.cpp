// SPDX-License-Identifier: TBD
// TH25-CTRL: UNIT-402 InterProcessChannel 空殻 ユニット試験
//            (UTPR-TH25-001 v0.28 §7.2 と整合).
//
// 試験対象: src/th25_ctrl/include/th25_ctrl/inter_process_channel.hpp /
//            src/th25_ctrl/src/inter_process_channel.cpp (UNIT-402 空殻).
// 適用範囲: SDD-TH25-001 v0.1.1 §4.14 で予告された Inc.4 で本格化予定の
//            プロセス間通信ユニット (本 v0.1 では空殻 + 常時拒否).
//
// IEC 62304 §5.5.4 クラス C 追加受入基準:
//   - 正常系 / 境界値 / 異常系 / 並行処理 / 資源 / データフロー
//
// テンプレ流用元: UNIT-210 CoreAuthenticationGateway
// (`tests/unit/test_core_authentication_gateway.cpp`).
//   **Severity マッピング自己セルフチェック (CLAUDE.md 新運用ルール、
//    CR-0026 / Step 39 制定) 適用済**:
//     [1] ErrorCode カテゴリ: IpcChannelClosed = 0x0601 → IPC 系 (0x06)
//     [2] SDD §6.2 マッピング表: IPC 系 (0x06) → Severity::High
//         (Internal 系 / Mode / Beam / Dose 系の Critical とは異なる、
//          Auth 系 / Magnet 系の Medium とも異なる)
//     [3] UNIT-200 test_common_types.cpp:141 で
//         severity_of(IpcChannelClosed) == Severity::High が既に網羅試験済
//     [4] テンプレ流用時の変更項目: UT-402-08 を `IpcChannelClosedIsHigh` に命名
//         (UNIT-210/UNIT-104 の Medium / UNIT-101〜103 の Critical とは異なる)
//
// Therac-25 主要因類型 (SPRP §4.3.1):
//   - A (race condition): std::atomic<std::uint64_t> + UT TSan 並行試験で
//                         機械検証 (空殻段階から並行設計確立).
//   - E (cryptic / bypass): connect/send/recv が同期 API として常時
//                            ErrorCode::IpcChannelClosed を返す構造により
//                            「IPC 未確立時はメッセージ送受信できない」
//                            インタフェース契約を IF 骨格段階で確立
//                            (SEP-001 構造的実体).
//   - F (legacy preconditions): is_always_lock_free static_assert (build 時検証).
//
// 本 v0.1 範囲の UT 限界:
//   実 gRPC 接続 + proto3 シリアライズ + 1 MB サイズチェック +
//   IpcDeserializationFailure 検出 + IpcQueueOverflow 検出は Inc.4 範囲のため、
//   本ファイルでは扱わない. 本 v0.1 では IF 骨格 + connect/send/recv 常時拒否 +
//   operation_count_ 集計 + close() noexcept + lock-free 表明 + race-free 並行
//   試験を対象とする.

#include "th25_ctrl/inter_process_channel.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include "th25_ctrl/common_types.hpp"

namespace th25_ctrl {

// ============================================================================
// UT-402-01: 初期状態 (operation_count_ = 0)
// ============================================================================
TEST(InterProcessChannel_Initial, DefaultState) {
    InterProcessChannel channel;
    EXPECT_EQ(channel.operation_count(), 0U);
}

// ============================================================================
// UT-402-02: connect() 単体呼出で常時 ErrorCode::IpcChannelClosed を返す
//            (空殻範囲: gRPC 未実装時は接続不可)
// ============================================================================
TEST(InterProcessChannel_Connect, AlwaysReturnsIpcChannelClosed) {
    InterProcessChannel channel;
    const auto result = channel.connect("/tmp/th25.sock");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error_code(), ErrorCode::IpcChannelClosed);
    EXPECT_EQ(channel.operation_count(), 1U);
}

// ============================================================================
// UT-402-03: send() 単体呼出で常時 ErrorCode::IpcChannelClosed を返す
//            (空殻範囲: gRPC 未実装時は送信不可)
// ============================================================================
TEST(InterProcessChannel_Send, AlwaysReturnsIpcChannelClosed) {
    InterProcessChannel channel;
    const SafetyCoreMessage msg{"placeholder message"};
    const auto result = channel.send(msg);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error_code(), ErrorCode::IpcChannelClosed);
    EXPECT_EQ(channel.operation_count(), 1U);
}

// ============================================================================
// UT-402-04: recv() 単体呼出で常時 ErrorCode::IpcChannelClosed を返す
//            (空殻範囲: gRPC 未実装時は受信不可)
// ============================================================================
TEST(InterProcessChannel_Recv, AlwaysReturnsIpcChannelClosed) {
    InterProcessChannel channel;
    const auto result = channel.recv();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error_code(), ErrorCode::IpcChannelClosed);
    EXPECT_EQ(channel.operation_count(), 1U);
}

// ============================================================================
// UT-402-05: connect/send/recv の累積呼出で operation_count() が増加
//            (close() は count 対象外)
// ============================================================================
TEST(InterProcessChannel_OperationCount, MultipleOperationsAccumulateCount) {
    InterProcessChannel channel;
    constexpr std::uint64_t kIterations = 30U;
    for (std::uint64_t i = 0U; i < kIterations; ++i) {
        static_cast<void>(channel.connect("/tmp/th25.sock"));
        static_cast<void>(channel.send(SafetyCoreMessage{"msg" + std::to_string(i)}));
        static_cast<void>(channel.recv());
        // close() は operation_count に含まれない設計
        channel.close();
    }
    // connect + send + recv = 3 × kIterations
    EXPECT_EQ(channel.operation_count(), kIterations * 3U);
}

// ============================================================================
// UT-402-06: 多様な socket_path / message 引数で常時 IpcChannelClosed
//            (空殻範囲: 入力依存しない構造、Inc.4 で gRPC 接続境界の予告)
// ============================================================================
TEST(InterProcessChannel_Inputs, RejectsAllInputCombinations) {
    InterProcessChannel channel;

    const std::vector<std::string> kSocketPaths = {
        "/tmp/th25.sock",
        "",                                       // 空パス
        "/var/run/th25/channel.sock",
        std::string(4096, 'a'),                   // 長大パス (PATH_MAX 超過想定)
        "unix:///tmp/th25.sock",                  // gRPC URI 形式
        "ソケット",  // Unicode (Inc.4 で文字種制約)
    };

    for (const auto& path : kSocketPaths) {
        const auto result = channel.connect(path);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error_code(), ErrorCode::IpcChannelClosed);
    }

    const std::vector<SafetyCoreMessage> kMessages = {
        SafetyCoreMessage{""},                    // 空メッセージ
        SafetyCoreMessage{"short"},
        SafetyCoreMessage(1024, 'x'),             // 1 KB
        SafetyCoreMessage(1024 * 1024 + 1, 'y'),  // 1 MB + 1 (Inc.4 で IpcMessageTooLarge)
    };

    for (const auto& msg : kMessages) {
        const auto result = channel.send(msg);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error_code(), ErrorCode::IpcChannelClosed);
    }

    EXPECT_EQ(channel.operation_count(),
              kSocketPaths.size() + kMessages.size());
}

// ============================================================================
// UT-402-07: ErrorCode::IpcChannelClosed は IPC 系 (0x06) に属する
//            (SDD §6.1 ErrorCode 階層との整合確認)
// ============================================================================
TEST(InterProcessChannel_ErrorCategory, IpcChannelClosedBelongsToIpcCategory) {
    constexpr std::uint8_t kIpcCategory = 0x06U;
    static_assert(error_category(ErrorCode::IpcChannelClosed) == kIpcCategory,
        "IpcChannelClosed must belong to IPC category 0x06.");

    InterProcessChannel channel;
    const auto result = channel.connect("/tmp/th25.sock");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(error_category(result.error_code()), kIpcCategory);
}

// ============================================================================
// UT-402-08: ErrorCode::IpcChannelClosed は Severity::High
//            (SDD §6.2 Severity 階層との整合確認、IPC 系 0x06 マッピング、
//             Internal/Mode/Beam/Dose 系の Critical / Auth/Magnet 系の Medium
//             とは異なる)
//
// テンプレ流用元: UNIT-210 UT-210-XX / UNIT-104 UT-104-08.
//   Severity マッピング自己セルフチェック (CLAUDE.md 新運用ルール、
//   CR-0026 / Step 39) を適用し、SDD §6.2 で IpcChannelClosed (0x0601) が
//   IPC 系 (0x06) → Severity::High にマップされることを再確認済.
//   UNIT-200 test_common_types.cpp:141 で網羅試験済の値と一致.
// ============================================================================
TEST(InterProcessChannel_Severity, IpcChannelClosedIsHigh) {
    static_assert(
        severity_of(ErrorCode::IpcChannelClosed) == Severity::High,
        "IpcChannelClosed must be High severity (IPC category 0x06).");
    SUCCEED();
}

// ============================================================================
// UT-402-09: Copy/Move 禁止 (compile-time 検証、所有権独立、内部 atomic 保持)
// ============================================================================
TEST(InterProcessChannel_Structure, NonCopyableNonMovable) {
    static_assert(!std::is_copy_constructible_v<InterProcessChannel>);
    static_assert(!std::is_copy_assignable_v<InterProcessChannel>);
    static_assert(!std::is_move_constructible_v<InterProcessChannel>);
    static_assert(!std::is_move_assignable_v<InterProcessChannel>);
    SUCCEED();
}

// ============================================================================
// UT-402-10: Default 構築可能 + close() noexcept (compile-time)
// ============================================================================
TEST(InterProcessChannel_Structure, DefaultConstructibleAndCloseNoexcept) {
    static_assert(std::is_default_constructible_v<InterProcessChannel>);
    static_assert(std::is_nothrow_default_constructible_v<InterProcessChannel>);
    static_assert(noexcept(std::declval<InterProcessChannel&>().close()));
    SUCCEED();
}

// ============================================================================
// UT-402-11: 並行 N threads × M operations race-free
//            (HZ-002 機械検証、tsan プリセット必須)
//
// 設計:
//   - 4 threads が並行に connect/send/recv を 500 回ずつ呼出
//     (合計 2000 件 × 3 操作 = 6000 件、socket_path/message を thread × 反復で変化).
//   - 各操作は IpcChannelClosed を返却 + operation_count_ を 1 増加.
//   - 終了時 operation_count() == 4 * 500 * 3 == 6000 を機械検証.
//   - tsan プリセットで data race detection 0 を機械検証.
//
// 教訓水平展開 (CR-0021 / Step 33):
//   reader/checker パターンを使わない (本 UT は writer のみ並行) ため
//   do-while パターンは適用対象外.
// ============================================================================
TEST(InterProcessChannel_Concurrency, MultiThreadOperationsRaceFree) {
    InterProcessChannel channel;

    constexpr int kThreads = 4;
    constexpr int kIterations = 500;

    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(kThreads));

    std::atomic<int> connect_hits{0};
    std::atomic<int> send_hits{0};
    std::atomic<int> recv_hits{0};

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&channel, &connect_hits, &send_hits, &recv_hits, t]() {
            for (int i = 0; i < kIterations; ++i) {
                const std::string path =
                    "/tmp/th25_" + std::to_string(t) + "_" + std::to_string(i) + ".sock";
                const SafetyCoreMessage msg{
                    "msg" + std::to_string(t) + "_" + std::to_string(i)};

                if (const auto cr = channel.connect(path);
                    !cr.has_value() && cr.error_code() == ErrorCode::IpcChannelClosed) {
                    connect_hits.fetch_add(1, std::memory_order_relaxed);
                }
                if (const auto sr = channel.send(msg);
                    !sr.has_value() && sr.error_code() == ErrorCode::IpcChannelClosed) {
                    send_hits.fetch_add(1, std::memory_order_relaxed);
                }
                if (const auto rr = channel.recv();
                    !rr.has_value() && rr.error_code() == ErrorCode::IpcChannelClosed) {
                    recv_hits.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    constexpr int kPerThread = kIterations;
    constexpr std::uint64_t kTotalOps =
        static_cast<std::uint64_t>(kThreads) *
        static_cast<std::uint64_t>(kPerThread) * 3U;
    EXPECT_EQ(channel.operation_count(), kTotalOps);
    EXPECT_EQ(connect_hits.load(std::memory_order_relaxed), kThreads * kPerThread);
    EXPECT_EQ(send_hits.load(std::memory_order_relaxed), kThreads * kPerThread);
    EXPECT_EQ(recv_hits.load(std::memory_order_relaxed), kThreads * kPerThread);
}

// ============================================================================
// UT-402-12: HZ-007 lock-free 単一表明
//            (std::atomic<std::uint64_t>::is_always_lock_free)
//
// 21 ユニット目に拡大. compiler / 標準ライブラリ更新で is_always_lock_free が
// false になった場合、ビルド時点で fail-stop.
// ============================================================================
TEST(InterProcessChannel_HZ007, LockFreeAssertion) {
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
        "std::atomic<std::uint64_t> must be always lock-free for "
        "InterProcessChannel operation_count_ "
        "(SDD §4.14, RCM-002, HZ-002/HZ-007 structural prevention).");
    SUCCEED();
}

}  // namespace th25_ctrl
