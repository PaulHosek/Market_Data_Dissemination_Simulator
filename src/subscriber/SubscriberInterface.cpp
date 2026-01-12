//
// Created by paul on 11-Jan-26.
//
#include "SubscriberInterface.h"
#include <spdlog/spdlog.h>
#include <utility>
#include <zmq.hpp>
#include <zmq_addon.hpp> // For modern send/recv
#include <string_view>

SubscriberInterface::SubscriberInterface(std::string_view multicast_address, std::string_view tcp_address)
    : context_(1),
      multicast_sub_(context_, zmq::socket_type::sub),
      tcp_req_(context_, zmq::socket_type::req) {

    multicast_sub_.connect(std::string{multicast_address});
    multicast_sub_.set(zmq::sockopt::subscribe, "");

    tcp_req_.connect(std::string{tcp_address});
}

// Overload for testing
SubscriberInterface::SubscriberInterface(zmq::socket_t& multicast_sub, zmq::socket_t& tcp_req)
    : context_(0), // don't need context if move sockets in
      multicast_sub_(std::move(multicast_sub)),
      tcp_req_(std::move(tcp_req)) {}

SubscriberInterface::~SubscriberInterface() {
    stop();
}

void SubscriberInterface::start() {
    receiver_thread_ = std::jthread([this](std::stop_token st) { receive_loop(std::move(st)); });
}

void SubscriberInterface::stop() {
    multicast_sub_.close();
    tcp_req_.close();
}

void SubscriberInterface::subscribe(std::string_view symbol) {
    subscriptions_.insert(std::string{symbol});

    zmq::message_t request(symbol.data(), symbol.size());

    if (auto res = tcp_req_.send(request, zmq::send_flags::none); !res) {
        spdlog::error("Failed to send subscription request for {}", symbol);
        return;
    }
    zmq::message_t reply;
    if (auto res_recv = tcp_req_.recv(reply, zmq::recv_flags::none)) {
        spdlog::info("Subscribed to {}", symbol);
    }
}

void SubscriberInterface::unsubscribe(std::string_view symbol) {
    subscriptions_.erase(std::string{symbol});

    std::string unsub = "unsubscribe " + std::string{symbol};
    zmq::message_t request(unsub.data(), unsub.size());

    if (tcp_req_.send(request, zmq::send_flags::none)) {
        zmq::message_t reply;
        (void)tcp_req_.recv(reply, zmq::recv_flags::none); // TODO log this results of this later, void is placeholder
        spdlog::info("Unsubscribed from {}", symbol);
    }
}

void SubscriberInterface::receive_loop(std::stop_token st) {
    while (!st.stop_requested()) {
        zmq::message_t msg;

        // ensure we check stop_token
        auto res = multicast_sub_.recv(msg, zmq::recv_flags::none);

        if (!res || msg.size() < 1) continue;

        // deserialize, get type then use memcpy to ensure alignment is right
        uint8_t type = static_cast<const uint8_t*>(msg.data())[0];
        if (type == 0 && msg.size() >= sizeof(types::Quote) + 1) {
            types::Quote quote;
            std::memcpy(&quote, static_cast<const uint8_t*>(msg.data()) + 1, sizeof(types::Quote));

            if (subscriptions_.contains(quote.symbol)) {
                deliver_to_client(quote);
            }
        }
        else if (type == 1 && msg.size() >= sizeof(types::Trade) + 1) {
            types::Trade trade;
            std::memcpy(&trade, static_cast<const uint8_t*>(msg.data()) + 1, sizeof(types::Trade));

            if (subscriptions_.contains(trade.symbol)) {
                deliver_to_client(trade);
            }
        }
    }
}

void SubscriberInterface::deliver_to_client(const types::Quote& quote) {
    spdlog::info("Delivered quote for {}", quote.symbol);
    // stub: send to client via tcp or shared memory in full implementation
}

void SubscriberInterface::deliver_to_client(const types::Trade& trade) {
    spdlog::info("Delivered trade for {}", trade.symbol);
    // stub: send to client
}