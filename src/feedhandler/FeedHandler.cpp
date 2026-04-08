#include "FeedHandler.h"
#include <spdlog/spdlog.h>
#include <utility>

FeedHandler::FeedHandler(std::string_view multicast_address)
    : context_(1),
      multicast_sub_(context_, zmq::socket_type::sub) {
    multicast_sub_.connect(std::string{multicast_address});
    multicast_sub_.set(zmq::sockopt::rcvtimeo, 1000);
    // Note: We removed the `subscribe, ""` catch-all.
    // It will receive nothing until subscribe() is called.
}

FeedHandler::FeedHandler(zmq::socket_t&& multicast_sub)
    : context_(1),
      multicast_sub_(std::move(multicast_sub)) {
    this->multicast_sub_.set(zmq::sockopt::rcvtimeo, 200);
}

FeedHandler::~FeedHandler() { stop(); }

void FeedHandler::subscribe(std::string_view symbol) {
    std::string q_topic = "Q:" + std::string(symbol);
    std::string t_topic = "T:" + std::string(symbol);
    multicast_sub_.set(zmq::sockopt::subscribe, q_topic);
    multicast_sub_.set(zmq::sockopt::subscribe, t_topic);
}

void FeedHandler::unsubscribe(std::string_view symbol) {
    std::string q_topic = "Q:" + std::string(symbol);
    std::string t_topic = "T:" + std::string(symbol);
    multicast_sub_.set(zmq::sockopt::unsubscribe, q_topic);
    multicast_sub_.set(zmq::sockopt::unsubscribe, t_topic);
}

void FeedHandler::start() {
    receiver_thread_ = std::jthread([this](std::stop_token st) { receive_loop(std::move(st)); });
}

void FeedHandler::stop() {
    if (receiver_thread_.joinable()) {
        receiver_thread_.request_stop();
        receiver_thread_.join();
    }
    if (multicast_sub_.handle() != nullptr) {
        multicast_sub_.set(zmq::sockopt::linger, 0);
        multicast_sub_.close();
    }
}

void FeedHandler::receive_loop(std::stop_token st) {
    while (!st.stop_requested()) {
        try {
            zmq::message_t topic_msg;
            auto res = multicast_sub_.recv(topic_msg, zmq::recv_flags::none);
            if (!res) continue; // Timeout

            // We expect multi-part messages; verify there is a payload frame
            if (!multicast_sub_.get(zmq::sockopt::rcvmore)) {
                spdlog::warn("Received incomplete multipart message.");
                continue;
            }

            zmq::message_t payload_msg;
            auto res2 = multicast_sub_.recv(payload_msg, zmq::recv_flags::none);
            if (!res2 || topic_msg.size() < 1) continue;

            // First byte of the topic frame dictates the payload type
            const char type = static_cast<const char*>(topic_msg.data())[0];

            if (type == 'Q' && payload_msg.size() == sizeof(types::Quote)) {
                types::Quote quote;
                std::memcpy(&quote, payload_msg.data(), sizeof(types::Quote));
                deliver_to_client(quote);
                // Notice: No unordered_set lookup needed anymore!
            }
            else if (type == 'T' && payload_msg.size() == sizeof(types::Trade)) {
                types::Trade trade;
                std::memcpy(&trade, payload_msg.data(), sizeof(types::Trade));
                deliver_to_client(trade);
            }

        } catch (const zmq::error_t& e) {
            if (e.num() == ETERM || e.num() == ENOTSOCK) break;
            spdlog::error("ZMQ error in receive loop: {}", e.what());
        }
    }
}

void FeedHandler::deliver_to_client(const types::Quote& quote) {
    spdlog::info("Delivered quote for {}", std::string_view(quote.symbol, 8));
}

void FeedHandler::deliver_to_client(const types::Trade& trade) {
    spdlog::info("Delivered trade for {}", std::string_view(trade.symbol, 8));
}