#ifndef LSM_RUNTIME_TASK_HPP
#define LSM_RUNTIME_TASK_HPP

#include <cstdint>
#include <cstddef>

enum class TaskState {
    Ready,
    Running,
    Suspended,
    Dead
};


struct alignas(16) ExecutionContext {
    uint64_t rsp = 0;
    uint64_t rbp = 0;
    uint64_t rbx = 0;
    uint64_t r12 = 0;
    uint64_t r13 = 0;
    uint64_t r14 = 0;
    uint64_t r15 = 0;
    uint64_t rip = 0;
    alignas(16) double xmm[16] = {0}; 
};

using TaskEntryFunction = void (*)(void*);

class Task {
public:
    uint64_t id;
    TaskState state = TaskState::Ready;
    ExecutionContext context;
    
    uint8_t* stackBuffer = nullptr;
    size_t stackSize = 0;
    
    TaskEntryFunction entryFunc = nullptr;
    void* argument = nullptr;

    Task(uint64_t taskId, TaskEntryFunction func, void* arg, size_t stackSizeBytes = 64 * 1024)
        : id(taskId), entryFunc(func), argument(arg), stackSize(stackSizeBytes) {
        
        stackBuffer = new uint8_t[stackSize];
        uintptr_t stackTop = reinterpret_cast<uintptr_t>(stackBuffer + stackSize);
        stackTop = stackTop & ~0xFULL; 

        context.rsp = stackTop - 64;
        context.rbp = context.rsp;
        context.rip = reinterpret_cast<uint64_t>(entryFunc);
    }

    ~Task() {
        if (stackBuffer) {
            delete[] stackBuffer;
            stackBuffer = nullptr;
        }
    }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    Task(Task&& other) noexcept 
        : id(other.id), state(other.state), context(other.context),
          stackBuffer(other.stackBuffer), stackSize(other.stackSize),
          entryFunc(other.entryFunc), argument(other.argument) {
        other.stackBuffer = nullptr;
    }
};

#endif 