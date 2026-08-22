#include "Parser.hpp"

Parser::Parser(std::vector<Token> t) {
    setTokens(std::move(t));
}

std::unique_ptr<ProgramNode> Parser::parseProgram() {
    auto prog = std::make_unique<ProgramNode>(getSpan(peek()));
    if (match(LsmTokenType::FullDev)) {
        prog->fullDev = true;
        cstRoot->addChild(std::make_shared<CSTNode>(CSTNodeKind::Token, "fulldev"));
    }

    while (!atEnd()) {
        try {
            std::vector<std::unique_ptr<AttributeNode>> attrs;
            while (isAttributeLookahead()) {
                auto attr = parseAttributes();
                if (attr) attrs.push_back(std::unique_ptr<AttributeNode>(static_cast<AttributeNode*>(attr.release())));
            }

            auto stmt = parseStatement();
            if (stmt) {
                if (!attrs.empty()) applyAttributes(stmt, attrs);
                prog->stmts.push_back(std::move(stmt));
            }
        } catch (const ParseError&) {
            synchronize();
        }
    }
    return prog;
}