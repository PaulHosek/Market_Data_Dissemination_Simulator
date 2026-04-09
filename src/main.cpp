#include <iostream>

#include "generator/MarketDataGenerator.h"
#include "utils/types.h"
#include "chrono"


#include "generator/RandomWalkGenerator.h"
#include "disseminator/ZmqDisseminator.h"
#include "feedhandler/ZmqFeedHandler.h"

#include <thread>
#include "utils/WaitableSpscQueue.h"
#include "monitor/LatencyMonitor.h" // <-- Include the new monitor

#include <boost/lockfree/spsc_queue.hpp>
#include "utils/CustomSpscQueue.h"
#include "utils/SpinSpscQueue.h"

int main() {
    using MsgType = types::MarketDataMsg;
    constexpr size_t Cap = 8192;

    // options in queus
    using BoostStorage = boost::lockfree::spsc_queue<MsgType, boost::lockfree::capacity<Cap>>;
    WaitableSpscQueue<MsgType, BoostStorage> safe_queue;
    using MyStorage = CustomSpscQueue<MsgType, Cap>;
    SpinSpscQueue<MsgType, MyStorage> fast_queue;

    std::cout << "--- Starting Latency Benchmark ---\n";


    const std::string symbols_file = "test_symbols.txt";
    std::ofstream out(symbols_file);
    out << "AAPL\nMSFT\nGOOG\n";
    out.close();

    std::string zmq_address = "tcp://127.0.0.1:5555";

    ZmqDisseminator<decltype(fast_queue)> disseminator(fast_queue, zmq_address);
    RandomWalkGenerator<decltype(fast_queue)> generator(fast_queue);
    ZmqFeedHandler feed_handler(zmq_address);

    LatencyMonitor monitor(500'000);

    feed_handler.set_quote_callback([&monitor](const types::Quote& q) {
        monitor.on_quote(q);
    });
    feed_handler.set_trade_callback([&monitor](const types::Trade& t) {
        monitor.on_trade(t);
    });

    generator.configure(50'000, symbols_file);
    feed_handler.subscribe("AAPL");
    feed_handler.subscribe("MSFT");
    feed_handler.subscribe("GOOG");

    feed_handler.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    disseminator.start();
    generator.start();

    std::cout << "Running benchmark for 5 seconds...\n";
    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::cout << "Shutting down...\n";
    generator.stop();
    disseminator.stop();
    feed_handler.stop();

    return 0;

    /*
     Generator:
     - start creates a jthread and passes it the generation loop and the stop token of the generator object
     - stop calls the stop token and resets it after
     - generation loop
        - spins on the time, the delta needed for n iterations
        is larger than the expected it generates a trade or quote with 50/50 probability
        -
     */


    /*
     Disseminator:
     - Ctor: Link the 2 queues and create a zmq pub socket, bind the multicast address and set linger to 0
        - context is thread safe so could use inproc communication (faster) if feedhandler and disseminator share the context
    - start -> calls process
    - process: launch 2 jthreads in workers array (consume threads and consume quotes)
        - consume_quotes:
            - pop the top quote in the queue into a temporary
            - copy that element into the zmq message
            - lock the socket, send the msg -> lots of locking here Fix it
    - stop: clear the worker array
     */

    /*
     FeedHandler:
     - Ctor: creates new context and configures the multicast feedhandler socket
        - subscribe to the same address as the disseminator is sending out
    - start: launches thread with receive loop
    - receive_loop: handles both quotes and trades.
        - checks if that quote is subscribed to by ANY and then calls deliver_to_client -> weird, see doc
     */









    // using namespace std::chrono_literals;
    // // QueueType_Quote x{};
    // // auto i = x;
    // types::QueueType_Quote qq{};
    // types::QueueType_Trade tq{};
    // MarketDataGenerator mdg(qq, tq);
    //
    // mdg.configure(1000, "../data/tickers.txt");
    // mdg.start();

    // TODO dissemination engine should pop from queue and forward to the right subscribers
    // Once this works we can think about having a local order book that we use to keep track of old symbols. A feedhandler could then also get the history.

    // TODO working here right now
    // Part 1: get dissemination engine to just read from the queue
    // 1. read from queue
    // 2. check if there are subscribers requesting this symbol

    // Part 2:
    // 1. implement feedhandler and TCP handshake with dissemination engine
    // 2. implement the UDP multicast of symbols

    // Part 3:
    // 1. Check which performances we want to monitor
    // 2. implement performance monitor



    // mdg.stop();







}