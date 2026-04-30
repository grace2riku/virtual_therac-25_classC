// SPDX-License-Identifier: TBD (See LICENSE in repo root once defined)
// TH25-CTRL: UNIT-202 TreatmentModeManager
//
// IEC 62304 Class C, C++20.
// Implementation of SDD-TH25-001 v0.1.1 §4.3 (UNIT-202).
//
// 役割: 治療モード (TreatmentMode) の状態と、SRS-002 のモード遷移シーケンス
// (6 ステップ) を実装する. **HZ-001 (電子モード+X 線ターゲット非挿入) への
// 直接対応の中核ユニット**.
//
// SDD §4.3 公開 API (3 種、Electron/XRay/Light に分離):
//   - request_mode_change(Electron, Energy_MeV, BeamState) : Electron 用
//   - request_mode_change(XRay,     Energy_MV,  BeamState) : XRay 用
//   - request_light_mode(BeamState)                        : Light 用
//   - verify_mode_consistency(requested_mode)              : ビームオン時整合再確認
//   - current_mode()                                       : 現在モード取得
//
// SDD §4.3 表 モード遷移許可表 (SRS-002):
//   Light/Electron/XRay 間の任意の遷移を許可 (全 9 通り = 3 × 3、同モードも
//   no-op として許可).
//
// エネルギー範囲 (SRS-004 / SDD §4.1):
//   - Electron: 1.0 〜 25.0 MeV
//   - XRay:     5.0 〜 25.0 MV
//
// 不正条件 (SDD §4.3 異常系):
//   - BeamState != Off          → ModeBeamOnNotAllowed (RCM-001 中核)
//   - エネルギー範囲外          → ModeInvalidTransition (Mode 系該当)
//   - API とモードの不一致      → ModeInvalidTransition
//
// Therac-25 hazard mapping:
//   - HZ-001 (mode/target mismatch): モード遷移許可表 + BeamState 事前条件 +
//     エネルギー範囲検証で「Electron モードで X 線エネルギー設定」「BeamState
//     確認漏れ」「ターンテーブル位置不整合」(Tyler/Hamilton/East Texas で発現)
//     の発現経路を構造的に拒否する.
//   - HZ-002 (race condition): current_mode_ を std::atomic<TreatmentMode>
//     で保持 (release-acquire ordering).
//   - HZ-005 (dose calculation): 強い型 Energy_MeV / Energy_MV による
//     コンパイル時の単位安全 (RCM-008).
//   - HZ-007 (legacy preconditions): static_assert(is_always_lock_free) で
//     ビルド時 fail-stop. UNIT-200/401/201 と同パターンを 4 ユニット目に拡大.
//
// Step 20 範囲制約:
//   verify_mode_consistency の SDD §4.3「センサ 3 系統と要求モードの不整合時
//   ModePositionMismatch」については、UNIT-205 TurntableManager 未実装の
//   ため Step 20 では「内部状態の整合性チェック (current_mode == requested_mode)」
//   のみを実装する. 3 系統センサ依存判定は Step 21+ TurntableManager 結線
//   時に完全化する.

#pragma once

#include "th25_ctrl/common_types.hpp"

#include <atomic>

namespace th25_ctrl {

// ============================================================================
// SRS-004 / SDD §4.1: エネルギー範囲定数
// ============================================================================
inline constexpr double kElectronEnergyMinMeV = 1.0;
inline constexpr double kElectronEnergyMaxMeV = 25.0;
inline constexpr double kXrayEnergyMinMV = 5.0;
inline constexpr double kXrayEnergyMaxMV = 25.0;

// ============================================================================
// TreatmentModeManager (SDD §4.3 UNIT-202).
//
// HZ-001 直接対応の中核ユニット.
// ============================================================================
class TreatmentModeManager {
public:
    // SDD §4.3 「初期モードはセンサ整合がとれた既知の安全位置」.
    // Therac-25 の標準起動状態である Light モードを既定値とする
    // (X 線ターゲット・電子線とも非アクティブな安全位置).
    explicit TreatmentModeManager(
        TreatmentMode initial_mode = TreatmentMode::Light) noexcept;

    // 所有権を一意に保つためコピー/ムーブ禁止.
    TreatmentModeManager(const TreatmentModeManager&) = delete;
    TreatmentModeManager(TreatmentModeManager&&) = delete;
    auto operator=(const TreatmentModeManager&) -> TreatmentModeManager& = delete;
    auto operator=(TreatmentModeManager&&) -> TreatmentModeManager& = delete;
    ~TreatmentModeManager() = default;

    // ----- SDD §4.3 公開 API -----

    // Electron モード要求 (引数: 新モード = Electron、電子線エネルギー、現在の
    // BeamState). 事前条件: BeamState == Off. エラー時:
    //   - ModeBeamOnNotAllowed (BeamState != Off)
    //   - ModeInvalidTransition (new_mode != Electron / エネルギー範囲外)
    [[nodiscard]] auto request_mode_change(
        TreatmentMode new_mode,
        Energy_MeV electron_energy,
        BeamState current_beam_state) noexcept -> Result<void, ErrorCode>;

    // XRay モード要求 (引数: 新モード = XRay、X 線エネルギー、現在の BeamState).
    // 事前条件・エラー処理は Electron 版と同じ.
    [[nodiscard]] auto request_mode_change(
        TreatmentMode new_mode,
        Energy_MV xray_energy,
        BeamState current_beam_state) noexcept -> Result<void, ErrorCode>;

    // Light モード要求 (エネルギー指定不要、治療終了時のターンテーブル位置
    // 移動のみ). 事前条件は同じ.
    [[nodiscard]] auto request_light_mode(
        BeamState current_beam_state) noexcept -> Result<void, ErrorCode>;

    // ビームオン時整合再確認 (SDD §4.3, SRS-003).
    // Step 20 範囲: 内部状態の整合性のみチェック.
    // Step 21+ TurntableManager 結線時に 3 系統センサ依存判定を追加.
    [[nodiscard]] auto verify_mode_consistency(TreatmentMode requested_mode)
        const noexcept -> Result<void, ErrorCode>;

    // 現在モードを取得 (read-only). std::atomic acquire load.
    [[nodiscard]] auto current_mode() const noexcept -> TreatmentMode;

    // ----- 静的純粋関数 (UT / 設計レビュー用に公開) -----

    // SDD §4.3 表に従った遷移可否. Light/Electron/XRay 間は全許可 (no-op 含む).
    [[nodiscard]] static auto is_mode_transition_allowed(
        TreatmentMode from, TreatmentMode to) noexcept -> bool;

    // SRS-004 範囲: 1.0 ≤ MeV ≤ 25.0
    [[nodiscard]] static auto is_electron_energy_in_range(
        Energy_MeV energy) noexcept -> bool;

    // SRS-004 範囲: 5.0 ≤ MV ≤ 25.0
    [[nodiscard]] static auto is_xray_energy_in_range(
        Energy_MV energy) noexcept -> bool;

private:
    std::atomic<TreatmentMode> current_mode_;

    // ----- HZ-007 構造的予防 (SDD §7 SOUP-003/004 機能要求) -----
    // UNIT-200/401/201 と同パターンを 4 ユニット目に拡大.
    static_assert(std::atomic<TreatmentMode>::is_always_lock_free,
        "std::atomic<TreatmentMode> must be always lock-free for "
        "TreatmentModeManager (SDD §4.3, RCM-001, HZ-001/HZ-002/HZ-007 "
        "structural prevention).");
};

}  // namespace th25_ctrl
