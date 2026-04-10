//
// Created by paul on 08-Apr-26.
//

#ifndef IFEEDHANDLER_H
#define IFEEDHANDLER_H

#include <thread>
#include <stop_token>
#include <string_view>
#include <spdlog/spdlog.h>
#include "../utils/types.h"

template <typename Derived>
class IFeedHandler {
public:
    void start() {
        if (!receiver_thread_.joinable()) {
            receiver_thread_ = std::jthread([this](std::stop_token st) {
                static_cast<Derived*>(this)->receive_loop_impl(std::move(st));
            });
        }
    }

    void stop() {
        if (receiver_thread_.joinable()) {
            receiver_thread_.request_stop();
            receiver_thread_.join();
        }
    }

    void subscribe(std::string_view symbol) {
        static_cast<Derived*>(this)->subscribe_impl(symbol);
    }

    void unsubscribe(std::string_view symbol) {
        static_cast<Derived*>(this)->unsubscribe_impl(symbol);
    }

    void set_quote_callback(std::function<void(const types::Quote&, uint64_t)> cb) { on_quote_ = std::move(cb); }
    void set_trade_callback(std::function<void(const types::Trade&, uint64_t)> cb) { on_trade_ = std::move(cb); }


protected:
    IFeedHandler() = default;
    ~IFeedHandler() = default;

    void deliver_to_client(const types::Quote& quote) {
        if (on_quote_) {
            uint64_t t3 = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            on_quote_(quote, t3); // callback should handle msg and receive_timestamp
        }
    }

    void deliver_to_client(const types::Trade& trade) {
        if (on_trade_) {
            uint64_t t3 = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            on_trade_(trade, t3);
        }
    }


private:
    std::jthread receiver_thread_;
    std::function<void(const types::Quote&, uint64_t)> on_quote_;
    std::function<void(const types::Trade&, uint64_t)> on_trade_;
};

#endif
