//
// Created by paul on 01-Dec-25.
//

#include "Disseminator.h"
#include <cstring>
#include <thread>
#include <spdlog/spdlog.h>
#include "../utils/types.h"

Disseminator::Disseminator(types::QueueType_Quote& quotes,
                           types::QueueType_Trade& trades,
                           std::string_view multicast_addr,
                           std::string_view tcp_addr)
    : trades_(trades),
      quotes_(quotes),
      context_(1),
      pub_socket_(context_, zmq::socket_type::pub),
      rep_socket_(context_, zmq::socket_type::rep)
{
    try {
        pub_socket_.bind(std::string{multicast_addr});
        pub_socket_.set(zmq::sockopt::linger, 0);

        rep_socket_.bind(std::string{tcp_addr});
        rep_socket_.set(zmq::sockopt::linger, 0);
        rep_socket_.set(zmq::sockopt::rcvtimeo, 200);

        spdlog::info("Disseminator bound to PUB: {} and REP: {}", multicast_addr, tcp_addr);
    } catch (const zmq::error_t& e) {
        spdlog::error("ZMQ Bind Error: {}", e.what());
        throw;
    }
}

Disseminator::~Disseminator() {
    // avoid calling virtual function in destructor
    stop_internal_();
}

void Disseminator::start() {
    process();
}

void Disseminator::stop() {
    stop_internal_();
}

void Disseminator::stop_internal_() {
    workers_.clear();

    if (control_thread_.joinable()) {
        control_thread_.request_stop();
        control_thread_.join();
    }

    if (pub_socket_.handle() != nullptr) {
        pub_socket_.set(zmq::sockopt::linger, 0);
        pub_socket_.close();
    }
    if (rep_socket_.handle() != nullptr) {
        rep_socket_.set(zmq::sockopt::linger, 0);
        rep_socket_.close();
    }
}

void Disseminator::process() {
    if (!workers_.empty()) return;

    workers_.emplace_back([this](std::stop_token st) { consume_quotes(quotes_, st); });
    workers_.emplace_back([this](std::stop_token st) { consume_trades(trades_, st); });

    control_thread_ = std::jthread([this](std::stop_token st) { run_control_plane(st); });
}

void Disseminator::consume_quotes(types::QueueType_Quote& q, const std::stop_token stoken) {
    types::Quote qt;
    static constinit auto msg_size = sizeof(uint8_t) + sizeof(types::Quote);
    while (!stoken.stop_requested()) {
        if (q.pop(qt)) {
            zmq::message_t msg(msg_size);

            uint8_t type = 0;
            auto* ptr = static_cast<uint8_t*>(msg.data());

            std::memcpy(ptr, &type, sizeof(type));
            std::memcpy(ptr + sizeof(type), &qt, sizeof(types::Quote));

            {
                std::lock_guard<std::mutex> lock(pub_mutex_);
                pub_socket_.send(msg, zmq::send_flags::none);
            }
        }
    }
}

void Disseminator::consume_trades(types::QueueType_Trade& q, std::stop_token stoken) {
    types::Trade tr;
    static constinit auto msg_size = sizeof(uint8_t) + sizeof(types::Trade);
    while (!stoken.stop_requested()) {
        if (q.pop(tr)) {
            zmq::message_t msg(msg_size);

            uint8_t type = 1;
            auto* ptr = static_cast<uint8_t*>(msg.data());

            std::memcpy(ptr, &type, sizeof(type));
            std::memcpy(ptr + sizeof(type), &tr, sizeof(types::Trade));

            {
                std::lock_guard<std::mutex> lock(pub_mutex_);
                pub_socket_.send(msg, zmq::send_flags::none);
            }
        }
    }
}

void Disseminator::run_control_plane(std::stop_token stoken) {
    while (!stoken.stop_requested()) {
        zmq::message_t request;
        if (auto res = rep_socket_.recv(request, zmq::recv_flags::none)) {

            std::string_view req_str(static_cast<char*>(request.data()), request.size());
            spdlog::debug("Disseminator received control msg: {}", req_str);

            zmq::message_t reply(2);
            std::memcpy(reply.data(), "OK", 2);
            rep_socket_.send(reply, zmq::send_flags::none);
        }
    }
}



// TODO specific case first with 1 queue type, then convert using template and concept or compile type polymorphism

