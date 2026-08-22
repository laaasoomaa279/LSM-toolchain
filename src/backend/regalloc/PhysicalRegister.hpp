#ifndef LSM_PHYSICAL_REGISTER_HPP
#define LSM_PHYSICAL_REGISTER_HPP

#include <string>
#include <vector>
#include <cstdint>

enum class ArchType {
    X86_64,
    X86_32, 
    ARM64
};

enum class RegClass {
    GPR,   
    FPR    
};

struct PhysicalReg {
    uint32_t id;
    std::string name;
    RegClass regClass;
    bool isCalleeSaved;
    bool isReserved;

    bool operator==(const PhysicalReg& other) const { 
        return id == other.id && regClass == other.regClass; 
    }
    bool operator!=(const PhysicalReg& other) const { return !(*this == other); }
};

class TargetRegisters {
public:
    static std::vector<PhysicalReg> getX86_64AllocatableGPRs() {
        return {
            {3,  "rbx", RegClass::GPR, true,  false},
            {10, "r10", RegClass::GPR, false, false},
            {11, "r11", RegClass::GPR, false, false},
            {12, "r12", RegClass::GPR, true,  false},
            {13, "r13", RegClass::GPR, true,  false},
            {14, "r14", RegClass::GPR, true,  false},
            {15, "r15", RegClass::GPR, true,  false},
            {6,  "rsi", RegClass::GPR, false, false},
            {7,  "rdi", RegClass::GPR, false, false}
        };
    }

    static std::vector<PhysicalReg> getX86_32AllocatableGPRs() {
        return {
            {3, "ebx", RegClass::GPR, true,  false},
            {6, "esi", RegClass::GPR, true,  false},
            {7, "edi", RegClass::GPR, true,  false},
            {2, "ecx", RegClass::GPR, false, false},
            {1, "edx", RegClass::GPR, false, false}
        };
    }

    static std::vector<PhysicalReg> getX86_64AllocatableFPRs() {
        std::vector<PhysicalReg> fprs;
        for (uint32_t i = 2; i <= 15; ++i) {
            fprs.push_back({i, "xmm" + std::to_string(i), RegClass::FPR, false, false});
        }
        return fprs;
    }

    static std::vector<PhysicalReg> getARM64AllocatableGPRs() {
        std::vector<PhysicalReg> gprs;
        for (uint32_t i = 9; i <= 15; ++i) {
            gprs.push_back({i, "x" + std::to_string(i), RegClass::GPR, false, false});
        }
        for (uint32_t i = 19; i <= 28; ++i) {
            gprs.push_back({i, "x" + std::to_string(i), RegClass::GPR, true, false});
        }
        return gprs;
    }
};

#endif 