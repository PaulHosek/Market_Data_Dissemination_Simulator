//
// Created by paul on 09-Apr-26.
//
#include <gtest/gtest.h>
#include <boost/lockfree/spsc_queue.hpp>
#include "../src/disseminator/UdpDisseminator.h"
#include "../src/utils/WaitableSpscQueue.h"

using Storage = boost::lockfree::spsc_queue<types::MarketDataMsg, boost::lockfree::capacity<1024>>;
using TestQueue = WaitableSpscQueue<types::MarketDataMsg, Storage>;

class UdpDisseminatorTest : public ::testing::Test {
protected:
    TestQueue queue_;
};

TEST_F(UdpDisseminatorTest, StartsAndStopsCleanly) {
    UdpDisseminator<TestQueue> disseminator(queue_, "127.0.0.1", 55550);

    EXPECT_NO_THROW(disseminator.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_NO_THROW(disseminator.stop());
}

TEST_F(UdpDisseminatorTest, ConsumesFromQueue) {
    UdpDisseminator<TestQueue> disseminator(queue_, "127.0.0.1", 55551);
    disseminator.start();

    types::Quote q{};
    std::strncpy(q.symbol, "AAPL    ", 8);
    queue_.push(q);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EXPECT_TRUE(queue_.empty());
    disseminator.stop();
}