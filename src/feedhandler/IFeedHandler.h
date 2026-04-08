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

protected:
    IFeedHandler() = default;
    ~IFeedHandler() = default;

    // Delivery logic is centralized here in the base class
    void deliver_to_client(const types::Quote& quote) {
        spdlog::info("Delivered quote for {}", std::string_view(quote.symbol, 8));
        // TODO: Pass to client callback or internal orderbook later
    }

    void deliver_to_client(const types::Trade& trade) {
        spdlog::info("Delivered trade for {}", std::string_view(trade.symbol, 8));
        // TODO: Pass to client callback or internal orderbook later
    }

private:
    std::jthread receiver_thread_;
};

#endif //IFEEDHANDLER_H
