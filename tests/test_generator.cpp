
// Created by paul on 30/04/2025.
//
#include <fstream>
#include <gtest/gtest.h>


#include "../src/generator/MarketDataGenerator.h"
// #include "MarketDataGenerator.h"


class MarketDataGeneratorTest : public ::testing::Test {
protected:
    std::filesystem::path test_file_path = "test_tickers.txt";
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
    }

    void TearDown() override {
        delete_test_file();
    }

};
