#ifndef I_DISSEMINATOR_H
#define I_DISSEMINATOR_H

#include <thread>
#include <stop_token>
#include <cstring>
#include <variant>
#include "../utils/types.h"

template <typename Derived, typename MarketDataQueue>
class IDisseminator {
public:
    void start() {
        if (!worker_.joinable()) {
            worker_ = std::jthread([this](std::stop_token st) { run_loop(std::move(st)); });
        }
    }

    void stop() {
        if (worker_.joinable()) {
            worker_.request_stop();
            worker_.join();
        }
    }

protected:
    // derived classes can instantiate this class only
    explicit IDisseminator(MarketDataQueue& queue) : queue_(queue) {}
    
    ~IDisseminator() = default;

private:
    void run_loop(std::stop_token stoken) {
        typename MarketDataQueue::value_type msg; 
        
        char topic_buf[10]; 
        topic_buf[1] = ':'; // e.g., "Q:SYMBOL  " or "T:SYMBOL  "

        while (queue_.pop(msg, stoken)) {
            std::visit([&](auto&& payload) {
                using T = std::decay_t<decltype(payload)>;
                payload.disseminate_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch()).count();

                if constexpr (std::is_same_v<T, types::Quote>) {
                    topic_buf[0] = 'Q';
                    std::memcpy(&topic_buf[2], payload.symbol, 8);
                } 
                else if constexpr (std::is_same_v<T, types::Trade>) {
                    topic_buf[0] = 'T';
                    std::memcpy(&topic_buf[2], payload.symbol, 8);
                }

                static_cast<Derived*>(this)->send_impl(topic_buf, &payload, sizeof(T));
                
            }, msg);
        }
    }

    MarketDataQueue& queue_;
    std::jthread worker_;
};

#endif