#include "X86_64Encoder.hpp"
#include <iostream>
#include <cstring>
#include <list>

static std::list<std::string> g_StringPool;

extern "C" void __lsm_native_print_str(const char* str) {
    if (str) std::cout << str << std::endl;
}
extern "C" void __lsm_native_print_num(int64_t num) {
    std::cout << num << std::endl;
}
extern "C" void __lsm_native_print_float(double num) {
    std::cout << num << std::endl;
}

void X86_64Encoder::emitByte(uint8_t b) { code.push_back(b); }

void X86_64Encoder::emitInt32(int32_t val) {
    uint8_t* p = reinterpret_cast<uint8_t*>(&val);
    for (int i = 0; i < 4; i++) emitByte(p[i]);
}

void X86_64Encoder::emitInt64(int64_t val) {
    uint8_t* p = reinterpret_cast<uint8_t*>(&val);
    for (int i = 0; i < 8; i++) emitByte(p[i]);
}

void X86_64Encoder::emitDouble(double val) {
    uint8_t* p = reinterpret_cast<uint8_t*>(&val);
    for (int i = 0; i < 8; i++) emitByte(p[i]);
}

void X86_64Encoder::emitREX(bool w, bool r, bool x, bool b) {
    uint8_t rex = 0x40;
    if (w) rex |= 0x08; if (r) rex |= 0x04; if (x) rex |= 0x02; if (b) rex |= 0x01;
    emitByte(rex);
}

int X86_64Encoder::getStackOffset(const std::string& varName) {
    if (stackOffsets.find(varName) == stackOffsets.end()) {
        currentStackOffset -= 8;
        stackOffsets[varName] = currentStackOffset;
    }
    return stackOffsets[varName];
}

void X86_64Encoder::emitStackLoad(uint8_t reg, int offset) {
    bool isExt = (reg >= 8);
    uint8_t r = reg & 7;
    emitREX(true, isExt, false, false);
    if (offset >= -128 && offset <= 127) {
        emitByte(0x8B); emitByte(0x45 | (r << 3)); emitByte(static_cast<uint8_t>(offset));
    } else {
        emitByte(0x8B); emitByte(0x85 | (r << 3)); emitInt32(offset);
    }
}

void X86_64Encoder::emitStackStore(uint8_t reg, int offset) {
    bool isExt = (reg >= 8);
    uint8_t r = reg & 7;
    emitREX(true, isExt, false, false);
    if (offset >= -128 && offset <= 127) {
        emitByte(0x89); emitByte(0x45 | (r << 3)); emitByte(static_cast<uint8_t>(offset));
    } else {
        emitByte(0x89); emitByte(0x85 | (r << 3)); emitInt32(offset);
    }
}

void X86_64Encoder::emitValueLoad(uint8_t targetReg, const SSAValue& val) {
    if (val.isConstant) {
        bool isExt = (targetReg >= 8);
        emitREX(true, false, false, isExt);
        emitByte(0xB8 + (targetReg & 7));
        if (val.type == LsmStaticType::String) {
            g_StringPool.push_back(val.constStr);
            const char* strPtr = g_StringPool.back().c_str();
            emitInt64(reinterpret_cast<int64_t>(strPtr));
        } else {
            emitInt64(val.constNum);
        }
    } else {
        emitStackLoad(targetReg, getStackOffset(val.name));
    }
}

void X86_64Encoder::emitValueStore(uint8_t srcReg, const std::string& varName) {
    emitStackStore(srcReg, getStackOffset(varName));
}

void X86_64Encoder::emitXmmLoad(uint8_t xmmReg, int offset) {
    emitByte(0xF2); emitByte(0x0F); emitByte(0x10);
    if (offset >= -128 && offset <= 127) {
        emitByte(0x45 | (xmmReg << 3)); emitByte(static_cast<uint8_t>(offset));
    } else {
        emitByte(0x85 | (xmmReg << 3)); emitInt32(offset);
    }
}

void X86_64Encoder::emitXmmStore(uint8_t xmmReg, int offset) {
    emitByte(0xF2); emitByte(0x0F); emitByte(0x11);
    if (offset >= -128 && offset <= 127) {
        emitByte(0x45 | (xmmReg << 3)); emitByte(static_cast<uint8_t>(offset));
    } else {
        emitByte(0x85 | (xmmReg << 3)); emitInt32(offset);
    }
}

