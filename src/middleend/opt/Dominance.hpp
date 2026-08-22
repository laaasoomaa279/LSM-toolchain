#ifndef LSM_OPT_DOMINANCE_HPP
#define LSM_OPT_DOMINANCE_HPP

#include "../../middleend/ssa/SSAInstruction.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <algorithm>

class DominanceAnalyzer {
public:
    std::unordered_map<std::string, std::unordered_set<std::string>> dominators;
    std::unordered_map<std::string, std::string> idom; 
    std::unordered_map<std::string, std::unordered_set<std::string>> dominanceFrontiers;

    void buildCFG(SSAFunction& func) {
        for (auto& block : func.blocks) {
            block.predecessors.clear();
            block.successors.clear();
        }
        for (auto& block : func.blocks) {
            for (const auto& inst : block.instructions) {
                if (inst.op == SSAOp::Branch) {
                    block.successors.push_back(inst.operands[0].constStr);
                } else if (inst.op == SSAOp::CondBranch) {
                    block.successors.push_back(inst.operands[1].constStr);
                    block.successors.push_back(inst.operands[2].constStr);
                }
            }
        }
        for (const auto& block : func.blocks) {
            for (const auto& succ : block.successors) {
                for (auto& b : func.blocks) {
                    if (b.label == succ) b.predecessors.push_back(block.label);
                }
            }
        }
    }

    void computeDominance(SSAFunction& func) {
        buildCFG(func);
        dominators.clear(); idom.clear(); dominanceFrontiers.clear();
        if (func.blocks.empty()) return;

        std::unordered_set<std::string> allBlocks;
        for (const auto& b : func.blocks) allBlocks.insert(b.label);

        std::string entry = func.blocks[0].label;
        dominators[entry] = {entry};

        for (size_t i = 1; i < func.blocks.size(); ++i) {
            dominators[func.blocks[i].label] = allBlocks;
        }

        bool changed = true;
        while (changed) {
            changed = false;
            for (size_t i = 1; i < func.blocks.size(); ++i) {
                const auto& b = func.blocks[i];
                std::unordered_set<std::string> newDom = allBlocks;
                for (const auto& pred : b.predecessors) {
                    std::unordered_set<std::string> inter;
                    for (const auto& d : dominators[pred]) {
                        if (newDom.count(d)) inter.insert(d);
                    }
                    newDom = inter;
                }
                newDom.insert(b.label);
                if (newDom != dominators[b.label]) {
                    dominators[b.label] = newDom;
                    changed = true;
                }
            }
        }
        computeImmediateDominators(func);
        computeDominanceFrontiers(func);
    }

    const std::unordered_map<std::string, std::unordered_set<std::string>>& getDominanceFrontiers() const {
        return dominanceFrontiers;
    }

private:
    void computeImmediateDominators(const SSAFunction& func) {
        for (const auto& b : func.blocks) {
            std::string n = b.label;
            for (const auto& dom : dominators[n]) {
                if (dom != n) {
                    bool isImmediate = true;
                    for (const auto& otherDom : dominators[n]) {
                        if (otherDom != n && otherDom != dom && dominators[otherDom].count(dom)) {
                            isImmediate = false; break;
                        }
                    }
                    if (isImmediate) idom[n] = dom;
                }
            }
        }
    }

    void computeDominanceFrontiers(const SSAFunction& func) {
        for (const auto& b : func.blocks) {
            if (b.predecessors.size() >= 2) {
                for (const auto& pred : b.predecessors) {
                    std::string runner = pred;
                    while (runner != idom[b.label] && !runner.empty()) {
                        dominanceFrontiers[runner].insert(b.label);
                        runner = idom[runner];
                    }
                }
            }
        }
    }
};

#endif 