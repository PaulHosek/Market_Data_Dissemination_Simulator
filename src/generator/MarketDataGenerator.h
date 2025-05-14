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
#include <string_view>

//TODO moved the queues to the types, maybe should make this namespace?

// Generator pushes values onto two queues for trades and quotes
class MarketDataGenerator : public IGenerator{
public:


    MarketDataGenerator(QueueType_Quote quote_queue, QueueType_Trade trade_queue);
    void configure(uint32_t messages_per_second, const std::filesystem::path &symbols_file, const uint32_t seed) override;

    //TODO:
    //1. Test if configuration happened (correctly)
    //2. set running to true
    //3. push generation loop to the generator thread
    // void start() override;

    // TODO:
    // set running to false
    // join the generator thread if joinable
    // TODO maybe want to think about coroutines here for the stop and go & since they should be more lightweight than threads
    // void stop() override;


private:
    std::vector<std::string> symbols_;
    uint32_t messages_per_sec_;
    std::mt19937 rng_; // TODO maybe we can make this std::variant for mt19937 or uint for the set
    std::chrono::nanoseconds interval_;
    QueueType_Quote& quote_queue_;
    QueueType_Trade& trade_queue_;
    uint32_t seed_; // TODO add to the logic, not used for now
    std::atomic<bool> running_;
    std::vector<double> current_prices_;
    static std::vector<std::string> read_symbols_file(std::filesystem::path const& filename);

    //TODO:
    //1. initialise distribution (for choice quote/trade and symbol) & clock
    //2. while running
    // 3. get time, calculate elapsed time
    // if waited for long enough (this sets the frequency) -> maybe there is a better way than sth like std::this_thread::sleep_for
    //-> call generate trade/quote and push onto queue
    void generation_loop_();

    // TODO:
    // generate small price change & random size of quote
    // random walk the price with the price change + some fixed volatility -> can make this more complex later
    // (modularity makes it easy to replace this method) -> could make it an interface if I am interested in different generation methods
    // create a new quote struct and fill it with the generated information & current time
    Quote generate_quote(std::string const& symbol);

    // TODO:
    // same thing as the trade but now a quote, generation step very similar(different distribution for volumne vs size
    // think about what we may want to set as parameters later or maybe inherit from some configuration object
    Trade generate_trade(std::string const& symbol);

};



#endif //MARKETDATAGENERATOR_H
