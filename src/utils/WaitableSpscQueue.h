#ifndef WAITABLESPSCQUEUE_H
#define WAITABLESPSCQUEUE_H

#include <boost/lockfree/spsc_queue.hpp>
#include <atomic>
#include <stop_token>

#include "QueueConcepts.h"



template<typename T, typename UnderlyingQueue_T>
requires SpscQueueStorage<UnderlyingQueue_T, T>
class WaitableSpscQueue {
public:
    using value_type = T;
public:
    bool push(const T& item) {
        if (queue_.push(item)) {
            if (is_sleeping_.load(std::memory_order_relaxed)) {
                is_sleeping_.store(false, std::memory_order_release);
                is_sleeping_.notify_one();
            }
            return true;
        }
        return false;
    }

    bool pop(T& item, std::stop_token stoken) {
        std::stop_callback callback(stoken, [this]() {
            is_sleeping_.store(false, std::memory_order_release);
            is_sleeping_.notify_all();
        });

        while (!stoken.stop_requested()) {
            if (queue_.pop(item)) {
                return true;
            }
            
            is_sleeping_.store(true, std::memory_order_seq_cst);
            
            if (queue_.pop(item)) {
                is_sleeping_.store(false, std::memory_order_relaxed);
                return true; 
            }
            
            is_sleeping_.wait(true, std::memory_order_acquire);
        }
        return false;
    }
    
    [[nodiscard]] bool empty() { return queue_.empty(); }

private:
    UnderlyingQueue_T queue_;
    std::atomic<bool> is_sleeping_{false};
};

#endif //WAITABLESPSCQUEUE_H