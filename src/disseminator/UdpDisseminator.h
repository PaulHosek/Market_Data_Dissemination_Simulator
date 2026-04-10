//
// Created by paul on 09-Apr-26.
//

#ifndef UDP_DISSEMINATOR_H
#define UDP_DISSEMINATOR_H

#include "IDisseminator.h"
#include "../utils/types.h"

#include <cstddef>
#include <string>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <spdlog/spdlog.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

template <typename MarketDataQueue>
class UdpDisseminator final : public IDisseminator<UdpDisseminator<MarketDataQueue>, MarketDataQueue> {
public:
    UdpDisseminator(MarketDataQueue& queue, const std::string& ip, unsigned short port)
        : IDisseminator<UdpDisseminator<MarketDataQueue>, MarketDataQueue>(queue) {

        sock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock_ < 0) {
            throw std::runtime_error("Failed to create UDP socket.");
        }


        // increase buffer
        int snd_buf = 1024 * 1024;
        setsockopt(sock_, SOL_SOCKET, SO_SNDBUF, &snd_buf, sizeof(snd_buf));

        unsigned char mc_ttl = 1; // don't want packet to escape local subnet
        setsockopt(sock_, IPPROTO_IP, IP_MULTICAST_TTL, &mc_ttl, sizeof(mc_ttl));

        struct sockaddr_in dest_addr{};
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &dest_addr.sin_addr);

        dest_addr_ = dest_addr;
    }

    ~UdpDisseminator() {
        this->stop();
        if (sock_ >= 0) {
            close(sock_);
        }
    }

    inline void send_impl(const char* topic_buf, const void* payload_data, size_t payload_size) {
        constexpr size_t buffer_size = types::topic_header_size + std::max(sizeof(types::Quote), sizeof(types::Trade));

        std::byte datagram[buffer_size];

        std::memcpy(datagram, topic_buf, types::topic_header_size);
        std::memcpy(datagram + types::topic_header_size, payload_data, payload_size);

        sendto(sock_,
               datagram,
               types::topic_header_size + payload_size, // only send the actual size
               0,
               reinterpret_cast<const struct sockaddr*>(&dest_addr_),
               sizeof(dest_addr_));
    }

private:
    int sock_{-1};
    struct sockaddr_in dest_addr_{};
};



#endif //UDPDISSEMINATOR_H
