#include <iostream>

#include "generator/MarketDataGenerator.h"
#include "utils/types.h"
#include "chrono"

int main() {
    // using namespace types;
    using namespace std::chrono_literals;
    // QueueType_Quote x{};
    // auto i = x;
    types::QueueType_Quote qq{};
    types::QueueType_Trade tq{};
    MarketDataGenerator mdg(qq, tq);

    mdg.configure(1000, "../data/tickers.txt");
    mdg.start();

    // TODO dissemination engine should pop from queue and forward to the right subscribers
    // Once this works we can think about having a local order book that we use to keep track of old symbols. A subscriber could then also get the history.

    // TODO working here right now
    // Part 1: get dissemination engine to just read from the queue
    // 1. read from queue
    // 2. check if there are subscribers requesting this symbol

    // Part 2:
    // 1. implement subscriber and TCP handshake with dissemination engine
    // 2. implement the UDP multicast of symbols

    // Part 3:
    // 1. Check which performances we want to monitor
    // 2. implement performance monitor



    mdg.stop();







}