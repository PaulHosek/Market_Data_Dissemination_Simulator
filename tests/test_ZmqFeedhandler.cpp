#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <zmq.hpp>
#include <zmq_addon.hpp>

#include "../src/feedhandler/IFeedHandler.h"
#include "../src/feedhandler/ZmqFeedHandler.h"
#include "../src/utils/types.h"

class MockFeedHandler final : public IFeedHandler<MockFeedHandler> {
public:
    inline void subscribe_impl(std::string_view symbol) {
        last_subscribed = symbol;
    }

    inline void unsubscribe_impl(std::string_view symbol) {
        last_unsubscribed = symbol;
    }

    inline void receive_loop_impl(std::stop_token st) {
        while (!st.stop_requested()) {
            std::this_thread::yield();
        }
    }

    std::string last_subscribed;
    std::string last_unsubscribed;
};

class FeedHandlerUnitTest : public ::testing::Test {
protected:
    MockFeedHandler feed_handler_;
};

TEST_F(FeedHandlerUnitTest, StartsAndStopsSafely) {
    EXPECT_NO_THROW(feed_handler_.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_NO_THROW(feed_handler_.stop());
}

TEST_F(FeedHandlerUnitTest, CallsDerivedSubscribeImpl) {
    feed_handler_.subscribe("AAPL");
    EXPECT_EQ(feed_handler_.last_subscribed, "AAPL");
}

TEST_F(FeedHandlerUnitTest, CallsDerivedUnsubscribeImpl) {
    feed_handler_.unsubscribe("MSFT");
    EXPECT_EQ(feed_handler_.last_unsubscribed, "MSFT");
}