//
// Created by paul on 01-Dec-25.
//
#ifndef DISSEMINATOR_H
#define DISSEMINATOR_H

#include <thread>
#include <vector>
#include <mutex>
#include <string>
#include <zmq.hpp>
#include <zmq_addon.hpp>

#include "IDisseminator.h"
#include "../utils/types.h"

class Disseminator : public IDisseminator{
public:
    Disseminator(types::QueueType_Quote& quotes,
                 types::QueueType_Trade& trades,
                 std::string_view multicast_addr,
                 std::string_view tcp_addr);

    ~Disseminator() override;

    void start() override;
    void stop() override;

private:
    void process() override;
    void consume_quotes(types::QueueType_Quote& q, const std::stop_token& stoken);
    void consume_trades(types::QueueType_Trade& q, std::stop_token stoken);

    // New: Handle TCP subscription requests
    void run_control_plane(std::stop_token stoken);

    types::QueueType_Trade& trades_;
    types::QueueType_Quote& quotes_;

    std::vector<std::jthread> workers_;
    std::jthread control_thread_; // for TCP req/rep

    zmq::context_t context_;
    zmq::socket_t pub_socket_; // data plane (multicast)
    zmq::socket_t rep_socket_; // control plane (tcp)

    // ZMQ sockets are NOT thread-safe, since we have multiple worker threads writing to one PUB socket we need a lock.
    std::mutex pub_mutex_;
};

#endif // DISSEMINATOR_H