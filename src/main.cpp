#include <iostream>
#include <stdexcept>
#include <cxxopts.hpp>
#include <spdlog/spdlog.h>

#include "./utils/config.h"
#include "./utils/CustomSpscQueue.h"
#include "./utils/SpinSpscQueue.h"
#include "./utils/WaitableSpscQueue.h"
#include "./disseminator/UdpDisseminator.h"
#include "./disseminator/ZmqDisseminator.h"
#include "./generator/RandomWalkGenerator.h"
#include "./monitor/LatencyMonitor.h"
#include "./feedhandler/UdpFeedhandler.h"
#include "./feedhandler/ZmqFeedHandler.h"

template <typename MarketDataQueue, typename DisseminatorType, typename FeedHandlerType>
void run_benchmark_pipeline(const BenchmarkConfig& config,
                            MarketDataQueue& queue,
                            DisseminatorType& disseminator,
                            FeedHandlerType& feedhandler) {

    spdlog::info("Starting benchmark: Transport={}, QueueStrategy={}, Size={}, Rate={}, Duration={}s",
                 (config.transport == TransportProtocol::UdpMulticast ? "UDP" : "ZMQ"),
                 (config.queue_strategy == QueueWaitStrategy::Spin ? "Spin" : "Waitable"),
                 config.queue_size, config.message_rate, config.duration_sec);

    LatencyMonitor monitor(config.message_rate * config.duration_sec, config.out_dir);

    feedhandler.set_quote_callback([&monitor](const types::Quote& q, uint64_t recv_ts) {
        monitor.on_quote(q, recv_ts);
    });
    feedhandler.set_trade_callback([&monitor](const types::Trade& t, uint64_t recv_ts) {
        monitor.on_trade(t, recv_ts);
    });

    RandomWalkGenerator<MarketDataQueue> generator(queue);
    generator.configure(config.message_rate, config.symbols_file);

    // start everything in reverse order (Consumer -> Publisher -> Generator)
    feedhandler.start();


    // feedhandler.subscribe("AAPL");
    // feedhandler.subscribe("MSFT");

    std::ifstream sym_file(config.symbols_file);
    std::string sym;
    int sub_count = 0;
    while (std::getline(sym_file, sym)) {
        sym.erase(std::remove(sym.begin(), sym.end(), '\r'), sym.end());
        sym.erase(std::remove(sym.begin(), sym.end(), '\n'), sym.end());
        sym.erase(std::remove_if(sym.begin(), sym.end(), ::isspace), sym.end());

        if (!sym.empty() && sym.length() < 9) {
            feedhandler.subscribe(sym);
            sub_count++;
        }
    }
    spdlog::info("Feedhandler subscribed to {} symbols.", sub_count);

    disseminator.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));  // for the handshake if zmq tcp
    generator.start();

    std::this_thread::sleep_for(std::chrono::seconds(config.duration_sec));

    spdlog::info("Benchmark duration met. Stopping generator...");
    generator.stop();

    spdlog::info("Draining queues and network buffers (1 second)...");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    disseminator.stop();
    feedhandler.stop();

    spdlog::info("Benchmark completed.");
}

