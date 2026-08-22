#include "JITEngine.hpp"
#include <cstring>
#include <stdexcept>
#include <iostream>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
#endif

size_t JITEngine::alignToPage(size_t size) const {
    size_t pageSize = 4096;
#if !defined(_WIN32) && !defined(_WIN64)
    long sysPage = sysconf(_SC_PAGESIZE);
    if (sysPage > 0) pageSize = static_cast<size_t>(sysPage);
#endif
    return (size + pageSize - 1) & ~(pageSize - 1);
}

void* JITEngine::allocateRWXMemory(size_t size) {
#if defined(_WIN32) || defined(_WIN64)
    void* mem = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!mem) throw std::runtime_error("JIT: VirtualAlloc failed to allocate memory.");
    return mem;
#else
    void* mem = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) throw std::runtime_error("JIT: mmap failed to allocate memory.");
    return mem;
#endif
}

void JITEngine::makeMemoryExecutable(void* ptr, size_t size) {
#if defined(_WIN32) || defined(_WIN64)
    DWORD oldProtect;
    if (!VirtualProtect(ptr, size, PAGE_EXECUTE_READ, &oldProtect)) {
        throw std::runtime_error("JIT: VirtualProtect failed to set PAGE_EXECUTE_READ.");
    }
#else
    if (mprotect(ptr, size, PROT_READ | PROT_EXEC) != 0) {
        throw std::runtime_error("JIT: mprotect failed to set PROT_READ | PROT_EXEC.");
    }
#endif
}

void JITEngine::freeExecutableMemory(void* ptr, size_t size) {
    if (!ptr) return;
#if defined(_WIN32) || defined(_WIN64)
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    munmap(ptr, size);
#endif
}

int64_t JITEngine::execute(const std::vector<uint8_t>& machineCode) {
    if (machineCode.empty()) return 0;

    size_t codeSize = machineCode.size();
    size_t allocSize = alignToPage(codeSize);

    
    void* execMemory = allocateRWXMemory(allocSize);

    
    std::memcpy(execMemory, machineCode.data(), codeSize);

    
    try {
        makeMemoryExecutable(execMemory, allocSize);
    } catch (...) {
        freeExecutableMemory(execMemory, allocSize);
        throw;
    }

    
    using JITFunction = int64_t (*)();
    auto func = reinterpret_cast<JITFunction>(execMemory);

    int64_t result = 0;
    try {
        result = func();
    } catch (const std::exception& e) {
        freeExecutableMemory(execMemory, allocSize);
        throw std::runtime_error(std::string("JIT Execution Error: ") + e.what());
    } catch (...) {
        freeExecutableMemory(execMemory, allocSize);
        throw std::runtime_error("JIT: Native CPU execution crashed (Segmentation Fault/Bus Error).");
    }

    
    freeExecutableMemory(execMemory, allocSize);
    return result;
}