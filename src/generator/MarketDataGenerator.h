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

    MarketDataGenerator(uint32_t messages_per_second, std::filesystem::path symbols_file)
        : symbols{read_symbols_file(ticker_fname)},
        message_per_sec{messages_per_second}{};
    void setup(uint32_t message_per_sec) override;


private:
    const std::vector<const std::string> symbols;
    uint32_t message_per_sec;
    static std::vector<std::string> read_symbols_file(std::filesystem::path const& filename);

};



#endif //MARKETDATAGENERATOR_H
