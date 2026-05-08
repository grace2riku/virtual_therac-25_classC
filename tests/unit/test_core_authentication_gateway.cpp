// SPDX-License-Identifier: TBD
// TH25-CTRL: UNIT-210 CoreAuthenticationGateway 空殻 ユニット試験
//            (UTPR-TH25-001 v0.16 §7.2 と整合).
//
// 試験対象: src/th25_ctrl/include/th25_ctrl/core_authentication_gateway.hpp /
//            src/th25_ctrl/src/core_authentication_gateway.cpp (UNIT-210 空殻).
// 適用範囲: SDD-TH25-001 v0.1.1 §4.11 で確定された Inc.4 で本格化予定の
//            操作者認証ゲートウェイユニット (本 v0.1 では空殻 + 常時拒否).
//
// IEC 62304 §5.5.4 クラス C 追加受入基準:
//   - 正常系 / 境界値 / 異常系 / 並行処理 / 資源 / データフロー
//
// Therac-25 主要因類型 (SPRP §4.3.1):
//   - A (race condition): std::atomic<std::uint64_t> + UT TSan 並行試験で
//                         機械検証 (空殻段階から並行設計確立).
//   - E (cryptic / bypass): verify() が同期 API として常時 ErrorCode::AuthRequired
//                            を返す構造により「未認証時は治療パラメータ確定操作を
//                            行えない」インタフェース契約を IF 骨格段階で確立.
//   - F (legacy preconditions): is_always_lock_free static_assert (build 時検証).
//
// 本 v0.1 範囲の UT 限界:
//   実認証ロジック + MessageBus 経由非同期化 + AuditLogger 転送は Inc.4 範囲の
//   ため、本ファイルでは扱わない. 本 v0.1 では IF 骨格 + verify() の常時拒否 +
//   verify_count_ 集計 + lock-free 表明 + race-free 並行試験を対象とする.

#include "th25_ctrl/core_authentication_gateway.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <type_traits>
#include <vector>

namespace th25_ctrl {

// ============================================================================
// UT-210-01: 初期状態 (verify_count_ = 0)
// ============================================================================
TEST(CoreAuthenticationGateway_Initial, DefaultState) {
    CoreAuthenticationGateway gw;
    EXPECT_EQ(gw.verify_count(), 0U);
}

// ============================================================================
// UT-210-02: verify() 単体呼出で常時 ErrorCode::AuthRequired を返す
//            (SRS-SEC-001 IF 契約: 未認証時は拒否)
// ============================================================================
TEST(CoreAuthenticationGateway_Verify, AlwaysReturnsAuthRequired) {
    CoreAuthenticationGateway gw;
    const auto result = gw.verify("operator-001", "password-001");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error_code(), ErrorCode::AuthRequired);
}

// ============================================================================
// UT-210-03: verify() 呼出で verify_count() が 1 増加
// ============================================================================
TEST(CoreAuthenticationGateway_Verify, SingleVerifyIncrementsCount) {
    CoreAuthenticationGateway gw;
    EXPECT_EQ(gw.verify_count(), 0U);
    static_cast<void>(gw.verify("operator-001", "password-001"));
    EXPECT_EQ(gw.verify_count(), 1U);
}

// ============================================================================
// UT-210-04: 累積 verify() 呼出で verify_count() が呼出回数だけ増加
// ============================================================================
TEST(CoreAuthenticationGateway_Verify, MultipleVerifyAccumulatesCount) {
    CoreAuthenticationGateway gw;
    constexpr std::uint64_t kIterations = 100U;
    for (std::uint64_t i = 0U; i < kIterations; ++i) {
        const auto result = gw.verify("operator-001", "password-001");
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error_code(), ErrorCode::AuthRequired);
    }
    EXPECT_EQ(gw.verify_count(), kIterations);
}

// ============================================================================
// UT-210-05: 多様な OperatorId / Credential 引数でも常時 AuthRequired
//            (実認証ロジックは Inc.4 のため、入力値に依存しない構造)
// ============================================================================
TEST(CoreAuthenticationGateway_Verify, RejectsAllInputsRegardlessOfArgs) {
    CoreAuthenticationGateway gw;

    const std::vector<std::pair<OperatorId, Credential>> kInputs = {
        {"", ""},                                       // 空文字
        {"operator-001", ""},                           // 空 credential
        {"", "password-001"},                           // 空 ID
        {"admin", "admin"},                             // 弱い既知 ID/PW
        {"operator-with-very-long-id-name-12345",       // 長文字列
         "very-long-password-with-special-chars-!@#$%"},
    };

    for (const auto& [id, cred] : kInputs) {
        const auto result = gw.verify(id, cred);
        ASSERT_FALSE(result.has_value())
            << "verify('" << id << "', '" << cred
            << "') should reject in v0.1 (Inc.4 で実認証)";
        EXPECT_EQ(result.error_code(), ErrorCode::AuthRequired);
    }
    EXPECT_EQ(gw.verify_count(), kInputs.size());
}

// ============================================================================
// UT-210-06: reset_for_test() で verify_count_ が 0 に戻る
// ============================================================================
TEST(CoreAuthenticationGateway_Reset, ResetClearsVerifyCount) {
    CoreAuthenticationGateway gw;
    static_cast<void>(gw.verify("operator-001", "password-001"));
    static_cast<void>(gw.verify("operator-002", "password-002"));
    EXPECT_EQ(gw.verify_count(), 2U);

    gw.reset_for_test();
    EXPECT_EQ(gw.verify_count(), 0U);

    // reset 後も verify() は引き続き AuthRequired を返す.
    const auto result = gw.verify("operator-003", "password-003");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error_code(), ErrorCode::AuthRequired);
    EXPECT_EQ(gw.verify_count(), 1U);
}

