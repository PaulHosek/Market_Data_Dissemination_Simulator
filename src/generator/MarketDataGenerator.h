//
// Created by paul on 11/05/2025.
//

#ifndef MARKETDATAGENERATOR_H
#define MARKETDATAGENERATOR_H

#include <filesystem>
#include <random>
#include <string>
#include <vector>
#include <boost/lockfree/spsc_queue.hpp>
#include "IGenerator.h"
#include "types.h"


class MarketDataGenerator : public IGenerator{
public:
    using QueueType_Quote = boost::lockfree::spsc_queue<Quote, boost::lockfree::capacity<10'000>>;
    using QueueType_Trade = boost::lockfree::spsc_queue<Trade, boost::lockfree::capacity<10'000>>;

    MarketDataGenerator(const uint32_t messages_per_second, std::filesystem::path symbols_file);


    void setup(uint32_t message_per_sec) override;


private:
    const std::vector<std::string> symbols_;
    uint32_t message_per_sec_;
    std::mt19937 rng_;
    std::chrono::nanoseconds interval_;

    static std::vector<std::string> read_symbols_file(std::filesystem::path const& filename);

};



#endif //MARKETDATAGENERATOR_H
