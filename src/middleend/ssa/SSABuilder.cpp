#include "SSABuilder.hpp"
#include <stdexcept>

SSABasicBlock* SSABuilder::getCurrentBlock() {
    for (auto& block : currentFunc.blocks) {
        if (block.label == currentBlockLabel) return &block;
    }
    return nullptr;
}

SSABasicBlock* SSABuilder::getBlockByLabel(const std::string& label) {
    for (auto& block : currentFunc.blocks) {
        if (block.label == label) return &block;
    }
    return nullptr;
}

void SSABuilder::emit(SSAInstruction inst) {
    auto* block = getCurrentBlock();
    if (block) {
        block->instructions.push_back(inst);
        if (inst.op == SSAOp::Copy || inst.op == SSAOp::Add || inst.op == SSAOp::Sub) {
            if (!inst.result.name.empty()) {
                variableDefBlocks[inst.result.name].insert(currentBlockLabel);
            }
        }
    }
}

void SSABuilder::insertPhiNodes(SSAFunction& func, const DominanceAnalyzer& dom) {
    auto frontiers = dom.getDominanceFrontiers();
    
    for (const auto& pair : variableDefBlocks) {
        std::string varName = pair.first;
        const auto& defBlocks = pair.second;
        
        std::queue<std::string> workList;
        std::unordered_set<std::string> inWorkList;
        std::unordered_set<std::string> insertedPhi;

        for (const auto& b : defBlocks) {
            workList.push(b);
            inWorkList.insert(b);
        }

        while (!workList.empty()) {
            std::string n = workList.front();
            workList.pop();

            if (frontiers.count(n)) {
                for (const auto& df : frontiers.at(n)) {
                    if (insertedPhi.find(df) == insertedPhi.end()) {
                        SSABasicBlock* targetBlock = getBlockByLabel(df);
                        if (targetBlock) {
                            SSAValue varVal = varVersionTable[varName]; 
                            targetBlock->instructions.insert(targetBlock->instructions.begin(), 
                                SSAInstruction{SSAOp::Phi, varVal, {}});
                        }
                        insertedPhi.insert(df);
                        
                        if (inWorkList.find(df) == inWorkList.end()) {
                            workList.push(df);
                            inWorkList.insert(df);
                        }
                    }
                }
            }
        }
    }
}

bool SSABuilder::tryFoldBinary(SSAOp op, const SSAValue& left, const SSAValue& right, SSAValue& foldedResult) {
    if (!left.isConstant || !right.isConstant) return false;

    if ((left.type == LsmStaticType::Int64 || left.type == LsmStaticType::Int32 || left.type == LsmStaticType::Int16 || left.type == LsmStaticType::Int8) && 
        (right.type == LsmStaticType::Int64 || right.type == LsmStaticType::Int32 || right.type == LsmStaticType::Int16 || right.type == LsmStaticType::Int8)) {
        int64_t l = left.constNum;
        int64_t r = right.constNum;
        LsmStaticType intT = getDefaultIntType();
        switch (op) {
            case SSAOp::Add:          foldedResult = SSAValue::makeConstNum(l + r, intT); return true;
            case SSAOp::Sub:          foldedResult = SSAValue::makeConstNum(l - r, intT); return true;
            case SSAOp::Mul:          foldedResult = SSAValue::makeConstNum(l * r, intT); return true;
            case SSAOp::Div:          foldedResult = SSAValue::makeConstNum(r != 0 ? l / r : 0, intT); return true;
            case SSAOp::Mod:          foldedResult = SSAValue::makeConstNum(r != 0 ? l % r : 0, intT); return true;
            case SSAOp::BitAnd:       foldedResult = SSAValue::makeConstNum(l & r, intT); return true;
            case SSAOp::BitOr:        foldedResult = SSAValue::makeConstNum(l | r, intT); return true;
            case SSAOp::BitXor:       foldedResult = SSAValue::makeConstNum(l ^ r, intT); return true;
            case SSAOp::Shl:          foldedResult = SSAValue::makeConstNum(l << r, intT); return true;
            case SSAOp::Shr:          foldedResult = SSAValue::makeConstNum(l >> r, intT); return true;
            case SSAOp::Equal:        foldedResult = SSAValue::makeConstNum(l == r ? 1 : 0, intT); return true;
            case SSAOp::NotEqual:     foldedResult = SSAValue::makeConstNum(l != r ? 1 : 0, intT); return true;
            case SSAOp::LessThan:     foldedResult = SSAValue::makeConstNum(l < r ? 1 : 0, intT); return true;
            case SSAOp::LessEqual:    foldedResult = SSAValue::makeConstNum(l <= r ? 1 : 0, intT); return true;
            case SSAOp::GreaterThan:  foldedResult = SSAValue::makeConstNum(l > r ? 1 : 0, intT); return true;
            case SSAOp::GreaterEqual: foldedResult = SSAValue::makeConstNum(l >= r ? 1 : 0, intT); return true;
            default: return false;
        }
    }

    if (left.type == LsmStaticType::Float64 && right.type == LsmStaticType::Float64) {
        double l = left.constFloat;
        double r = right.constFloat;
        switch (op) {
            case SSAOp::FAdd: foldedResult = SSAValue::makeConstFloat(l + r); return true;
            case SSAOp::FSub: foldedResult = SSAValue::makeConstFloat(l - r); return true;
            case SSAOp::FMul: foldedResult = SSAValue::makeConstFloat(l * r); return true;
            case SSAOp::FDiv: foldedResult = SSAValue::makeConstFloat(r != 0.0 ? l / r : 0.0); return true;
            default: return false;
        }
    }
    return false;
}

