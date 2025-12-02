//
// Created by paul on 01-Dec-25.
//

#ifndef DISSEMINATOR_H
#define DISSEMINATOR_H
#include <string>
#include <unordered_map>

#include <memory>
#include <thread>
#include <vector>
#include "IDisseminator.h"
#include "utils/types.h"

class Disseminator : public IDisseminator{
public:
    Disseminator(types::QueueType_Quote& quotes, types::QueueType_Trade& trades);
    ~Disseminator() override;


    void start() override;
    void stop() override;

private:

    void process() override;
    void consume_quotes(types::QueueType_Quote& q, const std::stop_token &stoken);
    void consume_trades(types::QueueType_Trade& q, const std::stop_token &stoken);

    types::QueueType_Trade& trades_;
    types::QueueType_Quote& quotes_;


    // hashmap mapping <symbol ID, quotes>
    std::unordered_map<std::string, types::QueueType_Quote> symbols_quotes_;
    std::unordered_map<std::string, types::QueueType_Trade> symbols_trades_;
    std::vector<std::jthread> workers_;


    // TODO hashset mapping <symbol ID, subscribers that want that symbol (int as placeholder)>
};



#endif //DISSEMINATOR_H
