//
// Created by paul on 11/05/2025.
//

#ifndef IGENERATOR_H
#define IGENERATOR_H

class IGenerator {
public:
    virtual ~IGenerator() {};
    virtual void configure(uint32_t messages_per_second, const std::filesystem::path &symbols_file) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
};




#endif //IGENERATOR_H
