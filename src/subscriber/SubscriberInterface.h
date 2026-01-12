//
// Created by paul on 12-Jan-26.
//

//
// Created by paul on 12-Jan-26.
//
#ifndef SUBSCRIBERINTERFACE_H
#define SUBSCRIBERINTERFACE_H

#include <string_view>
#include <unordered_set>
#include <vector>
#include <thread>
#include <stop_token>
#include <zmq.hpp>
#include "utils/types.h"

class SubscriberInterface {
public:
    SubscriberInterface(std::string_view multicast_address, std::string_view tcp_address);
    // overload for testing with mock sockets
    SubscriberInterface(zmq::socket_t& multicast_sub, zmq::socket_t& tcp_req);
    ~SubscriberInterface();

    void start();
    void stop();
    void subscribe(std::string_view symbol);
    void unsubscribe(std::string_view symbol);

private:
    zmq::context_t context_;
    zmq::socket_t multicast_sub_;
    zmq::socket_t tcp_req_;
    std::unordered_set<std::string> subscriptions_;
    std::jthread receiver_thread_;
    std::stop_source stop_source_;

    void receive_loop(std::stop_token st);
    virtual void deliver_to_client(const types::Quote& quote); // only virtual for mocking in the tests later
    virtual void deliver_to_client(const types::Trade& trade);
};

#endif // SUBSCRIBERINTERFACE_H