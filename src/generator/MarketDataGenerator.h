//
// Created by paul on 11/05/2025.
//

#ifndef MARKETDATAGENERATOR_H
#define MARKETDATAGENERATOR_H

#include <atomic>
#include <filesystem>
#include <random>
#include <string>
#include <vector>
#include <boost/lockfree/spsc_queue.hpp>
#include "IGenerator.h"
#include "types.h"

// Generator pushes values onto two queues for trades and quotes
class MarketDataGenerator : public IGenerator{
public:


    MarketDataGenerator(QueueType_Quote quote_queue, QueueType_Trade trade_queue);
    void configure(uint32_t messages_per_second, const std::filesystem::path &symbols_file) override;


private:
    std::vector<std::string> symbols_;
    uint32_t messages_per_sec_;
    std::mt19937 rng_;
    std::chrono::nanoseconds interval_;
    QueueType_Quote& quote_queue_;
    QueueType_Trade& trade_queue_;
    std::atomic<bool> running_;
    std::vector<std::int64_t> current_prices_;
    static std::vector<std::string> read_symbols_file(std::filesystem::path const& filename);

};



#endif //MARKETDATAGENERATOR_H
