#ifndef LSM_OPT_AUTO_VECTORIZER_HPP
#define LSM_OPT_AUTO_VECTORIZER_HPP

#include "../../middleend/ssa/SSAInstruction.hpp"

class AutoVectorizer {
public:
    
    void optimize(SSAFunction& func) {
        for (auto& block : func.blocks) {
            auto it = block.instructions.begin();
            while (it != block.instructions.end()) {
                auto nextIt = it + 1;
                if (nextIt != block.instructions.end()) {
                    
                    if (it->op == SSAOp::FAdd && nextIt->op == SSAOp::FAdd) {
                        if (it->result.name != nextIt->operands[0].name && 
                            it->result.name != nextIt->operands[1].name) {
                            
                            
                            
                            SSAInstruction vecInst;
                            vecInst.op = SSAOp::Copy; 
                            vecInst.result = SSAValue::makeTemp(0, LsmStaticType::Float64);
                            vecInst.operands.push_back(it->operands[0]);
                            vecInst.operands.push_back(nextIt->operands[0]);
                            
                            *it = vecInst;
                            block.instructions.erase(nextIt);
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