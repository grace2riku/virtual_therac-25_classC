// SPDX-License-Identifier: TBD (See LICENSE in repo root once defined)
// TH25-CTRL: SafetyEventObserver — dispatch 機構の最小基盤 (Step 41 / CR-0028).
//
// IEC 62304 Class C, C++20.
// Inc.1 後半 SafetyCoreOrchestrator dispatch 機構整備の最小基盤として、安全クリ
// ティカルイベントを発火元 (Producer) から購読元 (Observer) に通知するための
// abstract interface を定義する.
//
// 本 Step (Step 41) の範囲:
//   - SafetyEvent enum 定義 (現時点では DoseTargetReached 1 種、後続 Step で拡張)
//   - SafetyEventObserver pure virtual interface (純粋仮想 + RAII、コピー/ムーブ禁止)
//
// 本 Step 範囲外 (Step 42+ で追加):
//   - UNIT-201 SafetyCoreOrchestrator が SafetyEventObserver を実装する結線
//   - UNIT-203 BeamController への request_beam_off() dispatch
//   - MessageBus 経由 BeamOff 指令の SDD §4.5 サンプル完成
//
// SDD §4.5 「目標到達時 IF-U-002 経由で BeamOff 要求送信」サンプルにおいて、
// UNIT-204 DoseManager は本 interface に通知することで「target_reached_ フラグ
// 立て」の副作用として後続ユニット (UNIT-201 → UNIT-203) を起動できる構造に
// 進化する. これにより SDD §4.5 サンプル「目標到達 → BeamOff < 1 ms 連鎖」の
// 構造的前提が本 Step 41 で確立される.
//
// Therac-25 hazard mapping:
//   - HZ-002 (race condition): 本 interface 自体は state を持たず, 実装側で
//     atomic / observer registration により race-free を担保する.
//   - HZ-005 (dose calculation): UNIT-204 の is_target_reached() エッジ検出後の
//     notify 呼出が SafetyCoreOrchestrator (UNIT-201) を起動し、Step 42 で
//     UNIT-203 BeamController へ即時 BeamOff 指令を発行する経路の起点となる.

#pragma once

#include <cstdint>

namespace th25_ctrl {

// ============================================================================
// 安全クリティカルイベント種別.
//
// 現時点 (Step 41) では DoseTargetReached のみ. 後続 Step で:
//   - BeamOnRequested / BeamOffRequested (Step 42+)
//   - ModeChangeRequested (Step Inc.1 後半)
//   - InterlockTripped (Inc.2 で追加)
// 等を順次拡張する想定. enum class で型安全を担保.
// ============================================================================
enum class SafetyEvent : std::uint8_t {
    DoseTargetReached = 0,  // UNIT-204 DoseManager が is_target_reached() エッジ検出時に発火
};

// ============================================================================
// SafetyEventObserver: pure virtual interface.
//
// 設計原則 (SAD §9 SEP-001 / SEP-003 整合):
//   - 純粋仮想のみ、データメンバなし (実装側で state を持つ)
//   - コピー/ムーブ禁止 (registration の所有権を明確化)
//   - virtual destructor で派生クラスの安全な破棄を保証
//   - on_safety_event は noexcept (Producer 側の exception propagation を排除)
//
// 呼出契約 (本 interface を attach する側、すなわち UNIT-204 等の Producer の責務):
//   - on_safety_event は同期呼出 (return まで Producer 側はブロック)
//   - SDD §4.5 「目標到達 → BeamOff < 1 ms 連鎖」を実現するため、Observer
//     実装側は本コールバック内で **< 10 ms 以内** に処理を完了することが要求される
//   - 重い処理が必要な場合は本コールバック内では MessageBus への enqueue のみを行い、
//     後段の処理は別スレッドに委ねる (Inc.3 で完成予定の MessageBus 経由非同期化)
//
// 範囲外 (Step 42+ で具体化):
//   - 複数 Observer の同時 attach (現時点では 1 Producer × 1 Observer の最小構成)
//   - イベントフィルタ (SafetyEvent 種別ごとに購読制御)
// ============================================================================
class SafetyEventObserver {
public:
    SafetyEventObserver() = default;
    virtual ~SafetyEventObserver() = default;

    SafetyEventObserver(const SafetyEventObserver&) = delete;
    SafetyEventObserver(SafetyEventObserver&&) = delete;
    auto operator=(const SafetyEventObserver&) -> SafetyEventObserver& = delete;
    auto operator=(SafetyEventObserver&&) -> SafetyEventObserver& = delete;

    // 安全クリティカルイベントを通知する.
    //
    // 事前条件:
    //   - Producer 側 (e.g. UNIT-204 DoseManager) で attach 済 (atomic load で nullptr チェック後の呼出).
    //
    // 事後条件:
    //   - 本メソッドの戻り後、Producer 側は次の状態遷移に進む.
    //   - 実装側は noexcept を厳守し、exception を propagate しない.
    virtual auto on_safety_event(SafetyEvent event) noexcept -> void = 0;
};

}  // namespace th25_ctrl
