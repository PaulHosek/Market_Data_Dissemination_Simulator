//
// Created by paul on 11/05/2025.
//

#ifndef TYPES_H
#define TYPES_H


#include <cstdint>
#include <cstring>

namespace types {
    // Cache lines are 64 byte on my machine
    // Quote is 32 bit and trade 24
    struct Quote {
        char symbol[8];
        int32_t bid_price{0};
        int32_t ask_price{0};
        uint32_t bid_size{0};
        uint32_t ask_size{0};
        uint64_t timestamp{0};

        Quote() {
            std::memset(symbol,0, sizeof(symbol));
        };

    };

    // TODO think about if we want to align this as 32 bit
    struct alignas(32) Trade {
        char symbol[8];
        uint64_t timestamp{0};
        uint32_t volume{0};
        int32_t price{0};
        Trade() {
            std::memset(symbol,0, sizeof(symbol));
        };
    };

    // find the size of quote
    // template<int N>
    // class TD;
    // TD<sizeof(Quote)> td;

    // static_assert(sizeof(Quote) == 1, "false");
    using QueueType_Quote = boost::lockfree::spsc_queue<Quote, boost::lockfree::capacity<10'000>>;
    using QueueType_Trade = boost::lockfree::spsc_queue<Trade, boost::lockfree::capacity<10'000>>;
}


#endif //TYPES_H
