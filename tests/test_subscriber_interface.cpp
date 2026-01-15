//
// Created by paul on 13-Jan-26.
//

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <spdlog/spdlog.h>
#include <memory> // For std::unique_ptr
#include "../src/subscriber/SubscriberInterface.h"
#include "utils/types.h"

class SubscriberInterfaceTest : public ::testing::Test {
protected:
    zmq::context_t context_;
    zmq::socket_t mock_multicast_pub_;
    zmq::socket_t mock_tcp_rep_;
    std::unique_ptr<SubscriberInterface> subscriber_;

    void SetUp() override {
        context_ = zmq::context_t(1);
        mock_multicast_pub_ = zmq::socket_t(context_, zmq::socket_type::pub);
        mock_multicast_pub_.bind("inproc://multicast_test");

        mock_tcp_rep_ = zmq::socket_t(context_, zmq::socket_type::rep);
        mock_tcp_rep_.bind("inproc://tcp_test");

        zmq::socket_t mock_multicast_sub(context_, zmq::socket_type::sub);
        mock_multicast_sub.connect("inproc://multicast_test");

        zmq::socket_t mock_tcp_req(context_, zmq::socket_type::req);
        mock_tcp_req.connect("inproc://tcp_test");

        subscriber_ = std::make_unique<SubscriberInterface>(std::move(mock_multicast_sub), std::move(mock_tcp_req));
    }

    void TearDown() override {
        subscriber_->stop();
    }
};

TEST_F(SubscriberInterfaceTest, ConstructorSetsUpSockets) {
    EXPECT_NO_THROW(SubscriberInterface(std::string_view{"inproc://multicast"}, std::string_view{"inproc://tcp"}));
}

