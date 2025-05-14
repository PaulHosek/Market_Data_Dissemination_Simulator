//
// Created by paul on 11/05/2025.
//

#ifndef TYPES_H
#define TYPES_H


#include <cstdint>
#include <cstring>

// Cache lines are 64 byte on my machine
struct alignas(64) Quote {
    double bid_price{0};
    double ask_price{0};
    char symbol[8];
    uint32_t bid_size{0};
    uint32_t ask_size{0};
    uint64_t timestamp{0};

    Quote() {
        std::memset(symbol,0, sizeof(symbol));
    };

};

struct alignas(32) Trade {
    uint32_t volume{0};
    char symbol[8];
    double price{0};
    uint64_t timestamp{0};
    Trade() {
        std::memset(symbol,0, sizeof(symbol));
    };
};

using QueueType_Quote = boost::lockfree::spsc_queue<Quote, boost::lockfree::capacity<10'000>>;
using QueueType_Trade = boost::lockfree::spsc_queue<Trade, boost::lockfree::capacity<10'000>>;




#endif //TYPES_H
