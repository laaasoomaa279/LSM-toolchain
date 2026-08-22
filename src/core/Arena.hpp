#ifndef LSM_CORE_ARENA_HPP
#define LSM_CORE_ARENA_HPP

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>
#include <utility>

constexpr size_t LSM_CACHE_LINE = 64;

class Arena {
protected:
    uint8_t* rawBuffer = nullptr;
    uint8_t* buffer = nullptr;
    size_t capacity = 0;
    size_t offset = 0;
    bool isExternalBuffer = false;

public:
    explicit Arena(size_t sizeBytes = 1024 * 1024) 
        : capacity(sizeBytes), offset(0), isExternalBuffer(false) {
        size_t totalAlloc = capacity + LSM_CACHE_LINE;
        rawBuffer = static_cast<uint8_t*>(std::malloc(totalAlloc));
        if (rawBuffer) {
            uintptr_t rawAddr = reinterpret_cast<uintptr_t>(rawBuffer);
            uintptr_t alignedAddr = (rawAddr + (LSM_CACHE_LINE - 1)) & ~(LSM_CACHE_LINE - 1);
            buffer = reinterpret_cast<uint8_t*>(alignedAddr);
        }
    }

    
    Arena(void* memoryPointer, size_t sizeBytes)
        : rawBuffer(nullptr), buffer(static_cast<uint8_t*>(memoryPointer)),
          capacity(sizeBytes), offset(0), isExternalBuffer(true) {}

    virtual ~Arena() {
        if (!isExternalBuffer && rawBuffer) {
            std::free(rawBuffer);
            rawBuffer = nullptr;
            buffer = nullptr;
        }
    }

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    Arena(Arena&& other) noexcept
        : rawBuffer(other.rawBuffer), buffer(other.buffer),
          capacity(other.capacity), offset(other.offset),
          isExternalBuffer(other.isExternalBuffer) {
        other.rawBuffer = nullptr;
        other.buffer = nullptr;
        other.capacity = 0;
        other.offset = 0;
    }

    Arena& operator=(Arena&& other) noexcept {
        if (this != &other) {
            if (!isExternalBuffer && rawBuffer) std::free(rawBuffer);
            rawBuffer = other.rawBuffer;
            buffer = other.buffer;
            capacity = other.capacity;
            offset = other.offset;
            isExternalBuffer = other.isExternalBuffer;

            other.rawBuffer = nullptr;
            other.buffer = nullptr;
            other.capacity = 0;
            other.offset = 0;
        }
        return *this;
    }

    void* alloc(size_t sizeBytes, size_t alignment = 8) {
        if (!buffer || sizeBytes == 0 || alignment == 0) return nullptr;

        uintptr_t currentAddr = reinterpret_cast<uintptr_t>(buffer + offset);
        uintptr_t alignedAddr = (currentAddr + (alignment - 1)) & ~(alignment - 1);
        size_t padding = alignedAddr - currentAddr;

        if (offset + padding + sizeBytes > capacity) {
            return nullptr;
        }

        offset += padding;
        void* result = buffer + offset;
        offset += sizeBytes;
        return result;
    }

    template <typename T, typename... Args>
    T* allocTyped(Args&&... args) {
        void* mem = alloc(sizeof(T), alignof(T));
        if (!mem) return nullptr;
        return new (mem) T(std::forward<Args>(args)...);
    }

    char* allocString(const char* str, size_t len) {
        if (!str) return nullptr;
        char* dest = static_cast<char*>(alloc(len + 1, 1));
        if (dest) {
            std::memcpy(dest, str, len);
            dest[len] = '\0';
        }
        return dest;
    }

    void reset() { offset = 0; }
    size_t getUsedBytes() const { return offset; }
    size_t getCapacity() const { return capacity; }
    size_t getFreeBytes() const { return (capacity > offset) ? (capacity - offset) : 0; }
    uint8_t* getBasePointer() const { return buffer; }
};

#endif 