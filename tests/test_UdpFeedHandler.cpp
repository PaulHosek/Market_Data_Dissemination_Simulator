//
// Created by paul on 09-Apr-26.
//
#include <gtest/gtest.h>
#include "../src/feedhandler/UdpFeedHandler.h"

TEST(UdpFeedHandlerTest, StartsAndStopsCleanly) {
    UdpFeedHandler handler(55552);

    EXPECT_NO_THROW(handler.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_NO_THROW(handler.stop());
}

TEST(UdpFeedHandlerTest, CanSubmitSubscriptionsWithoutCrash) {
    UdpFeedHandler handler(55553);
    handler.start();

    EXPECT_NO_THROW(handler.subscribe("AAPL"));
    EXPECT_NO_THROW(handler.subscribe("MSFT"));
    EXPECT_NO_THROW(handler.unsubscribe("AAPL"));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    handler.stop();
}