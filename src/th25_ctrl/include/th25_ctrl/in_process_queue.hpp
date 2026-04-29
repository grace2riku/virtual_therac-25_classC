// SPDX-License-Identifier: TBD (See LICENSE in repo root once defined)
// TH25-CTRL: UNIT-401 InProcessQueue
//
// IEC 62304 Class C, C++20.
// Implementation of SDD-TH25-001 v0.1.1 §4.13 (UNIT-401) and §6.6 (MessageBus).
//
// Design: Single-Producer Single-Consumer (SPSC) lock-free ring buffer.
//   - Classical Lamport algorithm (1983) — head/tail indices in std::atomic<size_t>.
//   - Wait-free for both producer and consumer when not full / empty.
//   - Capacity must be a power of two (compile-time enforced) for fast modulo
//     via bit-AND with mask.
//   - Producer (try_publish): release-store on tail after writing value.
//   - Consumer (try_consume): release-store on head after reading value.
//   - Both sides use acquire-load on the opposite index for ordering.
//
// SDD §10 で「folly::ProducerConsumerQueue または独自実装」とされていた選択肢を、
// Step 18 (CR-0006) で「独自 SPSC 実装」として確定. 理由:
//   (a) 学習プロジェクトとしての透明性 (~80 行のヘッダオンリー実装)
//   (b) 依存最小化 (folly 全体は boost / libdouble-conversion 等を含む)
//   (c) SDD §4.13 構造的要件 (lock-free SPSC + is_lock_free 表明) は完全充足
//
// Therac-25 hazard mapping:
//   - HZ-002 (race condition): release-acquire memory ordering で producer の書込が
//     consumer に確実に可視化される. UT-401-XX で 100k メッセージ concurrent 試験.
//   - HZ-007 (legacy preconditions): static_assert(std::atomic<size_t>::
//     is_always_lock_free) で SOUP 更新時 build 時 fail-stop.

#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <optional>
#include <type_traits>
#include <utility>

namespace th25_ctrl {

// SDD §6.5: MessageBus デフォルト容量 = 4096.
inline constexpr std::size_t kDefaultMessageBusCapacity = 4096U;

// SPSC lock-free ring buffer.
//
// Type requirements (T):
//   - default-constructible (内部 std::array<T, Capacity> 初期化のため)
//   - move-constructible / move-assignable
//   - noexcept move 推奨 (例外時の queue 整合性のため)
//
// Capacity requirements:
//   - must be a power of two and >= 2 (compile-time static_assert)
template <typename T, std::size_t Capacity = kDefaultMessageBusCapacity>
class InProcessQueue {
public:
    // ----- 型要求 -----
    static_assert(std::is_default_constructible_v<T>,
        "InProcessQueue<T, ...>: T must be default-constructible.");
    static_assert(std::is_move_constructible_v<T>,
        "InProcessQueue<T, ...>: T must be move-constructible.");
    static_assert(std::is_move_assignable_v<T>,
        "InProcessQueue<T, ...>: T must be move-assignable.");

    // ----- 容量要求 -----
    static_assert(Capacity >= 2U,
        "InProcessQueue<T, Capacity>: Capacity must be at least 2.");
    static_assert((Capacity & (Capacity - 1U)) == 0U,
        "InProcessQueue<T, Capacity>: Capacity must be a power of two "
        "(for fast bit-AND wrap-around).");

    // ----- HZ-007 構造的予防 (SDD §7 SOUP-003/004 機能要求) -----
    // std::atomic<std::size_t> が lock-free でなければビルド時 fail-stop.
    // コンパイラ・標準ライブラリ更新時にこの保証が崩れた場合を即時検出する.
    static_assert(std::atomic<std::size_t>::is_always_lock_free,
        "std::atomic<std::size_t> must be always lock-free for InProcessQueue "
        "(SDD §4.13, RCM-019, HZ-002/HZ-007 structural prevention).");

    InProcessQueue() noexcept = default;

    // SPSC は producer / consumer がそれぞれ単一スレッドであることが前提.
    // インスタンスのコピー / ムーブを許すと所有権の追跡が困難になるため禁止.
    InProcessQueue(const InProcessQueue&) = delete;
    InProcessQueue(InProcessQueue&&) = delete;
    auto operator=(const InProcessQueue&) -> InProcessQueue& = delete;
    auto operator=(InProcessQueue&&) -> InProcessQueue& = delete;
    ~InProcessQueue() = default;

    // 公開容量 (constexpr).
    [[nodiscard]] static constexpr auto capacity() noexcept -> std::size_t {
        return Capacity;
    }

    // producer 側 API. 単一の producer スレッドからのみ呼出可.
    // 戻り値:
    //   - true: enqueue 成功
    //   - false: queue full (consumer がまだ読み出していない)
    [[nodiscard]] auto try_publish(T value) noexcept -> bool {
        const std::size_t cur_tail = tail_.load(std::memory_order_relaxed);
        const std::size_t next_tail = (cur_tail + 1U) & kMask;
        // queue full の判定: 次の tail が consumer の head に追いつくと満杯.
        // (1 スロットを sentinel として消費する古典的 Lamport 設計).
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false;
        }
        buffer_[cur_tail] = std::move(value);
        // release: 上記書込を consumer から acquire で確実に見えるようにする.
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    // consumer 側 API. 単一の consumer スレッドからのみ呼出可.
    // 戻り値:
    //   - 取得成功時: std::optional<T> に値を格納
    //   - empty 時: std::nullopt
    [[nodiscard]] auto try_consume() noexcept -> std::optional<T> {
        const std::size_t cur_head = head_.load(std::memory_order_relaxed);
        // queue empty の判定: head == tail なら何も読み出すものがない.
        if (cur_head == tail_.load(std::memory_order_acquire)) {
            return std::nullopt;
        }
        T value = std::move(buffer_[cur_head]);
        const std::size_t next_head = (cur_head + 1U) & kMask;
        // release: 上記読出を producer から acquire で確実に見えるようにする.
        head_.store(next_head, std::memory_order_release);
        return std::optional<T>{std::move(value)};
    }

    // 観測専用 API (debug / 試験補助). スレッド安全だが整合性は保証されない
    // (producer / consumer が同時に進めば値はすぐに古くなる).
    [[nodiscard]] auto approximate_size() const noexcept -> std::size_t {
        const std::size_t t = tail_.load(std::memory_order_acquire);
        const std::size_t h = head_.load(std::memory_order_acquire);
        return (t - h) & kMask;
    }

    [[nodiscard]] auto empty_approx() const noexcept -> bool {
        return tail_.load(std::memory_order_acquire) == head_.load(std::memory_order_acquire);
    }

private:
    static constexpr std::size_t kMask = Capacity - 1U;

    // false sharing 回避のための alignas は将来検討
    // (alignas(std::hardware_destructive_interference_size) で head_ / tail_
    // を別キャッシュラインに分離すると性能改善が見込めるが、Inc.1 範囲では
    // 機能正しさを優先).
    std::atomic<std::size_t> head_{0};   // consumer が次に読む index
    std::atomic<std::size_t> tail_{0};   // producer が次に書く index
    std::array<T, Capacity> buffer_{};
};

}  // namespace th25_ctrl
