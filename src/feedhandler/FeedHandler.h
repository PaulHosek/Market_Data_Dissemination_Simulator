#ifndef SUBSCRIBERINTERFACE_H
#define SUBSCRIBERINTERFACE_H

#include <string_view>
#include <string>
#include <thread>
#include <stop_token>
#include <zmq.hpp>
#include "utils/types.h"

class FeedHandler {
public:
    explicit FeedHandler(std::string_view multicast_address);
    explicit FeedHandler(zmq::socket_t &&multicast_sub);

    ~FeedHandler();

    void start();
    void stop();

    void subscribe(std::string_view symbol);
    void unsubscribe(std::string_view symbol);

private:
    void receive_loop(std::stop_token st);
    void deliver_to_client(const types::Quote& quote);
    void deliver_to_client(const types::Trade& trade);

private:
    zmq::context_t context_;
    zmq::socket_t multicast_sub_;

    std::jthread receiver_thread_;


};

#endif // SUBSCRIBERINTERFACE_H