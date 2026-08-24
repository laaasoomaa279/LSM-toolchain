#ifndef LSM_OPT_PASS_MANAGER_HPP
#define LSM_OPT_PASS_MANAGER_HPP

#include "Dominance.hpp"
#include "Mem2Reg.hpp"
#include "ConstantProp.hpp"
#include "DeadCodeElim.hpp"
#include "LICM.hpp"
#include "AutoVectorizer.hpp"
#include "../../middleend/ssa/SSAInstruction.hpp"
#include <vector>

class OptimizationPassManager {
private:
    DominanceAnalyzer domAnalyzer;
    Mem2Reg mem2reg;
    ConstantPropagator cp;
    DeadCodeEliminator dce;
    LICM licm;
    AutoVectorizer av;

public:
    void runOptimizations(std::vector<SSAFunction>& program, bool optimizeO3) {
        if (!optimizeO3) return;

        for (auto& func : program) {
            mem2reg.optimize(func, domAnalyzer);
            cp.optimize(func);
            dce.optimize(func);
            licm.optimize(func, domAnalyzer);
            av.optimize(func);
            cp.optimize(func);
            dce.optimize(func);
        }
    }
};

#endif