#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <memory>
#include <zmq.hpp>
#include <zmq_addon.hpp>

// Include our new CRTP and Queue headers
#include "../src/disseminator/IDisseminator.h"
#include "../src/disseminator/ZmqDisseminator.h"
#include "../src/utils/types.h"
#include "../src/utils/WaitableSpscQueue.h"

// ==========================================
// 1. MOCK CLASS FOR PURE UNIT TESTS
// ==========================================
// This tests the unwrapping and routing logic WITHOUT touching the network
template <typename MarketDataQueue>
class MockDisseminator final : public IDisseminator<MockDisseminator<MarketDataQueue>, MarketDataQueue> {
public:
    explicit MockDisseminator(MarketDataQueue& queue)
        : IDisseminator<MockDisseminator<MarketDataQueue>, MarketDataQueue>(queue) {}

    inline void send_impl(const char* topic_buf, const void* payload_data, size_t payload_size) {
        messages_sent++;
        last_topic = std::string(topic_buf, 10);
        last_payload_size = payload_size;
    }

    std::atomic<int> messages_sent{0};
    std::string last_topic;
    size_t last_payload_size{0};
};

class ZmqDisseminatorUnitTest : public ::testing::Test {
protected:
    types::MarketDataQueue queue_;
    MockDisseminator<types::MarketDataQueue> disseminator_{queue_};

    size_t count_remaining_messages() {
        size_t count = 0;
        types::MarketDataMsg msg;
        // Since it's a test, we don't want to block, so we use stop_token
        std::stop_source ss;
        ss.request_stop();
        while (queue_.pop(msg, ss.get_token())) count++;
        return count;
    }

    void TearDown() override {
        disseminator_.stop();
    }
};

// --- Unit Tests ---

TEST_F(ZmqDisseminatorUnitTest, StartsAndStops) {
    EXPECT_NO_THROW(disseminator_.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_NO_THROW(disseminator_.stop());
}

TEST_F(ZmqDisseminatorUnitTest, ConsumesQuotesAndFormatsTopic) {
    types::Quote quote1{};
    std::strncpy(quote1.symbol, "AAPL", sizeof(quote1.symbol) - 1);
    queue_.push(quote1);

    disseminator_.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    disseminator_.stop();

    EXPECT_EQ(disseminator_.messages_sent.load(), 1);
    EXPECT_TRUE(disseminator_.last_topic.starts_with("Q:AAPL"));
    EXPECT_EQ(disseminator_.last_payload_size, sizeof(types::Quote));
}

TEST_F(ZmqDisseminatorUnitTest, ConsumesTradesAndFormatsTopic) {
    types::Trade trade1{};
    std::strncpy(trade1.symbol, "MSFT", sizeof(trade1.symbol) - 1);
    queue_.push(trade1);

    disseminator_.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    disseminator_.stop();

    EXPECT_EQ(disseminator_.messages_sent.load(), 1);
    EXPECT_TRUE(disseminator_.last_topic.starts_with("T:MSFT"));
    EXPECT_EQ(disseminator_.last_payload_size, sizeof(types::Trade));
}

TEST_F(ZmqDisseminatorUnitTest, HandlesEmptyQueues) {
    disseminator_.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_NO_THROW(disseminator_.stop());
    EXPECT_EQ(disseminator_.messages_sent.load(), 0);
}