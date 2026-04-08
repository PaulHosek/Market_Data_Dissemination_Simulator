#ifndef SPIN_SPSC_QUEUE_H
#define SPIN_SPSC_QUEUE_H

#include <boost/lockfree/spsc_queue.hpp>
#include <stop_token>
#include <thread>

template<typename T, size_t Capacity>
class SpinSpscQueue {
public:
    using value_type = T;

    bool push(const T& item) {
        return queue_.push(item);
    }

    bool pop(T& item, std::stop_token stoken) {
        // Pure busy-spin loop. Lowest possible latency, but burns 100% CPU.
        while (!stoken.stop_requested()) {
            if (queue_.pop(item)) {
                return true;
            }
            // Optional: You can put a PAUSE instruction or yield here to prevent 
            // the CPU from thermally throttling, but true zero-latency skips this.
            // std::this_thread::yield(); 
        }
        return false;
    }
    
    [[nodiscard]] bool empty() const { return queue_.empty(); }

private:
    boost::lockfree::spsc_queue<T, boost::lockfree::capacity<Capacity>> queue_;
};

#endif // SPIN_SPSC_QUEUE_H