SSAFunction SSABuilder::buildFunction(FuncDeclNode* funcNode) {
    tempCounter = 0;
    blockCounter = 0;
    variableDefBlocks.clear();
    deferStack.clear();

    escapeAnalyzer.analyzeFunction(funcNode);

    currentFunc.name = funcNode->name;
    currentFunc.isNaked = funcNode->isNaked;
    currentFunc.isInline = funcNode->isInline;
    currentFunc.blocks.clear();
    currentFunc.params.clear();

    for (const auto& p : funcNode->params) {
        LsmStaticType pType = (p.type == "Float" || p.type == "float" || p.type == "f64") ? LsmStaticType::Float64 : getDefaultIntType();
        currentFunc.params.push_back({p.name, pType});
    }

    if (funcNode->returnType == "Float" || funcNode->returnType == "float" || funcNode->returnType == "f64") {
        currentFunc.returnType = LsmStaticType::Float64;
    } else {
        currentFunc.returnType = getDefaultIntType();
    }

    SSABasicBlock entryBB;
    entryBB.label = funcNode->name + "_entry";
    currentFunc.blocks.push_back(entryBB);
    currentBlockLabel = entryBB.label;

    if (!funcNode->isNaked) {
        emit({SSAOp::RegionEnter, {}, {SSAValue::makeConstStr(funcNode->name)}});
    }

    for (const auto& p : currentFunc.params) {
        SSAValue pVal = SSAValue::makeVar(p.first, p.second);
        varVersionTable[p.first] = pVal;
        variableDefBlocks[p.first].insert(currentBlockLabel);
    }

    if (funcNode->body) buildStatement(funcNode->body.get());

    auto* curBB = getCurrentBlock();
    if (curBB && (curBB->instructions.empty() || curBB->instructions.back().op != SSAOp::Return)) {
        for (auto it = deferStack.rbegin(); it != deferStack.rend(); ++it) {
            buildStatement(*it);
        }
        if (!funcNode->isNaked) {
            emit({SSAOp::RegionExit, {}, {}});
        }
        SSAInstruction retInst;
        retInst.op = SSAOp::Return;
        retInst.operands = {currentFunc.returnType == LsmStaticType::Float64 ? SSAValue::makeConstFloat(0.0) : SSAValue::makeConstNum(0, getDefaultIntType())};
        curBB->instructions.push_back(retInst);
    }

    dominanceAnalyzer.computeDominance(currentFunc);
    insertPhiNodes(currentFunc, dominanceAnalyzer);
    return currentFunc;
}