void X86_64Encoder::emitXmmValueLoad(uint8_t xmmReg, const SSAValue& val) {
    if (val.isConstant) {
        emitREX(true, false, false, false); emitByte(0xB8);
        emitDouble(val.constFloat);
        emitByte(0x66); emitREX(true, false, false, false);
        emitByte(0x0F); emitByte(0x6E); emitByte(0xC0 | (xmmReg << 3));
    } else {
        emitXmmLoad(xmmReg, getStackOffset(val.name));
    }
}

void X86_64Encoder::emitXmmValueStore(uint8_t xmmReg, const std::string& varName) {
    emitXmmStore(xmmReg, getStackOffset(varName));
}

void X86_64Encoder::registerExternalSymbol(const std::string& lib, const std::string& sym) {
    uintptr_t addr = dynamicLoader.resolveSymbol(lib, sym);
    if (addr) externalSymbols[sym] = addr;
}

uint8_t getX86NativeRegisterCode(const std::string& reg) {
    if (reg == "rax") return 0;
    if (reg == "rcx") return 1;
    if (reg == "rdx") return 2;
    if (reg == "rbx") return 3;
    if (reg == "rsp") return 4;
    if (reg == "rbp") return 5;
    if (reg == "rsi") return 6;
    if (reg == "rdi") return 7;
    return 0;
}

