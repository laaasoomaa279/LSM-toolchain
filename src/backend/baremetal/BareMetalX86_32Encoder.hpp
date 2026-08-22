#ifndef LSM_BAREMETAL_X86_32_ENCODER_HPP
#define LSM_BAREMETAL_X86_32_ENCODER_HPP

#include "../../middleend/ssa/SSAInstruction.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

struct BareMetalJumpPatch32 {
    size_t offsetLocation;
    std::string targetLabel;
};

class BareMetalX86_32Encoder {
private:
    std::vector<uint8_t> code;
    std::unordered_map<std::string, int> stackOffsets;
    int currentStackOffset = 0;

    std::unordered_map<std::string, size_t> blockOffsets;
    std::vector<BareMetalJumpPatch32> jumpPatches;

    void emitByte(uint8_t b);
    void emitInt32(int32_t val);

    int getStackOffset(const std::string& varName);
    
    void emitStackLoad(uint8_t regCode, int offset);
    void emitStackStore(uint8_t regCode, int offset);
    void emitValueLoad(uint8_t targetReg, const SSAValue& val);
    void emitValueStore(uint8_t srcReg, const std::string& varName);

    void encodeInstruction(const SSAInstruction& inst);
    void encodeFunctionInternal(const SSAFunction& func);

public:
    BareMetalX86_32Encoder() = default;
    std::vector<uint8_t> encodeProgram(const std::vector<SSAFunction>& program);
};

#endif 