std::vector<SSAFunction> SSABuilder::buildProgram(ProgramNode* root) {
    std::vector<SSAFunction> programSSA;
    if (!root) return programSSA;

    functionReturnTypes.clear();

    for (const auto& stmt : root->stmts) {
        if (stmt && stmt->type == ASTNodeType::VarAssign) {
            auto vAssign = static_cast<VarAssignNode*>(stmt.get());
            if (vAssign->value && vAssign->value->type == ASTNodeType::IntLit) {
                int64_t constVal = static_cast<IntLitNode*>(vAssign->value.get())->val;
                varVersionTable[vAssign->name] = SSAValue::makeConstNum(constVal, getDefaultIntType());
            }
        }
    }

    for (const auto& stmt : root->stmts) {
        if (stmt && stmt->type == ASTNodeType::FuncDecl) {
            auto fn = static_cast<FuncDeclNode*>(stmt.get());
            LsmStaticType retType = getDefaultIntType();
            if (fn->returnType == "Float" || fn->returnType == "float" || fn->returnType == "f64") retType = LsmStaticType::Float64;
            else if (fn->returnType == "String" || fn->returnType == "str") retType = LsmStaticType::String;
            else if (fn->returnType == "void" || fn->returnType.empty()) retType = LsmStaticType::Void;
            functionReturnTypes[fn->name] = retType;
        } else if (stmt && stmt->type == ASTNodeType::ExternFuncDecl) {
            auto fn = static_cast<ExternFuncDeclNode*>(stmt.get());
            LsmStaticType retType = getDefaultIntType();
            if (fn->returnType == "Float" || fn->returnType == "float" || fn->returnType == "f64") retType = LsmStaticType::Float64;
            else if (fn->returnType == "String" || fn->returnType == "str") retType = LsmStaticType::String;
            else if (fn->returnType == "void" || fn->returnType.empty()) retType = LsmStaticType::Void;
            functionReturnTypes[fn->name] = retType;
        }
    }

    for (auto& stmt : root->stmts) {
        if (stmt && stmt->type == ASTNodeType::FuncDecl) {
            auto fn = static_cast<FuncDeclNode*>(stmt.get());
            if (fn->name == entryPoint) {
                programSSA.push_back(buildFunction(fn));
                break;
            }
        }
    }

    for (auto& stmt : root->stmts) {
        if (stmt && stmt->type == ASTNodeType::FuncDecl) {
            auto fn = static_cast<FuncDeclNode*>(stmt.get());
            if (fn->name != entryPoint) {
                programSSA.push_back(buildFunction(fn));
            }
        }
    }

    return programSSA;
}

