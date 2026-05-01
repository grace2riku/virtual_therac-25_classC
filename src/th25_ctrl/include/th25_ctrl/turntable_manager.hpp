// SPDX-License-Identifier: TBD (See LICENSE in repo root once defined)
// TH25-CTRL: UNIT-205 TurntableManager
//
// IEC 62304 Class C, C++20.
// Implementation of SDD-TH25-001 v0.1.1 §4.6 (UNIT-205).
//
// 役割: ターンテーブル位置指令と 3 系統独立センサ集約 + 偏差判定.
// **HZ-001(電子モード+X 線ターゲット非挿入)直接対応 + RCM-005(in-position 冗長確認)の中核**.
//
// SDD §4.6 公開 API (3 種):
//   - move_to(Position_mm target)        : 位置指令
//   - read_position()                    : 3 系統センサ集約 (TurntablePosition)
//   - is_in_position(Position_mm)        : 期待位置との偏差判定 (±0.5 mm)
//
// 補助 API (UT / シミュレーション用):
//   - inject_sensor_readings(s0, s1, s2) : センサ値設定 (将来 UNIT-303/402 結線時はセンサ
//                                          からの実読取に置換、Step 23 では UT 駆動可能)
//   - current_target()                   : 設定済 target 取得
//
// 静的純粋関数 (UT 用):
//   - compute_median(s0, s1, s2)         : 3 値中央値 (med-of-3)
//   - has_discrepancy(s0, s1, s2, th)    : max - min が threshold 超か
//
// SDD §4.6 設計判断 (Step 23 範囲):
//   - 4 つの std::atomic<double> (sensor_0/1/2 + target) で並行アクセス保護
//   - TurntablePosition は POD struct (ヒープ確保なし、SDD §4.6 サンプルそのまま)
//   - is_in_position: 3 系統不一致時 false (in-position と認めない、SDD §4.6 サンプル)
//   - SRS-006 所定 3 位置 (Electron=0.0/XRay=50.0/Light=-50.0 mm) は範囲定数で外部化
//   - SRS-D-007 範囲 (-100.0〜+100.0 mm) を move_to で検証、範囲外は TurntableOutOfPosition
//
// Step 23 範囲制約:
//   SDD §4.6「InterProcessChannel 経由で TurntableSim に位置指令、3 系統センサが目標位置
//   ±0.5 mm 以内に到達するまで待機(タイムアウト 500 ms)」は UNIT-402/UNIT-303 が未実装の
//   ため、Step 23 では target_ 保持まで. read_position は inject_sensor_readings で設定された
//   値を返す. 結線 + 待機ロジックは Step 24+ (UNIT-303/402 実装時).
//
// Therac-25 hazard mapping:
//   - HZ-001 (mode/target mismatch): Tyler/Hamilton/East Texas で発現した「ターンテーブル
//     位置不整合」を 3 系統センサ集約 + has_discrepancy で構造的に検出.
//   - HZ-002 (race condition): 4 atomic + UT 並行 TSan で機械検証.
//   - HZ-003 構造的予防: 整数演算ではないが、SRS-RCM-005 で HZ-003 にも対応(センサ通信
//     プロトコルでの累積カウンタ問題).
//   - HZ-007 (legacy preconditions): static_assert(std::atomic<double>::is_always_lock_free)
//     で 7 ユニット目に拡大. ARM64/x86_64 で通常 lock-free だが、build 時 fail-stop で
//     プラットフォーム差を検出.

#pragma once

#include "th25_ctrl/common_types.hpp"

#include <atomic>

namespace th25_ctrl {

// ============================================================================
// SRS-006 所定 3 位置 (SDD §4.6).
// ============================================================================
inline constexpr Position_mm kElectronPositionMm{0.0};
inline constexpr Position_mm kXRayPositionMm{50.0};
inline constexpr Position_mm kLightPositionMm{-50.0};

// SRS-D-007 範囲 (Position_mm 内部値の範囲).
inline constexpr double kPositionMinMm{-100.0};
inline constexpr double kPositionMaxMm{100.0};

// SRS-006 in-position 許容偏差 (med と expected の差) ±0.5 mm.
inline constexpr double kInPositionToleranceMm{0.5};

// SRS-ALM-003 3 系統センサ不一致閾値 (max - min 差).
inline constexpr double kSensorDiscrepancyThresholdMm{1.0};

// ============================================================================
// TurntablePosition (SDD §4.6 サンプルそのまま).
// 3 系統センサ値 + median + has_discrepancy.
// ============================================================================
struct TurntablePosition {
    Position_mm sensor_0;
    Position_mm sensor_1;
    Position_mm sensor_2;

    // 集約値: 3 系統の中央値 (med-of-3).
    [[nodiscard]] auto median() const noexcept -> Position_mm;

