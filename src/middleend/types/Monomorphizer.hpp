#ifndef LSM_MONOMORPHIZER_HPP
#define LSM_MONOMORPHIZER_HPP

#include "../../frontend/ast/AST.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

class Monomorphizer {
private:
    std::unordered_map<std::string, std::unique_ptr<FuncDeclNode>> genericFuncs;
    std::vector<std::unique_ptr<FuncDeclNode>> generatedSpecializations;
    std::unordered_map<std::string, bool> instantiated;

    std::string buildMonomorphizedName(const std::string& base, const std::vector<std::string>& typeArgs) {
        std::string name = base;
        for (const auto& t : typeArgs) name += "_" + t;
        return name;
    }

    std::unique_ptr<ASTNode> cloneAndSubstitute(ASTNode* node, const std::unordered_map<std::string, std::string>& typeMap) {
        if (!node) return nullptr;

        switch (node->type) {
            case ASTNodeType::IntLit: {
                auto n = static_cast<IntLitNode*>(node);
                return std::make_unique<IntLitNode>(n->val, n->span);
            }
            case ASTNodeType::FloatLit: {
                auto n = static_cast<FloatLitNode*>(node);
                return std::make_unique<FloatLitNode>(n->val, n->span);
            }
            case ASTNodeType::StringLit: {
                auto n = static_cast<StringLitNode*>(node);
                return std::make_unique<StringLitNode>(n->val, n->span);
            }
            case ASTNodeType::Var: {
                auto n = static_cast<VarNode*>(node);
                return std::make_unique<VarNode>(n->name, n->span);
            }
            case ASTNodeType::Unary: {
                auto n = static_cast<UnaryNode*>(node);
                return std::make_unique<UnaryNode>(n->op, cloneAndSubstitute(n->operand.get(), typeMap), n->span);
            }
            case ASTNodeType::Binary: {
                auto n = static_cast<BinaryNode*>(node);
                auto l = cloneAndSubstitute(n->left.get(), typeMap);
                auto r = cloneAndSubstitute(n->right.get(), typeMap);
                return std::make_unique<BinaryNode>(n->op, std::move(l), std::move(r), n->span);
            }
            case ASTNodeType::Ternary: {
                auto n = static_cast<TernaryNode*>(node);
                return std::make_unique<TernaryNode>(
                    cloneAndSubstitute(n->cond.get(), typeMap),
                    cloneAndSubstitute(n->thenExpr.get(), typeMap),
                    cloneAndSubstitute(n->elseExpr.get(), typeMap),
                    n->span
                );
            }
            case ASTNodeType::ArrAccess: {
                auto n = static_cast<ArrAccessNode*>(node);
                return std::make_unique<ArrAccessNode>(
                    cloneAndSubstitute(n->arr.get(), typeMap),
                    cloneAndSubstitute(n->index.get(), typeMap),
                    n->span
                );
            }
            case ASTNodeType::ArrAssign: {
                auto n = static_cast<ArrAssignNode*>(node);
                return std::make_unique<ArrAssignNode>(
                    cloneAndSubstitute(n->arr.get(), typeMap),
                    cloneAndSubstitute(n->index.get(), typeMap),
                    cloneAndSubstitute(n->value.get(), typeMap),
                    n->op, n->span
                );
            }

            
            case ASTNodeType::NativeSetReg: {
                auto n = static_cast<NativeSetRegNode*>(node);
                return std::make_unique<NativeSetRegNode>(n->regName, cloneAndSubstitute(n->value.get(), typeMap), n->span);
            }
            case ASTNodeType::NativeGetReg: {
                auto n = static_cast<NativeGetRegNode*>(node);
                return std::make_unique<NativeGetRegNode>(n->regName, n->span);
            }
            case ASTNodeType::NativePortOut: {
                auto n = static_cast<NativePortOutNode*>(node);
                return std::make_unique<NativePortOutNode>(
                    cloneAndSubstitute(n->port.get(), typeMap),
                    cloneAndSubstitute(n->value.get(), typeMap),
                    n->span
                );
            }
            case ASTNodeType::NativePortIn: {
                auto n = static_cast<NativePortInNode*>(node);
                return std::make_unique<NativePortInNode>(cloneAndSubstitute(n->port.get(), typeMap), n->span);
            }
            case ASTNodeType::NativeMmioWrite: {
                auto n = static_cast<NativeMmioWriteNode*>(node);
                return std::make_unique<NativeMmioWriteNode>(
                    cloneAndSubstitute(n->address.get(), typeMap),
                    cloneAndSubstitute(n->value.get(), typeMap),
                    n->span
                );
            }
            case ASTNodeType::NativeMmioRead: {
                auto n = static_cast<NativeMmioReadNode*>(node);
                return std::make_unique<NativeMmioReadNode>(cloneAndSubstitute(n->address.get(), typeMap), n->span);
            }
            case ASTNodeType::NativeHalt: {
                return std::make_unique<NativeHaltNode>(node->span);
            }
            case ASTNodeType::NativeCli: {
                return std::make_unique<NativeCliNode>(node->span);
            }
            case ASTNodeType::NativeSti: {
                return std::make_unique<NativeStiNode>(node->span);
            }

            case ASTNodeType::Block: {
                auto n = static_cast<BlockNode*>(node);
                auto block = std::make_unique<BlockNode>(n->span);
                for (const auto& s : n->stmts) {
                    block->stmts.push_back(cloneAndSubstitute(s.get(), typeMap));
                }
                return block;
            }
            case ASTNodeType::Return: {
                auto n = static_cast<ReturnNode*>(node);
                return std::make_unique<ReturnNode>(cloneAndSubstitute(n->expr.get(), typeMap), n->span);
            }
            case ASTNodeType::VarAssign: {
                auto n = static_cast<VarAssignNode*>(node);
                std::string actualType = n->explicitType;
                if (typeMap.count(actualType)) actualType = typeMap.at(actualType);
                return std::make_unique<VarAssignNode>(n->name, actualType,
                    cloneAndSubstitute(n->value.get(), typeMap), n->op, n->span);
            }
            case ASTNodeType::If: {
                auto n = static_cast<IfNode*>(node);
                return std::make_unique<IfNode>(
                    cloneAndSubstitute(n->cond.get(), typeMap),
                    cloneAndSubstitute(n->thenBranch.get(), typeMap),
                    cloneAndSubstitute(n->elseBranch.get(), typeMap),
                    n->span
                );
            }
            case ASTNodeType::While: {
                auto n = static_cast<WhileNode*>(node);
                return std::make_unique<WhileNode>(
                    cloneAndSubstitute(n->cond.get(), typeMap),
                    cloneAndSubstitute(n->body.get(), typeMap),
                    n->span
                );
            }
            case ASTNodeType::FuncCall: {
                auto call = static_cast<FuncCallNode*>(node);
                std::vector<std::unique_ptr<ASTNode>> clonedArgs;
                for (auto& a : call->args) {
                    clonedArgs.push_back(cloneAndSubstitute(a.get(), typeMap));
                }
                return std::make_unique<FuncCallNode>(
                    cloneAndSubstitute(call->callee.get(), typeMap),
                    call->typeArgs,
                    std::move(clonedArgs),
                    call->span
                );
            }
            case ASTNodeType::ExprStmt: {
                auto e = static_cast<ExprStmtNode*>(node);
                return std::make_unique<ExprStmtNode>(cloneAndSubstitute(e->expr.get(), typeMap), e->span);
            }
            case ASTNodeType::Print: {
                auto p = static_cast<PrintNode*>(node);
                return std::make_unique<PrintNode>(cloneAndSubstitute(p->expr.get(), typeMap), p->span);
            }
            default:
                return nullptr;
        }
    }

public:
    Monomorphizer() = default;

