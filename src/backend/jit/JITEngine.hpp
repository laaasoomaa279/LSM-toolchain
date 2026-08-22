#ifndef LSM_JIT_ENGINE_HPP
#define LSM_JIT_ENGINE_HPP

#include <vector>
#include <cstdint>
#include <cstddef>
#include <string>

class JITEngine {
private:
    void* allocateRWXMemory(size_t size);
    void makeMemoryExecutable(void* ptr, size_t size);
    void freeExecutableMemory(void* ptr, size_t size);
    size_t alignToPage(size_t size) const;

public:
    JITEngine() = default;
    ~JITEngine() = default;

    
    JITEngine(const JITEngine&) = delete;
    JITEngine& operator=(const JITEngine&) = delete;

    
    int64_t execute(const std::vector<uint8_t>& machineCode);
};

#endif 