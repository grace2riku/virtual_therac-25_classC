// SPDX-License-Identifier: TBD
// TH25-CTRL: smoke test (Step 14).
//
// 目的: ビルドシステム + GoogleTest + Sanitizer の動作確認のみ.
// 機能要求(SRS-001 ..) に対する UT は Step 17+ で SDD に基づき追加する.

#include "th25_ctrl/version.hpp"
#include "th25_sim/sim_stub.hpp"

#include <gtest/gtest.h>

#include <string_view>

TEST(SmokeTest, VersionStringIsNonEmpty) {
    const std::string_view version = th25_ctrl::version();
    EXPECT_FALSE(version.empty());
}

TEST(SmokeTest, VersionStringMatchesConstant) {
    EXPECT_EQ(th25_ctrl::version(), th25_ctrl::kVersion);
}

TEST(SmokeTest, SimStubLabelIsNonEmpty) {
    const std::string_view label = th25_sim::stub_label();
    EXPECT_FALSE(label.empty());
}
