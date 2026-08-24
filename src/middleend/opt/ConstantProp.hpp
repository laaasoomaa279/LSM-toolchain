#ifndef LSM_OPT_CONSTANT_PROP_HPP
#define LSM_OPT_CONSTANT_PROP_HPP

#include "../../middleend/ssa/SSAInstruction.hpp"
#include <unordered_map>
#include <unordered_set>

class ConstantPropagator {
private:
    bool tryFold(SSAOp op, const SSAValue& left, const SSAValue& right, SSAValue& foldedResult) {
        if (!left.isConstant || !right.isConstant) return false;
        
        if (left.type == LsmStaticType::Int64 && right.type == LsmStaticType::Int64) {
            int64_t l = left.constNum;
            int64_t r = right.constNum;
            switch (op) {
                case SSAOp::Add: foldedResult = SSAValue::makeConstNum(l + r); return true;
                case SSAOp::Sub: foldedResult = SSAValue::makeConstNum(l - r); return true;
                case SSAOp::Mul: foldedResult = SSAValue::makeConstNum(l * r); return true;
                case SSAOp::Div: foldedResult = SSAValue::makeConstNum(r != 0 ? l / r : 0); return true;
                default: return false;
            }
        }
        return false;
    }

public:
    void optimize(SSAFunction& func) {
        std::unordered_map<std::string, int> assignCounts;
        for (const auto& block : func.blocks) {
            for (const auto& inst : block.instructions) {
                if (!inst.result.name.empty()) {
                    assignCounts[inst.result.name]++;
                }
            }
        }

        std::unordered_map<std::string, SSAValue> constantTable;
        for (const auto& block : func.blocks) {
            for (const auto& inst : block.instructions) {
                if (inst.op == SSAOp::Copy && inst.operands[0].isConstant) {
                    if (assignCounts[inst.result.name] == 1) {
                        constantTable[inst.result.name] = inst.operands[0];
                    }
                }
            }
        }

        for (auto& block : func.blocks) {
            for (auto& inst : block.instructions) {
                for (auto& op : inst.operands) {
                    if (!op.isConstant && constantTable.count(op.name)) {
                        op = constantTable[op.name];
                    }
                }

                if (inst.operands.size() == 2) {
                    SSAValue folded;
                    if (tryFold(inst.op, inst.operands[0], inst.operands[1], folded)) {
                        inst.op = SSAOp::Copy;
                        inst.operands = {folded};
                    }
                }
            }
        }
    }
};

#endif