#ifndef LSM_SSA_INSTRUCTION_HPP
#define LSM_SSA_INSTRUCTION_HPP

#include <string>
#include <vector>
#include <cstdint>
#include "../../backend/regalloc/PhysicalRegister.hpp"

enum class LsmStaticType { 
    Int64, 
    Int32,
    Int16,
    Int8,
    Float64, 
    String, 
    Ptr, 
    Ptr32,
    Bool,
    Void,
    Dynamic, 
    Unknown 
};

inline LsmStaticType getDefaultIntType(ArchType arch) {
    return (arch == ArchType::X86_32) ? LsmStaticType::Int32 : LsmStaticType::Int64;
}

inline LsmStaticType getDefaultPtrType(ArchType arch) {
    return (arch == ArchType::X86_32) ? LsmStaticType::Ptr32 : LsmStaticType::Ptr;
}

enum class SSAOp {
    ConstNum, 
    ConstFloat, 
    ConstStr,
    Copy,

    Add, Sub, Mul, Div, Mod,

    BitAnd, BitOr, BitXor, BitNot, Shl, Shr,

    FAdd, FSub, FMul, FDiv,

    Equal, NotEqual, LessThan, LessEqual, GreaterThan, GreaterEqual,
    FEqual, FNotEqual, FLessThan, FLessEqual, FGreaterThan, FGreaterEqual,

    Branch,
    CondBranch,
    Call,
    Return,

    ArrayAlloc,
    ArrayLoad,
    ArrayStore,

    RegionEnter,
    RegionExit,
    AllocLocal,
    AllocEscaped,

    TaskSpawn,
    TaskYield,

    SetRegister,
    GetRegister,
    CpuHalt,
    DisableInterrupts,
    EnableInterrupts,
    IOPortRead,
    IOPortWrite,
    IOPortRead16,
    IOPortWrite16,

    VolatileLoad,
    VolatileStore,

    AtomicRMW,
    Phi
};

struct SSAValue {
    std::string name;
    LsmStaticType type = LsmStaticType::Unknown;
    bool isConstant = false;
    int64_t constNum = 0;
    double constFloat = 0.0;
    std::string constStr = "";

    static SSAValue makeVar(const std::string& n, LsmStaticType t) {
        SSAValue v; v.name = n; v.type = t; return v;
    }
    static SSAValue makeTemp(size_t id, LsmStaticType t) {
        SSAValue v; v.name = "t" + std::to_string(id); v.type = t; return v;
    }
    static SSAValue makeConstNum(int64_t n, LsmStaticType t = LsmStaticType::Int64) {
        SSAValue v; v.isConstant = true; v.constNum = n; v.type = t; return v;
    }
    static SSAValue makeConstFloat(double f) {
        SSAValue v; v.isConstant = true; v.constFloat = f; v.type = LsmStaticType::Float64; return v;
    }
    static SSAValue makeConstStr(const std::string& s) {
        SSAValue v; v.isConstant = true; v.constStr = s; v.type = LsmStaticType::String; return v;
    }

    bool operator==(const SSAValue& other) const { 
        return name == other.name && constNum == other.constNum && constFloat == other.constFloat; 
    }
    bool operator!=(const SSAValue& other) const { return !(*this == other); }
};

struct SSAInstruction {
    SSAOp op;
    SSAValue result;
    std::vector<SSAValue> operands;
};

struct SSABasicBlock {
    std::string label;
    std::vector<SSAInstruction> instructions;
    std::vector<std::string> predecessors;
    std::vector<std::string> successors;
};

struct SSAFunction {
    std::string name;
    std::vector<std::pair<std::string, LsmStaticType>> params;
    LsmStaticType returnType = LsmStaticType::Void;
    std::vector<SSABasicBlock> blocks;
    bool isNaked = false;
    bool isInline = false;
};

#endif 