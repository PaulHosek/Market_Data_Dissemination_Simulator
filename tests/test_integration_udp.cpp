//
// Created by paul on 09-Apr-26.
//
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <boost/lockfree/spsc_queue.hpp>
#include "../src/disseminator/UdpDisseminator.h"
#include "../src/feedhandler/UdpFeedHandler.h"
#include "../src/utils/WaitableSpscQueue.h"

using Storage = boost::lockfree::spsc_queue<types::MarketDataMsg, boost::lockfree::capacity<1024>>;
using TestQueue = WaitableSpscQueue<types::MarketDataMsg, Storage>;

class UdpIntegrationTest : public ::testing::Test {
protected:
    const uint16_t TEST_PORT = 55555;
    TestQueue queue_;
    std::unique_ptr<UdpDisseminator<TestQueue>> disseminator_;
    std::unique_ptr<UdpFeedHandler> feedhandler_;

    void SetUp() override {
        feedhandler_ = std::make_unique<UdpFeedHandler>(TEST_PORT);
        disseminator_ = std::make_unique<UdpDisseminator<TestQueue>>(queue_, "127.0.0.1", TEST_PORT);

        feedhandler_->start();
        disseminator_->start();
    }

    void TearDown() override {
        if (feedhandler_) feedhandler_->stop();
        if (disseminator_) disseminator_->stop();
    }
};

TEST_F(UdpIntegrationTest, DeliversSubscribedQuote) {
    // Use release acquire to ensure that the payload is fully written before the receiving thread loads it.
    types::Quote received_quote{};
    std::atomic<bool> quote_received{false};

    feedhandler_->set_quote_callback([&](const types::Quote& q, uint64_t feedhandler_time) {
        received_quote = q;
        quote_received.store(true, std::memory_order_release);
        // force that the received_quote can't move after the store
    });

    feedhandler_->subscribe("NVDA    ");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    types::Quote sent_quote{};
    std::strncpy(sent_quote.symbol, "NVDA    ", 8);
    sent_quote.bid_price = 850.50;
    queue_.push(sent_quote);

    auto start_time = std::chrono::steady_clock::now();
    bool success = false;

    while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(1)) {
        if (quote_received.load(std::memory_order_acquire)) {
            success = true;
            break;
        }
        std::this_thread::yield();
    }

    ASSERT_TRUE(success) << "Timed out waiting for UDP packet.";

    // check the packet
    EXPECT_STREQ(received_quote.symbol, "NVDA    ");
    EXPECT_DOUBLE_EQ(received_quote.bid_price, 850.50);
}

TEST_F(UdpIntegrationTest, DropsUnsubscribedData) {
    int received_count{0};

    feedhandler_->set_quote_callback([&](const types::Quote&, uint64_t feedhandler_time) {
        received_count++;
    });

    feedhandler_->subscribe("MSFT    ");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    types::Quote sent_quote{};
    std::strncpy(sent_quote.symbol, "AAPL    ", 8);
    queue_.push(sent_quote);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(received_count, 0);
}