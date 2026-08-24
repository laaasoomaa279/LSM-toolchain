#ifndef LSM_OPT_MEM2REG_HPP
#define LSM_OPT_MEM2REG_HPP

#include "../../middleend/ssa/SSAInstruction.hpp"
#include "Dominance.hpp"
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <stack>
#include <string>
#include <algorithm>

class Mem2Reg {
private:
    std::unordered_map<std::string, std::stack<SSAValue>> varStacks;
    std::unordered_map<std::string, int> varCounters;
    std::unordered_map<std::string, std::vector<std::string>> domTreeChildren;

public:
    void optimize(SSAFunction& func, DominanceAnalyzer& domAnalyzer) {
        domAnalyzer.computeDominance(func);
        std::unordered_set<std::string> allocVars;
        std::unordered_map<std::string, LsmStaticType> varTypes;
        std::unordered_set<std::string> reassignedVars;

        for (const auto& block : func.blocks) {
            for (const auto& inst : block.instructions) {
                if (inst.op == SSAOp::AllocLocal && !inst.result.name.empty()) {
                    allocVars.insert(inst.result.name);
                    varTypes[inst.result.name] = inst.result.type;
                }
            }
        }

        if (allocVars.empty()) return;

        std::unordered_map<std::string, int> assignCount;
        for (const auto& b : func.blocks) {
            for (const auto& i : b.instructions) {
                if (i.op == SSAOp::Copy && allocVars.count(i.result.name)) {
                    assignCount[i.result.name]++;
                }
            }
        }
        for (const auto& pair : assignCount) {
            if (pair.second > 1) reassignedVars.insert(pair.first);
        }

        for (const auto& var : allocVars) {
            if (reassignedVars.count(var)) continue;

            std::unordered_set<std::string> defBlocks;
            for (const auto& b : func.blocks) {
                for (const auto& i : b.instructions) {
                    if (i.op == SSAOp::Copy && i.result.name == var) {
                        defBlocks.insert(b.label);
                    }
                }
            }

            std::unordered_set<std::string> phiBlocks;
            std::vector<std::string> worklist(defBlocks.begin(), defBlocks.end());
            
            while (!worklist.empty()) {
                std::string n = worklist.back();
                worklist.pop_back();

                const auto& domFrontiers = domAnalyzer.getDominanceFrontiers();
                auto it = domFrontiers.find(n);
                if (it != domFrontiers.end()) {
                    for (const auto& df : it->second) {
                        if (!phiBlocks.count(df)) {
                            phiBlocks.insert(df);
                            injectPhiNode(func, df, var, varTypes[var]);
                            if (!defBlocks.count(df)) {
                                defBlocks.insert(df);
                                worklist.push_back(df);
                            }
                        }
                    }
                }
            }
        }

        domTreeChildren.clear();
        for (const auto& pair : domAnalyzer.idom) {
            if (pair.first != pair.second && !pair.second.empty()) {
                domTreeChildren[pair.second].push_back(pair.first);
            }
        }

        varStacks.clear();
        varCounters.clear();
        for (const auto& var : allocVars) {
            varCounters[var] = 0;
            varStacks[var].push(SSAValue::makeTemp(0, varTypes[var]));
        }

        if (!func.blocks.empty()) {
            renameVariables(func, func.blocks[0].label, allocVars, reassignedVars);
        }

        for (auto& block : func.blocks) {
            auto it = block.instructions.begin();
            while (it != block.instructions.end()) {
                if (it->op == SSAOp::AllocLocal && allocVars.count(it->result.name) && !reassignedVars.count(it->result.name)) {
                    it = block.instructions.erase(it);
                } else if (it->op == SSAOp::Copy && allocVars.count(it->result.name) && !reassignedVars.count(it->result.name)) {
                    it = block.instructions.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

private:
    void injectPhiNode(SSAFunction& func, const std::string& blockLabel, const std::string& varName, LsmStaticType type) {
        for (auto& b : func.blocks) {
            if (b.label == blockLabel) {
                for (const auto& inst : b.instructions) {
                    if (inst.op == SSAOp::Phi && inst.result.name == varName) return;
                }
                SSAInstruction phi;
                phi.op = SSAOp::Phi;
                phi.result = SSAValue::makeTemp(999, type);
                phi.result.name = varName;
                b.instructions.insert(b.instructions.begin(), phi);
                break;
            }
        }
    }

    void renameVariables(SSAFunction& func, const std::string& blockLabel, 
                         const std::unordered_set<std::string>& allocVars,
                         const std::unordered_set<std::string>& reassignedVars) {
        std::unordered_map<std::string, int> pushedCounts;

        auto blockIt = std::find_if(func.blocks.begin(), func.blocks.end(), 
                                    [&](const SSABasicBlock& b) { return b.label == blockLabel; });
        if (blockIt == func.blocks.end()) return;

        for (auto& inst : blockIt->instructions) {
            if (inst.op != SSAOp::Phi && inst.op != SSAOp::AllocLocal) {
                for (auto& op : inst.operands) {
                    if (!op.isConstant && allocVars.count(op.name) && !reassignedVars.count(op.name)) {
                        if (!varStacks[op.name].empty()) {
                            op = varStacks[op.name].top();
                        }
                    }
                }
            }

            if (inst.op == SSAOp::Copy && allocVars.count(inst.result.name) && !reassignedVars.count(inst.result.name)) {
                int count = ++varCounters[inst.result.name];
                SSAValue newDef = SSAValue::makeTemp(count + 10000, inst.result.type);
                newDef.name = inst.result.name + "_" + std::to_string(count);
                varStacks[inst.result.name].push(newDef);
                pushedCounts[inst.result.name]++;
            } else if (inst.op == SSAOp::Phi && allocVars.count(inst.result.name) && !reassignedVars.count(inst.result.name)) {
                int count = ++varCounters[inst.result.name];
                SSAValue newDef = SSAValue::makeTemp(count + 10000, inst.result.type);
                newDef.name = inst.result.name + "_" + std::to_string(count);
                varStacks[inst.result.name].push(newDef);
                pushedCounts[inst.result.name]++;
                inst.result = newDef;
            }
        }

        for (const auto& succLabel : blockIt->successors) {
            auto succIt = std::find_if(func.blocks.begin(), func.blocks.end(), 
                                       [&](const SSABasicBlock& b) { return b.label == succLabel; });
            if (succIt != func.blocks.end()) {
                for (auto& inst : succIt->instructions) {
                    if (inst.op == SSAOp::Phi) {
                        size_t pos = inst.result.name.find('_');
                        std::string originalVarName = (pos != std::string::npos) ? inst.result.name.substr(0, pos) : inst.result.name;
                        std::string targetVar = allocVars.count(originalVarName) ? originalVarName : inst.result.name;
                        if (allocVars.count(targetVar) && !reassignedVars.count(targetVar) && !varStacks[targetVar].empty()) {
                            SSAValue topVal = varStacks[targetVar].top();
                            inst.operands.push_back(topVal);
                        }
                    }
                }
            }
        }

        for (const auto& child : domTreeChildren[blockLabel]) {
            renameVariables(func, child, allocVars, reassignedVars);
        }

        for (const auto& pair : pushedCounts) {
            for (int i = 0; i < pair.second; ++i) {
                if (!varStacks[pair.first].empty()) {
                    varStacks[pair.first].pop();
                }
            }
        }
    }
};

#endif