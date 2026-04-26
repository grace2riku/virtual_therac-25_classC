// SPDX-License-Identifier: TBD (See LICENSE in repo root once defined)
// TH25-CTRL: Therac-25 Virtual Control Software (Inc.1 placeholder).
//
// This header is a Step 14 placeholder establishing the build skeleton.
// Real types (TreatmentMode, Energy_MeV, DoseUnit_cGy, etc.) per SRS-D-001..010
// will be added in Step 17+ following SDD-TH25-001 v0.1.

#pragma once

#include <string_view>

namespace th25_ctrl {

constexpr std::string_view kVersion = "0.2.0-inc1-skeleton";

[[nodiscard]] auto version() noexcept -> std::string_view;

}  // namespace th25_ctrl
