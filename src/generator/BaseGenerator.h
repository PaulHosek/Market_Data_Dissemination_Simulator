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
        auto next_time = std::chrono::high_resolution_clock::now();

        while (!stop_tok.stop_requested()) {
            auto now = std::chrono::high_resolution_clock::now();

            if (now >= next_time) {
                types::MarketDataMsg msg = static_cast<Derived*>(this)->generate_msg_impl();

                std::visit([](auto&& arg) {
                    arg.enqueue_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                }, msg);

                while (!stop_tok.stop_requested() && !queue_.push(msg)) {
                }

                next_time += interval_;
            } else {
                if (next_time - now > std::chrono::milliseconds(2)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        }
    }

    std::jthread generating_thread_;
    std::stop_source stop_source_;
};

#endif // BASE_GENERATOR_H