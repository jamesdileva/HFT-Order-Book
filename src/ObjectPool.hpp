// ObjectPool.hpp
#pragma once

#include <cstddef>
#include <new>

template<typename T, std::size_t PoolSize = 4096>
class ObjectPool {
public:
    ObjectPool() {
        for (std::size_t i = 0; i < PoolSize - 1; ++i)
            pool_[i].next = &pool_[i + 1];
        pool_[PoolSize - 1].next = nullptr;
        head_ = &pool_[0];
    }

    T* acquire() {
        if (!head_) return new T{};
        Node* node = head_;
        head_ = head_->next;
        return reinterpret_cast<T*>(node->data);
    }

    void release(T* obj) {
        obj->~T();
        Node* node = reinterpret_cast<Node*>(obj);
        node->next = head_;
        head_ = node;
    }

private:
    union Node {
        alignas(T) char data[sizeof(T)];
        Node* next;
    };
    Node   pool_[PoolSize];
    Node*  head_;
};