#ifndef RANDOM_WALK_GENERATOR_H
#define RANDOM_WALK_GENERATOR_H

#include "BaseGenerator.h"
#include <filesystem>
#include <random>
#include <string>
#include <vector>
#include <fstream>
#include <spdlog/spdlog.h>

template <typename MarketDataQueue>
class RandomWalkGenerator final : public BaseGenerator<RandomWalkGenerator<MarketDataQueue>, MarketDataQueue> {
public:
    explicit RandomWalkGenerator(MarketDataQueue& queue)
        : BaseGenerator<RandomWalkGenerator<MarketDataQueue>, MarketDataQueue>(queue),
          rng_(std::random_device{}()) {}

    void configure(uint32_t messages_per_second, const std::filesystem::path &symbols_file) {
        this->set_rate(messages_per_second);
        symbols_ = read_symbols_file(symbols_file);
        
        if (symbols_.empty()) throw std::logic_error("Symbols file empty or invalid.");
        
        current_prices_.resize(symbols_.size(), 100.0);
        spdlog::info("RandomWalkGenerator configured: {} msgs/sec, {} symbols", messages_per_second, symbols_.size());
    }

    types::MarketDataMsg generate_msg_impl() {
        std::uniform_int_distribution<size_t> symbol_distr(0, symbols_.size() - 1);
        std::uniform_int_distribution<uint8_t> type_dist(0, 1);

        const std::size_t idx = symbol_distr(rng_);
        const std::string& symbol = symbols_[idx];

        if (type_dist(rng_)) {
            return generate_quote(symbol, idx);
        } else {
            return generate_trade(symbol, idx);
        }
    }

private:
    types::Quote generate_quote(std::string const &symbol, size_t idx) {
        std::normal_distribution<double> price_step(0.0, 0.1);
        std::uniform_int_distribution<uint32_t> quote_size(50, 500);

        current_prices_[idx] += price_step(rng_);
        const double price = current_prices_[idx];
        const double bid_ask_spread = price * 0.001;

        types::Quote next_quote{};
        std::strncpy(next_quote.symbol, symbol.c_str(), sizeof(next_quote.symbol) - 1);
        next_quote.bid_price = price - bid_ask_spread / 2;
        next_quote.ask_price = price + bid_ask_spread / 2;
        next_quote.bid_size = quote_size(rng_);
        next_quote.ask_size = quote_size(rng_);
        next_quote.enqueue_timestamp= std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        return next_quote;
    }

    types::Trade generate_trade(std::string const &symbol, size_t idx) {
        std::normal_distribution<double> price_step(0.0, 0.05);
        std::uniform_int_distribution<uint32_t> trade_size(10, 100);

        current_prices_[idx] += price_step(rng_);
        const double price = current_prices_[idx];

        types::Trade next_trade{};
        std::strncpy(next_trade.symbol, symbol.c_str(), sizeof(next_trade.symbol) - 1);
        next_trade.price = price;
        next_trade.size = trade_size(rng_);
        next_trade.enqueue_timestamp= std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        return next_trade;
    }

    std::vector<std::string> read_symbols_file(std::filesystem::path const &filename) {
        std::vector<std::string> tickers;
        std::ifstream file(filename);
        std::string str;

        // had some problems with linux/windows CRLF vs R so now strip all
        while (std::getline(file, str)) {
            str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
            str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());

            str.erase(std::remove_if(str.begin(), str.end(), ::isspace), str.end());

            if (!str.empty() && str.length() < 9) {
                tickers.push_back(str);
            }
        }
        return tickers;
    }

    std::vector<std::string> symbols_;
    std::mt19937 rng_;
    std::vector<double> current_prices_;
};

#endif