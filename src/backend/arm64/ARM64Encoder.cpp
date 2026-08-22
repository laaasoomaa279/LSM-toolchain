#include "ARM64Encoder.hpp"
#include <cstring>
#include <cmath>

void ARM64Encoder::emitInst32(uint32_t inst) {
    code.push_back(static_cast<uint8_t>(inst & 0xFF));
    code.push_back(static_cast<uint8_t>((inst >> 8) & 0xFF));
    code.push_back(static_cast<uint8_t>((inst >> 16) & 0xFF));
    code.push_back(static_cast<uint8_t>((inst >> 24) & 0xFF));
}

int ARM64Encoder::getStackOffset(const std::string& varName) {
    if (stackOffsets.find(varName) == stackOffsets.end()) {
        currentStackOffset += 8;
        stackOffsets[varName] = currentStackOffset;
    }
    return stackOffsets[varName];
}

void ARM64Encoder::emitStackStore(uint32_t rt, int offset) {
    uint32_t imm12 = (offset / 8) & 0xFFF;
    emitInst32(0xF90003E0 | (imm12 << 10) | (rt & 0x1F));
}

void ARM64Encoder::emitStackLoad(uint32_t rt, int offset) {
    uint32_t imm12 = (offset / 8) & 0xFFF;
    emitInst32(0xF94003E0 | (imm12 << 10) | (rt & 0x1F));
}

void ARM64Encoder::emitFPRStackStore(uint32_t vt, int offset) {
    uint32_t imm12 = (offset / 8) & 0xFFF;
    emitInst32(0xBD0003E0 | (imm12 << 10) | (vt & 0x1F));
}

void ARM64Encoder::emitFPRStackLoad(uint32_t vt, int offset) {
    uint32_t imm12 = (offset / 8) & 0xFFF;
    emitInst32(0xBD4003E0 | (imm12 << 10) | (vt & 0x1F));
}

void ARM64Encoder::emitLoadImm64(uint32_t rd, int64_t val) {
    uint16_t p0 = val & 0xFFFF;
    uint16_t p1 = (val >> 16) & 0xFFFF;
    uint16_t p2 = (val >> 32) & 0xFFFF;
    uint16_t p3 = (val >> 48) & 0xFFFF;

    emitInst32(0xD2800000 | (static_cast<uint32_t>(p0) << 5) | (rd & 0x1F));
    if (p1) emitInst32(0xF2A00000 | (static_cast<uint32_t>(p1) << 5) | (rd & 0x1F));
    if (p2) emitInst32(0xF2C00000 | (static_cast<uint32_t>(p2) << 5) | (rd & 0x1F));
    if (p3) emitInst32(0xF2E00000 | (static_cast<uint32_t>(p3) << 5) | (rd & 0x1F));
}

void ARM64Encoder::emitLoadGPROrImm(uint32_t rd, const SSAValue& val) {
    if (val.isConstant) {
        emitLoadImm64(rd, val.constNum);
    } else {
        emitStackLoad(rd, getStackOffset(val.name));
    }
}

void ARM64Encoder::emitLoadFPROrImm(uint32_t vd, const SSAValue& val) {
    if (val.isConstant) {
        int64_t rawBits = 0;
        std::memcpy(&rawBits, &val.constFloat, sizeof(double));
        emitLoadImm64(9, rawBits);
        emitInst32(0x9E670120 | (vd & 0x1F)); 
    } else {
        emitFPRStackLoad(vd, getStackOffset(val.name));
    }
}

uint32_t getARM64RegisterCode(const std::string& reg) {
    if (reg == "rax" || reg == "x0") return 0;
    if (reg == "rcx" || reg == "x1") return 1;
    if (reg == "rdx" || reg == "x2") return 2;
    if (reg == "rbx" || reg == "x3") return 3;
    if (reg == "rsi" || reg == "x4") return 4;
    if (reg == "rdi" || reg == "x5") return 5;
    if (reg == "rbp" || reg == "x29") return 29;
    if (reg == "rsp" || reg == "sp") return 31;
    return 0;
}

