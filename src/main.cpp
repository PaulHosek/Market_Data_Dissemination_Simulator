#include <iostream>

#include "generator/MarketDataGenerator.h"
#include "utils/types.h"
#include "chrono"



#include "utils/types.h"
#include "generator/MarketDataGenerator.h"
#include "disseminator/ZmqDisseminator.h"
#include "feedhandler/FeedHandler.h"

int main() {
    // using namespace std::chrono_literals;
    //
    // types::QueueType_Quote qq;
    // types::QueueType_Trade tq;
    //
    // MarketDataGenerator generator(qq, tq);
    // generator.configure(1000, "../data/tickers.txt");
    // generator.start();
    //
    // std::this_thread::sleep_for(10s);
    // generator.stop();



    // 2. Instantiate the unified lock-free queue
    types::MarketDataQueue queue;

    // Use TCP for local testing to avoid network interface multicast routing issues
    std::string zmq_address = "tcp://127.0.0.1:5555";

    // 3. Instantiate the architecture components
    ZmqDisseminator<types::MarketDataQueue> disseminator(queue, zmq_address);
    MarketDataGenerator generator(queue);
    ZmqFeedHandler subscriber(zmq_address);

    // 4. Configure the system
    generator.configure(1500, "../data/tickers.txt");


    // Test the ZMQ multi-part filtering!
    // We only subscribe to AAPL and TSLA. We should NOT see GOOG or MSFT in the logs.
    subscriber.subscribe("AAPL");
    subscriber.subscribe("TSLA");

    // 5. Start the engines
    // Best practice: start feedhandler first, then disseminator, then generator
    subscriber.start();

    // Give the feedhandler a few milliseconds to establish the TCP connection
    // before the disseminator starts blasting data
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    disseminator.start();
    generator.start();

    std::cout << "System running. Generating data for 5 seconds...\n";

    // Let the system run for 5 seconds to watch the logs
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // 6. Graceful shutdown
    std::cout << "\n--- Initiating Shutdown ---\n";

    // Stop upstream to downstream
    generator.stop();
    disseminator.stop();
    subscriber.stop();



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