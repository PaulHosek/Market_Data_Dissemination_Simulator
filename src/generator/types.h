//
// Created by paul on 11/05/2025.
//

#ifndef TYPES_H
#define TYPES_H


#include <cstdint>
#include <cstring>

// Cache lines are 64 byte on my machine
struct alignas(64) Quote {
    int64_t bid_price{0};
    int64_t ask_price{0};
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
    int64_t price{0};
    uint64_t timestamp{0};
    Trade() {
        std::memset(symbol,0, sizeof(symbol));
    };
};




#endif //TYPES_H
