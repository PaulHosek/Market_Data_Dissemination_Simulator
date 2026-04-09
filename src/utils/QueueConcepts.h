//
// Created by paul on 09-Apr-26.
//

#ifndef QUEUECONCEPTS_H
#define QUEUECONCEPTS_H
#include <concepts>

template <typename QueueType, typename ElementType>
concept SpscQueueStorage = requires(QueueType q, const ElementType& in_item, ElementType& out_item) {
    { q.push(in_item) } -> std::same_as<bool>;
    { q.pop(out_item) } -> std::same_as<bool>;
    { q.empty() } -> std::convertible_to<bool>;
};
#endif //QUEUECONCEPTS_H
