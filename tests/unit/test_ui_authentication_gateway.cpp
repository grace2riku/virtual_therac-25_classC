// SPDX-License-Identifier: TBD
// TH25-UI: UNIT-104 UIAuthenticationGateway 空殻 ユニット試験
//          (UTPR-TH25-001 v0.25 §7.2 と整合).
//
// 試験対象: src/th25_ui/include/th25_ui/ui_authentication_gateway.hpp /
//            src/th25_ui/src/ui_authentication_gateway.cpp (UNIT-104 空殻).
// 適用範囲: SDD-TH25-001 v0.1.1 §3.2 で予告された Inc.4 で本格化予定の
//            操作者認証 UI ユニット (本 v0.1 では空殻 + 常時拒否).
//            SDD §4 独立セクションは Inc.4 SDD 改訂 (SDD v0.4) で §4.18 として
//            正式追記される予定.
//
// IEC 62304 §5.5.4 クラス C 追加受入基準:
//   - 正常系 / 境界値 / 異常系 / 並行処理 / 資源 / データフロー
//
// Therac-25 主要因類型 (SPRP §4.3.1):
//   - A (race condition): std::atomic<std::uint64_t> + UT TSan 並行試験で
//                         機械検証 (空殻段階から並行設計確立).
//   - E (cryptic / bypass): request_authentication() が同期 API として常時
//                            ErrorCode::AuthRequired を返す構造により
//                            「未認証時は治療パラメータ確定操作を行えない」
//                            インタフェース契約を IF 骨格段階で確立 (UNIT-210
//                            CoreAuthenticationGateway と一対の整合、
//                            HZ-010 構造的位置づけ + RCM-015 中核の
//                            UI 側ゲートウェイ).
//   - F (legacy preconditions): is_always_lock_free static_assert (build 時検証).
//
// 本 v0.1 範囲の UT 限界:
//   実 UI ID/パスワード入力 + Core 側 UNIT-210 への IF-U-010 経由
//   AuthenticationVerify 送信 + AuthenticationResult 待ち + 認証成功時の
//   操作者セッション確立 + AuditLogger 転送は Inc.4 範囲のため、本ファイルでは
//   扱わない. 本 v0.1 では IF 骨格 + request_authentication() の常時拒否 +
//   attempt_count_ 集計 + lock-free 表明 + race-free 並行試験を対象とする.

#include "th25_ui/ui_authentication_gateway.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include "th25_ctrl/common_types.hpp"
#include "th25_ctrl/core_authentication_gateway.hpp"

namespace th25_ui {

// ============================================================================
// UT-104-01: 初期状態 (attempt_count_ = 0)
// ============================================================================
TEST(UIAuthenticationGateway_Initial, DefaultState) {
    UIAuthenticationGateway gateway;
    EXPECT_EQ(gateway.attempt_count(), 0U);
}

// ============================================================================
// UT-104-02: request_authentication() 単体呼出で常時 ErrorCode::AuthRequired
//            を返す (空殻範囲: UI 層 + Core 連携未実装時は受付不可、
//            UNIT-210 と一対の整合)
// ============================================================================
TEST(UIAuthenticationGateway_Request, AlwaysReturnsAuthRequired) {
    UIAuthenticationGateway gateway;
    const th25_ctrl::OperatorId id{"op001"};
    const th25_ctrl::Credential cred{"password"};
    const auto result = gateway.request_authentication(id, cred);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error_code(), th25_ctrl::ErrorCode::AuthRequired);
}

// ============================================================================
// UT-104-03: request_authentication() 呼出で attempt_count() が 1 増加
// ============================================================================
TEST(UIAuthenticationGateway_Request, SingleRequestIncrementsCount) {
    UIAuthenticationGateway gateway;
    EXPECT_EQ(gateway.attempt_count(), 0U);
    static_cast<void>(gateway.request_authentication(
        th25_ctrl::OperatorId{"op001"}, th25_ctrl::Credential{"pw"}));
    EXPECT_EQ(gateway.attempt_count(), 1U);
}

// ============================================================================
// UT-104-04: 累積 request_authentication() 呼出で attempt_count() が呼出回数
//            だけ増加
// ============================================================================
TEST(UIAuthenticationGateway_Request, MultipleRequestsAccumulateCount) {
    UIAuthenticationGateway gateway;
    constexpr std::uint64_t kIterations = 100U;
    for (std::uint64_t i = 0U; i < kIterations; ++i) {
        const auto result = gateway.request_authentication(
            th25_ctrl::OperatorId{"op001"},
            th25_ctrl::Credential{"pw" + std::to_string(i)});
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error_code(), th25_ctrl::ErrorCode::AuthRequired);
    }
    EXPECT_EQ(gateway.attempt_count(), kIterations);
}

