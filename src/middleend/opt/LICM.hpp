#ifndef LSM_OPT_LICM_HPP
#define LSM_OPT_LICM_HPP

#include "../../middleend/ssa/SSAInstruction.hpp"
#include "Dominance.hpp"
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <algorithm>

class LICM {
private:
    bool hasSideEffects(SSAOp op) const {
        switch (op) {
            case SSAOp::Call: case SSAOp::Return: case SSAOp::ArrayStore:
            case SSAOp::ArrayLoad: case SSAOp::VolatileStore: case SSAOp::VolatileLoad:
            case SSAOp::IOPortWrite: case SSAOp::IOPortRead: case SSAOp::SetRegister:
            case SSAOp::CpuHalt: case SSAOp::DisableInterrupts: case SSAOp::EnableInterrupts:
            case SSAOp::Branch: case SSAOp::CondBranch: case SSAOp::Phi:
                return true;
            default:
                return false;
        }
    }

public:
    void optimize(SSAFunction& func, DominanceAnalyzer& domAnalyzer) {
        domAnalyzer.computeDominance(func);

        std::unordered_map<std::string, std::string> defBlockMap;
        for (const auto& b : func.blocks) {
            for (const auto& inst : b.instructions) {
                if (!inst.result.name.empty()) {
                    defBlockMap[inst.result.name] = b.label;
                }
            }
        }

        for (size_t i = 0; i < func.blocks.size(); ++i) {
            const auto& header = func.blocks[i];
            std::unordered_set<std::string> loopBlocks;
            bool isLoopHeader = false;

            for (const auto& pred : header.predecessors) {
                if (domAnalyzer.dominators[pred].count(header.label)) {
                    isLoopHeader = true;
                    loopBlocks.insert(pred);
                }
            }

            if (!isLoopHeader) continue;
            loopBlocks.insert(header.label);

            std::string preHeader = "";
            for (const auto& pred : header.predecessors) {
                if (!loopBlocks.count(pred)) {
                    preHeader = pred;
                    break;
                }
            }
            if (preHeader.empty()) continue;

            auto preIt = std::find_if(func.blocks.begin(), func.blocks.end(), 
                                      [&](const SSABasicBlock& b){ return b.label == preHeader; });
            if (preIt == func.blocks.end()) continue;

            std::unordered_set<std::string> loopDefinedVars;
            for (const auto& b : func.blocks) {
                if (loopBlocks.count(b.label)) {
                    for (const auto& inst : b.instructions) {
                        if (!inst.result.name.empty()) {
                            loopDefinedVars.insert(inst.result.name);
                        }
                    }
                }
            }

            std::vector<SSAInstruction> invariantInsts;
            bool changed = true;

            while (changed) {
                changed = false;
                for (auto& block : func.blocks) {
                    if (!loopBlocks.count(block.label)) continue;

                    auto it = block.instructions.begin();
                    while (it != block.instructions.end()) {
                        if (hasSideEffects(it->op) || it->result.name.empty()) {
                            ++it;
                            continue;
                        }

                        bool allOperandsInvariant = true;
                        for (const auto& op : it->operands) {
                            if (op.isConstant) continue;
                            if (loopDefinedVars.count(op.name)) {
                                allOperandsInvariant = false;
                                break;
                            }
                        }

                        if (allOperandsInvariant) {
                            invariantInsts.push_back(*it);
                            loopDefinedVars.erase(it->result.name);
                            defBlockMap[it->result.name] = preHeader;
                            it = block.instructions.erase(it);
                            changed = true;
                        } else {
                            ++it;
                        }
                    }
                }
            }

            if (!invariantInsts.empty()) {
                auto insertPos = preIt->instructions.end();
                if (!preIt->instructions.empty() && 
                   (preIt->instructions.back().op == SSAOp::Branch || 
                    preIt->instructions.back().op == SSAOp::CondBranch)) {
                    --insertPos;
                }
                preIt->instructions.insert(insertPos, invariantInsts.begin(), invariantInsts.end());
            }
        }
    }
};

#endif