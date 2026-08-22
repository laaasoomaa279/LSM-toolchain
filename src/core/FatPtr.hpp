#ifndef LSM_CORE_FAT_PTR_HPP
#define LSM_CORE_FAT_PTR_HPP

#include <cstdint>
#include <cstddef>

enum class FatPtrKind : uint32_t {
    Raw,
    Interface, 
    Slice,     
    Closure    
};

struct FatPtr {
    void* dataPtr = nullptr;
    union {
        void* vtablePtr;       
        size_t sliceLength;    
        void* envContext;      
        uintptr_t rawMetadata;
    } meta;
    FatPtrKind kind = FatPtrKind::Raw;

    static FatPtr makeInterface(void* data, void* vtable) {
        FatPtr p;
        p.dataPtr = data;
        p.meta.vtablePtr = vtable;
        p.kind = FatPtrKind::Interface;
        return p;
    }

    static FatPtr makeSlice(void* data, size_t len) {
        FatPtr p;
        p.dataPtr = data;
        p.meta.sliceLength = len;
        p.kind = FatPtrKind::Slice;
        return p;
    }

    static FatPtr makeClosure(void* fnPtr, void* env) {
        FatPtr p;
        p.dataPtr = fnPtr;
        p.meta.envContext = env;
        p.kind = FatPtrKind::Closure;
        return p;
    }

    bool isNull() const { return dataPtr == nullptr; }
};

#endif 