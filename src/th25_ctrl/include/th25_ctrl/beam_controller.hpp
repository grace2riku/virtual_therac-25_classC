// SPDX-License-Identifier: TBD (See LICENSE in repo root once defined)
// TH25-CTRL: UNIT-203 BeamController
//
// IEC 62304 Class C, C++20.
// Implementation of SDD-TH25-001 v0.1.1 §4.4 (UNIT-203).
//
// 役割: ビームオン許可フラグの管理と、ビームオン/オフ指令の即時遷移保証.
// **HZ-001(モード/ターゲット不整合)/ HZ-004(インターロック検知失敗)直接対応 +
// RCM-017(ビームオン → ビームオフ即時遷移、< 10 ms 保証)の中核ユニット**.
//
// SDD §4.4 公開 API (3 種):
//   - request_beam_on(LifecycleState) : ビームオン要求 (Ready 状態 + 許可フラグ true 必須)
//   - request_beam_off()              : ビームオフ要求 (任意状態から呼出可)
//   - current_state()                 : 現在の BeamState 取得
//
// SDD §4.4 補助 API (UNIT-202 結線用):
//   - set_beam_on_permission(bool)    : SRS-002 シーケンス完了時に UNIT-202 から呼出
//   - is_beam_on_permitted()          : 現在の許可フラグ読取
//
// SDD §4.4 ビームオン許可フラグの設定根拠 (RCM-001 連携):
//   ビームオン許可フラグは std::atomic<bool> で実装. UNIT-202 が SRS-002 シーケンス
//   (6 ステップ) 完了時のみ true に設定. request_beam_on() 内で compare_exchange_strong
//   で false に reset (= 1 回のビームオン要求で 1 回しか照射されない、消費型).
//   フラグが true でなければ BeamOnNotPermitted を返す.
//
//   これにより以下を保証:
//     1. SRS-002 シーケンス未完了状態でのビームオン要求は構造的に拒否
//     2. **1 回のビームオン要求で 1 回しか照射されない (compare_exchange による消費)**
//     3. race condition は発生しない (SAD §9 SEP-003 + std::atomic メモリオーダリング)
//
// BeamState 状態機械 (SDD §4.1, SRS-007/011):
//   Off → Arming → On → Stopping → Off (周期的)
//
//   - request_beam_on(): Off → Arming → On (許可フラグ消費後)
//   - request_beam_off(): On / Arming → Stopping → Off
//                         Off / Stopping → Off (no-op、許容)
//
// Step 21 範囲制約:
//   SDD §4.4「< 10 ms ウォッチドッグ」と「電子銃電流 0 mA 確認」は UNIT-301
//   ElectronGunSim + UNIT-402 InterProcessChannel が未実装のため、Step 21 範囲では
//   状態機械 + 許可フラグ管理のみを実装. < 10 ms 実測は Inc.1 完了時の IT-101 で実施.
//
// Therac-25 hazard mapping:
//   - HZ-001 (mode/target mismatch): 許可フラグ経由でモード遷移完了確認 (UNIT-202 連携).
//   - HZ-002 (race condition): std::atomic<BeamState> + std::atomic<bool> +
//     compare_exchange_strong による消費型設計で、Therac-25 当時の「許可確認と
//     ビームオン指令の race condition」を構造的に排除.
//   - HZ-004 (interlock): 許可フラグの 1 回限り消費で二重ビームオン要求を構造的に拒否.
//   - HZ-007 (legacy preconditions): static_assert(is_always_lock_free) 二重表明で
//     ビルド時 fail-stop. UNIT-200/401/201/202 と同パターンを 5 ユニット目に拡大.

#pragma once

#include "th25_ctrl/common_types.hpp"

#include <atomic>

namespace th25_ctrl {

// ============================================================================
// BeamController (SDD §4.4 UNIT-203、RCM-017 中核).
// ============================================================================
class BeamController {
public:
    BeamController() noexcept = default;

    // 所有権を一意に保つためコピー/ムーブ禁止.
    BeamController(const BeamController&) = delete;
    BeamController(BeamController&&) = delete;
    auto operator=(const BeamController&) -> BeamController& = delete;
    auto operator=(BeamController&&) -> BeamController& = delete;
    ~BeamController() = default;

    // ----- SDD §4.4 公開 API -----

    // ビームオン要求 (引数: 現在の LifecycleState、Ready 必須).
    // 事前条件:
    //   - lifecycle_state == LifecycleState::Ready
    //   - ビームオン許可フラグ true (UNIT-202 が SRS-002 シーケンス完了時に設定)
    // 事後条件 (成功時):
    //   - 許可フラグが false に消費される (compare_exchange_strong)
    //   - BeamState が Off → Arming → On に遷移
    // エラー時:
    //   - BeamOnNotPermitted (Lifecycle != Ready / 許可フラグ false / 既に Arming/On 状態)
    [[nodiscard]] auto request_beam_on(LifecycleState lifecycle_state) noexcept
        -> Result<void, ErrorCode>;

    // ビームオフ要求 (任意状態から呼出可).
    // 事後条件:
    //   - On / Arming → Stopping → Off に遷移
    //   - Off / Stopping は no-op で Off のまま
    // 注: SDD §4.4「< 10 ms 電子銃電流 0 mA 確認」は IT-101 で実測.
    [[nodiscard]] auto request_beam_off() noexcept -> Result<void, ErrorCode>;

    // 現在の BeamState 取得 (read-only). std::atomic acquire load.
    [[nodiscard]] auto current_state() const noexcept -> BeamState;

    // ----- SDD §4.4 補助 API (UNIT-202 結線用) -----

    // ビームオン許可フラグの設定 (UNIT-202 が SRS-002 シーケンス完了時に呼出).
    // 注: 真偽値どちらでも書込可だが、`request_beam_on` 内の compare_exchange による
    // 消費型動作 (true → false) に従い、本 API は通常 true 設定のみ使用.
    auto set_beam_on_permission(bool permitted) noexcept -> void;

    // 現在の許可フラグを読取 (read-only). 主に UT / 設計レビュー用.
    [[nodiscard]] auto is_beam_on_permitted() const noexcept -> bool;

private:
    std::atomic<BeamState> state_{BeamState::Off};
    std::atomic<bool> beam_on_permitted_{false};

    // ----- HZ-007 構造的予防 (SDD §7 SOUP-003/004 機能要求) -----
    // UNIT-200/401/201/202 と同パターンを 5 ユニット目に拡大.
    static_assert(std::atomic<BeamState>::is_always_lock_free,
        "std::atomic<BeamState> must be always lock-free for BeamController "
        "(SDD §4.4, RCM-017, HZ-001/HZ-002/HZ-004/HZ-007 structural prevention).");
    static_assert(std::atomic<bool>::is_always_lock_free,
        "std::atomic<bool> must be always lock-free for beam_on_permitted_ "
        "(compare_exchange-based consumption, SDD §4.4 §「ビームオン許可フラグ」).");
};

}  // namespace th25_ctrl
