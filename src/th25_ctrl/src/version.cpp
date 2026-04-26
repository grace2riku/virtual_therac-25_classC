// SPDX-License-Identifier: TBD
// TH25-CTRL: version() implementation (Inc.1 placeholder).

#include "th25_ctrl/version.hpp"

namespace th25_ctrl {

auto version() noexcept -> std::string_view {
    return kVersion;
}

}  // namespace th25_ctrl