void X86_64Encoder::encodeInstruction(const SSAInstruction& inst) {
    switch (inst.op) {
        case SSAOp::Add:
        case SSAOp::Sub:
        case SSAOp::Mul: {
            emitValueLoad(0, inst.operands[0]);
            emitValueLoad(1, inst.operands[1]);
            emitREX(true, false, false, false);
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
            emitREX(true, false, false, false); emitByte(0x99); 
            emitREX(true, false, false, false); emitByte(0xF7); emitByte(0xF9); 
            uint8_t targetReg = (inst.op == SSAOp::Div) ? 0 : 2;
            if (!inst.result.name.empty()) emitValueStore(targetReg, inst.result.name);
            break;
        }

        case SSAOp::FAdd:
        case SSAOp::FSub:
        case SSAOp::FMul:
        case SSAOp::FDiv: {
            emitXmmValueLoad(0, inst.operands[0]);
            emitXmmValueLoad(1, inst.operands[1]);
            emitByte(0xF2); emitByte(0x0F);
            if (inst.op == SSAOp::FAdd) { emitByte(0x58); emitByte(0xC1); }
            else if (inst.op == SSAOp::FSub) { emitByte(0x5C); emitByte(0xC1); }
            else if (inst.op == SSAOp::FMul) { emitByte(0x59); emitByte(0xC1); }
            else if (inst.op == SSAOp::FDiv) { emitByte(0x5E); emitByte(0xC1); }
            if (!inst.result.name.empty()) emitXmmValueStore(0, inst.result.name);
            break;
        }

        case SSAOp::Copy: {
            if (inst.operands[0].type == LsmStaticType::Float64) {
                emitXmmValueLoad(0, inst.operands[0]);
                if (!inst.result.name.empty()) emitXmmValueStore(0, inst.result.name);
            } else {
                emitValueLoad(0, inst.operands[0]);
                if (!inst.result.name.empty()) emitValueStore(0, inst.result.name);
            }
            break;
        }

        case SSAOp::Equal: case SSAOp::NotEqual:
        case SSAOp::LessThan: case SSAOp::LessEqual:
        case SSAOp::GreaterThan: case SSAOp::GreaterEqual: {
            emitValueLoad(0, inst.operands[0]);
            emitValueLoad(1, inst.operands[1]);
            emitREX(true, false, false, false); emitByte(0x3B); emitByte(0xC1);
            emitByte(0x0F);
            switch (inst.op) {
                case SSAOp::Equal:        emitByte(0x94); break;
                case SSAOp::NotEqual:     emitByte(0x95); break;
                case SSAOp::LessThan:     emitByte(0x9C); break;
                case SSAOp::LessEqual:    emitByte(0x9E); break;
                case SSAOp::GreaterThan:  emitByte(0x9F); break;
                case SSAOp::GreaterEqual: emitByte(0x9D); break;
                default: break;
            }
            emitByte(0xC0);
            emitREX(true, false, false, false); emitByte(0x0F); emitByte(0xB6); emitByte(0xC0);
            if (!inst.result.name.empty()) emitValueStore(0, inst.result.name);
            break;
        }

        case SSAOp::FEqual: case SSAOp::FNotEqual:
        case SSAOp::FLessThan: case SSAOp::FLessEqual:
        case SSAOp::FGreaterThan: case SSAOp::FGreaterEqual: {
            emitXmmValueLoad(0, inst.operands[0]);
            emitXmmValueLoad(1, inst.operands[1]);
            emitByte(0x66); emitByte(0x0F); emitByte(0x2E); emitByte(0xC1); 
            emitByte(0x0F);
            switch (inst.op) {
                case SSAOp::FEqual:        emitByte(0x94); break;
                case SSAOp::FNotEqual:     emitByte(0x95); break;
                case SSAOp::FLessThan:     emitByte(0x92); break;
                case SSAOp::FLessEqual:    emitByte(0x96); break;
                case SSAOp::FGreaterThan:  emitByte(0x97); break;
                case SSAOp::FGreaterEqual: emitByte(0x93); break;
                default: break;
            }
            emitByte(0xC0);
            emitREX(true, false, false, false); emitByte(0x0F); emitByte(0xB6); emitByte(0xC0);
            if (!inst.result.name.empty()) emitValueStore(0, inst.result.name);
            break;
        }

        case SSAOp::ArrayAlloc: {
            int64_t count = inst.operands[0].constNum;
            int totalBytes = static_cast<int>(count * 8);
            currentStackOffset -= totalBytes;
            arrayBaseOffsets[inst.result.name] = currentStackOffset;
            break;
        }

        case SSAOp::ArrayLoad: {
            std::string arrName = inst.operands[0].name;
            int baseOff = arrayBaseOffsets[arrName];
            emitValueLoad(1, inst.operands[1]); 
            emitREX(true, false, false, false); emitByte(0xC1); emitByte(0xE1); emitByte(0x03); 
            emitREX(true, false, false, false); emitByte(0x8D); emitByte(0x85); emitInt32(baseOff); 
            emitREX(true, false, false, false); emitByte(0x01); emitByte(0xC8); 
            emitREX(true, false, false, false); emitByte(0x8B); emitByte(0x00); 
            if (!inst.result.name.empty()) emitValueStore(0, inst.result.name);
            break;
        }

        case SSAOp::ArrayStore: {
            std::string arrName = inst.operands[0].name;
            int baseOff = arrayBaseOffsets[arrName];
            emitValueLoad(1, inst.operands[1]); 
            emitValueLoad(2, inst.operands[2]); 
            emitREX(true, false, false, false); emitByte(0xC1); emitByte(0xE1); emitByte(0x03); 
            emitREX(true, false, false, false); emitByte(0x8D); emitByte(0x85); emitInt32(baseOff); 
            emitREX(true, false, false, false); emitByte(0x01); emitByte(0xC8); 
            emitREX(true, false, false, false); emitByte(0x89); emitByte(0x10); 
            break;
        }

        case SSAOp::SetRegister: {
            std::string regName = inst.operands[0].constStr;
            uint8_t rCode = getX86NativeRegisterCode(regName);
            emitValueLoad(rCode, inst.operands[1]);
            break;
        }

        case SSAOp::GetRegister: {
            std::string regName = inst.operands[0].constStr;
            uint8_t rCode = getX86NativeRegisterCode(regName);
            if (rCode != 0) {
                emitREX(true, false, false, false); emitByte(0x89); emitByte(0xC0 | (rCode << 3));
            }
            if (!inst.result.name.empty()) emitValueStore(0, inst.result.name);
            break;
        }

        case SSAOp::CpuHalt: {
            emitByte(0xF4);
            break;
        }

        case SSAOp::DisableInterrupts: {
            emitByte(0xFA);
            break;
        }

        case SSAOp::EnableInterrupts: {
            emitByte(0xFB);
            break;
        }

        case SSAOp::IOPortRead: {
            emitValueLoad(2, inst.operands[0]);
            emitByte(0xEC);
            emitREX(true, false, false, false); emitByte(0x0F); emitByte(0xB6); emitByte(0xC0);
            if (!inst.result.name.empty()) emitValueStore(0, inst.result.name);
            break;
        }

        case SSAOp::IOPortWrite: {
            emitValueLoad(2, inst.operands[0]);
            emitValueLoad(0, inst.operands[1]);
            emitByte(0xEE);
            break;
        }

        case SSAOp::VolatileLoad: {
            emitValueLoad(0, inst.operands[0]);
            emitREX(true, false, false, false); emitByte(0x8B); emitByte(0x00);
            if (!inst.result.name.empty()) emitValueStore(0, inst.result.name);
            break;
        }

        case SSAOp::VolatileStore: {
            emitValueLoad(0, inst.operands[0]);
            emitValueLoad(1, inst.operands[1]);
            emitREX(true, false, false, false); emitByte(0x89); emitByte(0x08);
            break;
        }

        case SSAOp::CondBranch: {
            emitValueLoad(0, inst.operands[0]);
            emitREX(true, false, false, false); emitByte(0x85); emitByte(0xC0);
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

#if defined(_WIN32) || defined(_WIN64)
            size_t totalArgs = inst.operands.size() - 1;
            size_t stackArgsCount = (totalArgs > 4) ? (totalArgs - 4) : 0;
            
            int shadowAlloc = static_cast<int>((4 + stackArgsCount) * 8);
            shadowAlloc = (shadowAlloc + 15) & ~15;

            emitREX(true, false, false, false); emitByte(0x81); emitByte(0xEC); emitInt32(shadowAlloc);

            
            for (size_t i = 5; i < inst.operands.size(); ++i) {
                int stackArgOffset = static_cast<int>((i - 1) * 8);
                emitValueLoad(0, inst.operands[i]);
                emitREX(true, false, false, false); emitByte(0x89); emitByte(0x84); emitByte(0x24); emitInt32(stackArgOffset);
            }

            
            for (size_t i = 1; i < inst.operands.size() && i <= 4; ++i) {
                size_t argIdx = i - 1;
                if (inst.operands[i].type == LsmStaticType::Float64) {
                    emitXmmValueLoad(static_cast<uint8_t>(argIdx), inst.operands[i]);
                } else {
                    if (argIdx == 0) emitValueLoad(1, inst.operands[i]); 
                    else if (argIdx == 1) emitValueLoad(2, inst.operands[i]); 
                    else if (argIdx == 2) {
                        emitValueLoad(0, inst.operands[i]);
                        emitREX(true, false, false, true); emitByte(0x89); emitByte(0xC0); 
                    } else if (argIdx == 3) {
                        emitValueLoad(0, inst.operands[i]);
                        emitREX(true, false, false, true); emitByte(0x89); emitByte(0xC1); 
                    }
                }
            }
#else
            uint8_t sysvGPR[4] = {7, 6, 2, 1};
            for (size_t i = 1; i < inst.operands.size() && i <= 6; ++i) {
                size_t argIdx = i - 1;
                if (inst.operands[i].type == LsmStaticType::Float64) {
                    emitXmmValueLoad(static_cast<uint8_t>(argIdx), inst.operands[i]);
                } else {
                    if (argIdx < 4) emitValueLoad(sysvGPR[argIdx], inst.operands[i]);
                    else if (argIdx == 4) {
                        emitValueLoad(0, inst.operands[i]);
                        emitREX(true, false, false, true); emitByte(0x89); emitByte(0xC0);
                    } else if (argIdx == 5) {
                        emitValueLoad(0, inst.operands[i]);
                        emitREX(true, false, false, true); emitByte(0x89); emitByte(0xC1);
                    }
                }
            }
            int shadowAlloc = 32;
            emitREX(true, false, false, false); emitByte(0x83); emitByte(0xEC); emitByte(0x20);
#endif

            uintptr_t funcPtr = 0;
            if (fn == "__lsm_native_print_num") funcPtr = reinterpret_cast<uintptr_t>(&__lsm_native_print_num);
            else if (fn == "__lsm_native_print_float") funcPtr = reinterpret_cast<uintptr_t>(&__lsm_native_print_float);
            else if (fn == "__lsm_native_print_str") funcPtr = reinterpret_cast<uintptr_t>(&__lsm_native_print_str);
            else if (externalSymbols.count(fn)) funcPtr = externalSymbols[fn];

            if (funcPtr != 0) {
                emitREX(true, false, false, false); emitByte(0xB8); emitInt64(static_cast<int64_t>(funcPtr));
                emitByte(0xFF); emitByte(0xD0); 
            } else {
                emitByte(0xE8); 
                jumpPatches.push_back({code.size(), fn});
                emitInt32(0);
            }

            emitREX(true, false, false, false); emitByte(0x81); emitByte(0xC4); emitInt32(shadowAlloc);

            if (!inst.result.name.empty()) {
                if (inst.result.type == LsmStaticType::Float64) {
                    emitXmmValueStore(0, inst.result.name);
                } else {
                    emitValueStore(0, inst.result.name);
                }
            }
            break;
        }

        case SSAOp::Return: {
            if (!inst.operands.empty()) {
                if (inst.operands[0].type == LsmStaticType::Float64) {
                    emitXmmValueLoad(0, inst.operands[0]);
                } else {
                    emitValueLoad(0, inst.operands[0]);
                }
            }
            emitREX(true, false, false, false); emitByte(0x89); emitByte(0xEC); 
            emitByte(0x5D); 
            emitByte(0xC3); 
            break;
        }

        default: break;
    }
}

