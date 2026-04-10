#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <cstdint>

enum class QueueWaitStrategy {
    Spin,
    Waitable
};

enum class TransportProtocol {
    UdpMulticast,
    Zmq
};

struct BenchmarkConfig {
    QueueWaitStrategy queue_strategy = QueueWaitStrategy::Spin;
    TransportProtocol transport = TransportProtocol::UdpMulticast;
    
    std::size_t queue_size = 1024;
    uint32_t message_rate = 10000;
    uint32_t duration_sec = 10;
    
    std::string ip_address = "239.192.1.1";
    unsigned short port = 5555;

    std::string symbols_file = "tickers.txt";
    std::string out_dir = "../data";
};

#endif // CONFIG_H