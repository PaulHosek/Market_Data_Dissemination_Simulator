
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
    std::filesystem::path test_file_single_path = "test_ticker_singlesymb.txt";
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
        file.close(); // FIXME do I need to explicitly close this with ofstream?
    }
    void create_single_symb_file() {
        std::ofstream file(test_file_single_path);
        file << "SINGLE\n";
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

TEST_F(MarketDataGeneratorTest, configure_enables_start) {
    generator_->configure(1000, test_file_path);
    EXPECT_NO_THROW(generator_->start());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    generator_->stop();
    auto count = count_messages();
    std::cout << "Note: Messages generated in 50ms: " << count << std::endl;
    EXPECT_GT(count, 0);
}


TEST_F(MarketDataGeneratorTest, start_without_config_throws) {
    EXPECT_THROW(generator_->start(),std::logic_error);
}


TEST_F(MarketDataGeneratorTest, invalid_file_throws) {
    EXPECT_NO_THROW(generator_->configure(1000, "nonexistent_file.txt"));
    EXPECT_THROW(generator_->start(), std::logic_error);
}

/*
 * Starting after a stop accumulates.
 * i.e., if I call start, then stop multiple times the generator just continues where it left off
 */
TEST_F(MarketDataGeneratorTest, restart_continues){
    using namespace std::chrono_literals;
    generator_->configure(100, test_file_path);

    generator_->start();
    std::this_thread::sleep_for(10ms);
    generator_->stop();
    const size_t initial_count = count_messages();

    for (int i =0;i<2;i++) {
        std::this_thread::sleep_for(10ms);
        generator_->start();
        std::this_thread::sleep_for(10ms);
        generator_->stop();

    }
    const size_t final_count = count_messages();

    EXPECT_GT(final_count, initial_count);
    EXPECT_GT(initial_count, 0);
    EXPECT_GT(final_count, 0);
}



TEST_F(MarketDataGeneratorTest, test_valid_data) {
    generator_->configure(1000, test_file_path);
    generator_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    generator_->stop();

    types::Quote quote;
    types::Trade trade;
    bool has_valid_data = false;
    while (quote_queue_.pop(quote)) {
        if (std::string_view(quote.symbol).find("SINGLE") != std::string_view::npos &&
            quote.bid_price > 0 && quote.ask_price > quote.bid_price &&
            quote.bid_size > 0 && quote.ask_size > 0 && quote.timestamp > 0) {
            has_valid_data = true;
        }
    }
    while (trade_queue_.pop(trade)) {
        if (std::string_view(trade.symbol).find("SINGLE") != std::string_view::npos &&
            trade.price > 0 && trade.volume > 0 && trade.timestamp > 0) {
            has_valid_data = true;
        }
    }
    EXPECT_TRUE(has_valid_data);
}












