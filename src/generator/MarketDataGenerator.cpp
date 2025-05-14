//
// Created by paul on 11/05/2025.
//

#include "MarketDataGenerator.h"

#include <fstream>
#include "spdlog/spdlog.h"

MarketDataGenerator::MarketDataGenerator(QueueType_Quote quote_queue, QueueType_Trade trade_queue)
    : messages_per_sec_(0),
      rng_(std::random_device{}()),
      interval_(0),
      seed_{},
      quote_queue_(quote_queue),
      trade_queue_(trade_queue),
      running_(false) {
}

void MarketDataGenerator::configure(const uint32_t messages_per_second, const std::filesystem::path &symbols_file,
                                    const uint32_t seed) {
    messages_per_sec_ = messages_per_second;
    seed_ = seed;
    symbols_ = read_symbols_file(symbols_file);
    interval_ = std::chrono::nanoseconds(1'000'000 / messages_per_sec_);
    current_prices_.resize(symbols_.size(), 100); // initial symbol price
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
std::vector<std::string> MarketDataGenerator::read_symbols_file(std::filesystem::path const &filename) {
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



//
// void MarketDataGenerator::generation_loop() {
//
// }


/**
 * Generates a quote struct and fills it with values matching the random walk of a symbol.
 * Symbols are all initialised at 100 for now
 ***/
Quote MarketDataGenerator::generate_quote(std::string const &symbol) {
    // TODO cannot use string view here because of strncpy
    const auto idx{std::distance(symbols_.begin(), std::ranges::find(symbols_, symbol))};
    std::normal_distribution<double> price_step(0.0, 0.1);
    std::uniform_int_distribution<uint32_t> quote_size(50, 500);

    current_prices_[idx] += price_step(rng_);
    const double price{current_prices_[idx]};
    const double bid_ask_spread{price * 0.001}; // 1%

    Quote next_quote{};
    std::strncpy(next_quote.symbol, symbol.c_str(), sizeof(next_quote.symbol) - 1);
    next_quote.bid_price = price - bid_ask_spread / 2;
    next_quote.ask_price = price + bid_ask_spread / 2;
    next_quote.bid_size = quote_size(rng_);
    next_quote.ask_size = quote_size(rng_);
    next_quote.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    return next_quote;
}

Trade MarketDataGenerator::generate_trade(std::string const &symbol) {
    // TODO the distribution properties as well as bid-ask-spread are fixed for now
    const auto idx{std::distance(symbols_.begin(), std::ranges::find(symbols_, symbol))};
    std::normal_distribution<double> price_step(0.0, 0.05); // narrower than quotes
    std::uniform_int_distribution<uint32_t> trade_size(10, 100);

    current_prices_[idx] += price_step(rng_);
    const double price{current_prices_[idx]};

    Trade next_trade{};
    std::strncpy(next_trade.symbol, symbol.c_str(), sizeof(next_trade.symbol) - 1);
    next_trade.price = price;
    next_trade.volume = trade_size(rng_);
    next_trade.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();


    return next_trade;
}
