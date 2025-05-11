//
// Created by paul on 11/05/2025.
//

#ifndef MARKETDATAGENERATOR_H
#define MARKETDATAGENERATOR_H

#include <filesystem>
#include <string>
#include <vector>

#include "IGenerator.h"


class MarketDataGenerator : public IGenerator{
public:

    MarketDataGenerator(const uint32_t messages_per_second,std::filesystem::path symbols_file)
        :message_per_sec{messages_per_second},
        symbols{read_symbols_file(symbols_file)}
    {}
    void setup(uint32_t message_per_sec) override;


private:
    const std::vector<std::string> symbols;
    uint32_t message_per_sec;
    static std::vector<std::string> read_symbols_file(std::filesystem::path const& filename);

};



#endif //MARKETDATAGENERATOR_H
