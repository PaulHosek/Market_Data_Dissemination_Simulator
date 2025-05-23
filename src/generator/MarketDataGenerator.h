//
// Created by paul on 11/05/2025.
//

#ifndef MARKETDATAGENERATOR_H
#define MARKETDATAGENERATOR_H

#include <atomic>
#include <filesystem>
#include <random>
#include <string>
#include <vector>
#include <boost/lockfree/spsc_queue.hpp>
#include "IGenerator.h"
#include "types.h"
#include <thread>
//TODO moved the queues to the types, maybe should make this namespace?

// Generator pushes values onto two queues for trades and quotes
class MarketDataGenerator : public IGenerator{
public:


    MarketDataGenerator(types::QueueType_Quote& quote_queue, types::QueueType_Trade& trade_queue);
    void configure(uint32_t messages_per_second, const std::filesystem::path &symbols_file) override;

    //TODO:
    //1. Test if configuration happened (correctly)
    //2. set running to true
    //3. push generation loop to the generator thread
    void start() override;

    // TODO:
    // set running to false
    // join the generator thread if joinable
    // TODO maybe want to think about coroutines here for the stop and go & since they should be more lightweight than threads
    void stop() override;


private:

    // TODO using a vector makes search inefficient, but let's see if it makes a difference when we are done
    // since its strings, they should be on the heap anyways so we just need a pointer-stable structure here
    std::vector<std::string> symbols_;
    uint32_t messages_per_sec_;
    std::mt19937 rng_; // TODO maybe we can make this std::variant for mt19937 or uint for the set + threadlocal?
    std::chrono::nanoseconds interval_;
    types::QueueType_Quote& quote_queue_;
    types::QueueType_Trade& trade_queue_;
    std::jthread generating_thread_;
    std::vector<double> current_prices_;
    std::stop_source stop_source_;
    static std::vector<std::string> read_symbols_file(std::filesystem::path const& filename);


    //TODO:
    //1. initialise distribution (for choice quote/trade and symbol) & clock
    //2. while running
    // 3. get time, calculate elapsed time
    // if waited for long enough (this sets the frequency) -> maybe there is a better way than sth like std::this_thread::sleep_for
    //-> call generate trade/quote and push onto queue
    void generation_loop(const std::stop_token &stop_tok);

    // TODO:
    // generate small price change & random size of quote
    // random walk the price with the price change + some fixed volatility -> can make this more complex later
    // (modularity makes it easy to replace this method) -> could make it an interface if I am interested in different generation methods
    // create a new quote struct and fill it with the generated information & current time
    types::Quote generate_quote(std::string const& symbol);

    // TODO:
    // same thing as the trade but now a quote, generation step very similar(different distribution for volumne vs size
    // think about what we may want to set as parameters later or maybe inherit from some configuration object
    types::Trade generate_trade(std::string const& symbol);

    // TODO: Not sure if I should mark private methods somehow (e.g., __function)
};



#endif //MARKETDATAGENERATOR_H
