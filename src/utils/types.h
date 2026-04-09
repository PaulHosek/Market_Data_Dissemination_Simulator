//
// Created by paul on 11/05/2025.
//

#ifndef TYPES_H
#define TYPES_H


#include <cstdint>
#include <cstring>
#include <variant>
#include "WaitableSpscQueue.h"

namespace types {
    struct Quote {
        char symbol[8];
        double bid_price;
        double ask_price;
        uint32_t bid_size;
        uint32_t ask_size;
        uint64_t enqueue_timestamp;
        uint64_t disseminate_timestamp{0};

    };

    struct Trade {
        char symbol[8];
        double price;
        uint32_t size;
        uint64_t enqueue_timestamp;
        uint64_t disseminate_timestamp{0};
    };

    using MarketDataMsg = std::variant<Quote, Trade>;
    using MarketDataQueue = WaitableSpscQueue<MarketDataMsg, 4096>;
}


#endif //TYPES_H