// ============================================================================
// UT-104-05: 多様な OperatorId × Credential 組合せで常時 AuthRequired
//            (空殻範囲: 入力依存しない構造、HZ-010 + RCM-015 中核の構造的
//             位置づけ、UNIT-210 と一対の整合)
//
// 本 UT は Therac-25 主要因類型 E (操作者バイパス常態化) に隣接する HZ-010
// (操作者認証・権限管理不備) の構造的予防を空殻段階から確立: 多様な ID +
// Credential 組合せで UI 層 + Core 連携が未実装な間は受付不可.
// Inc.4 で本格 UI + Core 側 UNIT-210 への IF-U-010 dispatch + 認証成功時の
// 操作者セッション確立 + AuditLogger 転送 (RCM-015 中核) を完成する.
// ============================================================================
TEST(UIAuthenticationGateway_Request, RejectsAllIdCredentialCombinations) {
    UIAuthenticationGateway gateway;

    struct Input {
        th25_ctrl::OperatorId id;
        th25_ctrl::Credential cred;
    };

    const std::vector<Input> kInputs = {
        // 通常の有資格者風 (空殻範囲では拒否)
        {th25_ctrl::OperatorId{"op001"}, th25_ctrl::Credential{"validpass"}},
        // 空入力 (Inc.4 で空入力検証として AuthInvalid を返す予定の境界)
        {th25_ctrl::OperatorId{""}, th25_ctrl::Credential{""}},
        // ID のみ (Credential 空)
        {th25_ctrl::OperatorId{"op001"}, th25_ctrl::Credential{""}},
        // Credential のみ (ID 空)
        {th25_ctrl::OperatorId{""}, th25_ctrl::Credential{"pw"}},
        // 長大入力 (Inc.4 で長さ検証として AuthInvalid を返す予定の境界)
        {th25_ctrl::OperatorId(std::string(256, 'a')),
         th25_ctrl::Credential(std::string(1024, 'b'))},
        // 非英数字混入 (Inc.4 で英数字制約として AuthInvalid を返す予定の境界)
        {th25_ctrl::OperatorId{"op#001"},
         th25_ctrl::Credential{"pw!@#$%"}},
        // Unicode (Inc.4 で文字種制約として AuthInvalid を返す予定の境界)
        {th25_ctrl::OperatorId{"操作者001"},
         th25_ctrl::Credential{"パスワード"}},
        // 期限切れ風 (Inc.4 で AuthExpired を返す予定の境界)
        {th25_ctrl::OperatorId{"expired_op"},
         th25_ctrl::Credential{"old_pw"}},
    };

    for (const auto& inp : kInputs) {
        const auto result = gateway.request_authentication(inp.id, inp.cred);
        ASSERT_FALSE(result.has_value())
            << "request_authentication should reject in v0.1 "
            << "(Inc.4 で本格認証連携)";
        EXPECT_EQ(result.error_code(), th25_ctrl::ErrorCode::AuthRequired);
    }
    EXPECT_EQ(gateway.attempt_count(), kInputs.size());
}

// ============================================================================
// UT-104-06: reset_for_test() で attempt_count_ が 0 に戻る
// ============================================================================
TEST(UIAuthenticationGateway_Reset, ResetClearsAttemptCount) {
    UIAuthenticationGateway gateway;
    static_cast<void>(gateway.request_authentication(
        th25_ctrl::OperatorId{"op001"}, th25_ctrl::Credential{"pw"}));
    static_cast<void>(gateway.request_authentication(
        th25_ctrl::OperatorId{"op002"}, th25_ctrl::Credential{"pw2"}));
    EXPECT_EQ(gateway.attempt_count(), 2U);

    gateway.reset_for_test();
    EXPECT_EQ(gateway.attempt_count(), 0U);

    // reset 後も request_authentication() は引き続き AuthRequired を返す.
    const auto result = gateway.request_authentication(
        th25_ctrl::OperatorId{"op003"}, th25_ctrl::Credential{"pw3"});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error_code(), th25_ctrl::ErrorCode::AuthRequired);
    EXPECT_EQ(gateway.attempt_count(), 1U);
}

// ============================================================================
// UT-104-07: ErrorCode::AuthRequired は Auth 系 (0x07) に属する
//            (SDD §6.1 ErrorCode 階層との整合確認、空殻範囲の選択根拠を機械検証、
//             UNIT-210 と一対の整合)
// ============================================================================
TEST(UIAuthenticationGateway_ErrorCategory, AuthRequiredBelongsToAuthCategory) {
    constexpr std::uint8_t kAuthCategory = 0x07U;
    static_assert(
        th25_ctrl::error_category(th25_ctrl::ErrorCode::AuthRequired) ==
            kAuthCategory,
        "AuthRequired must belong to Auth category 0x07.");

    UIAuthenticationGateway gateway;
    const auto result = gateway.request_authentication(
        th25_ctrl::OperatorId{"op001"}, th25_ctrl::Credential{"pw"});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(th25_ctrl::error_category(result.error_code()), kAuthCategory);
}