void ARM64Encoder::encodeInstruction(const SSAInstruction& inst) {
    switch (inst.op) {
        case SSAOp::Add:
        case SSAOp::Sub:
        case SSAOp::Mul: {
            emitLoadGPROrImm(0, inst.operands[0]);
            emitLoadGPROrImm(1, inst.operands[1]);
            if (inst.op == SSAOp::Add) emitInst32(0x8B010000);      
            else if (inst.op == SSAOp::Sub) emitInst32(0xCB010000); 
            else if (inst.op == SSAOp::Mul) emitInst32(0x9B017C00); 
            if (!inst.result.name.empty()) emitStackStore(0, getStackOffset(inst.result.name));
            break;
        }

        case SSAOp::Div: {
            emitLoadGPROrImm(0, inst.operands[0]);
            emitLoadGPROrImm(1, inst.operands[1]);
            emitInst32(0x9AC10C00); 
            if (!inst.result.name.empty()) emitStackStore(0, getStackOffset(inst.result.name));
            break;
        }

        case SSAOp::Mod: {
            emitLoadGPROrImm(0, inst.operands[0]);
            emitLoadGPROrImm(1, inst.operands[1]);
            emitInst32(0x9AC10C02); 
            emitInst32(0x9B018040); 
            if (!inst.result.name.empty()) emitStackStore(0, getStackOffset(inst.result.name));
            break;
        }

        
        case SSAOp::SetRegister: {
            std::string regName = inst.operands[0].constStr;
            uint32_t rCode = getARM64RegisterCode(regName);
            emitLoadGPROrImm(rCode, inst.operands[1]);
            break;
        }

        case SSAOp::GetRegister: {
            std::string regName = inst.operands[0].constStr;
            uint32_t rCode = getARM64RegisterCode(regName);
            if (rCode != 0) {
                emitInst32(0xAA0003E0 | (rCode << 16)); 
            }
            if (!inst.result.name.empty()) emitStackStore(0, getStackOffset(inst.result.name));
            break;
        }

        case SSAOp::CpuHalt: {
            emitInst32(0xD503207F); 
            break;
        }

        case SSAOp::DisableInterrupts: {
            emitInst32(0xD50342DF); 
            break;
        }

        case SSAOp::EnableInterrupts: {
            emitInst32(0xD50342FF); 
            break;
        }

        case SSAOp::VolatileLoad:
        case SSAOp::IOPortRead: {
            emitLoadGPROrImm(0, inst.operands[0]); 
            emitInst32(0xF9400000);                 
            if (!inst.result.name.empty()) emitStackStore(0, getStackOffset(inst.result.name));
            break;
        }

        case SSAOp::VolatileStore:
        case SSAOp::IOPortWrite: {
            emitLoadGPROrImm(0, inst.operands[0]); 
            emitLoadGPROrImm(1, inst.operands[1]); 
            emitInst32(0xF9000001);                 
            break;
        }

        case SSAOp::CondBranch: {
            emitLoadGPROrImm(0, inst.operands[0]);
            branchPatches.push_back({code.size(), inst.operands[2].constStr, true});
            emitInst32(0xB4000000); 
            branchPatches.push_back({code.size(), inst.operands[1].constStr, false});
            emitInst32(0x14000000); 
            break;
        }

        case SSAOp::Branch: {
            branchPatches.push_back({code.size(), inst.operands[0].constStr, false});
            emitInst32(0x14000000); 
            break;
        }

        case SSAOp::Call: {
            std::string targetFunc = inst.operands[0].constStr;
            for (size_t i = 1; i < inst.operands.size() && i <= 8; ++i) {
                uint32_t regIdx = static_cast<uint32_t>(i - 1);
                if (inst.operands[i].type == LsmStaticType::Float64) {
                    emitLoadFPROrImm(regIdx, inst.operands[i]);
                } else {
                    emitLoadGPROrImm(regIdx, inst.operands[i]);
                }
            }
            branchPatches.push_back({code.size(), targetFunc, false});
            emitInst32(0x94000000); 
            if (!inst.result.name.empty()) {
                if (inst.result.type == LsmStaticType::Float64) emitFPRStackStore(0, getStackOffset(inst.result.name));
                else emitStackStore(0, getStackOffset(inst.result.name));
            }
            break;
        }

        case SSAOp::Return: {
            if (!inst.operands.empty()) {
                if (inst.operands[0].type == LsmStaticType::Float64) emitLoadFPROrImm(0, inst.operands[0]);
                else emitLoadGPROrImm(0, inst.operands[0]);
            }
            uint32_t imm7 = ((currentFrameSize / 8) & 0x7F) << 15;
            emitInst32(0xA8C07BFD | imm7); 
            emitInst32(0xD65F03C0);        
            break;
        }

        default: break;
    }
}

