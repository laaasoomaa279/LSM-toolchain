#include "BareMetalX86_32Encoder.hpp"

void BareMetalX86_32Encoder::emitByte(uint8_t b) { 
    code.push_back(b); 
}

void BareMetalX86_32Encoder::emitInt32(int32_t val) {
    uint8_t* p = reinterpret_cast<uint8_t*>(&val);
    for (int i = 0; i < 4; i++) emitByte(p[i]);
}

int BareMetalX86_32Encoder::getStackOffset(const std::string& varName) {
    if (stackOffsets.find(varName) == stackOffsets.end()) {
        currentStackOffset -= 4;
        stackOffsets[varName] = currentStackOffset;
    }
    return stackOffsets[varName];
}

void BareMetalX86_32Encoder::emitStackLoad(uint8_t reg, int offset) {
    uint8_t r = reg & 7;
    if (offset >= -128 && offset <= 127) {
        emitByte(0x8B); 
        emitByte(0x45 | (r << 3)); 
        emitByte(static_cast<uint8_t>(offset));
    } else {
        emitByte(0x8B); 
        emitByte(0x85 | (r << 3)); 
        emitInt32(offset);
    }
}

void BareMetalX86_32Encoder::emitStackStore(uint8_t reg, int offset) {
    uint8_t r = reg & 7;
    if (offset >= -128 && offset <= 127) {
        emitByte(0x89); 
        emitByte(0x45 | (r << 3)); 
        emitByte(static_cast<uint8_t>(offset));
    } else {
        emitByte(0x89); 
        emitByte(0x85 | (r << 3)); 
        emitInt32(offset);
    }
}

void BareMetalX86_32Encoder::emitValueLoad(uint8_t targetReg, const SSAValue& val) {
    if (val.isConstant) {
        emitByte(0xB8 + (targetReg & 7));
        emitInt32(static_cast<int32_t>(val.constNum));
    } else {
        emitStackLoad(targetReg, getStackOffset(val.name));
    }
}

void BareMetalX86_32Encoder::emitValueStore(uint8_t srcReg, const std::string& varName) {
    emitStackStore(srcReg, getStackOffset(varName));
}

