#include <fstream>
#include <gtest/gtest.h>
#include <vector>
#include <limits>

#include "../src/generator/MarketDataGenerator.h"
#include "../src/utils/types.h"
#include "generator/RandomWalkGenerator.h"

// DUMMY QUEUE FOR TESTING
struct MockQueue {
    std::vector<types::MarketDataMsg> items;
    size_t capacity_limit = std::numeric_limits<size_t>::max();

    bool push(const types::MarketDataMsg& item) {
        if (items.size() >= capacity_limit) {
            return false; // Simulate queue full
        }
        items.push_back(item);
        return true;
    }

    size_t size() const { return items.size(); }
};

// TEST FIXTURE
class MarketDataGeneratorTest : public ::testing::Test {
protected:
    std::filesystem::path test_file_path = "test_tickers.txt";
    std::filesystem::path test_file_single_path = "test_ticker_singlesymb.txt";

    MockQueue queue_;
    std::unique_ptr<RandomWalkGenerator<MockQueue>> generator_;

    void create_test_tickers_file() {
        std::ofstream file(test_file_path);
        file << "APPL\n" << "V\n" << "AMZN\n" << "SPRSTOCK\n" << "META\n";
    }

    void create_single_symb_file() {
        std::ofstream file(test_file_single_path);
        file << "SINGLE\n" << "SINGLE\n";
    }

    void SetUp() override {
        create_test_tickers_file();
        create_single_symb_file();
        generator_ = std::make_unique<RandomWalkGenerator<MockQueue>>(queue_);
    }

    void TearDown() override {
        generator_->stop();
        if (std::filesystem::exists(test_file_path)) std::filesystem::remove(test_file_path);
        if (std::filesystem::exists(test_file_single_path)) std::filesystem::remove(test_file_single_path);
    }
};

TEST_F(MarketDataGeneratorTest, configure_enables_start) {
    generator_->configure(1000, test_file_path);
    EXPECT_NO_THROW(generator_->start());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    generator_->stop();

    EXPECT_GT(queue_.size(), 0);
}

TEST_F(MarketDataGeneratorTest, start_without_config_throws) {
    EXPECT_THROW(generator_->start(), std::logic_error);
}

TEST_F(MarketDataGeneratorTest, invalid_file_throws) {
    EXPECT_THROW(generator_->configure(1000, "nonexistent_file.txt"), std::logic_error);
}

TEST_F(MarketDataGeneratorTest, restart_continues){
    using namespace std::chrono_literals;
    generator_->configure(1000, test_file_path);

    generator_->start();
    std::this_thread::sleep_for(10ms);
    generator_->stop();

    const size_t initial_count = queue_.size();

    for (int i = 0; i < 2; i++) {
        std::this_thread::sleep_for(10ms);
        generator_->start();
        std::this_thread::sleep_for(10ms);
        generator_->stop();
    }

    EXPECT_GT(queue_.size(), initial_count);
    EXPECT_GT(initial_count, 0);
}

TEST_F(MarketDataGeneratorTest, test_valid_data) {
    using namespace std::chrono_literals;
    generator_->configure(1000, test_file_single_path);
    generator_->start();
    std::this_thread::sleep_for(100ms);
    generator_->stop();

    bool has_valid_data = true;

    for (const auto& msg : queue_.items) {
        std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, types::Quote>) {
                if (std::string_view(arg.symbol) != "SINGLE" || arg.bid_price <= 0 ||
                    arg.ask_price <= arg.bid_price || arg.bid_size <= 0 || arg.ask_size <= 0 ||
                    arg.enqueue_timestamp <= 0) {
                    has_valid_data = false;
                }
            } else if constexpr (std::is_same_v<T, types::Trade>) {
                if (std::string_view(arg.symbol) != "SINGLE" || arg.price <= 0 ||
                    arg.size <= 0 || arg.enqueue_timestamp <= 0) {
                    has_valid_data = false;
                }
            }
        }, msg);

        if (!has_valid_data) break;
    }

    EXPECT_TRUE(has_valid_data);
}

TEST_F(MarketDataGeneratorTest, approximate_message_rate) {
    using namespace std::chrono_literals;
    generator_->configure(1000, test_file_path);
    generator_->start();
    std::this_thread::sleep_for(1s);
    generator_->stop();

    EXPECT_GE(queue_.size(), 800);
    EXPECT_LE(queue_.size(), 1200);
}

TEST_F(MarketDataGeneratorTest, handles_queue_full) {
    using namespace std::chrono_literals;

    queue_.capacity_limit = 4000;

    generator_->configure(20'000, test_file_path);
    generator_->start();
    std::this_thread::sleep_for(1s);
    generator_->stop();

    // verify it yielded correctly and didn't crash when it hit the 4k restriction
    EXPECT_EQ(queue_.size(), 4000);
}