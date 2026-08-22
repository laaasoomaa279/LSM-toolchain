#ifndef LSM_ARM64_ENCODER_HPP
#define LSM_ARM64_ENCODER_HPP

#include "../../middleend/ssa/SSAInstruction.hpp"
#include "../regalloc/RegisterAllocator.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

struct ARM64BranchPatch {
    size_t byteOffset;
    std::string targetLabel;
    bool isConditional;
};

class ARM64Encoder {
private:
    std::vector<uint8_t> code;
    std::unordered_map<std::string, int> stackOffsets;
    int currentStackOffset = 0;
    int currentFrameSize = 0;

    std::unordered_map<std::string, size_t> blockOffsets;
    std::vector<ARM64BranchPatch> branchPatches;

    RegisterAllocator regAllocator;
    AllocationResult currentAlloc;

    void emitInst32(uint32_t inst);
    int getStackOffset(const std::string& varName);

    void emitStackStore(uint32_t rt, int offset);
    void emitStackLoad(uint32_t rt, int offset);
    void emitFPRStackStore(uint32_t vt, int offset);
    void emitFPRStackLoad(uint32_t vt, int offset);

    void emitLoadImm64(uint32_t rd, int64_t val);
    void emitLoadGPROrImm(uint32_t rd, const SSAValue& val);
    void emitLoadFPROrImm(uint32_t vd, const SSAValue& val);

    void emitValueLoad(uint32_t targetReg, const SSAValue& val);
    void emitValueStore(uint32_t srcReg, const std::string& varName);

    void encodeInstruction(const SSAInstruction& inst);
    void encodeFunctionInternal(const SSAFunction& func);

public:
    ARM64Encoder() = default;
    std::vector<uint8_t> encodeProgram(const std::vector<SSAFunction>& program);
};

#endif 