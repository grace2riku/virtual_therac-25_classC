// SPDX-License-Identifier: TBD
// TH25-SIM: Virtual hardware model (Inc.1 placeholder).
//
// This header is a Step 14 placeholder establishing the build skeleton.
// Real virtual hardware abstractions (IElectronGun, IBendingMagnet, ITurntable,
// IIonChamber per SRS-IF-001..004) will be added in Step 17+ following
// SDD-TH25-001 v0.1.

#pragma once

#include <string_view>

namespace th25_sim {

[[nodiscard]] auto stub_label() noexcept -> std::string_view;

}  // namespace th25_sim
