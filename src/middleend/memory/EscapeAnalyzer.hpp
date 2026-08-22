#ifndef LSM_ESCAPE_ANALYZER_HPP
#define LSM_ESCAPE_ANALYZER_HPP

#include "../../frontend/ast/AST.hpp"
#include <unordered_set>
#include <string>

enum class EscapeState {
    NoEscape,    
    EscapesReturn 
};

class EscapeAnalyzer {
private:
    std::unordered_set<std::string> escapedVariables;

public:
    EscapeAnalyzer() = default;

    void analyzeFunction(FuncDeclNode* func) {
        escapedVariables.clear();
        if (!func || !func->body) return;
        scanNode(func->body.get());
    }

    EscapeState getEscapeState(const std::string& varName) const {
        if (escapedVariables.count(varName)) {
            return EscapeState::EscapesReturn;
        }
        return EscapeState::NoEscape;
    }

private:
    void scanNode(ASTNode* node) {
        if (!node) return;

        
        if (node->type == ASTNodeType::Return) {
            auto ret = static_cast<ReturnNode*>(node);
            if (ret->expr) {
                if (ret->expr->type == ASTNodeType::Var) {
                    auto v = static_cast<VarNode*>(ret->expr.get());
                    escapedVariables.insert(v->name);
                }
            }
            return;
        }

        if (node->type == ASTNodeType::Block) {
            auto b = static_cast<BlockNode*>(node);
            for (const auto& s : b->stmts) scanNode(s.get());
            return;
        }

        if (node->type == ASTNodeType::If) {
            auto ifN = static_cast<IfNode*>(node);
            scanNode(ifN->thenBranch.get());
            if (ifN->elseBranch) scanNode(ifN->elseBranch.get());
            return;
        }

        if (node->type == ASTNodeType::While) {
            auto w = static_cast<WhileNode*>(node);
            scanNode(w->body.get());
            return;
        }
    }
};

#endif 