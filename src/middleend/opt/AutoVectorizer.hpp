#ifndef LSM_OPT_AUTO_VECTORIZER_HPP
#define LSM_OPT_AUTO_VECTORIZER_HPP

#include "../../middleend/ssa/SSAInstruction.hpp"
#include <vector>
#include <string>

class AutoVectorizer {
private:
    int vectorCounter = 70000;

public:
    void optimize(SSAFunction& func) {
        for (auto& block : func.blocks) {
            if (block.instructions.size() < 2) continue;

            auto it = block.instructions.begin();
            while (it != block.instructions.end()) {
                auto nextIt = it + 1;
                if (nextIt != block.instructions.end()) {
                    if (it->op == SSAOp::FAdd && nextIt->op == SSAOp::FAdd) {
                        if (it->result.name != nextIt->operands[0].name &&
                            it->result.name != nextIt->operands[1].name &&
                            !it->result.name.empty() && !nextIt->result.name.empty()) {

                            SSAInstruction vecInst;
                            vecInst.op = SSAOp::VectorAdd; 
                            vecInst.result = SSAValue::makeTemp(++vectorCounter, LsmStaticType::Float64);
                            vecInst.result.name = "vec128_fadd_" + std::to_string(vectorCounter);
                            vecInst.operands.push_back(it->operands[0]);
                            vecInst.operands.push_back(it->operands[1]);
                            vecInst.operands.push_back(nextIt->operands[0]);
                            vecInst.operands.push_back(nextIt->operands[1]);

                            *it = vecInst;
                            it = block.instructions.erase(nextIt);
                            continue;
                        }
                    }
                }
                ++it;
            }
        }
    }
};

#endif