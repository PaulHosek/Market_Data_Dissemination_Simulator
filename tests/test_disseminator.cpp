//
// Created by paul on 11-Jan-26.
//

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "../src/disseminator/Disseminator.h"
#include "utils/types.h"

class DisseminatorTest : public ::testing::Test {
protected:
    types::QueueType_Quote quote_queue_;
    types::QueueType_Trade trade_queue_;
    Disseminator disseminator_{quote_queue_, trade_queue_};

    void TearDown() override {
        disseminator_.stop();
    }

    size_t count_remaining_messages() {
        size_t count = 0;
        types::Quote quote;
        types::Trade trade;
        while (quote_queue_.pop(quote)) count++;
        while (trade_queue_.pop(trade)) count++;
        return count;
    }
};

TEST_F(DisseminatorTest, StartsAndStops) {
    EXPECT_NO_THROW(disseminator_.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_NO_THROW(disseminator_.stop());
}

TEST_F(DisseminatorTest, ProcessStartsWorkers) {
    disseminator_.process();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    disseminator_.process();
    disseminator_.stop();
}

TEST_F(DisseminatorTest, ConsumesQuotes) {
    types::Quote quote1;
    std::strncpy(quote1.symbol, "AAPL", sizeof(quote1.symbol) - 1);
    quote1.bid_price = 150.0;
    quote1.ask_price = 150.5;
    quote1.bid_size = 100;
    quote1.ask_size = 100;
    quote1.timestamp = 1;
    quote_queue_.push(quote1);

    types::Quote quote2;
    std::strncpy(quote2.symbol, "MSFT", sizeof(quote2.symbol) - 1);
    quote2.bid_price = 300.0;
    quote2.ask_price = 300.5;
    quote2.bid_size = 200;
    quote2.ask_size = 200;
    quote2.timestamp = 2;
    quote_queue_.push(quote2);

    disseminator_.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    disseminator_.stop();

    EXPECT_EQ(count_remaining_messages(), 0);
}


TEST_F(DisseminatorTest, ConsumesTrades) {
    types::Trade trade1;
    std::strncpy(trade1.symbol, "AAPL", sizeof(trade1.symbol) - 1);
    trade1.price = 150.25;
    trade1.volume = 50;
    trade1.timestamp = 1;
    trade_queue_.push(trade1);

    types::Trade trade2;
    std::strncpy(trade2.symbol, "MSFT", sizeof(trade2.symbol) - 1);
    trade2.price = 300.25;
    trade2.volume = 75;
    trade2.timestamp = 2;
    trade_queue_.push(trade2);

    disseminator_.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    disseminator_.stop();

    EXPECT_EQ(count_remaining_messages(), 0);
}

TEST_F(DisseminatorTest, HandlesEmptyQueues) {
    disseminator_.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    disseminator_.stop();
    EXPECT_EQ(count_remaining_messages(), 0);
}

TEST_F(DisseminatorTest, StopBeforeStart) {
    EXPECT_NO_THROW(disseminator_.stop());
}

TEST_F(DisseminatorTest, MultipleStartsNoDuplication) {
    disseminator_.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    disseminator_.start(); // not add duplicate workers
    disseminator_.stop();
}