void BareMetalX86_32Encoder::encodeInstruction(const SSAInstruction& inst) {
    switch (inst.op) {
        case SSAOp::Add:
        case SSAOp::Sub:
        case SSAOp::Mul: {
            emitValueLoad(0, inst.operands[0]); 
            emitValueLoad(1, inst.operands[1]); 
            if (inst.op == SSAOp::Add) { emitByte(0x01); emitByte(0xC8); }      
            else if (inst.op == SSAOp::Sub) { emitByte(0x29); emitByte(0xC8); } 
            else if (inst.op == SSAOp::Mul) { emitByte(0x0F); emitByte(0xAF); emitByte(0xC1); } 
            if (!inst.result.name.empty()) emitValueStore(0, inst.result.name);
            break;
        }

        case SSAOp::Div:
        case SSAOp::Mod: {
            emitValueLoad(0, inst.operands[0]); 
            emitValueLoad(1, inst.operands[1]); 
            emitByte(0x99);                     
            emitByte(0xF7); emitByte(0xF9);     
            uint8_t resReg = (inst.op == SSAOp::Div) ? 0 : 2; 
            if (!inst.result.name.empty()) emitValueStore(resReg, inst.result.name);
            break;
        }

        case SSAOp::BitAnd:
        case SSAOp::BitOr:
        case SSAOp::BitXor: {
            emitValueLoad(0, inst.operands[0]);
            emitValueLoad(1, inst.operands[1]);
            if (inst.op == SSAOp::BitAnd) { emitByte(0x21); emitByte(0xC8); }     
            else if (inst.op == SSAOp::BitOr) { emitByte(0x09); emitByte(0xC8); }  
            else if (inst.op == SSAOp::BitXor) { emitByte(0x31); emitByte(0xC8); } 
            if (!inst.result.name.empty()) emitValueStore(0, inst.result.name);
            break;
        }

        case SSAOp::Shl:
        case SSAOp::Shr: {
            emitValueLoad(0, inst.operands[0]); 
            emitValueLoad(1, inst.operands[1]); 
            if (inst.op == SSAOp::Shl) { emitByte(0xD3); emitByte(0xE0); }      
            else if (inst.op == SSAOp::Shr) { emitByte(0xD3); emitByte(0xE8); } 
            if (!inst.result.name.empty()) emitValueStore(0, inst.result.name);
            break;
        }

        case SSAOp::Copy: {
            emitValueLoad(0, inst.operands[0]);
            if (!inst.result.name.empty()) emitValueStore(0, inst.result.name);
            break;
        }

        
        case SSAOp::VolatileLoad: {
            emitValueLoad(0, inst.operands[0]); 
            if (inst.result.type == LsmStaticType::Int32 || inst.result.type == LsmStaticType::Ptr32) {
                emitByte(0x8B); emitByte(0x00); 
            } else {
                emitByte(0x0F); emitByte(0xB6); emitByte(0x00); 
            }
            if (!inst.result.name.empty()) emitValueStore(0, inst.result.name);
            break;
        }

        
        case SSAOp::VolatileStore: {
            emitValueLoad(0, inst.operands[0]); 
            emitValueLoad(1, inst.operands[1]); 
            if (inst.operands[1].type == LsmStaticType::Int32 || inst.operands[1].type == LsmStaticType::Ptr32) {
                emitByte(0x89); emitByte(0x08); 
            } else {
                emitByte(0x88); emitByte(0x08); 
            }
            break;
        }

        case SSAOp::IOPortRead: {
            emitValueLoad(2, inst.operands[0]); 
            emitByte(0xEC);                     
            emitByte(0x0F); emitByte(0xB6); emitByte(0xC0); 
            if (!inst.result.name.empty()) emitValueStore(0, inst.result.name);
            break;
        }

        case SSAOp::IOPortWrite: {
            emitValueLoad(2, inst.operands[0]); 
            emitValueLoad(0, inst.operands[1]); 
            emitByte(0xEE);                     
            break;
        }

        case SSAOp::CondBranch: {
            emitValueLoad(0, inst.operands[0]);
            emitByte(0x85); emitByte(0xC0);     
            emitByte(0x0F); emitByte(0x84);     
            jumpPatches.push_back({code.size(), inst.operands[2].constStr});
            emitInt32(0);
            emitByte(0xE9);                     
            jumpPatches.push_back({code.size(), inst.operands[1].constStr});
            emitInt32(0);
            break;
        }

        case SSAOp::Branch: {
            emitByte(0xE9);                     
            jumpPatches.push_back({code.size(), inst.operands[0].constStr});
            emitInt32(0);
            break;
        }

        case SSAOp::Call: {
            std::string fn = inst.operands[0].constStr;

            
            if (fn.rfind("__lsm_native_print_", 0) == 0) {
                break; 
            }

            size_t totalArgs = inst.operands.size() - 1;

            
            for (size_t i = totalArgs; i >= 1; --i) {
                emitValueLoad(0, inst.operands[i]);
                emitByte(0x50); 
            }

            emitByte(0xE8); 
            jumpPatches.push_back({code.size(), fn});
            emitInt32(0);

            if (totalArgs > 0) {
                emitByte(0x81); emitByte(0xC4); emitInt32(static_cast<int32_t>(totalArgs * 4)); 
            }

            if (!inst.result.name.empty()) emitValueStore(0, inst.result.name);
            break;
        }

        case SSAOp::Return: {
            if (!inst.operands.empty()) {
                emitValueLoad(0, inst.operands[0]); 
            }
            emitByte(0x89); emitByte(0xEC); 
            emitByte(0x5D);                 
            emitByte(0xC3);                 
            break;
        }

        default: break;
    }
}

void BareMetalX86_32Encoder::encodeFunctionInternal(const SSAFunction& func) {
    stackOffsets.clear();
    currentStackOffset = 0;
    blockOffsets[func.name] = code.size();

    if (!func.isNaked) {
        emitByte(0x55);                 
        emitByte(0x89); emitByte(0xE5); 
        emitByte(0x81); emitByte(0xEC); emitInt32(2048); 

        for (size_t i = 0; i < func.params.size(); ++i) {
            std::string pName = func.params[i].first;
            int callerStackOffset = 8 + static_cast<int>(i * 4); 
            emitStackLoad(0, callerStackOffset);
            emitValueStore(0, pName);
        }
    }

    for (const auto& block : func.blocks) {
        blockOffsets[block.label] = code.size();
        for (const auto& inst : block.instructions) {
            encodeInstruction(inst);
        }
    }
}

std::vector<uint8_t> BareMetalX86_32Encoder::encodeProgram(const std::vector<SSAFunction>& program) {
    code.clear(); blockOffsets.clear(); jumpPatches.clear();

    SSAFunction startFunc; std::vector<SSAFunction> others;
    for (const auto& f : program) {
        if (f.name == "_start") startFunc = f;
        else others.push_back(f);
    }

    if (!startFunc.name.empty()) encodeFunctionInternal(startFunc);
    for (const auto& f : others) encodeFunctionInternal(f);

    for (const auto& patch : jumpPatches) {
        if (blockOffsets.count(patch.targetLabel)) {
            size_t target = blockOffsets[patch.targetLabel];
            int32_t rel = static_cast<int32_t>(static_cast<int64_t>(target) - static_cast<int64_t>(patch.offsetLocation + 4));
            uint8_t* p = reinterpret_cast<uint8_t*>(&rel);
            for (int i = 0; i < 4; ++i) code[patch.offsetLocation + i] = p[i];
        }
    }
    return code;
}