//
// Created by paul on 11/05/2025.
//

#ifndef TYPES_H
#define TYPES_H


#include <cstdint>
#include <cstring>
#include <boost/lockfree/spsc_queue.hpp>

#include "WaitableSpscQueue.h"

namespace types {
    // Cache lines are 64 byte on my machine
    // Quote is 32 bit and trade 24
    struct Quote {
        char symbol[8];
        double bid_price{0};
        double ask_price{0};
        uint32_t bid_size{0};
        uint32_t ask_size{0};
        uint64_t timestamp{0};

        Quote() {
            std::memset(symbol,0, sizeof(symbol));
        };

    };
    // TODO think about if we want to align this as 32 bit
    struct Trade {
        char symbol[8];
        uint64_t timestamp{0};
        uint32_t volume{0};
        double price{0};
        Trade() {
            std::memset(symbol,0, sizeof(symbol));
        };
    };



    // find the size of quote
    // template<int N>
    // class TD;
    // TD<sizeof(Quote)> td;
    // static_assert(sizeof(Quote) == 1, "false");
    using QueueType_Quote = WaitableSpscQueue<Quote, 10'000>;
    using QueueType_Trade = WaitableSpscQueue<Trade, 10'000>;

    // template<typename Q>
    // concept BoostSpscQueue = requires
    // {
    //     // requires std::same_as<<Q, boost::lockfree::spsc_queue<ValueType>>;
    //     requires std::same_as<Q, boost::lockfree::spsc_queue<types::Trade>> ||
    //     requires std::same_as<Q, boost::lockfree::spsc_queue<types::Quote>>;
    // };
    //
}


#endif //TYPES_H
