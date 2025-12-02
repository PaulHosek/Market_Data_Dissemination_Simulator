
// Created by paul on 30/04/2025.
//
#include <fstream>
#include <gtest/gtest.h>


#include "../src/generator/MarketDataGenerator.h"
// #include "MarketDataGenerator.h"
#include "../src/utils/types.h"


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
        file << "SINGLE\n";
        file.close();
    }

    void delete_test_files() {
        if (std::filesystem::exists(test_file_path)) {
            std::filesystem::remove(test_file_path);
        }
        if (std::filesystem::exists(test_file_single_path)){
            std::filesystem::remove(test_file_single_path);
        }
    }
    void SetUp() override {
        create_test_tickers_file();
        create_single_symb_file();
        generator_ = std::make_unique<MarketDataGenerator>(quote_queue_, trade_queue_);
    }

    void TearDown() override {
        delete_test_files();
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
    generator_->configure(1000, test_file_path);

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
    using namespace std::chrono_literals;
    generator_->configure(1000, test_file_single_path);
    generator_->start();
    std::this_thread::sleep_for(100ms);
    generator_->stop();

    types::Quote quote;
    types::Trade trade;
    bool has_valid_data = true;

    while (quote_queue_.pop(quote)) {
        bool valid_quote {std::string_view(quote.symbol) == "SINGLE" &&
        quote.bid_price > 0 && quote.ask_price > quote.bid_price &&
        quote.bid_size > 0 && quote.ask_size > 0 && quote.timestamp > 0};
        if (!valid_quote) {
            EXPECT_TRUE(has_valid_data);
            return;
        }
    }
    while (trade_queue_.pop(trade)) {
        bool valid_trade {std::string_view(trade.symbol) == "SINGLE" &&
            trade.price > 0 && trade.volume > 0 && trade.timestamp > 0};
        if (!valid_trade) {
            has_valid_data = false;
            EXPECT_TRUE(has_valid_data);
            return;
        }
    }

    EXPECT_TRUE(has_valid_data);
}


TEST_F(MarketDataGeneratorTest, approximate_message_rate) {
    using namespace std::chrono_literals;
    // TODO add different sizes to this test to test
    generator_->configure(1000, test_file_path);
    generator_->start();
    std::this_thread::sleep_for(1s);
    generator_->stop();
    const size_t total_messages = count_messages();
    EXPECT_GE(total_messages, 800);
    EXPECT_LE(total_messages, 1200);
}


TEST_F(MarketDataGeneratorTest, handles_queue_full) {
    using namespace std::chrono_literals;
   generator_->configure(10'000, test_file_path);
    generator_->start();
    std::this_thread::sleep_for(1s);
    generator_->stop();

    const std::size_t queue_size = count_messages();
    EXPECT_GE(queue_size , 9000);
    EXPECT_LE(queue_size ,10200);

}























