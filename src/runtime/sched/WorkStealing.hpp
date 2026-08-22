#ifndef LSM_RUNTIME_WORK_STEALING_HPP
#define LSM_RUNTIME_WORK_STEALING_HPP

#include "LockFreeQueue.hpp"
#include <vector>
#include <memory>
#include <random>

class WorkStealingPool {
private:
    std::vector<std::unique_ptr<LockFreeTaskQueue>> queues;
    size_t numWorkers;

public:
    explicit WorkStealingPool(size_t workers) : numWorkers(workers) {
        for (size_t i = 0; i < numWorkers; ++i) {
            queues.push_back(std::make_unique<LockFreeTaskQueue>());
        }
    }

    LockFreeTaskQueue* getQueue(size_t workerId) {
        return queues[workerId % numWorkers].get();
    }

    
    Task* stealTask(size_t currentWorkerId) {
        if (numWorkers <= 1) return nullptr;

        
        thread_local std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<size_t> dist(0, numWorkers - 1);

        size_t target = dist(gen);
        if (target == currentWorkerId) {
            target = (target + 1) % numWorkers;
        }

        return queues[target]->steal();
    }
};

#endif 