// ============================================================================
// UT-104-08: ErrorCode::AuthRequired は Severity::Medium
//            (SDD §6.2 Severity 階層との整合確認: Auth 系 0x07 は Medium、
//             Internal 系 0xFF Critical / Mode 0x01・Beam 0x02・Dose 0x03
//             Critical とは異なる、UNIT-210 と一対の整合)
// ============================================================================
TEST(UIAuthenticationGateway_Severity, AuthRequiredIsMedium) {
    static_assert(
        th25_ctrl::severity_of(th25_ctrl::ErrorCode::AuthRequired) ==
            th25_ctrl::Severity::Medium,
        "AuthRequired must be Medium severity (Auth category 0x07).");
    SUCCEED();
}

// ============================================================================
// UT-104-09: Copy/Move 禁止 (compile-time 検証、所有権独立、内部 atomic 保持)
// ============================================================================
TEST(UIAuthenticationGateway_Structure, NonCopyableNonMovable) {
    static_assert(!std::is_copy_constructible_v<UIAuthenticationGateway>);
    static_assert(!std::is_copy_assignable_v<UIAuthenticationGateway>);
    static_assert(!std::is_move_constructible_v<UIAuthenticationGateway>);
    static_assert(!std::is_move_assignable_v<UIAuthenticationGateway>);
    SUCCEED();
}

// ============================================================================
// UT-104-10: Default 構築可能 (compile-time)
// ============================================================================
TEST(UIAuthenticationGateway_Structure, DefaultConstructible) {
    static_assert(std::is_default_constructible_v<UIAuthenticationGateway>);
    static_assert(std::is_nothrow_default_constructible_v<UIAuthenticationGateway>);
    SUCCEED();
}

// ============================================================================
// UT-104-11: 並行 N threads × M request_authentication() race-free
//            (HZ-002 機械検証、tsan プリセット必須)
//
// 設計:
//   - 4 threads が並行に request_authentication() を 500 回ずつ呼出
//     (合計 2000 回、ID/Credential を thread × 反復で変化).
//   - 各 request_authentication() は AuthRequired を返却
//     + attempt_count_ を 1 増加.
//   - 終了時 attempt_count() == 4 * 500 == 2000 を機械検証.
//   - tsan プリセットで data race detection 0 を機械検証.
//
// 教訓水平展開 (CR-0021 / Step 33):
//   reader/checker パターンを使わない (本 UT は writer のみ並行) ため
//   do-while パターンは適用対象外. ただし将来 reader 並行を追加する場合は
//   PRB-0005/PRB-0006/CR-0021 教訓に従い do-while パターンを最初から採用.
// ============================================================================
TEST(UIAuthenticationGateway_Concurrency, MultiThreadRequestRaceFree) {
    UIAuthenticationGateway gateway;

    constexpr int kThreads = 4;
    constexpr int kIterations = 500;

    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(kThreads));

    std::atomic<int> auth_required_hits{0};

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&gateway, &auth_required_hits, t]() {
            for (int i = 0; i < kIterations; ++i) {
                const th25_ctrl::OperatorId id{
                    "op" + std::to_string(t) + "_" + std::to_string(i)};
                const th25_ctrl::Credential cred{
                    "pw" + std::to_string(t) + "_" + std::to_string(i)};
                const auto result = gateway.request_authentication(id, cred);
                if (!result.has_value() &&
                    result.error_code() ==
                        th25_ctrl::ErrorCode::AuthRequired) {
                    auth_required_hits.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    constexpr std::uint64_t kTotal =
        static_cast<std::uint64_t>(kThreads) *
        static_cast<std::uint64_t>(kIterations);
    EXPECT_EQ(gateway.attempt_count(), kTotal);
    EXPECT_EQ(auth_required_hits.load(std::memory_order_relaxed),
              static_cast<int>(kTotal));
}

// ============================================================================
// UT-104-12: HZ-007 lock-free 単一表明
//            (std::atomic<std::uint64_t>::is_always_lock_free)
//
// 20 ユニット目に拡大. compiler / 標準ライブラリ更新で is_always_lock_free が
// false になった場合、ビルド時点で fail-stop. (本 UT は static_assert を
// テストファイルからも明示確認することで二重防御).
// ============================================================================
TEST(UIAuthenticationGateway_HZ007, LockFreeAssertion) {
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
        "std::atomic<std::uint64_t> must be always lock-free for "
        "UIAuthenticationGateway attempt_count_ "
        "(SDD §3.2, RCM-002, HZ-002/HZ-007 structural prevention).");
    SUCCEED();
}

}  // namespace th25_ui
