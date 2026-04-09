#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <memory>
#include <cstring>
#include <boost/lockfree/spsc_queue.hpp>

#include "../src/disseminator/ZmqDisseminator.h"
#include "../src/feedhandler/ZmqFeedHandler.h"
#include "../src/utils/WaitableSpscQueue.h"
#include "../src/utils/types.h"

class IntegrationTest : public ::testing::Test {
protected:
    const std::string TEST_ADDR = "tcp://127.0.0.1:5577";
    
    using Storage = boost::lockfree::spsc_queue<types::MarketDataMsg, boost::lockfree::capacity<1024>>;
    using TestQueue = WaitableSpscQueue<types::MarketDataMsg, Storage>;
    TestQueue queue_;
    
    std::unique_ptr<ZmqDisseminator<TestQueue>> disseminator_;
    std::unique_ptr<ZmqFeedHandler> feed_handler_;

    void SetUp() override {
        disseminator_ = std::make_unique<ZmqDisseminator<TestQueue>>(queue_, TEST_ADDR);
        disseminator_->start();

        feed_handler_ = std::make_unique<ZmqFeedHandler>(TEST_ADDR);
        
        // let zmq bind the sockets
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override {
        if (feed_handler_) feed_handler_->stop();
        if (disseminator_) disseminator_->stop();
    }
};

TEST_F(IntegrationTest, EndToEndQuoteDelivery) {
    std::atomic<bool> quote_received{false};
    std::atomic<double> received_bid_price{0.0};

    // use call back for verification
    feed_handler_->set_quote_callback([&](const types::Quote& q) {
        if (std::string_view(q.symbol, 4) == "AAPL") {
            received_bid_price = q.bid_price;
            quote_received = true;
        }
    });

    feed_handler_->subscribe("AAPL");
    feed_handler_->start();

    // zmq processing time again
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    types::Quote q{};
    std::strncpy(q.symbol, "AAPL", sizeof(q.symbol) - 1);
    q.bid_price = 150.25;
    EXPECT_TRUE(queue_.push(q));

    for (int i = 0; i < 50; ++i) {
        if (quote_received.load()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(quote_received.load()) << "Integration test failed: FeedHandler never received the quote.";
    EXPECT_DOUBLE_EQ(received_bid_price.load(), 150.25);
}

TEST_F(IntegrationTest, NativeZmqFilteringDropsUnwantedData) {
    // Subscribe to appl but publish Msft, check if packet dropped by zmq on socket level.
    std::atomic<int> messages_received{0};

    feed_handler_->set_quote_callback([&](const types::Quote&) {
        messages_received++;
    });

    feed_handler_->subscribe("AAPL");
    feed_handler_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    types::Quote q{};
    std::strncpy(q.symbol, "MSFT", sizeof(q.symbol) - 1);
    q.bid_price = 300.00;
    queue_.push(q);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_EQ(messages_received.load(), 0);
}