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
            case SSAOp::Phi:
                return true;
            default:
                return false;
        }
    }

    
    bool isInvariant(const SSAInstruction& inst, 
                     const std::vector<std::string>& loopBlocks, 
                     const std::unordered_map<std::string, std::string>& defMap) {
        
        
        if (hasSideEffects(inst.op)) {
            return false;
        }

        
        for (const auto& op : inst.operands) {
            if (op.isConstant) continue;

            auto defIt = defMap.find(op.name);
            if (defIt != defMap.end()) {
                
                if (std::find(loopBlocks.begin(), loopBlocks.end(), defIt->second) != loopBlocks.end()) {
                    return false;
                }
            }
        }
        return true;
    }

public:
    void optimize(SSAFunction& func, DominanceAnalyzer& domAnalyzer) {
        domAnalyzer.computeDominance(func);

        
        std::unordered_map<std::string, std::string> definedInBlock;
        for (const auto& b : func.blocks) {
            for (const auto& inst : b.instructions) {
                if (!inst.result.name.empty()) {
                    definedInBlock[inst.result.name] = b.label;
                }
            }
        }

        
        for (size_t i = 0; i < func.blocks.size(); ++i) {
            auto& header = func.blocks[i];
            
            std::vector<std::string> loopBlocks;
            bool isLoopHeader = false;

            
            for (const auto& pred : header.predecessors) {
                if (domAnalyzer.dominators[pred].count(header.label)) {
                    isLoopHeader = true;
                    loopBlocks.push_back(pred);
                }
            }

            if (isLoopHeader) {
                loopBlocks.push_back(header.label);
                std::string preHeader = header.predecessors.empty() ? "" : header.predecessors[0];

                for (const auto& lb : loopBlocks) {
                    auto blockIt = std::find_if(func.blocks.begin(), func.blocks.end(), 
                                                [&](const SSABasicBlock& b){ return b.label == lb; });
                    if (blockIt == func.blocks.end()) continue;

                    auto instIt = blockIt->instructions.begin();
                    while (instIt != blockIt->instructions.end()) {
                        if (isInvariant(*instIt, loopBlocks, definedInBlock)) {
                            if (!preHeader.empty()) {
                                auto preIt = std::find_if(func.blocks.begin(), func.blocks.end(), 
                                                          [&](const SSABasicBlock& b){ return b.label == preHeader; });
                                if (preIt != func.blocks.end()) {
                                    
                                    auto insertPos = preIt->instructions.end();
                                    if (!preIt->instructions.empty() && 
                                       (preIt->instructions.back().op == SSAOp::Branch || 
                                        preIt->instructions.back().op == SSAOp::CondBranch)) {
                                        insertPos--;
                                    }
                                    preIt->instructions.insert(insertPos, *instIt);
                                    instIt = blockIt->instructions.erase(instIt);
                                    continue;
                                }
                            }
                        }
                        ++instIt;
                    }
                }
            }
        }
    }
};

#endif 