SSAValue SSABuilder::buildExpr(ASTNode* node) {
    if (!node) return SSAValue::makeConstNum(0, getDefaultIntType());

    switch (node->type) {
        case ASTNodeType::IntLit: 
            return SSAValue::makeConstNum(static_cast<IntLitNode*>(node)->val, getDefaultIntType());

        case ASTNodeType::FloatLit: 
            return SSAValue::makeConstFloat(static_cast<FloatLitNode*>(node)->val);
        
        case ASTNodeType::StringLit: 
            return SSAValue::makeConstStr(static_cast<StringLitNode*>(node)->val);

        case ASTNodeType::OffsetOfNode: {
            auto offNode = static_cast<OffsetOfNode*>(node);
            return SSAValue::makeConstNum(static_cast<int64_t>(offNode->computedOffset), getDefaultIntType());
        }
        
        case ASTNodeType::Var: {
            auto vNode = static_cast<VarNode*>(node);
            if (varVersionTable.count(vNode->name)) return varVersionTable[vNode->name];
            return SSAValue::makeVar(vNode->name, getDefaultIntType());
        }

        case ASTNodeType::CastNode: {
            auto castNode = static_cast<CastNode*>(node);
            SSAValue val = buildExpr(castNode->expr.get());
            SSAValue res = getNextTemp(castNode->targetType);
            emit({SSAOp::Copy, res, {val}});
            return res;
        }

        case ASTNodeType::NativeGetReg: {
            auto gr = static_cast<NativeGetRegNode*>(node);
            SSAValue res = getNextTemp(getDefaultIntType());
            emit({SSAOp::GetRegister, res, {SSAValue::makeConstStr(gr->regName)}});
            return res;
        }

        case ASTNodeType::NativePortIn: {
            auto pi = static_cast<NativePortInNode*>(node);
            SSAValue port = buildExpr(pi->port.get());
            SSAValue res = getNextTemp(getDefaultIntType());
            emit({SSAOp::IOPortRead, res, {port}});
            return res;
        }

        case ASTNodeType::NativeMmioRead: {
            auto mr = static_cast<NativeMmioReadNode*>(node);
            SSAValue addr = buildExpr(mr->address.get());
            SSAValue res = getNextTemp(LsmStaticType::Int8); 
            emit({SSAOp::VolatileLoad, res, {addr}});
            return res;
        }

        
        case ASTNodeType::VolatileLoad: {
            auto vl = static_cast<VolatileLoadNode*>(node);
            SSAValue addr = buildExpr(vl->address.get());
            LsmStaticType szType = (vl->bitSize == 8) ? LsmStaticType::Int8 :
                                   (vl->bitSize == 16) ? LsmStaticType::Int16 :
                                   (vl->bitSize == 32) ? LsmStaticType::Int32 : LsmStaticType::Int64;
            SSAValue res = getNextTemp(szType);
            emit({SSAOp::VolatileLoad, res, {addr}});
            return res;
        }

        case ASTNodeType::ArrAccess: {
            auto aNode = static_cast<ArrAccessNode*>(node);
            std::string arrName = (aNode->arr->type == ASTNodeType::Var) ? static_cast<VarNode*>(aNode->arr.get())->name : "";
            SSAValue idx = buildExpr(aNode->index.get());
            SSAValue res = getNextTemp(getDefaultIntType());
            emit({SSAOp::ArrayLoad, res, {SSAValue::makeVar(arrName, getDefaultPtrType()), idx}});
            return res;
        }

        case ASTNodeType::VarAssign: {
            auto assign = static_cast<VarAssignNode*>(node);
            
            if (assign->value && assign->value->type == ASTNodeType::ArrayAlloc) {
                auto allocNode = static_cast<ArrayAllocNode*>(assign->value.get());
                SSAValue arrVar = SSAValue::makeVar(assign->name, getDefaultPtrType());
                varVersionTable[assign->name] = arrVar;
                emit({SSAOp::ArrayAlloc, arrVar, {SSAValue::makeConstNum(allocNode->size, getDefaultIntType())}});
                return arrVar;
            }

            SSAValue val = buildExpr(assign->value.get());
            LsmStaticType declaredT = val.type;
            if (assign->explicitType == "Float" || assign->explicitType == "f64") declaredT = LsmStaticType::Float64;
            else if (assign->explicitType == "Int" || assign->explicitType == "i64" || assign->explicitType == "i32" || assign->explicitType == "Int32") declaredT = getDefaultIntType();
            else if (assign->explicitType == "String" || assign->explicitType == "str") declaredT = LsmStaticType::String;

            SSAValue var = SSAValue::makeVar(assign->name, declaredT);
            varVersionTable[assign->name] = var;

            if (declaredT == LsmStaticType::Float64) {
                if (assign->op == "+=") emit({SSAOp::FAdd, var, {var, val}});
                else if (assign->op == "-=") emit({SSAOp::FSub, var, {var, val}});
                else if (assign->op == "*=") emit({SSAOp::FMul, var, {var, val}});
                else if (assign->op == "/=") emit({SSAOp::FDiv, var, {var, val}});
                else emit({SSAOp::Copy, var, {val}});
            } else {
                if (assign->op == "+=") emit({SSAOp::Add, var, {var, val}});
                else if (assign->op == "-=") emit({SSAOp::Sub, var, {var, val}});
                else if (assign->op == "*=") emit({SSAOp::Mul, var, {var, val}});
                else if (assign->op == "/=") emit({SSAOp::Div, var, {var, val}});
                else emit({SSAOp::Copy, var, {val}});
            }
            return var;
        }

        case ASTNodeType::ArrAssign: {
            auto aAssign = static_cast<ArrAssignNode*>(node);
            std::string arrName = (aAssign->arr->type == ASTNodeType::Var) ? static_cast<VarNode*>(aAssign->arr.get())->name : "";
            SSAValue idx = buildExpr(aAssign->index.get());
            SSAValue val = buildExpr(aAssign->value.get());
            emit({SSAOp::ArrayStore, {}, {SSAValue::makeVar(arrName, getDefaultPtrType()), idx, val}});
            return val;
        }

        case ASTNodeType::Binary: {
            auto bin = static_cast<BinaryNode*>(node);
            SSAValue left = buildExpr(bin->left.get());
            SSAValue right = buildExpr(bin->right.get());

            bool isFloat = (left.type == LsmStaticType::Float64 || right.type == LsmStaticType::Float64);
            SSAOp op = SSAOp::Add;

            if (bin->op == "+") op = isFloat ? SSAOp::FAdd : SSAOp::Add;
            else if (bin->op == "-") op = isFloat ? SSAOp::FSub : SSAOp::Sub;
            else if (bin->op == "*") op = isFloat ? SSAOp::FMul : SSAOp::Mul;
            else if (bin->op == "/") op = isFloat ? SSAOp::FDiv : SSAOp::Div;
            else if (bin->op == "%") op = SSAOp::Mod;
            else if (bin->op == "&") op = SSAOp::BitAnd;
            else if (bin->op == "|") op = SSAOp::BitOr;
            else if (bin->op == "^") op = SSAOp::BitXor;
            else if (bin->op == "<<") op = SSAOp::Shl;
            else if (bin->op == ">>") op = SSAOp::Shr;
            else if (bin->op == "==") op = isFloat ? SSAOp::FEqual : SSAOp::Equal;
            else if (bin->op == "!=") op = isFloat ? SSAOp::FNotEqual : SSAOp::NotEqual;
            else if (bin->op == "<")  op = isFloat ? SSAOp::FLessThan : SSAOp::LessThan;
            else if (bin->op == "<=") op = isFloat ? SSAOp::FLessEqual : SSAOp::LessEqual;
            else if (bin->op == ">")  op = isFloat ? SSAOp::FGreaterThan : SSAOp::GreaterThan;
            else if (bin->op == ">=") op = isFloat ? SSAOp::FGreaterEqual : SSAOp::GreaterEqual;
            else if (bin->op == "&&") op = SSAOp::Mul;
            else if (bin->op == "||") op = SSAOp::Add;

            SSAValue folded;
            if (tryFoldBinary(op, left, right, folded)) return folded;

            LsmStaticType resType = getDefaultIntType();
            if (op == SSAOp::FAdd || op == SSAOp::FSub || op == SSAOp::FMul || op == SSAOp::FDiv) {
                resType = LsmStaticType::Float64;
            } else if (op == SSAOp::Equal || op == SSAOp::NotEqual || op == SSAOp::LessThan || 
                       op == SSAOp::LessEqual || op == SSAOp::GreaterThan || op == SSAOp::GreaterEqual ||
                       op == SSAOp::FEqual || op == SSAOp::FNotEqual || op == SSAOp::FLessThan || 
                       op == SSAOp::FLessEqual || op == SSAOp::FGreaterThan || op == SSAOp::FGreaterEqual) {
                resType = LsmStaticType::Bool;
            }

            SSAValue res = getNextTemp(resType);
            emit({op, res, {left, right}});
            return res;
        }

        case ASTNodeType::FuncCall: {
            auto call = static_cast<FuncCallNode*>(node);
            std::string calleeName = "anonymous";
            if (call->callee && call->callee->type == ASTNodeType::Var) {
                calleeName = static_cast<VarNode*>(call->callee.get())->name;
            }

            LsmStaticType retType = getDefaultIntType();
            if (functionReturnTypes.count(calleeName)) {
                retType = functionReturnTypes[calleeName];
            }

            SSAValue res = getNextTemp(retType);
            SSAInstruction inst;
            inst.op = SSAOp::Call;
            inst.result = res;
            inst.operands.push_back(SSAValue::makeConstStr(calleeName));
            for (auto& arg : call->args) {
                inst.operands.push_back(buildExpr(arg.get()));
            }
            emit(inst);
            return res;
        }

        default: break;
    }
    return SSAValue::makeConstNum(0, getDefaultIntType());
}

