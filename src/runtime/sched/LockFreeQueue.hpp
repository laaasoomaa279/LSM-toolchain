#ifndef LSM_RUNTIME_LOCK_FREE_QUEUE_HPP
#define LSM_RUNTIME_LOCK_FREE_QUEUE_HPP

#include "Task.hpp"
#include <atomic>
#include <vector>
#include <memory>

class LockFreeTaskQueue {
private:
    static constexpr size_t INITIAL_CAPACITY = 1024;
    std::vector<Task*> buffer;
    std::atomic<int64_t> top{0};
    std::atomic<int64_t> bottom{0};

public:
    LockFreeTaskQueue(size_t capacity = INITIAL_CAPACITY) 
        : buffer(capacity, nullptr) {}

    
    void push(Task* task) {
        int64_t b = bottom.load(std::memory_order_relaxed);
        int64_t t = top.load(std::memory_order_acquire);

        if (b - t >= static_cast<int64_t>(buffer.size())) {
            
            return;
        }

        buffer[b % buffer.size()] = task;
        std::atomic_thread_fence(std::memory_order_release);
        bottom.store(b + 1, std::memory_order_relaxed);
    }

    
    Task* pop() {
        int64_t b = bottom.load(std::memory_order_relaxed) - 1;
        bottom.store(b, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);

        int64_t t = top.load(std::memory_order_relaxed);
        Task* task = nullptr;

        if (t <= b) {
            task = buffer[b % buffer.size()];
            if (t == b) {
                if (!top.compare_exchange_strong(t, t + 1, 
                                                 std::memory_order_seq_cst, 
                                                 std::memory_order_relaxed)) {
                    task = nullptr;
                }
                bottom.store(b + 1, std::memory_order_relaxed);
            }
        } else {
            bottom.store(b + 1, std::memory_order_relaxed);
        }

        return task;
    }

    
    Task* steal() {
        int64_t t = top.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        int64_t b = bottom.load(std::memory_order_acquire);

        if (t < b) {
            Task* task = buffer[t % buffer.size()];
            if (!top.compare_exchange_strong(t, t + 1, 
                                             std::memory_order_seq_cst, 
                                             std::memory_order_relaxed)) {
                return nullptr;
            }
            return task;
        }

        return nullptr;
    }

    bool empty() const {
        int64_t b = bottom.load(std::memory_order_relaxed);
        int64_t t = top.load(std::memory_order_relaxed);
        return b <= t;
    }
};

#endif 