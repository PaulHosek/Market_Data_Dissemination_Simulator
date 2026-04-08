#ifndef WAITABLESPSCQUEUE_H
#define WAITABLESPSCQUEUE_H

#include <boost/lockfree/spsc_queue.hpp>
#include <atomic>
#include <thread>
#include <stop_token>

template<typename T, size_t Capacity>
class WaitableSpscQueue {
public:
    using value_type = T;

private:
    boost::lockfree::spsc_queue<T, boost::lockfree::capacity<Capacity>> queue_;
    std::atomic<bool> is_sleeping_{false};

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
    
    [[nodiscard]] bool empty() const { return queue_.empty(); }
};

#endif //WAITABLESPSCQUEUE_H