//
// Created by paul on 02-Dec-25.
//

#ifndef WAITABLESPSCQUEUE_H
#define WAITABLESPSCQUEUE_H

#include <boost/lockfree/spsc_queue.hpp>
#include <atomic>
#include <thread>

template<typename T, size_t Capacity>
class WaitableSpscQueue {
    boost::lockfree::spsc_queue<T, boost::lockfree::capacity<Capacity>> queue_;
    std::atomic<bool> has_data_{false};

public:
    using value_type = T;

    bool push(const T& item) {
        const bool success = queue_.push(item);
        if (success) has_data_.store(true, std::memory_order_release);
        return success;
    }

    bool push(T&& item) {
        const bool success = queue_.push(std::move(item));
        if (success) has_data_.store(true, std::memory_order_release);
        return success;
    }

    bool pop(T& item) {
        const bool success = queue_.pop(item);
        if (!success) {
            has_data_.store(false, std::memory_order_release);
        }
        return success;
    }

    // Blocking wait — zero CPU
    void wait() const {
        while (!has_data_.load(std::memory_order_acquire)) {
            has_data_.wait(false, std::memory_order_acquire);
        }
    }

    // Non-blocking check
    bool empty() const { return queue_.empty(); }

    void notify() { has_data_.notify_one(); }
};




#endif //WAITABLESPSCQUEUE_H