template <std::size_t Size>
void dispatch_types(const BenchmarkConfig& config) {
    using BaseQueue = CustomSpscQueue<types::MarketDataMsg, Size>;

    if (config.queue_strategy == QueueWaitStrategy::Spin) {
        using QueueType = SpinSpscQueue<types::MarketDataMsg, BaseQueue>;
        QueueType queue;

        if (config.transport == TransportProtocol::UdpMulticast) {
            UdpDisseminator<QueueType> disseminator(queue, config.ip_address, config.port);
            UdpFeedHandler feedhandler(config.ip_address, config.port);
            run_benchmark_pipeline(config, queue, disseminator, feedhandler);
        } else {
            // std::string zmq_bind = "ecmp://127.0.0.1:" + config.ip_address + ":" + std::to_string(config.port);
            std::string zmq_bind = "tcp://127.0.0.1:" + std::to_string(config.port);
            ZmqDisseminator<QueueType> disseminator(queue, zmq_bind);
            ZmqFeedHandler feedhandler(zmq_bind);
            run_benchmark_pipeline(config, queue, disseminator, feedhandler);
        }
    } else {
        using QueueType = WaitableSpscQueue<types::MarketDataMsg, BaseQueue>;
        QueueType queue;

        if (config.transport == TransportProtocol::UdpMulticast) {
            UdpDisseminator<QueueType> disseminator(queue, config.ip_address, config.port);
            UdpFeedHandler feedhandler(config.ip_address, config.port);
            run_benchmark_pipeline(config, queue, disseminator, feedhandler);
        } else {
            // std::string zmq_bind = "tcp://127.0.0.1:" + config.ip_address + ":" + std::to_string(config.port);
            std::string zmq_bind = "tcp://127.0.0.1:" + std::to_string(config.port);
            ZmqDisseminator<QueueType> disseminator(queue, zmq_bind);
            ZmqFeedHandler feedhandler(zmq_bind);
            run_benchmark_pipeline(config, queue, disseminator, feedhandler);
        }
    }
}

void dispatch_size(const BenchmarkConfig& config) {
    switch (config.queue_size) {
        case 128:     dispatch_types<128>(config); break;
        case 512:     dispatch_types<512>(config); break;
        case 1024:    dispatch_types<1024>(config); break;
        case 4096:    dispatch_types<4096>(config); break;
        case 16384:   dispatch_types<16384>(config); break;
        case 65536:   dispatch_types<65536>(config); break;
        default:
            throw std::invalid_argument("Unsupported queue size. Allowed: 128, 512, 1024, 4096, 16384, 65536");
    }
}

int main(int argc, char** argv) {
    cxxopts::Options options("MarketBench", "Low latency market data disseminator benchmark");

    options.add_options()
        ("q,queue", "Queue type (spin/waitable)", cxxopts::value<std::string>()->default_value("spin"))
        ("s,size", "Queue size (128, 512, 1024, 4096, 16384, 65536)", cxxopts::value<std::size_t>()->default_value("1024"))
        ("t,transport", "Transport (udp/zmq)", cxxopts::value<std::string>()->default_value("udp"))
        ("r,rate", "Message rate (msgs/sec)", cxxopts::value<uint32_t>()->default_value("10000"))
        ("d,duration", "Benchmark duration in seconds", cxxopts::value<uint32_t>()->default_value("10"))
        ("h,help", "Print usage")
        ("f,symbols", "Path to symbols.txt", cxxopts::value<std::string>()->default_value("../data/symbols.txt"))
        ("o,out", "Output directory for CSVs", cxxopts::value<std::string>()->default_value("../data"));

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    BenchmarkConfig config;
    config.queue_size = result["size"].as<std::size_t>();
    config.message_rate = result["rate"].as<uint32_t>();
    config.duration_sec = result["duration"].as<uint32_t>();
    config.symbols_file = result["symbols"].as<std::string>();
    config.out_dir = result["out"].as<std::string>();

    // queue
    std::string q_type = result["queue"].as<std::string>();
    if (q_type == "spin") config.queue_strategy = QueueWaitStrategy::Spin;
    else if (q_type == "waitable") config.queue_strategy = QueueWaitStrategy::Waitable;
    else throw std::invalid_argument("Invalid queue type. Use 'spin' or 'waitable'.");

    // transport
    std::string t_type = result["transport"].as<std::string>();
    if (t_type == "udp") {
        config.transport = TransportProtocol::UdpMulticast;
    }
    else if (t_type == "zmq") config.transport = TransportProtocol::Zmq;
    else throw std::invalid_argument("Invalid transport type. Use 'udp' or 'zmq'.");

    try {
        dispatch_size(config);
    } catch (const std::exception& e) {
        spdlog::error("Benchmark failed: {}", e.what());
        return 1;
    }

    return 0;
}