void SSABuilder::buildIf(IfNode* ifNode) {
    SSAValue cond = buildExpr(ifNode->cond.get());

    std::string thenLbl = currentFunc.name + "_then_" + std::to_string(blockCounter++);
    std::string elseLbl = currentFunc.name + "_else_" + std::to_string(blockCounter++);
    std::string mergeLbl = currentFunc.name + "_merge_" + std::to_string(blockCounter++);

    emit({SSAOp::CondBranch, {}, {cond, SSAValue::makeConstStr(thenLbl), SSAValue::makeConstStr(elseLbl)}});

    currentFunc.blocks.push_back({thenLbl, {}, {}, {}});
    currentBlockLabel = thenLbl;
    if (ifNode->thenBranch) buildStatement(ifNode->thenBranch.get());
    emit({SSAOp::Branch, {}, {SSAValue::makeConstStr(mergeLbl)}});

    currentFunc.blocks.push_back({elseLbl, {}, {}, {}});
    currentBlockLabel = elseLbl;
    if (ifNode->elseBranch) buildStatement(ifNode->elseBranch.get());
    emit({SSAOp::Branch, {}, {SSAValue::makeConstStr(mergeLbl)}});

    currentFunc.blocks.push_back({mergeLbl, {}, {}, {}});
    currentBlockLabel = mergeLbl;
}

