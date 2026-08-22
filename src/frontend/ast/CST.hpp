#ifndef LSM_CST_HPP
#define LSM_CST_HPP

#include <string>
#include <vector>
#include <memory>

enum class CSTNodeKind {
    Token, SourceFile, FunctionDefinition, BlockStatement,
    VariableDeclaration, BinaryExpression, Comment, Whitespace
};

struct CSTNode {
    CSTNodeKind kind;
    std::string text;
    std::vector<std::shared_ptr<CSTNode>> children;

    explicit CSTNode(CSTNodeKind k, std::string t = "") : kind(k), text(std::move(t)) {}
    void addChild(std::shared_ptr<CSTNode> child) { children.push_back(child); }
};

#endif 