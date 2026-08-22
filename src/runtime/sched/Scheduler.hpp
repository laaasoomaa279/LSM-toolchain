#ifndef LSM_RUNTIME_SCHEDULER_HPP
#define LSM_RUNTIME_SCHEDULER_HPP

#include "Task.hpp"
#include "WorkStealing.hpp"
#include <thread>
#include <atomic>
#include <vector>

class Scheduler {
private:
    size_t numThreads;
    WorkStealingPool pool;
    std::vector<std::thread> workers;
    std::atomic<bool> running{false};
    std::atomic<uint64_t> nextTaskId{1};

    static void switchContext(ExecutionContext* from, ExecutionContext* to);
    void workerLoop(size_t workerId);

public:
    explicit Scheduler(size_t threads = std::thread::hardware_concurrency())
        : numThreads(threads > 0 ? threads : 1), pool(numThreads) {}

    ~Scheduler() { stop(); }

    uint64_t spawn(TaskEntryFunction fn, void* arg = nullptr, size_t stackSize = 64 * 1024) {
        auto task = new Task(nextTaskId++, fn, arg, stackSize);
        size_t targetWorker = task->id % numThreads;
        pool.getQueue(targetWorker)->push(task);
        return task->id;
    }

    void start();
    void stop();
    void yield(size_t workerId);
};

extern thread_local Scheduler* g_CurrentScheduler;

#endif 