// SPDX-License-Identifier: TBD (See LICENSE in repo root once defined)
// TH25-CTRL: UNIT-200 CommonTypes
//
// IEC 62304 Class C, C++20.
// Implementation of SDD-TH25-001 v0.1 §4.1 / §6.1 / §6.2 / §6.3 / §6.4.
//
// Provided types:
//   - enum class TreatmentMode / LifecycleState / BeamState / ErrorCode / Severity
//   - Strong types (Energy_MeV, Energy_MV, DoseUnit_cGy, MagnetCurrent_A,
//     Position_mm, ElectronGunCurrent_mA, PulseCount)
//   - PulseCounter (std::atomic<uint64_t> wrapper, lock-free verified at build)
//   - Result<T, E> (C++23 std::expected interface, implemented in C++20)
//
// Therac-25 hazard mapping:
//   - HZ-001 (mode / target mismatch) — TreatmentMode + LifecycleState
//   - HZ-002 (race condition) — PulseCounter (std::atomic, lock-free assert)
//   - HZ-003 (integer overflow) — PulseCounter (uint64_t, ~58 million years
//     of continuous 1 kHz operation before overflow)
//   - HZ-005 (dose calculation) — Strong types prevent unit confusion at compile
//   - HZ-006 (cryptic UI) — ErrorCode hierarchy + Severity force structured msgs
//   - HZ-007 (legacy preconditions) — static_assert on is_always_lock_free
//     fails the build if compiler / std lib regression occurs

#pragma once

#include <atomic>
#include <compare>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace th25_ctrl {

// ============================================================================
// §6.1 LifecycleState (SafetyCoreOrchestrator UNIT-201 が使用)
// ============================================================================
enum class LifecycleState : std::uint8_t {
    Init = 0,
    SelfCheck = 1,
    Idle = 2,
    PrescriptionSet = 3,
    Ready = 4,
    BeamOn = 5,
    Halted = 6,
    Error = 7,
};

// ============================================================================
// §4.1 TreatmentMode (UNIT-202 / RCM-001 / HZ-001 直接対応)
// ============================================================================
enum class TreatmentMode : std::uint8_t {
    Electron = 1,
    XRay = 2,
    Light = 3,
};

// ============================================================================
// §4.1 BeamState (UNIT-203 / SRS-007 / SRS-011)
// ============================================================================
enum class BeamState : std::uint8_t {
    Off = 0,
    Arming = 1,
    On = 2,
    Stopping = 3,
};

// ============================================================================
// §4.1.1 ErrorCode 8 系統階層 (上位 8 bit で系統判定)
// SDD §6.2 と完全一致
// ============================================================================
enum class ErrorCode : std::uint16_t {
    // Mode 系 (0x01) — Critical
    ModeInvalidTransition = 0x0101,
    ModePositionMismatch = 0x0102,
    ModeBeamOnNotAllowed = 0x0103,

    // Beam 系 (0x02) — Critical
    BeamOnNotPermitted = 0x0201,
    BeamOffTimeout = 0x0202,
    ElectronGunCurrentOutOfRange = 0x0203,

    // Dose 系 (0x03) — Critical
    DoseOutOfRange = 0x0301,
    DoseTargetExceeded = 0x0302,
    DoseSensorFailure = 0x0303,
    DoseOverflow = 0x0304,

    // Turntable 系 (0x04) — Critical / High
    TurntableOutOfPosition = 0x0401,
    TurntableSensorDiscrepancy = 0x0402,
    TurntableMoveTimeout = 0x0403,

    // Magnet 系 (0x05) — High / Medium
    MagnetCurrentDeviation = 0x0501,
    MagnetCurrentSensorFailure = 0x0502,

    // IPC 系 (0x06) — High
    IpcChannelClosed = 0x0601,
    IpcMessageTooLarge = 0x0602,
    IpcDeserializationFailure = 0x0603,
    IpcQueueOverflow = 0x0604,

    // Auth 系 (0x07) — High (Inc.4 で本格化)
    AuthRequired = 0x0701,
    AuthInvalid = 0x0702,
    AuthExpired = 0x0703,

    // Internal 系 (0xFF) — Critical (fail-stop)
    InternalAssertion = 0xFF01,
    InternalQueueFull = 0xFF02,
    InternalUnexpectedState = 0xFF03,
};

// 系統判定: 上位 8 ビットを取得
[[nodiscard]] constexpr auto error_category(ErrorCode ec) noexcept -> std::uint8_t {
    const auto raw = static_cast<std::uint16_t>(ec);
    return static_cast<std::uint8_t>(raw >> 8U);
}

// ============================================================================
// §6.2 Severity — Critical はバイパス禁止 (SRS-UX-002 / RCM-010)
// ============================================================================
enum class Severity : std::uint8_t {
    Critical = 0,
    High = 1,
    Medium = 2,
    Low = 3,
};

