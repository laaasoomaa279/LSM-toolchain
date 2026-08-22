#ifndef LSM_BAREMETAL_X86_MODERN_ENCODER_HPP
#define LSM_BAREMETAL_X86_MODERN_ENCODER_HPP

#include "../../middleend/ssa/SSAInstruction.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

struct BareMetalModernJumpPatch {
    size_t offsetLocation;
    std::string targetLabel;
};

class BareMetalX86ModernEncoder {
private:
    std::vector<uint8_t> code;
    std::unordered_map<std::string, int> stackOffsets;
    int currentStackOffset = 0;

    std::unordered_map<std::string, size_t> blockOffsets;
    std::vector<BareMetalModernJumpPatch> jumpPatches;

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
    BareMetalX86ModernEncoder() = default;
    std::vector<uint8_t> encodeProgram(const std::vector<SSAFunction>& program);
};

#endif 