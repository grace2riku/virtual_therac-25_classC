// SPDX-License-Identifier: TBD
// TH25-CTRL: UNIT-401 InProcessQueue ユニット試験 (UTPR-TH25-001 v0.2 §7.2 と整合).
//
// 試験対象: src/th25_ctrl/include/th25_ctrl/in_process_queue.hpp (UNIT-401).
// 適用範囲: SDD-TH25-001 v0.1.1 §4.13 / §6.6 で確定された SPSC lock-free queue.
//
// IEC 62304 §5.5.4 クラス C 追加受入基準:
//   - 正常系 / 境界値 / 並行処理 / 資源 / データフロー
//
// Therac-25 主要因類型 (SPRP §4.3.1):
//   - A (race condition): 1 producer + 1 consumer concurrent fetch (UT-401-12)
//   - F (レガシー前提喪失): is_always_lock_free static_assert (build 時検証)

#include "th25_ctrl/in_process_queue.hpp"

#include "th25_ctrl/common_types.hpp"  // Result<T,E> 等のメッセージ型独立性試験.

#include <gtest/gtest.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <type_traits>
#include <vector>

namespace th25_ctrl {

// ============================================================================
// UT-401-01: 初期状態 — empty
// ============================================================================
TEST(InProcessQueue_Init, IsEmptyInitially) {
    InProcessQueue<int, 16> q;
    EXPECT_TRUE(q.empty_approx());
    EXPECT_EQ(q.approximate_size(), 0U);
    EXPECT_EQ(q.try_consume(), std::nullopt);
}

// ============================================================================
// UT-401-02: capacity() の compile-time 値
// ============================================================================
TEST(InProcessQueue_Capacity, ReportsTemplateValue) {
    static_assert(InProcessQueue<int, 16>::capacity() == 16U);
    static_assert(InProcessQueue<int, 64>::capacity() == 64U);
    SUCCEED() << "Compile-time capacity() checks passed.";
}

// ============================================================================
// UT-401-03: 単一 publish + consume — 値の整合性
// ============================================================================
TEST(InProcessQueue_SingleThreaded, PublishConsumeRoundTrip) {
    InProcessQueue<int, 8> q;
    EXPECT_TRUE(q.try_publish(42));
    EXPECT_EQ(q.approximate_size(), 1U);

    auto v = q.try_consume();
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, 42);
    EXPECT_TRUE(q.empty_approx());
}

// ============================================================================
// UT-401-04: FIFO 順序保証
// ============================================================================
TEST(InProcessQueue_SingleThreaded, PreservesFifoOrder) {
    InProcessQueue<int, 8> q;
    for (int i = 0; i < 5; ++i) {
        ASSERT_TRUE(q.try_publish(i * 10));
    }
    for (int i = 0; i < 5; ++i) {
        auto v = q.try_consume();
        ASSERT_TRUE(v.has_value());
        EXPECT_EQ(*v, i * 10);
    }
    EXPECT_TRUE(q.empty_approx());
}

// ============================================================================
// UT-401-05: 容量境界 — Capacity-1 まで詰めて、Capacity 目で full
// (古典的 Lamport 設計では 1 スロットを sentinel として消費するため、
//  実質的に格納可能なのは Capacity - 1 件).
// ============================================================================
TEST(InProcessQueue_Boundary, FullAtCapacityMinusOne) {
    constexpr std::size_t kCap = 8U;
    InProcessQueue<int, kCap> q;

    // Capacity - 1 件は格納できる.
    for (std::size_t i = 0; i < kCap - 1U; ++i) {
        ASSERT_TRUE(q.try_publish(static_cast<int>(i)));
    }
    EXPECT_EQ(q.approximate_size(), kCap - 1U);

    // 最後の 1 件は full で拒否される.
    EXPECT_FALSE(q.try_publish(999));

    // 1 件 consume すれば 1 件 publish できるようになる.
    auto v = q.try_consume();
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, 0);
    EXPECT_TRUE(q.try_publish(999));
}

// ============================================================================
// UT-401-06: empty 状態での try_consume — nullopt
// ============================================================================
TEST(InProcessQueue_Boundary, ConsumeOnEmptyReturnsNullopt) {
    InProcessQueue<int, 4> q;
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(q.try_consume(), std::nullopt);
    }
}

// ============================================================================
// UT-401-07: ring wrap-around — Capacity を超えた累積 publish/consume
// 古典的 wrap-around バグ (off-by-one、modulo 計算誤り) の境界検出.
// ============================================================================
TEST(InProcessQueue_Boundary, RingWrapAroundOverManyCycles) {
    constexpr std::size_t kCap = 8U;
    InProcessQueue<int, kCap> q;

    // capacity の何倍も回す. 各サイクルで publish → consume.
    for (int cycle = 0; cycle < 100; ++cycle) {
        for (std::size_t i = 0; i < kCap - 1U; ++i) {
            ASSERT_TRUE(q.try_publish(cycle * 1000 + static_cast<int>(i)));
        }
        for (std::size_t i = 0; i < kCap - 1U; ++i) {
            auto v = q.try_consume();
            ASSERT_TRUE(v.has_value());
            EXPECT_EQ(*v, cycle * 1000 + static_cast<int>(i));
        }
        EXPECT_TRUE(q.empty_approx());
    }
}