void SSABuilder::buildWhile(WhileNode* whileNode) {
    std::string condLbl = currentFunc.name + "_while_cond_" + std::to_string(blockCounter++);
    std::string bodyLbl = currentFunc.name + "_while_body_" + std::to_string(blockCounter++);
    std::string endLbl = currentFunc.name + "_while_end_" + std::to_string(blockCounter++);

    emit({SSAOp::Branch, {}, {SSAValue::makeConstStr(condLbl)}});

    currentFunc.blocks.push_back({condLbl, {}, {}, {}});
    currentBlockLabel = condLbl;
    SSAValue cond = buildExpr(whileNode->cond.get());
    emit({SSAOp::CondBranch, {}, {cond, SSAValue::makeConstStr(bodyLbl), SSAValue::makeConstStr(endLbl)}});

    currentFunc.blocks.push_back({bodyLbl, {}, {}, {}});
    currentBlockLabel = bodyLbl;
    if (whileNode->body) buildStatement(whileNode->body.get());
    emit({SSAOp::Branch, {}, {SSAValue::makeConstStr(condLbl)}});

    currentFunc.blocks.push_back({endLbl, {}, {}, {}});
    currentBlockLabel = endLbl;
}

void SSABuilder::buildFor(ForNode* forNode) {
    std::string condLbl = currentFunc.name + "_for_cond_" + std::to_string(blockCounter++);
    std::string bodyLbl = currentFunc.name + "_for_body_" + std::to_string(blockCounter++);
    std::string endLbl = currentFunc.name + "_for_end_" + std::to_string(blockCounter++);

    SSAValue loopVar = SSAValue::makeVar(forNode->var, getDefaultIntType());
    varVersionTable[forNode->var] = loopVar;
    variableDefBlocks[forNode->var].insert(currentBlockLabel);

    if (forNode->iterable && forNode->iterable->type == ASTNodeType::Range) {
        auto rNode = static_cast<RangeNode*>(forNode->iterable.get());
        SSAValue startVal = buildExpr(rNode->start.get());
        SSAValue endVal = buildExpr(rNode->end.get());

        emit({SSAOp::Copy, loopVar, {startVal}});
        emit({SSAOp::Branch, {}, {SSAValue::makeConstStr(condLbl)}});

        currentFunc.blocks.push_back({condLbl, {}, {}, {}});
        currentBlockLabel = condLbl;
        SSAValue condRes = getNextTemp(LsmStaticType::Bool);
        emit({rNode->inclusive ? SSAOp::LessEqual : SSAOp::LessThan, condRes, {loopVar, endVal}});
        emit({SSAOp::CondBranch, {}, {condRes, SSAValue::makeConstStr(bodyLbl), SSAValue::makeConstStr(endLbl)}});

        currentFunc.blocks.push_back({bodyLbl, {}, {}, {}});
        currentBlockLabel = bodyLbl;
        if (forNode->body) buildStatement(forNode->body.get());

        emit({SSAOp::Add, loopVar, {loopVar, SSAValue::makeConstNum(1, getDefaultIntType())}});
        emit({SSAOp::Branch, {}, {SSAValue::makeConstStr(condLbl)}});

        currentFunc.blocks.push_back({endLbl, {}, {}, {}});
        currentBlockLabel = endLbl;
    }
}

void SSABuilder::buildReturn(ReturnNode* retNode) {
    SSAValue retVal = retNode->expr ? buildExpr(retNode->expr.get()) : 
        (currentFunc.returnType == LsmStaticType::Float64 ? SSAValue::makeConstFloat(0.0) : SSAValue::makeConstNum(0, getDefaultIntType()));

    for (auto it = deferStack.rbegin(); it != deferStack.rend(); ++it) {
        buildStatement(*it);
    }

    if (!currentFunc.isNaked) {
        emit({SSAOp::RegionExit, {}, {}});
    }
    emit({SSAOp::Return, {}, {retVal}});
}

