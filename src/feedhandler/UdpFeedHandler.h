//
// Created by paul on 09-Apr-26.
//

#ifndef UDP_FEED_HANDLER_H
#define UDP_FEED_HANDLER_H

#include "IFeedHandler.h"
#include "../utils/types.h"
#include "../utils/CustomSpscQueue.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <string_view>
#include <unordered_set>
#include <stdexcept>
#include <thread>
#include <stop_token>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

// The idea is that we shove the 8 char symbol into this 64bit uint
//      Should make the set faster too.
inline uint64_t pack_symbol(std::string_view sym) {
    // Random walk generator will pad with \0 terminators.
    uint64_t id = 0;
    std::memcpy(&id, sym.data(), std::min(sym.size(), size_t{8}));
    return id;
}

struct SubCommand {
    bool is_subscribe;
    uint64_t symbol_id;
};

class UdpFeedHandler final : public IFeedHandler<UdpFeedHandler> {
public:
    UdpFeedHandler(const std::string& ip, unsigned short port) {
        sock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock_ < 0) throw std::runtime_error("Failed to create UDP socket");

        // put the socket into TIME_WAIT state for 1 minute after a crash so that we can bind again without it being upset
        int opt = 1;
        setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        // increased the OS receive buffer to 8MB -> should handle burst workloads better
        int rcv_buf = 1024 * 1024 * 8;
        setsockopt(sock_, SOL_SOCKET, SO_RCVBUF, &rcv_buf, sizeof(rcv_buf));

        struct sockaddr_in bind_addr{};
        bind_addr.sin_family = AF_INET;
        bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        bind_addr.sin_port = htons(port);

        if (bind(sock_, reinterpret_cast<struct sockaddr*>(&bind_addr), sizeof(bind_addr)) < 0) {
            throw std::runtime_error("Failed to bind UDP socket");
        }

        struct ip_mreq mreq{};
        inet_pton(AF_INET, ip.c_str(), &mreq.imr_multiaddr.s_addr);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY); // Let OS pick the default network interface

        if (setsockopt(sock_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            throw std::runtime_error("Failed to join multicast group. Check if IP is a valid multicast address (e.g., 239.x.x.x)");
        }

        // want the socket in non-blocking mode.
        // then I can spin on the socket and it just returns -1 with EAGAIN
        // Note: I don't need a watcher like epoll/poll/select there because there is only 1 socket.
        // We can just spin on that one.
        int flags = fcntl(sock_, F_GETFL, 0);
        if (flags == -1) throw std::runtime_error("Failed to get socket flags");

        if (fcntl(sock_, F_SETFL, flags | O_NONBLOCK) == -1) {
            throw std::runtime_error("Failed to set non-blocking socket");
        }
    }

    ~UdpFeedHandler() {
        this->stop();
        if (sock_ >= 0) close(sock_);
    }

    // the strategy/client of the feedhandler would call these to subscribe and unsubscribe to a symbol
    void subscribe_impl(std::string_view symbol) {
        command_queue_.push({true, pack_symbol(symbol)});
    }

    void unsubscribe_impl(std::string_view symbol) {
        command_queue_.push({false, pack_symbol(symbol)});
    }

    void receive_loop_impl(std::stop_token st) {
        std::byte buffer[1024];

        std::unordered_set<uint64_t> local_subscriptions_;

        while (!st.stop_requested()) {
            // process changes in the subscriptions by the client/ strategy
            SubCommand cmd;
            while (command_queue_.pop(cmd)) {
                if (cmd.is_subscribe) {
                    local_subscriptions_.insert(cmd.symbol_id);
                } else {
                    local_subscriptions_.erase(cmd.symbol_id);
                }
            }

            // read from the network, check if packet is valid
            ssize_t bytes_recvd = recvfrom(sock_, buffer, sizeof(buffer), 0, nullptr, nullptr);
            if (bytes_recvd < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    std::this_thread::yield();
                }
                continue;
            }
            if (bytes_recvd < types::topic_header_size) {
                continue;
            }

            uint64_t incoming_symbol;
            // skipping the 2-byte prefix, e.g., 'Q:'
            std::memcpy(&incoming_symbol, buffer + 2, 8);

            if (!local_subscriptions_.contains(incoming_symbol)) {
                continue;
            }

            // unpack and deliver
            char msg_type = static_cast<char>(buffer[0]);
            size_t payload_size = bytes_recvd - types::topic_header_size;
            const std::byte* payload_data = buffer + types::topic_header_size;

            if (msg_type == 'Q' && payload_size == sizeof(types::Quote)) {
                types::Quote quote;
                std::memcpy(&quote, payload_data, sizeof(types::Quote));
                this->deliver_to_client(quote);
            }
            else if (msg_type == 'T' && payload_size == sizeof(types::Trade)) {
                types::Trade trade;
                std::memcpy(&trade, payload_data, sizeof(types::Trade));
                this->deliver_to_client(trade);
            }
        }
    }

private:
    int sock_{-1};

    // lockfree spscQ such that the client to the feedhandler can subscribe and unsubscribe
    CustomSpscQueue<SubCommand, 128> command_queue_;
};

#endif // UDP_FEED_HANDLER_H