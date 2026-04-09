#ifndef SPIN_SPSC_QUEUE_H
#define SPIN_SPSC_QUEUE_H

#include <stop_token>
#include "QueueConcepts.h"

template<typename T, typename UnderlyingQueue_T>
requires SpscQueueStorage<UnderlyingQueue_T, T>
class SpinSpscQueue {
public:
    using value_type = T;

    bool push(const T& item) {
        return queue_.push(item);
    }

    bool pop(T& item, std::stop_token stoken) {
        while (!stoken.stop_requested()) {
            if (queue_.pop(item)) {
                return true;
            }
        }
        return false;
    }
    
    [[nodiscard]] bool empty() const { return queue_.empty(); }

private:
    UnderlyingQueue_T queue_;
};

#endif