#ifndef LSM_OPT_PASS_MANAGER_HPP
#define LSM_OPT_PASS_MANAGER_HPP

#include "Dominance.hpp"
#include "DeadCodeElim.hpp"
#include "ConstantProp.hpp"
#include "LICM.hpp"
#include "AutoVectorizer.hpp"
#include "../../middleend/ssa/SSAInstruction.hpp"
#include <vector>

class OptimizationPassManager {
private:
    DominanceAnalyzer domAnalyzer;
    DeadCodeEliminator dce;
    ConstantPropagator cp;
    LICM licm;
    AutoVectorizer av;

public:
    void runOptimizations(std::vector<SSAFunction>& program, bool optimizeO3) {
        if (!optimizeO3) return;

        for (auto& func : program) {
            
            cp.optimize(func);

            
            dce.optimize(func);

            
            licm.optimize(func, domAnalyzer);

            
            av.optimize(func);

            
            dce.optimize(func);
        }
    }
};

#endif 