void ARM64Encoder::encodeFunctionInternal(const SSAFunction& func) {
    stackOffsets.clear();
    currentStackOffset = 0;
    blockOffsets[func.name] = code.size();

    currentFrameSize = 512; 
    uint32_t imm7 = ((-currentFrameSize / 8) & 0x7F) << 15;

    emitInst32(0xA9807BFD | imm7); 
    emitInst32(0x910003FD);         

    for (size_t i = 0; i < func.params.size() && i < 8; ++i) {
        int offset = getStackOffset(func.params[i].first);
        if (func.params[i].second == LsmStaticType::Float64) emitFPRStackStore(static_cast<uint32_t>(i), offset);
        else emitStackStore(static_cast<uint32_t>(i), offset);
    }

    for (const auto& block : func.blocks) {
        blockOffsets[block.label] = code.size();
        for (const auto& inst : block.instructions) {
            encodeInstruction(inst);
        }
    }
}

std::vector<uint8_t> ARM64Encoder::encodeProgram(const std::vector<SSAFunction>& program) {
    code.clear();
    blockOffsets.clear();
    branchPatches.clear();

    SSAFunction startFunc;
    SSAFunction mainFunc;
    std::vector<SSAFunction> otherFuncs;

    for (const auto& f : program) {
        if (f.name == "_start") startFunc = f;
        else if (f.name == "main") mainFunc = f;
        else otherFuncs.push_back(f);
    }

    if (!startFunc.name.empty()) encodeFunctionInternal(startFunc);
    if (!mainFunc.name.empty()) encodeFunctionInternal(mainFunc);
    for (const auto& f : otherFuncs) encodeFunctionInternal(f);

    for (const auto& patch : branchPatches) {
        if (blockOffsets.count(patch.targetLabel)) {
            size_t target = blockOffsets[patch.targetLabel];
            int32_t offsetInst = static_cast<int32_t>(static_cast<int64_t>(target) - static_cast<int64_t>(patch.byteOffset)) / 4;

            uint32_t currentInst = 0;
            currentInst |= code[patch.byteOffset];
            currentInst |= (static_cast<uint32_t>(code[patch.byteOffset + 1]) << 8);
            currentInst |= (static_cast<uint32_t>(code[patch.byteOffset + 2]) << 16);
            currentInst |= (static_cast<uint32_t>(code[patch.byteOffset + 3]) << 24);

            if (patch.isConditional) {
                currentInst |= ((static_cast<uint32_t>(offsetInst) & 0x7FFFF) << 5);
            } else {
                currentInst |= (static_cast<uint32_t>(offsetInst) & 0x3FFFFFF);
            }

            code[patch.byteOffset]     = static_cast<uint8_t>(currentInst & 0xFF);
            code[patch.byteOffset + 1] = static_cast<uint8_t>((currentInst >> 8) & 0xFF);
            code[patch.byteOffset + 2] = static_cast<uint8_t>((currentInst >> 16) & 0xFF);
            code[patch.byteOffset + 3] = static_cast<uint8_t>((currentInst >> 24) & 0xFF);
        }
    }
    return code;
}