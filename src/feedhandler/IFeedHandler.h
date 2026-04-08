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
                // CRTP Call: Dispatch to the derived class's network loop
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

    void set_quote_callback(std::function<void(const types::Quote&)> cb) { on_quote_ = std::move(cb); }
    void set_trade_callback(std::function<void(const types::Trade&)> cb) { on_trade_ = std::move(cb); }


protected:
    IFeedHandler() = default;
    ~IFeedHandler() = default;

    void deliver_to_client(const types::Quote& quote) {
        if (on_quote_) on_quote_(quote);
    }

    void deliver_to_client(const types::Trade& trade) {
        if (on_trade_) on_trade_(trade);
    }


private:
    std::jthread receiver_thread_;
    std::function<void(const types::Quote&)> on_quote_;
    std::function<void(const types::Trade&)> on_trade_;
};

#endif
