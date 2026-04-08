#ifndef ZMQ_DISSEMINATOR_H
#define ZMQ_DISSEMINATOR_H

#include "IDisseminator.h"
#include <string_view>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <spdlog/spdlog.h>

template <typename MarketDataQueue>
class ZmqDisseminator final : public IDisseminator<ZmqDisseminator<MarketDataQueue>, MarketDataQueue> {
public:
    ZmqDisseminator(MarketDataQueue& queue, std::string_view multicast_addr)
        : IDisseminator<ZmqDisseminator<MarketDataQueue>, MarketDataQueue>(queue),
          context_(1),
          pub_socket_(context_, zmq::socket_type::pub)
    {
        try {
            pub_socket_.bind(std::string{multicast_addr});
            pub_socket_.set(zmq::sockopt::linger, 0);
            spdlog::info("ZmqDisseminator bound to PUB: {}", multicast_addr);
        } catch (const zmq::error_t& e) {
            spdlog::error("ZMQ Bind Error: {}", e.what());
            throw;
        }
    }

    ~ZmqDisseminator() {
        this->stop();
        if (pub_socket_.handle() != nullptr) {
            pub_socket_.close();
        }
    }

    inline void send_impl(const char* topic_buf, const void* payload_data, size_t payload_size) {
        zmq::message_t topic_msg(topic_buf, 10);
        zmq::message_t payload_msg(payload_size);
        std::memcpy(payload_msg.data(), payload_data, payload_size);

        pub_socket_.send(topic_msg, zmq::send_flags::sndmore);
        pub_socket_.send(payload_msg, zmq::send_flags::none);
    }

private:
    zmq::context_t context_;
    zmq::socket_t pub_socket_;
};

#endif