// ============================================================================
// UT-210-07: AuthenticationVerify / AuthenticationResult 構造体の構造的検証
//            (IF-U-010 と整合、Inc.4 で MessageBus 経由非同期化時に使用)
// ============================================================================
TEST(CoreAuthenticationGateway_Structure, AuthMessageStructsLayout) {
    // AuthenticationVerify: id + cred + request_id を保持し、値型として動作.
    AuthenticationVerify req;
    req.id = "operator-007";
    req.cred = "password-007";
    req.request_id = 42U;
    EXPECT_EQ(req.id, "operator-007");
    EXPECT_EQ(req.cred, "password-007");
    EXPECT_EQ(req.request_id, 42U);

    // AuthenticationResult: request_id + outcome を保持. デフォルト初期化時は
    // outcome.error_code() == AuthRequired.
    AuthenticationResult res;
    EXPECT_EQ(res.request_id, 0U);
    ASSERT_FALSE(res.outcome.has_value());
    EXPECT_EQ(res.outcome.error_code(), ErrorCode::AuthRequired);

    // 値型: コピー可能 + 独立性 (SDD §5.1 IF-U メッセージ不変条件).
    static_assert(std::is_copy_constructible_v<AuthenticationVerify>);
    static_assert(std::is_copy_assignable_v<AuthenticationVerify>);
    static_assert(std::is_move_constructible_v<AuthenticationVerify>);
    static_assert(std::is_move_assignable_v<AuthenticationVerify>);
}

// ============================================================================
// UT-210-08: OperatorId / Credential 型の構造的検証
//            (空殻段階では std::string ベース、Inc.4 で本格化)
// ============================================================================
TEST(CoreAuthenticationGateway_Structure, OperatorIdCredentialAreString) {
    static_assert(std::is_same_v<OperatorId, std::string>,
        "OperatorId は v0.1 範囲では std::string. Inc.4 で構造体化予定.");
    static_assert(std::is_same_v<Credential, std::string>,
        "Credential は v0.1 範囲では std::string. Inc.4 で SecureString 化予定.");

    OperatorId id = "operator-008";
    Credential cred = "password-008";
    EXPECT_EQ(id.size(), 12U);
    EXPECT_EQ(cred.size(), 12U);
}

// ============================================================================
// UT-210-09: ErrorCode::AuthRequired は Auth 系 (0x07) に属する
//            (SDD §6.1 ErrorCode 階層との整合確認)
// ============================================================================
TEST(CoreAuthenticationGateway_ErrorCategory, AuthRequiredBelongsToAuthCategory) {
    constexpr std::uint8_t kAuthCategory = 0x07U;
    static_assert(error_category(ErrorCode::AuthRequired) == kAuthCategory,
        "AuthRequired must belong to Auth category 0x07.");

    CoreAuthenticationGateway gw;
    const auto result = gw.verify("operator-009", "password-009");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(error_category(result.error_code()), kAuthCategory);
}

// ============================================================================
// UT-210-10: Copy/Move 禁止 (compile-time 検証、所有権独立、内部 atomic 保持)
// ============================================================================
TEST(CoreAuthenticationGateway_Structure, NonCopyableNonMovable) {
    static_assert(!std::is_copy_constructible_v<CoreAuthenticationGateway>);
    static_assert(!std::is_copy_assignable_v<CoreAuthenticationGateway>);
    static_assert(!std::is_move_constructible_v<CoreAuthenticationGateway>);
    static_assert(!std::is_move_assignable_v<CoreAuthenticationGateway>);
    SUCCEED();
}

// ============================================================================
// UT-210-11: 並行 N threads × M verify() race-free
//            (HZ-002 機械検証、tsan プリセット必須)
//
// 設計:
//   - 4 threads が並行に verify() を 500 回ずつ呼出 (合計 2000 回).
//   - 各 verify() は AuthRequired を返却 + verify_count_ を 1 増加.
//   - 終了時 verify_count() == 4 * 500 == 2000 を機械検証.
//   - tsan プリセットで data race detection 0 を機械検証.
// ============================================================================
TEST(CoreAuthenticationGateway_Concurrency, MultiThreadVerifyRaceFree) {
    CoreAuthenticationGateway gw;

    constexpr int kThreads = 4;
    constexpr int kIterations = 500;

    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(kThreads));

    std::atomic<int> auth_required_hits{0};

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&gw, &auth_required_hits]() {
            for (int i = 0; i < kIterations; ++i) {
                const auto result =
                    gw.verify("operator-concurrent", "password-concurrent");
                if (!result.has_value() &&
                    result.error_code() == ErrorCode::AuthRequired) {
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
    EXPECT_EQ(gw.verify_count(), kTotal);
    EXPECT_EQ(auth_required_hits.load(std::memory_order_relaxed),
              static_cast<int>(kTotal));
}

// ============================================================================
// UT-210-12: HZ-007 lock-free 単一表明
//            (std::atomic<std::uint64_t>::is_always_lock_free)
//
// 16 ユニット目に拡大. compiler / 標準ライブラリ更新で is_always_lock_free が
// false になった場合、ビルド時点で fail-stop. (本 UT は static_assert を
// テストファイルからも明示確認することで二重防御).
// ============================================================================
TEST(CoreAuthenticationGateway_HZ007, LockFreeAssertion) {
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
        "std::atomic<std::uint64_t> must be always lock-free for "
        "CoreAuthenticationGateway verify_count_ "
        "(SDD §4.11, RCM-002, HZ-002/HZ-007 structural prevention).");
    SUCCEED();
}

}  // namespace th25_ctrl