// SDD §6.2 Severity 判定規則
[[nodiscard]] constexpr auto severity_of(ErrorCode ec) noexcept -> Severity {
    const std::uint8_t cat = error_category(ec);
    switch (cat) {
        case 0x01U:  // Mode
        case 0x02U:  // Beam
        case 0x03U:  // Dose
        case 0xFFU:  // Internal
            return Severity::Critical;
        case 0x04U:  // Turntable
        case 0x06U:  // IPC
            return Severity::High;
        case 0x05U:  // Magnet
        case 0x07U:  // Auth
            return Severity::Medium;
        default:
            return Severity::Low;
    }
}

// ============================================================================
// §4.1 / §6.3 強い型 (Strong types — SI 単位混在の静的防止)
// ============================================================================
//
// 共通パターン (RCM-008 / HZ-005 構造的予防):
//   - explicit constexpr コンストラクタ (暗黙変換禁止)
//   - value() getter (基底値の取得)
//   - operator<=> + operator== のみ提供 (比較は単位混在の危険なし)
//   - 算術演算子 (+ - * /) は提供しない (明示的な専用関数経由)
//

class Energy_MeV {
public:
    explicit constexpr Energy_MeV(double value) noexcept : value_(value) {}
    [[nodiscard]] constexpr auto value() const noexcept -> double { return value_; }
    constexpr auto operator<=>(const Energy_MeV&) const noexcept = default;
    constexpr auto operator==(const Energy_MeV&) const noexcept -> bool = default;

private:
    double value_;
};

// X 線エネルギー (Energy_MeV と別型: 暗黙変換禁止)
class Energy_MV {
public:
    explicit constexpr Energy_MV(double value) noexcept : value_(value) {}
    [[nodiscard]] constexpr auto value() const noexcept -> double { return value_; }
    constexpr auto operator<=>(const Energy_MV&) const noexcept = default;
    constexpr auto operator==(const Energy_MV&) const noexcept -> bool = default;

private:
    double value_;
};

class DoseUnit_cGy {
public:
    explicit constexpr DoseUnit_cGy(double value) noexcept : value_(value) {}
    [[nodiscard]] constexpr auto value() const noexcept -> double { return value_; }
    constexpr auto operator<=>(const DoseUnit_cGy&) const noexcept = default;
    constexpr auto operator==(const DoseUnit_cGy&) const noexcept -> bool = default;

    // 累積のみ許可 (SDD §4.1 「減算非提供 — 放射線は累積のみ」).
    // 戻り値は新インスタンス (本体は immutable).
    [[nodiscard]] constexpr auto add_dose(DoseUnit_cGy delta) const noexcept -> DoseUnit_cGy {
        return DoseUnit_cGy{value_ + delta.value_};
    }

private:
    double value_;
};

class MagnetCurrent_A {
public:
    explicit constexpr MagnetCurrent_A(double value) noexcept : value_(value) {}
    [[nodiscard]] constexpr auto value() const noexcept -> double { return value_; }
    constexpr auto operator<=>(const MagnetCurrent_A&) const noexcept = default;
    constexpr auto operator==(const MagnetCurrent_A&) const noexcept -> bool = default;

private:
    double value_;
};

class Position_mm {
public:
    explicit constexpr Position_mm(double value) noexcept : value_(value) {}
    [[nodiscard]] constexpr auto value() const noexcept -> double { return value_; }
    constexpr auto operator<=>(const Position_mm&) const noexcept = default;
    constexpr auto operator==(const Position_mm&) const noexcept -> bool = default;

    // SDD §4.6 TurntableManager の偏差判定で使用 (RCM-005).
    [[nodiscard]] constexpr auto abs_diff(Position_mm other) const noexcept -> Position_mm {
        const double diff = value_ - other.value_;
        return Position_mm{diff < 0.0 ? -diff : diff};
    }

private:
    double value_;
};

class ElectronGunCurrent_mA {
public:
    explicit constexpr ElectronGunCurrent_mA(double value) noexcept : value_(value) {}
    [[nodiscard]] constexpr auto value() const noexcept -> double { return value_; }
    constexpr auto operator<=>(const ElectronGunCurrent_mA&) const noexcept = default;
    constexpr auto operator==(const ElectronGunCurrent_mA&) const noexcept -> bool = default;

private:
    double value_;
};

// ============================================================================
// §4.1 PulseCount (値スナップショット) + PulseCounter (atomic ラップ)
// HZ-002 (race condition) + HZ-003 (整数オーバフロー) の構造的予防
// ============================================================================

