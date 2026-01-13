//
// Created by paul on 11-Jan-26.
//
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <utility>
#include "utils/WaitableSpscQueue.h"
using TestQueue = WaitableSpscQueue<int, 10>;

TEST(WaitableSpscQueueTest, PushAndPop) {
    TestQueue queue;
    EXPECT_TRUE(queue.push(42));
    int value;
    EXPECT_TRUE(queue.pop(value));
    EXPECT_EQ(value, 42);
    EXPECT_FALSE(queue.pop(value));
}

TEST(WaitableSpscQueueTest, WaitWhenEmpty) {
    TestQueue queue;
    std::jthread consumer([&] {
        auto st = std::stop_token();
        queue.wait(st);
        int value;
        EXPECT_TRUE(queue.pop(value));
        EXPECT_EQ(value, 42);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(queue.push(42));
}

TEST(WaitableSpscQueueTest, NotificationWakesUp) {
    TestQueue queue;
    std::jthread consumer([&](std::stop_token st) {
        queue.wait(std::move(st));
        int value;
        queue.pop(value);
        EXPECT_EQ(value, 100);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    queue.push(100);
}

TEST(WaitableSpscQueueTest, HandlesFullQueue) {
    TestQueue queue;
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(queue.push(i));
    }
    EXPECT_FALSE(queue.push(10));

    int value;
    EXPECT_TRUE(queue.pop(value));
    EXPECT_EQ(value, 0);
    EXPECT_TRUE(queue.push(10));
}

TEST(WaitableSpscQueueTest, MultiplePushesAndPops) {
    TestQueue queue;
    for (int i = 0; i < 5; ++i) {
        queue.push(i);
    }
    int value;
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(queue.pop(value));
        EXPECT_EQ(value, i);
    }
    EXPECT_FALSE(queue.pop(value));
}

TEST(WaitableSpscQueueTest, ConcurrentProducerConsumer) {
    TestQueue queue;
    std::jthread producer([&] {
        for (int i = 0; i < 20; ++i) {
            queue.push(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::jthread consumer([&] {
        int count = 0;
        while (count < 20) {
            queue.wait();
            int value;
            if (queue.pop(value)) {
                EXPECT_EQ(value, count);
                count++;
            }
        }
    });

}

TEST(WaitableSpscQueueTest, WaitTimeout) {
    TestQueue queue;
    auto start = std::chrono::steady_clock::now();
    std::jthread waiter([&](std::stop_token st) {
        queue.wait(st);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // trigger callback
    waiter.request_stop();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    EXPECT_GE(duration.count(), 100);
}