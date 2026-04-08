// #ifndef MARKETDATAGENERATOR_H
// #define MARKETDATAGENERATOR_H
//
// #include <filesystem>
// #include <random>
// #include <string>
// #include <vector>
// #include <thread>
// #include <fstream>
// #include <stdexcept>
// #include <chrono>
// #include <stop_token>
// #include <spdlog/spdlog.h>
// #include "BaseGenerator.h"
// #include "../utils/types.h"
//
// template <typename MarketDataQueue>
// class MarketDataGenerator : public IGenerator {
// public:
//     explicit MarketDataGenerator(MarketDataQueue& queue)
//         : messages_per_sec_(0),
//           rng_(std::random_device{}()),
//           interval_(0),
//           queue_(queue),
//           stop_source_() {}
//
//     ~MarketDataGenerator() override { stop(); }
//
//     void configure(uint32_t messages_per_second, const std::filesystem::path &symbols_file) override {
//         if (messages_per_second < 1000) {
//             throw std::invalid_argument("Message rate must be 1000 or larger.");
//         }
//         messages_per_sec_ = messages_per_second;
//         symbols_ = read_symbols_file(symbols_file);
//         interval_ = std::chrono::nanoseconds(1'000'000'000 / messages_per_sec_);
//         current_prices_.resize(symbols_.size(), 100.0);
//         spdlog::info("Generator configuration complete: {} messages/sec, {} symbols", messages_per_second, symbols_.size());
//     }
//
//     void start() override {
//         if (messages_per_sec_ == 0 || symbols_.empty()) {
//             throw std::logic_error("Generator has not been configured. Call configure first.");
//         }
//         if (!generating_thread_.joinable()) {
//             stop_source_ = std::stop_source();
//             generating_thread_ = std::jthread{[this](){ generation_loop(stop_source_.get_token()); }};
//         }
//     }
//
//     void stop() override {
//         if (generating_thread_.joinable()) {
//             stop_source_.request_stop();
//             generating_thread_.join();
//         }
//     }
//
// private:
//     std::vector<std::string> read_symbols_file(std::filesystem::path const &filename) {
//         std::vector<std::string> tickers;
//         std::ifstream file(filename);
//         std::string str;
//         while (std::getline(file, str)) {
//             if (!str.empty() && str.length() < 9) tickers.push_back(str);
//         }
//         return tickers;
//     }
//
//     void generation_loop(const std::stop_token &stop_tok) {
//         std::uniform_int_distribution<size_t> symbol_distr(0, symbols_.size() - 1);
//         std::uniform_int_distribution<uint8_t> type_dist(0, 1);
//         auto previous_time = std::chrono::high_resolution_clock::now();
//
//         while (!stop_tok.stop_requested()) {
//             auto now = std::chrono::high_resolution_clock::now();
//
//             if (auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(now - previous_time); elapsed >= interval_) {
//                 const std::size_t idx = symbol_distr(rng_);
//                 const std::string& symbol = symbols_[idx];
//
//                 if (type_dist(rng_)) {
//                     types::Quote quote = generate_quote(symbol);
//                     quote.enqueue_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
//                         std::chrono::high_resolution_clock::now().time_since_epoch()).count();
//
//                     while (!stop_tok.stop_requested() && !queue_.push(quote)) {
//                         std::this_thread::yield();
//                     }
//                 } else {
//                     types::Trade trade = generate_trade(symbol);
//                     trade.enqueue_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
//                         std::chrono::high_resolution_clock::now().time_since_epoch()).count();
//
//                     while (!stop_tok.stop_requested() && !queue_.push(trade)) {
//                         std::this_thread::yield();
//                     }
//                 }
//                 previous_time = now;
//             } else {
//                 std::this_thread::sleep_for(interval_ - elapsed);
//             }
//         }
//     }
//
//     types::Quote generate_quote(std::string const &symbol) {
//         const auto idx{std::distance(symbols_.begin(), std::ranges::find(symbols_, symbol))};
//         std::normal_distribution<double> price_step(0.0, 0.1);
//         std::uniform_int_distribution<uint32_t> quote_size(50, 500);
//
//         current_prices_[idx] += price_step(rng_);
//         const double price{current_prices_[idx]};
//         const double bid_ask_spread{price * 0.001};
//
//         types::Quote next_quote{};
//         std::strncpy(next_quote.symbol, symbol.c_str(), sizeof(next_quote.symbol) - 1);
//         next_quote.bid_price = price - bid_ask_spread / 2;
//         next_quote.ask_price = price + bid_ask_spread / 2;
//         next_quote.bid_size = quote_size(rng_);
//         next_quote.ask_size = quote_size(rng_);
//         next_quote.enqueue_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
//             std::chrono::system_clock::now().time_since_epoch()).count();
//
//         return next_quote;
//     }
//
//     types::Trade generate_trade(std::string const &symbol) {
//         const auto idx{std::distance(symbols_.begin(), std::ranges::find(symbols_, symbol))};
//         std::normal_distribution<double> price_step(0.0, 0.05);
//         std::uniform_int_distribution<uint32_t> trade_size(10, 100);
//
//         current_prices_[idx] += price_step(rng_);
//         const double price{current_prices_[idx]};
//
//         types::Trade next_trade{};
//         std::strncpy(next_trade.symbol, symbol.c_str(), sizeof(next_trade.symbol) - 1);
//         next_trade.price = price;
//         next_trade.size = trade_size(rng_);
//         next_trade.enqueue_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
//             std::chrono::system_clock::now().time_since_epoch()).count();
//
//         return next_trade;
//     }
//
//     std::vector<std::string> symbols_;
//     uint32_t messages_per_sec_;
//     std::mt19937 rng_;
//     std::chrono::nanoseconds interval_;
//     MarketDataQueue& queue_;
//     std::jthread generating_thread_;
//     std::vector<double> current_prices_;
//     std::stop_source stop_source_;
// };
//
// #endif //MARKETDATAGENERATOR_H