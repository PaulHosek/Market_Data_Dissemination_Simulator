//
// Created by paul on 11/05/2025.
//

#include "MarketDataGenerator.h"

#include <fstream>
#include "spdlog/spdlog.h";

MarketDataGenerator::MarketDataGenerator(QueueType_Quote quote_queue, QueueType_Trade trade_queue)
    :messages_per_sec_(0),
    rng_(std::random_device{}()),
    interval_(0),
    quote_queue_(quote_queue),
    trade_queue_(trade_queue),
    running_(false)
{}

void MarketDataGenerator::configure(const uint32_t messages_per_second, const std::filesystem::path &symbols_file) {
    messages_per_sec_ = messages_per_second;
    symbols_ = read_symbols_file(symbols_file);
    interval_ = std::chrono::nanoseconds(1'000'000 / messages_per_sec_);
    current_prices_.resize(symbols_.size());
    spdlog::info("Generator configuration complete: {} messages/sec, {} symbols", messages_per_second, symbols_.size());
}




/**
 * Reads a list of stock tickers from a file.
 * Each ticker is expected to be on a separate line, with a maximum length of 8 characters.
 *
 * @param filename The path to the file containing the tickers.
 * @return A vector of strings representing the stock tickers.
 *
 * Note: This implementation uses std::ifstream for file reading.
 * Using a memory-mapped file (MMF) is an alternative approach for large files.
 */
std::vector<std::string> MarketDataGenerator::read_symbols_file(std::filesystem::path const& filename) {
    std::vector<std::string> tickers;
    std::ifstream file(filename);
    std::string str;
    while (std::getline(file, str)) {
        if (!str.empty() && str.length() < 9) {
           tickers.push_back(str);
        }
    }
    return tickers;
}
