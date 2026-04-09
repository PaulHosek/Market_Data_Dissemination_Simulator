#ifndef LATENCY_MONITOR_H
#define LATENCY_MONITOR_H

#include <vector>
#include <chrono>
#include <string>
#include <fstream>
#include <filesystem>
#include <spdlog/spdlog.h>
#include "../utils/types.h"

static std::string folder = "../data";

struct LatencyRecord {
    uint64_t queue_ns;
    uint64_t network_ns;
    uint64_t total_ns;
};

class LatencyMonitor {
public:
    explicit LatencyMonitor(size_t preallocate_count = 1'000'000) {
        quote_latencies_.reserve(preallocate_count);
        trade_latencies_.reserve(preallocate_count);
        if (!std::filesystem::exists(folder)) {
            std::filesystem::create_directory(folder);
        }
    }

    ~LatencyMonitor() { save_to_csv(); }

    inline void on_quote(const types::Quote& quote) {
        const uint64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();

        quote_latencies_.push_back({
            quote.disseminate_timestamp - quote.enqueue_timestamp,
            now - quote.disseminate_timestamp,
            now - quote.enqueue_timestamp
        });
    }

    inline void on_trade(const types::Trade& trade) {
        const uint64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();

        trade_latencies_.push_back({
            trade.disseminate_timestamp - trade.enqueue_timestamp,
            now - trade.disseminate_timestamp,
            now - trade.enqueue_timestamp
        });
    }

    void save_to_csv() const {
        spdlog::info("Saving latency data to disk...");

        if (!quote_latencies_.empty()) {
            std::ofstream q_file(folder + "/quote_latencies.csv");
            q_file << "queue_ns,network_ns,total_ns\n";
            for (const auto& lat : quote_latencies_) {
                q_file << lat.queue_ns << "," << lat.network_ns << "," << lat.total_ns << "\n";
            }
        }

        if (!trade_latencies_.empty()) {
            std::ofstream t_file(folder + "/trade_latencies.csv");
            t_file << "queue_ns,network_ns,total_ns\n";
            for (const auto& lat : trade_latencies_) {
                t_file << lat.queue_ns << "," << lat.network_ns << "," << lat.total_ns << "\n";
            }
        }
    }

private:
    std::vector<LatencyRecord> quote_latencies_;
    std::vector<LatencyRecord> trade_latencies_;
};

#endif // LATENCY_MONITOR_H