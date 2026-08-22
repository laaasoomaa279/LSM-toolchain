#ifndef LSM_REGION_LOWERING_HPP
#define LSM_REGION_LOWERING_HPP

#include "../../frontend/ast/AST.hpp"
#include "../memory/EscapeAnalyzer.hpp"
#include <memory>

class RegionLowering {
private:
    EscapeAnalyzer analyzer;

public:
    RegionLowering() = default;

    void lowerRegions(std::unique_ptr<ASTNode>& root) {
        if (!root) return;

        if (root->type == ASTNodeType::FuncDecl) {
            auto fn = static_cast<FuncDeclNode*>(root.get());
            analyzer.analyzeFunction(fn);
        }

        transform(root);
    }

private:
    void transform(std::unique_ptr<ASTNode>& node) {
        if (!node) return;

        if (node->type == ASTNodeType::Block) {
            auto block = static_cast<BlockNode*>(node.get());
            for (auto& s : block->stmts) transform(s);
            return;
        }

        if (node->type == ASTNodeType::If) {
            auto ifN = static_cast<IfNode*>(node.get());
            transform(ifN->thenBranch);
            transform(ifN->elseBranch);
            return;
        }

        if (node->type == ASTNodeType::While) {
            auto w = static_cast<WhileNode*>(node.get());
            transform(w->body);
            return;
        }

        if (node->type == ASTNodeType::FuncDecl) {
            auto fn = static_cast<FuncDeclNode*>(node.get());
            transform(fn->body);
            return;
        }
    }
};

#endif 