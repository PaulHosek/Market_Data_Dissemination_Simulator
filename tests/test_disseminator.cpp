#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <memory>
#include <zmq.hpp>
#include "../src/disseminator/Disseminator.h"
#include "../src/utils/types.h"

class DisseminatorTest : public ::testing::Test {
protected:
    // Shared State
    types::QueueType_Quote quote_queue_;
    types::QueueType_Trade trade_queue_;
    std::unique_ptr<Disseminator> disseminator_;

    // ZMQ Client-side (for external verification)
    zmq::context_t test_context_{1};
    zmq::socket_t sub_socket_{test_context_, zmq::socket_type::sub};
    zmq::socket_t req_socket_{test_context_, zmq::socket_type::req};

    const std::string MULTICAST_ADDR = "tcp://127.0.0.1:5501";
    const std::string TCP_ADDR = "tcp://127.0.0.1:5502";

    void SetUp() override {
        // Initialize Disseminator with required addresses
        disseminator_ = std::make_unique<Disseminator>(
            quote_queue_,
            trade_queue_,
            MULTICAST_ADDR,
            TCP_ADDR
        );

        // Prepare test sockets
        sub_socket_.connect(MULTICAST_ADDR);
        sub_socket_.set(zmq::sockopt::subscribe, "");
        sub_socket_.set(zmq::sockopt::rcvtimeo, 1000);

        req_socket_.connect(TCP_ADDR);
        req_socket_.set(zmq::sockopt::rcvtimeo, 1000);
    }

    void TearDown() override {
        if (disseminator_) {
            disseminator_->stop();
        }
        sub_socket_.close();
        req_socket_.close();
    }

    // Helper from old tests
    size_t count_remaining_messages() {
        size_t count = 0;
        types::Quote quote;
        types::Trade trade;
        while (quote_queue_.pop(quote)) count++;
        while (trade_queue_.pop(trade)) count++;
        return count;
    }
};

// ==========================================
// Test queue interactions

TEST_F(DisseminatorTest, StartsAndStops) {
    EXPECT_NO_THROW(disseminator_->start());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_NO_THROW(disseminator_->stop());
}

TEST_F(DisseminatorTest, ConsumesQuotes) {
    types::Quote quote1;
    std::strncpy(quote1.symbol, "AAPL", sizeof(quote1.symbol) - 1);
    quote_queue_.push(quote1);

    disseminator_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    disseminator_->stop();

    EXPECT_EQ(count_remaining_messages(), 0);
}

TEST_F(DisseminatorTest, ConsumesTrades) {
    types::Trade trade1;
    std::strncpy(trade1.symbol, "AAPL", sizeof(trade1.symbol) - 1);
    trade_queue_.push(trade1);

    disseminator_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    disseminator_->stop();

    EXPECT_EQ(count_remaining_messages(), 0);
}
TEST_F(DisseminatorTest, HandlesEmptyQueues) {
    // 1. Start with no data in queues
    disseminator_->start();

    // 2. Let the threads spin/yield/wait on empty queues for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 3. Stop should still work immediately and not hang
    EXPECT_NO_THROW(disseminator_->stop());

    // 4. Verify no ghost messages appeared
    EXPECT_EQ(count_remaining_messages(), 0);
}

TEST_F(DisseminatorTest, StopBeforeStart) {
    EXPECT_NO_THROW(disseminator_->stop());
}

TEST_F(DisseminatorTest, MultipleStartsNoDuplication) {
    disseminator_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_NO_THROW(disseminator_->start());
    disseminator_->stop();
}




// ==========================================
// Networking tests

TEST_F(DisseminatorTest, PublishQuote_VerifiesBinaryFormat) {
    // We need to call start() because new tests rely on networking
    disseminator_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // joiner delay

    types::Quote q;
    std::strncpy(q.symbol, "AAPL", sizeof(q.symbol) - 1);
    q.bid_price = 150.25;
    quote_queue_.push(q);

    zmq::message_t msg;
    auto res = sub_socket_.recv(msg, zmq::recv_flags::none);

    ASSERT_TRUE(res) << "Timed out waiting for quote message";
    ASSERT_EQ(msg.size(), 1 + sizeof(types::Quote));

    const uint8_t* data_ptr = static_cast<uint8_t*>(msg.data());
    EXPECT_EQ(data_ptr[0], 0); // type byte check

    types::Quote received_q;
    std::memcpy(&received_q, data_ptr + 1, sizeof(types::Quote));
    EXPECT_STREQ(received_q.symbol, "AAPL");
}

TEST_F(DisseminatorTest, ControlPlane_RespondsToSubscribe) {
    disseminator_->start();

    std::string sub_req = "subscribe GOOG";
    zmq::message_t request(sub_req.data(), sub_req.size());

    ASSERT_TRUE(req_socket_.send(request, zmq::send_flags::none));

    zmq::message_t reply;
    ASSERT_TRUE(req_socket_.recv(reply, zmq::recv_flags::none));

    std::string_view reply_str(static_cast<char*>(reply.data()), reply.size());
    EXPECT_EQ(reply_str, "OK");
}