void SSABuilder::buildStatement(ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case ASTNodeType::Block: {
            for (auto& s : static_cast<BlockNode*>(node)->stmts) buildStatement(s.get());
            break;
        }
        case ASTNodeType::ExprStmt: {
            buildExpr(static_cast<ExprStmtNode*>(node)->expr.get());
            break;
        }
        case ASTNodeType::VarAssign:
        case ASTNodeType::ArrAssign: {
            buildExpr(node);
            break;
        }
        case ASTNodeType::Defer: {
            auto defNode = static_cast<DeferNode*>(node);
            deferStack.push_back(defNode->call.get());
            break;
        }
        case ASTNodeType::NativeSetReg: {
            auto sr = static_cast<NativeSetRegNode*>(node);
            SSAValue val = buildExpr(sr->value.get());
            emit({SSAOp::SetRegister, {}, {SSAValue::makeConstStr(sr->regName), val}});
            break;
        }
        case ASTNodeType::NativePortOut: {
            auto po = static_cast<NativePortOutNode*>(node);
            SSAValue port = buildExpr(po->port.get());
            SSAValue val = buildExpr(po->value.get());
            emit({SSAOp::IOPortWrite, {}, {port, val}});
            break;
        }
        case ASTNodeType::NativeMmioWrite: {
            auto mw = static_cast<NativeMmioWriteNode*>(node);
            SSAValue addr = buildExpr(mw->address.get());
            SSAValue val = buildExpr(mw->value.get());
            val.type = LsmStaticType::Int8; 
            emit({SSAOp::VolatileStore, {}, {addr, val}});
            break;
        }
        
        
        case ASTNodeType::VolatileStore: {
            auto vs = static_cast<VolatileStoreNode*>(node);
            SSAValue addr = buildExpr(vs->address.get());
            SSAValue val = buildExpr(vs->value.get());
            LsmStaticType szType = (vs->bitSize == 8) ? LsmStaticType::Int8 :
                                   (vs->bitSize == 16) ? LsmStaticType::Int16 :
                                   (vs->bitSize == 32) ? LsmStaticType::Int32 : LsmStaticType::Int64;
            val.type = szType; 
            emit({SSAOp::VolatileStore, {}, {addr, val}});
            break;
        }

        case ASTNodeType::NativeHalt: {
            emit({SSAOp::CpuHalt, {}, {}});
            break;
        }
        case ASTNodeType::NativeCli: {
            emit({SSAOp::DisableInterrupts, {}, {}});
            break;
        }
        case ASTNodeType::NativeSti: {
            emit({SSAOp::EnableInterrupts, {}, {}});
            break;
        }
        case ASTNodeType::If: {
            buildIf(static_cast<IfNode*>(node));
            break;
        }
        case ASTNodeType::While: {
            buildWhile(static_cast<WhileNode*>(node));
            break;
        }
        case ASTNodeType::For: {
            buildFor(static_cast<ForNode*>(node));
            break;
        }
        case ASTNodeType::GoStmt: {
            auto goNode = static_cast<GoStmtNode*>(node);
            if (goNode->call && goNode->call->type == ASTNodeType::FuncCall) {
                auto call = static_cast<FuncCallNode*>(goNode->call.get());
                std::string calleeName = (call->callee->type == ASTNodeType::Var) ? static_cast<VarNode*>(call->callee.get())->name : "";
                emit({SSAOp::TaskSpawn, {}, {SSAValue::makeConstStr(calleeName)}});
            }
            break;
        }
        case ASTNodeType::Return: {
            buildReturn(static_cast<ReturnNode*>(node));
            break;
        }
        case ASTNodeType::Print: {
            auto printNode = static_cast<PrintNode*>(node);
            SSAValue val = buildExpr(printNode->expr.get());
            if (val.type == LsmStaticType::Float64) {
                emit({SSAOp::Call, {}, {SSAValue::makeConstStr("__lsm_native_print_float"), val}});
            } else if (val.type == LsmStaticType::String) {
                emit({SSAOp::Call, {}, {SSAValue::makeConstStr("__lsm_native_print_str"), val}});
            } else {
                emit({SSAOp::Call, {}, {SSAValue::makeConstStr("__lsm_native_print_num"), val}});
            }
            break;
        }
        case ASTNodeType::UnsafeBlock: {
            auto un = static_cast<UnsafeBlockNode*>(node);
            if (un->body) buildStatement(un->body.get());
            break;
        }
        default: {
            buildExpr(node);
            break;
        }
    }
}