class PulseCount {
public:
    explicit constexpr PulseCount(std::uint64_t value) noexcept : value_(value) {}
    [[nodiscard]] constexpr auto value() const noexcept -> std::uint64_t { return value_; }
    constexpr auto operator<=>(const PulseCount&) const noexcept = default;
    constexpr auto operator==(const PulseCount&) const noexcept -> bool = default;

private:
    std::uint64_t value_;
};

// SDD §4.1 PulseCounter: std::atomic<uint64_t> 内包、fetch_add のみ提供。
// HZ-002 / HZ-003 構造的予防の中核 (RCM-002 / RCM-003 / RCM-004 / RCM-019).
//
// 不変条件:
//   - コピー / ムーブ禁止 (atomic を含むため、所有権を一意に保つ)
//   - reset() を除き、accumulated_ は単調増加
//   - is_always_lock_free を build 時に static_assert
class PulseCounter {
public:
    PulseCounter() noexcept = default;
    PulseCounter(const PulseCounter&) = delete;
    PulseCounter(PulseCounter&&) = delete;
    auto operator=(const PulseCounter&) -> PulseCounter& = delete;
    auto operator=(PulseCounter&&) -> PulseCounter& = delete;
    ~PulseCounter() = default;

    // delta を加算し、加算 *前* の累積値を返す (std::atomic::fetch_add のセマンティクス).
    // memory_order_acq_rel: producer / consumer 両側で順序保証 (RCM-002).
    [[nodiscard]] auto fetch_add(PulseCount delta) noexcept -> PulseCount {
        return PulseCount{accumulated_.fetch_add(delta.value(), std::memory_order_acq_rel)};
    }

    // 現在の累積値スナップショットを取得 (acquire load).
    [[nodiscard]] auto load() const noexcept -> PulseCount {
        return PulseCount{accumulated_.load(std::memory_order_acquire)};
    }

    // SDD §4.5 set_dose_target 内で「累積ドーズ = 0 にリセット」する用途.
    // 治療セッション開始時のみ呼出可. release store で他スレッドへ反映.
    auto reset() noexcept -> void {
        accumulated_.store(0, std::memory_order_release);
    }

private:
    std::atomic<std::uint64_t> accumulated_{0};

    // SDD §7 SOUP-003/004 機能要求: std::atomic<uint64_t> lock-free 動作.
    // HZ-007 構造的予防: コンパイラ・標準ライブラリ更新で lock-free 性が
    // 失われた場合、ビルド時点で fail-stop となる.
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
        "std::atomic<uint64_t> must be always lock-free for PulseCounter "
        "(SDD §4.13, RCM-019, HZ-002/HZ-007 structural prevention).");
};

// ============================================================================
// §6.4 Result<T, E> — C++23 std::expected 互換 IF (C++20 で実装)
// std::variant<T, E> ベース. void 特殊化を別途提供.
// ============================================================================

template<class T, class E = ErrorCode>
class Result {
public:
    // 値コンストラクタ (暗黙変換許容: std::expected<T,E>(T) と同じ挙動).
    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
    Result(T value) noexcept(std::is_nothrow_move_constructible_v<T>)
        : storage_(std::in_place_index<0>, std::move(value)) {}

    // 失敗値の構築は明示的なファクトリで (T と E が同じ場合の曖昧性回避).
    [[nodiscard]] static auto error(E err) noexcept -> Result {
        return Result(std::in_place_index<1>, std::move(err));
    }

    [[nodiscard]] auto has_value() const noexcept -> bool { return storage_.index() == 0; }
    explicit operator bool() const noexcept { return has_value(); }

    [[nodiscard]] auto value() const& -> const T& { return std::get<0>(storage_); }
    [[nodiscard]] auto value() & -> T& { return std::get<0>(storage_); }

    [[nodiscard]] auto error_code() const& -> const E& { return std::get<1>(storage_); }

private:
    Result(std::in_place_index_t<1> idx, E err) noexcept
        : storage_(idx, std::move(err)) {}

    std::variant<T, E> storage_;
};

// void 特殊化 (Result<void, E> は std::expected<void, E> 相当).
template<class E>
class Result<void, E> {
public:
    // 成功値の構築.
    [[nodiscard]] static auto ok() noexcept -> Result { return Result{}; }

    // 失敗値の構築.
    [[nodiscard]] static auto error(E err) noexcept -> Result { return Result(std::move(err)); }

    [[nodiscard]] auto has_value() const noexcept -> bool { return !err_.has_value(); }
    explicit operator bool() const noexcept { return has_value(); }

    [[nodiscard]] auto error_code() const -> const E& { return *err_; }

private:
    Result() noexcept = default;
    explicit Result(E err) noexcept : err_(std::move(err)) {}

    std::optional<E> err_;
};

}  // namespace th25_ctrl
