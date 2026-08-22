#ifndef LSM_SSA_BUILDER_HPP
#define LSM_SSA_BUILDER_HPP

#include "SSAInstruction.hpp"
#include "../../frontend/ast/AST.hpp"
#include "../memory/EscapeAnalyzer.hpp"
#include "../opt/Dominance.hpp"
#include "../../backend/regalloc/PhysicalRegister.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <memory>

class SSABuilder {
private:
    SSAFunction currentFunc;
    std::string currentBlockLabel;
    std::string entryPoint = "_start";
    ArchType targetArch = ArchType::X86_64; 

    size_t tempCounter = 0;
    size_t blockCounter = 0;

    std::unordered_map<std::string, SSAValue> varVersionTable;
    std::unordered_map<std::string, std::unordered_set<std::string>> variableDefBlocks;
    std::unordered_map<std::string, LsmStaticType> functionReturnTypes;
    std::vector<ASTNode*> deferStack;

    DominanceAnalyzer dominanceAnalyzer;
    EscapeAnalyzer escapeAnalyzer;

    SSABasicBlock* getCurrentBlock();
    SSABasicBlock* getBlockByLabel(const std::string& label);
    void emit(SSAInstruction inst);

    
    LsmStaticType getDefaultIntType() const {
        return (targetArch == ArchType::X86_32) ? LsmStaticType::Int32 : LsmStaticType::Int64;
    }

    
    LsmStaticType getDefaultPtrType() const {
        return (targetArch == ArchType::X86_32) ? LsmStaticType::Ptr32 : LsmStaticType::Ptr;
    }

    SSAValue getNextTemp(LsmStaticType type) {
        return SSAValue::makeTemp(tempCounter++, type);
    }

    void insertPhiNodes(SSAFunction& func, const DominanceAnalyzer& dom);
    bool tryFoldBinary(SSAOp op, const SSAValue& left, const SSAValue& right, SSAValue& foldedResult);

    SSAValue buildExpr(ASTNode* node);
    void buildStatement(ASTNode* node);
    void buildIf(IfNode* ifNode);
    void buildWhile(WhileNode* whileNode);
    void buildFor(ForNode* forNode);
    void buildReturn(ReturnNode* retNode);

public:
    SSABuilder() = default;

    void setTargetArch(ArchType arch) {
        targetArch = arch;
    }

    void setEntryPoint(const std::string& name) { 
        entryPoint = name; 
    }

    SSAFunction buildFunction(FuncDeclNode* funcNode);
    std::vector<SSAFunction> buildProgram(ProgramNode* root);
};

#endif 