    // 偏差判定: max - min が threshold 超か (SRS-ALM-003、デフォルト 1.0 mm).
    [[nodiscard]] auto has_discrepancy(double threshold_mm = kSensorDiscrepancyThresholdMm)
        const noexcept -> bool;
};

// ============================================================================
// TurntableManager (SDD §4.6 UNIT-205、HZ-001 直接対応 + RCM-005 中核).
// ============================================================================
class TurntableManager {
public:
    TurntableManager() noexcept = default;

    // 所有権を一意に保つためコピー/ムーブ禁止 (4 atomic を含むため).
    TurntableManager(const TurntableManager&) = delete;
    TurntableManager(TurntableManager&&) = delete;
    auto operator=(const TurntableManager&) -> TurntableManager& = delete;
    auto operator=(TurntableManager&&) -> TurntableManager& = delete;
    ~TurntableManager() = default;

    // ----- SDD §4.6 公開 API -----

    // ターンテーブル目標位置指令.
    // 事前条件: target ∈ [-100.0, +100.0] mm (SRS-D-007).
    // 事後条件 (Step 23 範囲): target_ に保存. 実 move + タイムアウト待機は Step 24+ で
    // UNIT-303/402 結線時に完成.
    // エラー時:
    //   - 範囲外: TurntableOutOfPosition (SDD §4.1.1 Turntable 系)
    [[nodiscard]] auto move_to(Position_mm target) noexcept -> Result<void, ErrorCode>;

    // 3 系統センサ集約結果取得 (read-only, atomic acquire load × 3).
    // Step 23 範囲: inject_sensor_readings で設定された値を atomic 経由で取得.
    // 将来 (Step 24+ UNIT-303/402 結線時): IPC 経由でセンサから取得、IpcChannelClosed で
    // 失敗ケースを返却する可能性あり.
    [[nodiscard]] auto read_position() const noexcept -> Result<TurntablePosition, ErrorCode>;

    // 期待位置との偏差判定.
    // 事後条件:
    //   - 3 系統不一致 (max - min > 1.0 mm): false (in-position と認めない、SDD §4.6 サンプル)
    //   - median(s0, s1, s2) と expected の偏差 ≤ 0.5 mm: true
    //   - その他: false
    [[nodiscard]] auto is_in_position(Position_mm expected) const noexcept -> bool;

    // ----- 補助 API (UT / シミュレーション用) -----

    // 3 系統センサ値を設定 (UT 駆動 / シミュレーション用).
    // 将来 (Step 24+) UNIT-303 TurntableSim 経由のセンサ読取に置換予定.
    auto inject_sensor_readings(Position_mm s0, Position_mm s1, Position_mm s2) noexcept -> void;

    // 設定済 target 位置取得.
    [[nodiscard]] auto current_target() const noexcept -> Position_mm;

    // ----- 静的純粋関数 (UT 用) -----

    // 3 値中央値 (med-of-3).
    [[nodiscard]] static constexpr auto compute_median(double a, double b, double c) noexcept
        -> double {
        // ソートなしの分岐ベース median.
        if ((a >= b && a <= c) || (a <= b && a >= c)) {
            return a;
        }
        if ((b >= a && b <= c) || (b <= a && b >= c)) {
            return b;
        }
        return c;
    }

    // 偏差判定: max(a, b, c) - min(a, b, c) > threshold か.
    [[nodiscard]] static constexpr auto has_discrepancy_values(
        double a, double b, double c, double threshold_mm) noexcept -> bool {
        const double max_v = (a > b ? (a > c ? a : c) : (b > c ? b : c));
        const double min_v = (a < b ? (a < c ? a : c) : (b < c ? b : c));
        return (max_v - min_v) > threshold_mm;
    }

    // SRS-D-007 範囲チェック (UT 用 + move_to 内部使用).
    [[nodiscard]] static constexpr auto is_position_in_range(Position_mm target) noexcept
        -> bool {
        return target.value() >= kPositionMinMm && target.value() <= kPositionMaxMm;
    }

private:
    // 3 系統センサ値 + 目標位置を std::atomic<double> で保持.
    // Position_mm 値の内部表現 (double) を直接 atomic 化することで、可変な共有変数を
    // SAD §9 SEP-003 / RCM-002 / RCM-019 に従い lock-free atomic で保護.
    std::atomic<double> sensor_0_{0.0};
    std::atomic<double> sensor_1_{0.0};
    std::atomic<double> sensor_2_{0.0};
    std::atomic<double> target_{0.0};

    // ----- HZ-007 構造的予防 (SDD §7 SOUP-003/004 機能要求) -----
    // UNIT-200/401/201/202/203/204 と同パターンを 7 ユニット目に拡大.
    // ARM64/x86_64 で通常 lock-free だが、プラットフォーム差を build 時 fail-stop で検出.
    static_assert(std::atomic<double>::is_always_lock_free,
        "std::atomic<double> must be always lock-free for TurntableManager 4 atomic "
        "(SDD §4.6, RCM-002, HZ-002/HZ-007 structural prevention).");
};

}  // namespace th25_ctrl
