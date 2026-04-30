// SPDX-License-Identifier: TBD
// TH25-CTRL: version() implementation.
//
// Step 17 (CR-0005) で common_types.hpp の構文・static_assert を骨格ビルドで
// 検証する目的で本 .cpp にも include を追加した. UNIT-200 CommonTypes は
// 後続 Step で各 UNIT 実装が直接利用する.

#include "th25_ctrl/version.hpp"

#include "th25_ctrl/common_types.hpp"      // ヘッダオンリー型を翻訳単位に取り込み (構文検証).
#include "th25_ctrl/in_process_queue.hpp"  // SPSC ring buffer も同様に取り込み.
#include "th25_ctrl/safety_core_orchestrator.hpp"  // UNIT-201 (Step 19) の static_assert を発火.
#include "th25_ctrl/treatment_mode_manager.hpp"    // UNIT-202 (Step 20) の static_assert を発火.
#include "th25_ctrl/beam_controller.hpp"           // UNIT-203 (Step 21) の static_assert を発火.
#include "th25_ctrl/dose_manager.hpp"              // UNIT-204 (Step 22) の static_assert を発火.

namespace {
// in_process_queue.hpp / common_types.hpp / safety_core_orchestrator.hpp /
// treatment_mode_manager.hpp の static_assert を翻訳単位レベルで確実に
// 発火させるための「死の参照」. インスタンス化されない限り Capacity の
// power-of-two 検証等が走らないため、明示的に template instantiation を強制.
// SafetyCoreOrchestrator / TreatmentModeManager は本 .cpp とは別の翻訳単位で
// リンクされるが、ここで std::atomic<LifecycleState/TreatmentMode/bool>::
// is_always_lock_free の static_assert を発火させる.
[[maybe_unused]] inline constexpr auto kQueueCapacityProbe =
    th25_ctrl::InProcessQueue<int, 16>::capacity();
[[maybe_unused]] inline constexpr auto kOrchestratorEventQueueCapacityProbe =
    th25_ctrl::SafetyCoreOrchestrator::EventQueue::capacity();
[[maybe_unused]] inline constexpr auto kElectronEnergyMinProbe =
    th25_ctrl::kElectronEnergyMinMeV;
[[maybe_unused]] inline constexpr auto kDoseTargetMinProbe =
    th25_ctrl::kDoseTargetMinCGy;
}  // namespace

namespace th25_ctrl {

auto version() noexcept -> std::string_view {
    return kVersion;
}

}  // namespace th25_ctrl
