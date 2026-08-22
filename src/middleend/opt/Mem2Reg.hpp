#ifndef LSM_OPT_MEM2REG_HPP
#define LSM_OPT_MEM2REG_HPP

#include "../../middleend/ssa/SSAInstruction.hpp"
#include "Dominance.hpp"
#include <unordered_set>
#include <map>

class Mem2Reg {
public:
    void optimize(SSAFunction& func, DominanceAnalyzer& domAnalyzer) {
        domAnalyzer.computeDominance(func);
        std::unordered_set<std::string> allocVars;

        
        for (const auto& block : func.blocks) {
            for (const auto& inst : block.instructions) {
                if (inst.op == SSAOp::AllocLocal) {
                    allocVars.insert(inst.result.name);
                }
            }
        }

        
        for (const auto& var : allocVars) {
            std::unordered_set<std::string> defBlocks;
            for (const auto& b : func.blocks) {
                for (const auto& i : b.instructions) {
                    if ((i.op == SSAOp::Copy || i.op == SSAOp::ArrayStore) && !i.result.name.empty()) {
                        defBlocks.insert(b.label);
                    }
                }
            }

            std::unordered_set<std::string> phiBlocks;
            std::vector<std::string> worklist(defBlocks.begin(), defBlocks.end());
            
            while (!worklist.empty()) {
                std::string n = worklist.back();
                worklist.pop_back();

                for (const auto& df : domAnalyzer.dominanceFrontiers[n]) {
                    if (!phiBlocks.count(df)) {
                        phiBlocks.insert(df);
                        injectPhiNode(func, df, var);
                        if (!defBlocks.count(df)) {
                            defBlocks.insert(df);
                            worklist.push_back(df);
                        }
                    }
                }
            }
        }
        
    }

private:
    void injectPhiNode(SSAFunction& func, const std::string& blockLabel, const std::string& varName) {
        for (auto& b : func.blocks) {
            if (b.label == blockLabel) {
                SSAInstruction phi;
                phi.op = SSAOp::Phi;
                phi.result = SSAValue::makeTemp(999, LsmStaticType::Int64); 
                b.instructions.insert(b.instructions.begin(), phi);
                break;
            }
        }
    }
};

#endif 