// ============================================================================
// UT-401-08: lock-free 表明 (SDD §7 SOUP-003/004 / HZ-007 構造的)
// ============================================================================
TEST(InProcessQueue_LockFree, AtomicSizeIsAlwaysLockFree) {
    EXPECT_TRUE(std::atomic<std::size_t>::is_always_lock_free);
}

// ============================================================================
// UT-401-09: コピー / ムーブ禁止 (Compile-time 検証)
// ============================================================================
TEST(InProcessQueue_OwnershipTraits, IsNotCopyableNorMovable) {
    using Q = InProcessQueue<int, 8>;
    static_assert(!std::is_copy_constructible_v<Q>);
    static_assert(!std::is_copy_assignable_v<Q>);
    static_assert(!std::is_move_constructible_v<Q>);
    static_assert(!std::is_move_assignable_v<Q>);
}

// ============================================================================
// UT-401-10: メッセージ型独立性 — struct 型での publish/consume
// ============================================================================
struct TestMessage {
    int command;
    double payload;
    std::uint64_t request_id;

    // default-constructible (template constraint 充足).
    TestMessage() noexcept = default;
    TestMessage(int c, double p, std::uint64_t r) noexcept
        : command(c), payload(p), request_id(r) {}
};

TEST(InProcessQueue_MessageTypes, StructWithMultipleFieldsRoundTrip) {
    InProcessQueue<TestMessage, 8> q;
    ASSERT_TRUE(q.try_publish(TestMessage{7, 3.14, 999U}));

    auto v = q.try_consume();
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->command, 7);
    EXPECT_DOUBLE_EQ(v->payload, 3.14);
    EXPECT_EQ(v->request_id, 999U);
}

// ============================================================================
// UT-401-11: メッセージ型独立性 — TreatmentMode (UNIT-200 enum)
// ============================================================================
TEST(InProcessQueue_MessageTypes, EnumClassRoundTrip) {
    InProcessQueue<TreatmentMode, 8> q;
    ASSERT_TRUE(q.try_publish(TreatmentMode::Electron));
    ASSERT_TRUE(q.try_publish(TreatmentMode::XRay));
    ASSERT_TRUE(q.try_publish(TreatmentMode::Light));

    auto v1 = q.try_consume();
    auto v2 = q.try_consume();
    auto v3 = q.try_consume();
    ASSERT_TRUE(v1.has_value());
    ASSERT_TRUE(v2.has_value());
    ASSERT_TRUE(v3.has_value());
    EXPECT_EQ(*v1, TreatmentMode::Electron);
    EXPECT_EQ(*v2, TreatmentMode::XRay);
    EXPECT_EQ(*v3, TreatmentMode::Light);
}

// ============================================================================
// UT-401-12: 並行処理 — 1 producer + 1 consumer で 100k メッセージ全件受信
// SDD §4.13 / RCM-002 / RCM-019 / HZ-002 直接対応の中核試験.
// tsan プリセットで race condition 検出 0 を期待.
// ============================================================================
TEST(InProcessQueue_Concurrency, SpscOneHundredThousandMessages) {
    constexpr std::size_t kCap = 1024U;
    constexpr int kCount = 100'000;
    InProcessQueue<int, kCap> q;

    std::atomic<bool> producer_done{false};

    std::thread producer([&]() {
        for (int i = 0; i < kCount; ++i) {
            // queue full の場合は consumer を待つ (busy wait).
            while (!q.try_publish(i)) {
                std::this_thread::yield();
            }
        }
        producer_done.store(true, std::memory_order_release);
    });

    std::vector<int> received;
    received.reserve(kCount);

    std::thread consumer([&]() {
        while (received.size() < static_cast<std::size_t>(kCount)) {
            auto v = q.try_consume();
            if (v.has_value()) {
                received.push_back(*v);
            } else {
                // empty: producer が終わっていてさらに empty なら break.
                if (producer_done.load(std::memory_order_acquire) && q.empty_approx()) {
                    break;
                }
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    // 全件受信 + FIFO 順序保証.
    ASSERT_EQ(received.size(), static_cast<std::size_t>(kCount));
    for (int i = 0; i < kCount; ++i) {
        ASSERT_EQ(received[static_cast<std::size_t>(i)], i)
            << "Message order broken at index " << i;
    }
}

// ============================================================================
// UT-401-13: 並行処理 — burst pattern (producer が一気に詰めてから consumer が一気に取り出す)
// ============================================================================
TEST(InProcessQueue_Concurrency, BurstFillThenDrain) {
    constexpr std::size_t kCap = 64U;
    constexpr int kBursts = 50;
    InProcessQueue<int, kCap> q;

    std::atomic<int> total_received{0};

    std::thread consumer([&]() {
        const int target = kBursts * static_cast<int>(kCap - 1U);
        while (total_received.load(std::memory_order_relaxed) < target) {
            auto v = q.try_consume();
            if (v.has_value()) {
                total_received.fetch_add(1, std::memory_order_relaxed);
            } else {
                std::this_thread::yield();
            }
        }
    });

    std::thread producer([&]() {
        for (int b = 0; b < kBursts; ++b) {
            // 1 burst で capacity-1 件詰める (full までは詰められる).
            for (std::size_t i = 0; i < kCap - 1U; ++i) {
                while (!q.try_publish(b * 1000 + static_cast<int>(i))) {
                    std::this_thread::yield();
                }
            }
        }
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(total_received.load(), kBursts * static_cast<int>(kCap - 1U));
}

}  // namespace th25_ctrl