TEST_F(SubscriberInterfaceTest, SubscribeSendsRequest) {
    std::jthread mock_server([&] {
        zmq::message_t request;
        auto res = mock_tcp_rep_.recv(request, zmq::recv_flags::none);
        EXPECT_TRUE(res);

        std::string_view msg(static_cast<char*>(request.data()), request.size());

        // FIX: Match the "subscribe " prefix added in the implementation
        EXPECT_EQ(msg, "subscribe AAPL");

        // Send mock reply (Crucial: subscribe() is blocking on this)
        zmq::message_t reply(2);
        memcpy(reply.data(), "OK", 2);
        mock_tcp_rep_.send(reply, zmq::send_flags::none);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_NO_THROW(subscriber_->subscribe("AAPL"));
    mock_server.join();
}

TEST_F(SubscriberInterfaceTest, UnsubscribeSendsRequest) {
    std::jthread mock_subscribe_server([&] {
        zmq::message_t request;
        auto res = mock_tcp_rep_.recv(request, zmq::recv_flags::none);
        EXPECT_TRUE(res);
        zmq::message_t reply(2);
        memcpy(reply.data(), "OK", 2);
        mock_tcp_rep_.send(reply, zmq::send_flags::none);
    });

    subscriber_->subscribe("AAPL");
    mock_subscribe_server.join();

    std::jthread mock_unsubscribe_server([&] {
        zmq::message_t request;
        auto res = mock_tcp_rep_.recv(request, zmq::recv_flags::none);
        EXPECT_TRUE(res);
        std::string_view msg(static_cast<char*>(request.data()), request.size());
        EXPECT_EQ(msg, "unsubscribe AAPL");

        zmq::message_t reply(2);
        memcpy(reply.data(), "OK", 2);
        mock_tcp_rep_.send(reply, zmq::send_flags::none);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_NO_THROW(subscriber_->unsubscribe("AAPL"));
    mock_unsubscribe_server.join();
}



TEST_F(SubscriberInterfaceTest, ReceiveLoopProcessesQuote) {
    std::jthread mock_server([&] {
        zmq::message_t request;
        auto res = mock_tcp_rep_.recv(request, zmq::recv_flags::none);
        EXPECT_TRUE(res);
        zmq::message_t reply(2);
        memcpy(reply.data(), "OK", 2);
        mock_tcp_rep_.send(reply, zmq::send_flags::none);
    });

    subscriber_->subscribe("AAPL");
    mock_server.join();

    subscriber_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    uint8_t type = 0;
    types::Quote quote;
    std::strncpy(quote.symbol, "AAPL", sizeof(quote.symbol) - 1);
    quote.bid_price = 150.0;
    quote.ask_price = 150.5;
    quote.bid_size = 100;
    quote.ask_size = 100;
    quote.timestamp = 1;

    zmq::message_t msg(sizeof(type) + sizeof(quote));
    memcpy(msg.data(), &type, sizeof(type));
    memcpy(static_cast<char*>(msg.data()) + sizeof(type), &quote, sizeof(quote));
    mock_multicast_pub_.send(msg, zmq::send_flags::none);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    subscriber_->stop();
}

TEST_F(SubscriberInterfaceTest, ReceiveLoopProcessesTrade) {
    std::jthread mock_server([&] {
        zmq::message_t request;
        auto res = mock_tcp_rep_.recv(request, zmq::recv_flags::none);
        EXPECT_TRUE(res);
        zmq::message_t reply(2);
        memcpy(reply.data(), "OK", 2);
        mock_tcp_rep_.send(reply, zmq::send_flags::none);
    });

    subscriber_->subscribe("AAPL");
    mock_server.join();

    subscriber_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    uint8_t type = 1;
    types::Trade trade;
    std::strncpy(trade.symbol, "AAPL", sizeof(trade.symbol) - 1);
    trade.price = 150.25;
    trade.volume = 50;
    trade.timestamp = 1;

    zmq::message_t msg(sizeof(type) + sizeof(trade));
    memcpy(msg.data(), &type, sizeof(type));
    memcpy(static_cast<char*>(msg.data()) + sizeof(type), &trade, sizeof(trade));
    mock_multicast_pub_.send(msg, zmq::send_flags::none);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    subscriber_->stop();
}

TEST_F(SubscriberInterfaceTest, ReceiveLoopIgnoresUnsubscribed) {
    std::jthread mock_server([&] {
        zmq::message_t request;
        auto res = mock_tcp_rep_.recv(request, zmq::recv_flags::none);
        EXPECT_TRUE(res);
        zmq::message_t reply(2);
        memcpy(reply.data(), "OK", 2);
        mock_tcp_rep_.send(reply, zmq::send_flags::none);
    });

    subscriber_->subscribe("MSFT");
    mock_server.join();

    subscriber_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    uint8_t type = 0;
    types::Quote quote;
    std::strncpy(quote.symbol, "AAPL", sizeof(quote.symbol) - 1);
    quote.bid_price = 150.0;
    quote.ask_price = 150.5;
    quote.bid_size = 100;
    quote.ask_size = 100;
    quote.timestamp = 1;

    zmq::message_t msg(sizeof(type) + sizeof(quote));
    memcpy(msg.data(), &type, sizeof(type));
    memcpy(static_cast<char*>(msg.data()) + sizeof(type), &quote, sizeof(quote));
    mock_multicast_pub_.send(msg, zmq::send_flags::none);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    subscriber_->stop();
}

TEST_F(SubscriberInterfaceTest, StartAndStop) {
    EXPECT_NO_THROW(subscriber_->start());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_NO_THROW(subscriber_->stop());
}

TEST_F(SubscriberInterfaceTest, DestructorStopsThreads) {
    subscriber_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(SubscriberInterfaceTest, SubscribeInvalidSymbol) {
    std::jthread mock_server([&] {
        zmq::message_t request;
        mock_tcp_rep_.recv(request, zmq::recv_flags::none);
        zmq::message_t reply(5);
        memcpy(reply.data(), "ERROR", 5);
        mock_tcp_rep_.send(reply, zmq::send_flags::none);
    });

    EXPECT_NO_THROW(subscriber_->subscribe("INVALID"));
    mock_server.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

