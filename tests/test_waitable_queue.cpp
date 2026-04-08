#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>

// Adjust this path to wherever your WaitableSpscQueue is located
#include "../src/utils/WaitableSpscQueue.h"

// We use a tiny capacity so we can easily test the "full" state
constexpr size_t TEST_CAPACITY = 10;
using TestQueue = WaitableSpscQueue<int, TEST_CAPACITY>;

class WaitableQueueTest : public ::testing::Test {
protected:
    TestQueue queue_;
};

// ==========================================
// 1. Single-Threaded Logic Tests
// ==========================================

TEST_F(WaitableQueueTest, BasicPushPop) {
    EXPECT_TRUE(queue_.empty());

    // Push an item
    EXPECT_TRUE(queue_.push(42));
    EXPECT_FALSE(queue_.empty());

    // Pop the item
    int item = 0;
    std::stop_source ss;
    EXPECT_TRUE(queue_.pop(item, ss.get_token()));

    EXPECT_EQ(item, 42);
    EXPECT_TRUE(queue_.empty());
}

TEST_F(WaitableQueueTest, PushFailsWhenAtCapacity) {
    // Fill the queue to its maximum capacity
    for (int i = 0; i < TEST_CAPACITY; i++) {
        EXPECT_TRUE(queue_.push(i));
    }

    // The very next push should instantly return false
    EXPECT_FALSE(queue_.push(99));
}

TEST_F(WaitableQueueTest, PopReturnsFalseIfAlreadyStopped) {
    int item = 0;
    std::stop_source ss;

    // Stop the token before we even try to pop
    ss.request_stop();

    // Because the queue is empty and the token is stopped, it must return false
    EXPECT_FALSE(queue_.pop(item, ss.get_token()));
}

// ==========================================
// 2. Multi-Threaded Concurrency Tests
// ==========================================

TEST_F(WaitableQueueTest, ConsumerWakesUpWhenDataPushed) {
    std::stop_source ss;
    std::atomic<int> received_value{0};

    // 1. Start a consumer thread. It will immediately go to sleep
    // because the queue is empty.
    std::jthread consumer([&](std::stop_token st) {
        int item = 0;
        if (queue_.pop(item, st)) {
            received_value = item;
        }
    });

    // Give the consumer a tiny bit of time to hit the wait() state
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(received_value.load(), 0); // Ensure it's still waiting

    // 2. Push data. This should trigger `notify_one()` and wake the consumer.
    queue_.push(100);

    // Wait for the thread to finish processing
    consumer.join();

    EXPECT_EQ(received_value.load(), 100);
}

TEST_F(WaitableQueueTest, ConsumerWakesUpOnStopRequest) {
    std::stop_source ss;
    std::atomic<bool> pop_returned_false{false};

    // 1. Consumer goes to sleep on an empty queue
    std::jthread consumer([&]() {
        int item = 0;
        if (!queue_.pop(item, ss.get_token())) {
            pop_returned_false = true;
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 2. We never push data. Instead, we request a stop.
    // This tests that your `std::stop_callback` successfully calls `notify_all()`.
    ss.request_stop();

    consumer.join();

    EXPECT_TRUE(pop_returned_false.load())
        << "Pop should have safely returned false when woken by stop request.";
}

TEST_F(WaitableQueueTest, HighThroughputStressTest) {
    std::stop_source ss;
    const int NUM_MESSAGES = 500'000;
    std::atomic<int> messages_received{0};

    // Fast Consumer
    std::jthread consumer([&](std::stop_token st) {
        int item;
        while (queue_.pop(item, st)) {
            messages_received++;
        }
    });

    // Fast Producer
    for (int i = 0; i < NUM_MESSAGES; i++) {
        // Since capacity is only 10, the producer will frequently outpace the consumer.
        // We spin-yield until there's space to push.
        while (!queue_.push(i)) {
            std::this_thread::yield();
        }
    }

    // Wait for the consumer to drain the remaining items in the queue
    while (!queue_.empty()) {
        std::this_thread::yield();
    }

    // Shut everything down
    ss.request_stop();
    consumer.request_stop();
    consumer.join();

    EXPECT_EQ(messages_received.load(), NUM_MESSAGES);
}