    void process(ProgramNode* program) {
        if (!program) return;

        for (auto it = program->stmts.begin(); it != program->stmts.end();) {
            if ((*it)->type == ASTNodeType::FuncDecl) {
                auto fn = static_cast<FuncDeclNode*>(it->get());
                if (!fn->typeParams.empty()) {
                    std::string name = fn->name;
                    genericFuncs[name] = std::unique_ptr<FuncDeclNode>(static_cast<FuncDeclNode*>(it->release()));
                    it = program->stmts.erase(it);
                    continue;
                }
            }
            ++it;
        }

        for (auto& stmt : program->stmts) {
            scanAndMonomorphizeNode(stmt);
        }

        for (auto& spec : generatedSpecializations) {
            program->stmts.insert(program->stmts.begin(), std::move(spec));
        }
    }

    void scanAndMonomorphizeNode(std::unique_ptr<ASTNode>& node) {
        if (!node) return;

        if (node->type == ASTNodeType::FuncCall) {
            auto call = static_cast<FuncCallNode*>(node.get());
            if (!call->typeArgs.empty() && call->callee && call->callee->type == ASTNodeType::Var) {
                auto var = static_cast<VarNode*>(call->callee.get());
                std::string baseName = var->name;
                if (genericFuncs.count(baseName)) {
                    FuncDeclNode* templateFn = genericFuncs[baseName].get();
                    std::string specializedName = buildMonomorphizedName(baseName, call->typeArgs);

                    std::vector<std::string> savedArgs = call->typeArgs;
                    var->name = specializedName;
                    call->typeArgs.clear();

                    if (!instantiated.count(specializedName)) {
                        instantiated[specializedName] = true;

                        std::unordered_map<std::string, std::string> typeMap;
                        for (size_t i = 0; i < templateFn->typeParams.size() && i < savedArgs.size(); ++i) {
                            typeMap[templateFn->typeParams[i]] = savedArgs[i];
                        }

                        std::vector<FuncParam> specParams;
                        for (const auto& p : templateFn->params) {
                            std::string pT = p.type;
                            if (typeMap.count(pT)) pT = typeMap[pT];
                            specParams.push_back({p.name, pT});
                        }

                        std::string retT = templateFn->returnType;
                        if (typeMap.count(retT)) retT = typeMap[retT];

                        auto specBody = cloneAndSubstitute(templateFn->body.get(), typeMap);
                        auto specFn = std::make_unique<FuncDeclNode>(
                            specializedName, std::vector<std::string>{}, specParams,
                            retT, std::move(specBody), templateFn->isProc, templateFn->span
                        );
                        generatedSpecializations.push_back(std::move(specFn));
                    }
                }
            }
            for (auto& arg : call->args) scanAndMonomorphizeNode(arg);
        }
        else if (node->type == ASTNodeType::VarAssign) {
            auto a = static_cast<VarAssignNode*>(node.get());
            scanAndMonomorphizeNode(a->value);
        }
        else if (node->type == ASTNodeType::ArrAssign) {
            auto a = static_cast<ArrAssignNode*>(node.get());
            scanAndMonomorphizeNode(a->index);
            scanAndMonomorphizeNode(a->value);
        }
        else if (node->type == ASTNodeType::NativeSetReg) {
            auto sr = static_cast<NativeSetRegNode*>(node.get());
            scanAndMonomorphizeNode(sr->value);
        }
        else if (node->type == ASTNodeType::NativePortOut) {
            auto po = static_cast<NativePortOutNode*>(node.get());
            scanAndMonomorphizeNode(po->port);
            scanAndMonomorphizeNode(po->value);
        }
        else if (node->type == ASTNodeType::NativeMmioWrite) {
            auto mw = static_cast<NativeMmioWriteNode*>(node.get());
            scanAndMonomorphizeNode(mw->address);
            scanAndMonomorphizeNode(mw->value);
        }
        else if (node->type == ASTNodeType::ExprStmt) {
            auto e = static_cast<ExprStmtNode*>(node.get());
            scanAndMonomorphizeNode(e->expr);
        }
        else if (node->type == ASTNodeType::Return) {
            auto r = static_cast<ReturnNode*>(node.get());
            scanAndMonomorphizeNode(r->expr);
        }
        else if (node->type == ASTNodeType::Print) {
            auto p = static_cast<PrintNode*>(node.get());
            scanAndMonomorphizeNode(p->expr);
        }
        else if (node->type == ASTNodeType::Block) {
            auto b = static_cast<BlockNode*>(node.get());
            for (auto& s : b->stmts) scanAndMonomorphizeNode(s);
        }
        else if (node->type == ASTNodeType::If) {
            auto ifN = static_cast<IfNode*>(node.get());
            scanAndMonomorphizeNode(ifN->cond);
            scanAndMonomorphizeNode(ifN->thenBranch);
            scanAndMonomorphizeNode(ifN->elseBranch);
        }
        else if (node->type == ASTNodeType::While) {
            auto w = static_cast<WhileNode*>(node.get());
            scanAndMonomorphizeNode(w->cond);
            scanAndMonomorphizeNode(w->body);
        }
        else if (node->type == ASTNodeType::FuncDecl) {
            auto fn = static_cast<FuncDeclNode*>(node.get());
            scanAndMonomorphizeNode(fn->body);
        }
    }
};

#endif 