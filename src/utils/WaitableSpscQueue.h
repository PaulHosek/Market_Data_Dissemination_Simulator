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
    bool push(const T& item) {
        const bool success = queue_.push(item);
        if (success) {
            has_data_.store(true, std::memory_order_release);
            has_data_.notify_one();
        }
        return success;
    }

    bool pop(T& item) {
        if (queue_.pop(item)) {
            return true;
        }
        has_data_.store(false, std::memory_order_seq_cst);
        // check if data arrived while setting to false
        if (queue_.pop(item)) {
            has_data_.store(true, std::memory_order_relaxed);
            return true;
        }
        return false;
    }

    void wait(std::stop_token stoken = {}) {
        std::stop_callback callback(stoken, [this]() -> void{
            // fake data signal to wake listening threads and let them exit
            has_data_.store(true, std::memory_order_release);
            has_data_.notify_all();
        });

        while (!has_data_.load(std::memory_order_acquire) && !stoken.stop_requested()) {
            has_data_.wait(false, std::memory_order_acquire);
        }
    }

    bool empty() const { return queue_.empty(); }

    void notify() { has_data_.notify_one(); }
};




#endif //WAITABLESPSCQUEUE_H
