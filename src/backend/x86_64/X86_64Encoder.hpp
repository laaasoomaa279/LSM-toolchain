#ifndef LSM_X86_64_ENCODER_HPP
#define LSM_X86_64_ENCODER_HPP

#include "../../middleend/ssa/SSAInstruction.hpp"
#include "../ffi/DynamicLoader.hpp"
#include "../regalloc/RegisterAllocator.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

struct JumpPatch {
    size_t offsetLocation;
    std::string targetLabel;
};

class X86_64Encoder {
private:
    std::vector<uint8_t> code;
    std::unordered_map<std::string, int> stackOffsets;
    std::unordered_map<std::string, int> arrayBaseOffsets;
    int currentStackOffset = 0;
    int allocatedStackSize = 0;

    std::unordered_map<std::string, size_t> blockOffsets;
    std::vector<JumpPatch> jumpPatches;
    std::unordered_map<std::string, uintptr_t> externalSymbols;
    DynamicLoader dynamicLoader;

    RegisterAllocator regAllocator;
    AllocationResult currentAlloc;

    void emitByte(uint8_t b);
    void emitInt32(int32_t val);
    void emitInt64(int64_t val);
    void emitDouble(double val);
    void emitREX(bool w, bool r, bool x, bool b);

    int getStackOffset(const std::string& varName);
    
    void emitStackLoad(uint8_t regCode, int offset);
    void emitStackStore(uint8_t regCode, int offset);
    void emitValueLoad(uint8_t targetReg, const SSAValue& val);
    void emitValueStore(uint8_t srcReg, const std::string& varName);

    void emitXmmLoad(uint8_t xmmReg, int offset);
    void emitXmmStore(uint8_t xmmReg, int offset);
    void emitXmmValueLoad(uint8_t xmmReg, const SSAValue& val);
    void emitXmmValueStore(uint8_t xmmReg, const std::string& varName);

    void encodeInstruction(const SSAInstruction& inst);
    void encodeFunctionInternal(const SSAFunction& func);

public:
    X86_64Encoder() = default;
    void registerExternalSymbol(const std::string& lib, const std::string& sym);
    std::vector<uint8_t> encodeProgram(const std::vector<SSAFunction>& program);
};

#endif 