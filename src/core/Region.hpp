#ifndef LSM_CORE_REGION_HPP
#define LSM_CORE_REGION_HPP

#include "Arena.hpp"
#include <string_view>

class Region : public Arena {
private:
    Region* parentRegion = nullptr;
    std::string_view scopeName;
    size_t scopeDepth = 0;

    
    struct SlabChunk { SlabChunk* next; };
    static constexpr size_t SLAB_CLASSES_COUNT = 5;
    static constexpr size_t slabSizes[SLAB_CLASSES_COUNT] = {32, 64, 128, 256, 512};
    SlabChunk* freeLists[SLAB_CLASSES_COUNT] = {nullptr};

    int getSlabClassIndex(size_t size) {
        for (size_t i = 0; i < SLAB_CLASSES_COUNT; ++i) {
            if (size <= slabSizes[i]) return static_cast<int>(i);
        }
        return -1;
    }

public:
    
    explicit Region(size_t sizeBytes = 256 * 1024, Region* parent = nullptr, std::string_view name = "global")
        : Arena(sizeBytes), parentRegion(parent), scopeName(name) {
        scopeDepth = parentRegion ? (parentRegion->scopeDepth + 1) : 0;
    }

    
    Region(void* chunkMem, size_t sizeBytes, Region* parent, std::string_view name)
        : Arena(chunkMem, sizeBytes), parentRegion(parent), scopeName(name) {
        scopeDepth = parentRegion ? (parentRegion->scopeDepth + 1) : 0;
    }

    static Region* createChild(Region* parent, size_t sizeBytes = 64 * 1024, std::string_view name = "block") {
        if (!parent) return new Region(sizeBytes, nullptr, name);

        
        void* childObjMem = parent->alloc(sizeof(Region), alignof(Region));
        void* childDataMem = parent->alloc(sizeBytes, LSM_CACHE_LINE);

        if (!childObjMem || !childDataMem) {
            return new Region(sizeBytes, parent, name);
        }

        return new (childObjMem) Region(childDataMem, sizeBytes, parent, name);
    }

    Region* getParent() const { return parentRegion; }
    size_t getDepth() const { return scopeDepth; }
    std::string_view getName() const { return scopeName; }

    
    void* slabAlloc(size_t sizeBytes) {
        int idx = getSlabClassIndex(sizeBytes);
        if (idx != -1 && freeLists[idx] != nullptr) {
            SlabChunk* head = freeLists[idx];
            freeLists[idx] = head->next;
            return static_cast<void*>(head);
        }
        size_t allocSz = (idx != -1) ? slabSizes[idx] : sizeBytes;
        return alloc(allocSz, 8);
    }

    void slabFree(void* ptr, size_t sizeBytes) {
        int idx = getSlabClassIndex(sizeBytes);
        if (idx != -1 && ptr != nullptr) {
            SlabChunk* chunk = static_cast<SlabChunk*>(ptr);
            chunk->next = freeLists[idx];
            freeLists[idx] = chunk;
        }
    }

    void* allocEscaped(size_t sizeBytes, size_t alignment = 8) {
        if (parentRegion) {
            return parentRegion->alloc(sizeBytes, alignment);
        }
        return alloc(sizeBytes, alignment);
    }

    template <typename T, typename... Args>
    T* allocEscapedTyped(Args&&... args) {
        void* mem = allocEscaped(sizeof(T), alignof(T));
        if (!mem) return nullptr;
        return new (mem) T(std::forward<Args>(args)...);
    }

    void dispose() {
        reset();
        for (size_t i = 0; i < SLAB_CLASSES_COUNT; ++i) {
            freeLists[i] = nullptr;
        }
    }
};

#endif 