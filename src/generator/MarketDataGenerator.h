//
// Created by paul on 11/05/2025.
//

#ifndef MARKETDATAGENERATOR_H
#define MARKETDATAGENERATOR_H
#include <filesystem>

#include "IGenerator.h"
#include <vector>
#include <string>


class MarketDataGenerator : public IGenerator{
public:

    MarketDataGenerator(messages_per_second, symbols_file, ticker_fname)
        : symbols{read_symbols_file(ticker_fname)},
        message_per_sec{messages_per_second},
        symbols_filepath{symbols_file} {};
    void setup(uint32_t message_per_sec) override;


private:
    const std::vector<const std::string> symbols;
    uint32_t message_per_sec;
    std::filesystem::path symbols_filepath;
    static std::vector<std::string> read_symbols_file(std::string const& filename);

};



#endif //MARKETDATAGENERATOR_H
