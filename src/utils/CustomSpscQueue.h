//
// Created by paul on 08-Apr-26.
//

#include <atomic>
#include <memory>

/*
since i know that there is only 1 producer and one consumer, and the read_ pointer is only modified from this thread,
    I can mark it relaxed because C++ ensures correct ordering within the same thread. Similarly for push.
 */
#ifndef MYLOCKFREEQUEUE_H
#define MYLOCKFREEQUEUE_H


template <typename T, std::size_t Capacity>
class CustomSpscQueue {
public:
    static constexpr std::size_t RealCapacity = Capacity;

    CustomSpscQueue() {
        data_ = std::allocator<T>{}.allocate(RealCapacity);
    }

    ~CustomSpscQueue() {
        std::size_t r = read_.load(std::memory_order_relaxed);
        std::size_t w = write_.load(std::memory_order_relaxed);
        while (r < w) {
            std::destroy_at(data_ + (r % RealCapacity));
            r++;
        }
        std::allocator<T>{}.deallocate(data_, RealCapacity);
    }

    bool push(const T& item) {
        const std::size_t w = write_.load(std::memory_order_relaxed);
        const std::size_t r = read_.load(std::memory_order_acquire);

        if (w - r == RealCapacity) {
            return false;
        }

        std::construct_at(data_ + (w % RealCapacity), item);
        write_.store(w + 1, std::memory_order_release);
        return true;
    }

    bool pop(T& item) {
        const std::size_t r = read_.load(std::memory_order_relaxed);
        const std::size_t w = write_.load(std::memory_order_acquire);

        if (r == w) {
            return false;
        }

        std::size_t idx = r % RealCapacity;
        item = std::move(data_[idx]);
        std::destroy_at(data_ + idx);
        read_.store(r + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool empty() const {
        return read_.load(std::memory_order_acquire) == write_.load(std::memory_order_acquire);
    }

private:
    alignas(std::hardware_constructive_interference_size) std::atomic<std::size_t> read_{0};
    alignas(std::hardware_constructive_interference_size) std::atomic<std::size_t> write_{0};
    T* data_;
};

#endif //MYLOCKFREEQUEUE_H