void X86_64Encoder::encodeFunctionInternal(const SSAFunction& func) {
    stackOffsets.clear();
    currentStackOffset = 0;
    blockOffsets[func.name] = code.size();

    if (!func.isNaked) {
        emitByte(0x55); 
        emitREX(true, false, false, false); emitByte(0x89); emitByte(0xE5); 
        
        
        allocatedStackSize = 8192;
        emitREX(true, false, false, false); emitByte(0x81); emitByte(0xEC); emitInt32(allocatedStackSize);

#if defined(_WIN32) || defined(_WIN64)
        for (size_t i = 0; i < func.params.size(); ++i) {
            std::string pName = func.params[i].first;
            if (i < 4) {
                if (func.params[i].second == LsmStaticType::Float64) {
                    emitXmmValueStore(static_cast<uint8_t>(i), pName);
                } else {
                    if (i == 0) emitValueStore(1, pName); 
                    else if (i == 1) emitValueStore(2, pName); 
                    else if (i == 2) {
                        emitREX(true, true, false, false); emitByte(0x89); emitByte(0xC0); 
                        emitValueStore(0, pName);
                    } else if (i == 3) {
                        emitREX(true, true, false, false); emitByte(0x89); emitByte(0xC8); 
                        emitValueStore(0, pName);
                    }
                }
            } else {
                int callerOffset = 48 + static_cast<int>((i - 4) * 8);
                emitREX(true, false, false, false); emitByte(0x8B); emitByte(0x45); emitByte(static_cast<uint8_t>(callerOffset));
                emitValueStore(0, pName);
            }
        }
#else
        uint8_t sysvGPR[4] = {7, 6, 2, 1};
        for (size_t i = 0; i < func.params.size() && i < 6; ++i) {
            std::string pName = func.params[i].first;
            if (func.params[i].second == LsmStaticType::Float64) {
                emitXmmValueStore(static_cast<uint8_t>(i), pName);
            } else {
                if (i < 4) emitValueStore(sysvGPR[i], pName);
                else if (i == 4) {
                    emitREX(true, true, false, false); emitByte(0x89); emitByte(0xC0);
                    emitValueStore(0, pName);
                } else if (i == 5) {
                    emitREX(true, true, false, false); emitByte(0x89); emitByte(0xC8);
                    emitValueStore(0, pName);
                }
            }
        }
#endif
    }

    for (const auto& block : func.blocks) {
        blockOffsets[block.label] = code.size();
        for (const auto& inst : block.instructions) {
            encodeInstruction(inst);
        }
    }
}

std::vector<uint8_t> X86_64Encoder::encodeProgram(const std::vector<SSAFunction>& program) {
    code.clear();
    blockOffsets.clear();
    jumpPatches.clear();

    SSAFunction startFunc;
    SSAFunction mainFunc;
    std::vector<SSAFunction> others;

    for (const auto& f : program) {
        if (f.name == "_start") startFunc = f;
        else if (f.name == "main") mainFunc = f;
        else others.push_back(f);
    }

    if (!startFunc.name.empty()) encodeFunctionInternal(startFunc);
    if (!mainFunc.name.empty()) encodeFunctionInternal(mainFunc);
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