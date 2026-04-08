#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>

#include "../src/utils/WaitableSpscQueue.h"

constexpr size_t TEST_CAPACITY = 10;
using TestQueue = WaitableSpscQueue<int, TEST_CAPACITY>;

class WaitableQueueTest : public ::testing::Test {
protected:
    TestQueue queue_;
};

TEST_F(WaitableQueueTest, BasicPushPop) {
    EXPECT_TRUE(queue_.empty());

    EXPECT_TRUE(queue_.push(42));
    EXPECT_FALSE(queue_.empty());

    int item = 0;
    std::stop_source ss;
    EXPECT_TRUE(queue_.pop(item, ss.get_token()));

    EXPECT_EQ(item, 42);
    EXPECT_TRUE(queue_.empty());
}

TEST_F(WaitableQueueTest, PushFailsWhenAtCapacity) {
    for (int i = 0; i < TEST_CAPACITY; i++) {
        EXPECT_TRUE(queue_.push(i));
    }

    EXPECT_FALSE(queue_.push(99));
}

TEST_F(WaitableQueueTest, PopReturnsFalseIfAlreadyStopped) {
    int item = 0;
    std::stop_source ss;

    ss.request_stop();

    // Because the queue is empty and the token is stopped, it must return false
    EXPECT_FALSE(queue_.pop(item, ss.get_token()));
}


TEST_F(WaitableQueueTest, ConsumerWakesUpWhenDataPushed) {
    std::stop_source ss;
    std::atomic<int> received_value{0};

    // go to sleep immediately because queue empty
    std::jthread consumer([&](std::stop_token st) {
        int item = 0;
        if (queue_.pop(item, st)) {
            received_value = item;
        }
    });

    // force wait state
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(received_value.load(), 0); // test it's still waiting

    queue_.push(100);// This should trigger `notify_one()` and wake the consumer.

    consumer.join();

    EXPECT_EQ(received_value.load(), 100);
}

TEST_F(WaitableQueueTest, ConsumerWakesUpOnStopRequest) {
    std::stop_source ss;
    std::atomic<bool> pop_returned_false{false};

    std::jthread consumer([&]() {
        int item = 0;
        if (!queue_.pop(item, ss.get_token())) {
            pop_returned_false = true;
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    ss.request_stop();

    consumer.join();

    EXPECT_TRUE(pop_returned_false.load())
        << "Pop should have safely returned false when woken by stop request.";
}

TEST_F(WaitableQueueTest, HighThroughputStressTest) {
    const int NUM_MESSAGES = 50'000;
    std::atomic<int> messages_received{0};

    std::jthread consumer([&](std::stop_token st) {
        int item;
        while (queue_.pop(item, st)) {
            messages_received++;
            if (messages_received == NUM_MESSAGES) {
                break;
            }
        }
    });

    for (int i = 0; i < NUM_MESSAGES; i++) {
        int retries = 0;
        while (!queue_.push(i)) {
            if (++retries > 100) {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            } else {
                std::this_thread::yield();
            }
        }
    }

    consumer.join();

    EXPECT_EQ(messages_received.load(), NUM_MESSAGES);
}