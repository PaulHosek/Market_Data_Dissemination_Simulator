#ifndef BASE_GENERATOR_H
#define BASE_GENERATOR_H

#include <thread>
#include <chrono>
#include <stop_token>
#include <stdexcept>
#include <variant>
#include "../utils/types.h"

// CRTP Base Class
template <typename Derived, typename MarketDataQueue>
class BaseGenerator {
public:
    explicit BaseGenerator(MarketDataQueue& queue) : queue_(queue), messages_per_sec_(0), interval_(0) {}

    ~BaseGenerator() { stop(); }

    void set_rate(uint32_t messages_per_second) {
        if (messages_per_second < 1) {
            throw std::invalid_argument("Rate must be > 0");
        }
        messages_per_sec_ = messages_per_second;
        interval_ = std::chrono::nanoseconds(1'000'000'000 / messages_per_sec_);
    }

    void start() {
        if (messages_per_sec_ == 0) {
            throw std::logic_error("Generator rate has not been configured.");
        }
        if (!generating_thread_.joinable()) {
            stop_source_ = std::stop_source();
            generating_thread_ = std::jthread{[this](){ generation_loop(stop_source_.get_token()); }};
        }
    }

    void stop() {
        if (generating_thread_.joinable()) {
            stop_source_.request_stop();
            generating_thread_.join();
        }
    }

protected:
    MarketDataQueue& queue_;
    uint32_t messages_per_sec_;
    std::chrono::nanoseconds interval_;

private:
    void generation_loop(const std::stop_token &stop_tok) {
        auto previous_time = std::chrono::steady_clock::now();

        while (!stop_tok.stop_requested()) {
            auto now = std::chrono::steady_clock::now();

            if (auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(now - previous_time); elapsed >= interval_) {
                
                // CRTP Call: Get the next message from the derived specific strategy
                types::MarketDataMsg msg = static_cast<Derived*>(this)->generate_msg_impl();

                // Stamp enqueue time centrally right before pushing
                std::visit([](auto&& arg) {
                    arg.enqueue_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::chrono::steady_clock::now().time_since_epoch()).count();
                }, msg);

                // Try to push, yielding if the queue is temporarily full
                while (!stop_tok.stop_requested() && !queue_.push(msg)) {
                    std::this_thread::yield();
                }
                
                previous_time = now;
            } else {
                std::this_thread::sleep_for(interval_ - elapsed);
            }
        }
    }

    std::jthread generating_thread_;
    std::stop_source stop_source_;
};

#endif // BASE_GENERATOR_H