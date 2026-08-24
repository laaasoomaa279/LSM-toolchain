#ifndef LSM_OPT_DEAD_CODE_ELIM_HPP
#define LSM_OPT_DEAD_CODE_ELIM_HPP

#include "../../middleend/ssa/SSAInstruction.hpp"
#include <unordered_set>
#include <string>

class DeadCodeEliminator {
public:
    static bool hasSideEffects(SSAOp op) {
        switch (op) {
            case SSAOp::Call:
            case SSAOp::Return:
            case SSAOp::ArrayStore:
            case SSAOp::VolatileStore:
            case SSAOp::VolatileLoad:
            case SSAOp::IOPortWrite:
            case SSAOp::IOPortRead:
            case SSAOp::SetRegister:
            case SSAOp::CpuHalt:
            case SSAOp::DisableInterrupts:
            case SSAOp::EnableInterrupts:
            case SSAOp::Branch:
            case SSAOp::CondBranch:
                return true;
            default:
                return false;
        }
    }

    void optimize(SSAFunction& func) {
        bool changed = true;
        while (changed) {
            changed = false;
            std::unordered_set<std::string> usedVars;

            
            for (const auto& block : func.blocks) {
                for (const auto& inst : block.instructions) {
                    for (const auto& op : inst.operands) {
                        if (!op.isConstant && !op.name.empty()) {
                            usedVars.insert(op.name);
                        }
                    }
                }
            }

            
            for (auto& block : func.blocks) {
                auto it = block.instructions.begin();
                while (it != block.instructions.end()) {
                    if (!it->result.name.empty() && !usedVars.count(it->result.name)) {
                        if (!hasSideEffects(it->op)) {
                            it = block.instructions.erase(it);
                            changed = true;
                            continue;
                        }
                    }
                    ++it;
                }
            }
        }
    }
};

#endif 