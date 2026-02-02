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
    boost::lockfree::spsc_queue<T, boost::lockfree::capacity<Capacity>> queue_; // TODO: would like to contrast this with std::queue's performance
    std::atomic<bool> has_data_{false};

public:
    // Learning note: release here because want push to happen first.
    //  release ensures that all store operations before happen first -> like a barrier for those.
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
        // Learning note:
        // If producer wakes up in between the above check and the signaling here, we would get an invalid state.
        // Need to have strict fencing of load/store operations here.
        has_data_.store(false, std::memory_order_seq_cst);
        // check if data arrived while setting to false
        if (queue_.pop(item)) {
            // Learning note: only have single consumer and single producer, so relaxed is fine here.
                // if we had multiple producers and consumers we would need to be stricter
            has_data_.store(true, std::memory_order_relaxed);
            return true;
        }
        return false;
    }

    //
    void wait(std::stop_token stoken = {}) {
        std::stop_callback callback(stoken, [this]() -> void{
            // fake data signal to wake listening threads and let them exit
            has_data_.store(true, std::memory_order_release);
            has_data_.notify_all();
        });

        // Learning note: want aquire here because don't want to have the operations after this to happen before
        while (!has_data_.load(std::memory_order_acquire) && !stoken.stop_requested()) {
            has_data_.wait(false, std::memory_order_acquire);
        }
    }

    bool empty() const { return queue_.empty(); }

    void notify() { has_data_.notify_one(); }
};




#endif //WAITABLESPSCQUEUE_H
