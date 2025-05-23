
// Created by paul on 30/04/2025.
//
#include <fstream>
#include <gtest/gtest.h>


#include "../src/generator/MarketDataGenerator.h"
// #include "MarketDataGenerator.h"
#include "../src/generator/types.h"


class MarketDataGeneratorTest : public ::testing::Test {
protected:
    std::filesystem::path test_file_path = "test_tickers.txt";
    types::QueueType_Quote quote_queue_{};
    types::QueueType_Trade trade_queue_{};
    // std::unique_ptr<MarketDataGenerator> generator_;
    std::unique_ptr<MarketDataGenerator> generator_;
    // std::unique_ptr<MarketDataGenerator> generator_{quote_queue_, trade_queue_};

    void create_test_tickers_file() {
        std::ofstream file(test_file_path);
        file << "APPL\n";
        file << "V\n";
        file << "AMZN\n";
        file << "SPRSTOCK\n";
        file << "META\n";
        file.close();
    }

    void delete_test_file() {
        if (std::filesystem::exists(test_file_path)) {
            std::filesystem::remove(test_file_path);
        }
    }
    void SetUp() override {
        create_test_tickers_file();
        generator_ = std::make_unique<MarketDataGenerator>(quote_queue_, trade_queue_);
    }

    void TearDown() override {
        delete_test_file();
        generator_->stop();
    }

    size_t count_messages() {
        size_t count{0};
        types::Quote quote;
        types::Trade trade;
        while (quote_queue_.pop(quote)) count++;
        while (trade_queue_.pop(trade)) count++;
        return count;
    }

};

TEST_F(MarketDataGeneratorTest, ConfigureEnablesStart) {
    generator_->configure(1000, test_file_path);
    EXPECT_NO_THROW(generator_->start());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    generator_->stop();
    auto count = count_messages();
    std::cout << "Note: Messages generated in 50ms " << count << std::endl;
    EXPECT_GT(count, 0);
}