// SPDX-License-Identifier: TBD
// TH25-CTRL: version() implementation.
//
// Step 17 (CR-0005) で common_types.hpp の構文・static_assert を骨格ビルドで
// 検証する目的で本 .cpp にも include を追加した. UNIT-200 CommonTypes は
// 後続 Step で各 UNIT 実装が直接利用する.

#include "th25_ctrl/version.hpp"

#include "th25_ctrl/common_types.hpp"  // ヘッダオンリー型を翻訳単位に取り込み (構文検証).

namespace th25_ctrl {

auto version() noexcept -> std::string_view {
    return kVersion;
}

}  // namespace th25_ctrl
