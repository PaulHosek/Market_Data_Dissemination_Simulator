#ifndef LATENCY_MONITOR_H
#define LATENCY_MONITOR_H


#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <spdlog/spdlog.h>
#include "../utils/types.h"


struct LatencyRecord {
    uint64_t queue_ns;
    uint64_t network_ns;
    uint64_t total_ns;
};

class LatencyMonitor {
public:
    explicit LatencyMonitor(size_t preallocate_count, const std::string& out_dir)
        : out_dir_(out_dir) {
        quote_latencies_.reserve(preallocate_count);
        trade_latencies_.reserve(preallocate_count);

        if (!std::filesystem::exists(out_dir_)) {
            std::filesystem::create_directories(out_dir_);
        }
    }

    ~LatencyMonitor() { save_to_csv(); }

    inline void on_quote(const types::Quote& quote, uint64_t receive_timestamp) {
        quote_latencies_.push_back({
            quote.disseminate_timestamp - quote.enqueue_timestamp,
            receive_timestamp - quote.disseminate_timestamp,
            receive_timestamp - quote.enqueue_timestamp
        });
    }

    inline void on_trade(const types::Trade& trade, uint64_t receive_timestamp) {
        trade_latencies_.push_back({
            trade.disseminate_timestamp - trade.enqueue_timestamp,
            receive_timestamp - trade.disseminate_timestamp,
            receive_timestamp - trade.enqueue_timestamp
        });
    }

    void save_to_csv() const {
        spdlog::info("Saving latency data to disk...");

        std::ofstream q_file(out_dir_ + "/quote_latencies.csv");
        q_file << "queue_ns,network_ns,total_ns\n";
        for (const auto& lat : quote_latencies_) {
            q_file << lat.queue_ns << "," << lat.network_ns << "," << lat.total_ns << "\n";
        }

        std::ofstream t_file(out_dir_ + "/trade_latencies.csv");
        t_file << "queue_ns,network_ns,total_ns\n";
        for (const auto& lat : trade_latencies_) {
            t_file << lat.queue_ns << "," << lat.network_ns << "," << lat.total_ns << "\n";
        }
    }

private:
    std::string out_dir_;
    std::vector<LatencyRecord> quote_latencies_;
    std::vector<LatencyRecord> trade_latencies_;
};

#endif // LATENCY_MONITOR_H