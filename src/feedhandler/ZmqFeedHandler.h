#ifndef ZMQ_FEED_HANDLER_H
#define ZMQ_FEED_HANDLER_H

#include "IFeedHandler.h"
#include <string>
#include <zmq.hpp>
#include <zmq_addon.hpp>

class ZmqFeedHandler final : public IFeedHandler<ZmqFeedHandler> {
public:
    explicit ZmqFeedHandler(std::string_view multicast_address)
        : context_(1),
          multicast_sub_(context_, zmq::socket_type::sub) {
        multicast_sub_.connect(std::string{multicast_address});
        multicast_sub_.set(zmq::sockopt::rcvtimeo, 1000);
    }

    explicit ZmqFeedHandler(zmq::socket_t &&multicast_sub)
        : context_(1),
          multicast_sub_(std::move(multicast_sub)) {
        this->multicast_sub_.set(zmq::sockopt::rcvtimeo, 200);
    }

    ~ZmqFeedHandler() {
        // must stop the base class thread before destroying sockets
        this->stop();
        if (multicast_sub_.handle() != nullptr) {
            multicast_sub_.set(zmq::sockopt::linger, 0);
            multicast_sub_.close();
        }
    }

    inline void subscribe_impl(std::string_view symbol) {
        std::string q_topic = "Q:" + std::string(symbol);
        std::string t_topic = "T:" + std::string(symbol);
        multicast_sub_.set(zmq::sockopt::subscribe, q_topic);
        multicast_sub_.set(zmq::sockopt::subscribe, t_topic);
    }

    inline void unsubscribe_impl(std::string_view symbol) {
        std::string q_topic = "Q:" + std::string(symbol);
        std::string t_topic = "T:" + std::string(symbol);
        multicast_sub_.set(zmq::sockopt::unsubscribe, q_topic);
        multicast_sub_.set(zmq::sockopt::unsubscribe, t_topic);
    }

    inline void receive_loop_impl(std::stop_token st) {
        while (!st.stop_requested()) {
            try {
                zmq::message_t topic_msg;
                auto res = multicast_sub_.recv(topic_msg, zmq::recv_flags::none);
                if (!res) continue; // Timeout

                if (!multicast_sub_.get(zmq::sockopt::rcvmore)) {
                    spdlog::warn("Received incomplete multipart message.");
                    continue;
                }

                zmq::message_t payload_msg;
                auto res2 = multicast_sub_.recv(payload_msg, zmq::recv_flags::none);
                if (!res2 || topic_msg.size() < 1) continue;

                const char type = static_cast<const char*>(topic_msg.data())[0];

                if (type == 'Q' && payload_msg.size() == sizeof(types::Quote)) {
                    types::Quote quote;
                    std::memcpy(&quote, payload_msg.data(), sizeof(types::Quote));
                    this->deliver_to_client(quote);
                }
                else if (type == 'T' && payload_msg.size() == sizeof(types::Trade)) {
                    types::Trade trade;
                    std::memcpy(&trade, payload_msg.data(), sizeof(types::Trade));
                    this->deliver_to_client(trade);
                }

            } catch (const zmq::error_t& e) {
                if (e.num() == ETERM || e.num() == ENOTSOCK) break;
                spdlog::error("ZMQ error in receive loop: {}", e.what());
            }
        }
    }

private:
    zmq::context_t context_;
    zmq::socket_t multicast_sub_;
};

#endif