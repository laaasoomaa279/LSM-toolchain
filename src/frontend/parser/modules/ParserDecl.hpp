#ifndef LSM_PARSER_DECL_HPP
#define LSM_PARSER_DECL_HPP

#include "ParserExpr.hpp"

class ParserDecl : public virtual ParserExpr {
public:
    ParserDecl() = default;

protected:
    std::vector<std::string> parseTypeParams() {
        std::vector<std::string> params;
        if (match(LsmTokenType::Less)) {
            if (!check(LsmTokenType::Greater)) {
                do {
                    auto tp = consume(LsmTokenType::Ident, "Expected type parameter identifier");
                    params.push_back(std::string(tp.value));
                } while (match(LsmTokenType::Comma));
            }
            consume(LsmTokenType::Greater, "Expected '>' after type parameters");
        }
        return params;
    }

    std::vector<FuncParam> parseFuncParams() {
        std::vector<FuncParam> params;
        consume(LsmTokenType::LParen, "Expected '(' for parameter list");
        if (!check(LsmTokenType::RParen)) {
            do {
                auto name = consume(LsmTokenType::Ident, "Expected parameter name");
                std::string type = "dynamic";
                if (match(LsmTokenType::Colon)) {
                    type = std::string(consume(LsmTokenType::Ident, "Expected parameter type").value);
                }
                params.push_back({std::string(name.value), type});
            } while (match(LsmTokenType::Comma));
        }
        consume(LsmTokenType::RParen, "Expected ')' after parameter list");
        return params;
    }

    std::string parseReturnType() {
        if (match(LsmTokenType::ThinArrow)) {
            if (match(LsmTokenType::LParen)) {
                std::string result = "(";
                bool first = true;
                while (!check(LsmTokenType::RParen) && !atEnd()) {
                    if (!first) { result += ", "; }
                    auto t = consume(LsmTokenType::Ident, "Expected type name");
                    result += std::string(t.value);
                    first = false;
                    match(LsmTokenType::Comma);
                }
                consume(LsmTokenType::RParen, "Expected ')'");
                result += ")";
                return result;
            } else {
                return std::string(consume(LsmTokenType::Ident, "Expected return type").value);
            }
        }
        return "void";
    }

    std::unique_ptr<FuncDeclNode> parseFuncDecl(bool isProc) {
        SourceSpan span = getSpan(peek());
        auto name = consume(LsmTokenType::Ident, "Expected function name");
        auto typeParams = parseTypeParams();
        auto params = parseFuncParams();
        auto returnType = parseReturnType();
        auto body = parseBlock();
        return std::make_unique<FuncDeclNode>(
            std::string(name.value), typeParams, params,
            returnType, std::move(body), isProc, span
        );
    }

    std::unique_ptr<ASTNode> parseExternFuncDecl() {
        SourceSpan span = getSpan(peek());
        auto lib = consume(LsmTokenType::String, "Expected library string after 'extern'");
        if (!match(LsmTokenType::Fct) && !match(LsmTokenType::Proc)) {
            error("Expected 'fct' or 'proc' after library name in extern declaration", peek().line, peek().col);
        }
        auto name = consume(LsmTokenType::Ident, "Expected extern function name");
        auto params = parseFuncParams();
        auto returnType = parseReturnType();
        match(LsmTokenType::Semicolon);
        return std::make_unique<ExternFuncDeclNode>(
            std::string(lib.value), std::string(name.value), params, returnType, span
        );
    }

    std::unique_ptr<ASTNode> parseClassDecl() {
        SourceSpan span = getSpan(peek());
        auto name = consume(LsmTokenType::Ident, "Expected class name");
        auto typeParams = parseTypeParams();
        std::string parent;
        if (match(LsmTokenType::Extends)) {
            parent = std::string(consume(LsmTokenType::Ident, "Expected parent class name").value);
        }
        consume(LsmTokenType::LBrace, "Expected '{' to start class body");
        std::vector<std::string> fields;
        std::vector<std::unique_ptr<FuncDeclNode>> methods;

        while (!check(LsmTokenType::RBrace) && !atEnd()) {
            if (check(LsmTokenType::Fct) || check(LsmTokenType::Proc)) {
                bool isProc = match(LsmTokenType::Proc);
                if (!isProc) advance();
                methods.push_back(parseFuncDecl(isProc));
            } else {
                auto f = consume(LsmTokenType::Ident, "Expected field name");
                fields.push_back(std::string(f.value));
                match(LsmTokenType::Semicolon);
            }
        }
        consume(LsmTokenType::RBrace, "Expected '}' after class body");
        return std::make_unique<ClassDeclNode>(std::string(name.value), typeParams, parent, fields, std::move(methods), span);
    }

    std::unique_ptr<ASTNode> parseInterfaceDecl() {
        SourceSpan span = getSpan(peek());
        auto name = consume(LsmTokenType::Ident, "Expected interface name");
        auto typeParams = parseTypeParams();
        consume(LsmTokenType::LBrace, "Expected '{' to start interface");
        std::vector<std::string> methods;

        while (!check(LsmTokenType::RBrace) && !atEnd()) {
            if (check(LsmTokenType::Fct) || check(LsmTokenType::Proc)) {
                bool isProc = match(LsmTokenType::Proc);
                if (!isProc) advance();
                auto mName = consume(LsmTokenType::Ident, "Expected method signature name");
                std::string sig = std::string(mName.value) + "(";
                consume(LsmTokenType::LParen, "Expected '('");
                bool first = true;
                while (!check(LsmTokenType::RParen) && !atEnd()) {
                    if (!first) match(LsmTokenType::Comma);
                    auto p = consume(LsmTokenType::Ident, "Expected parameter");
                    sig += std::string(p.value);
                    if (match(LsmTokenType::Colon)) {
                        sig += ": " + std::string(consume(LsmTokenType::Ident, "Expected parameter type").value);
                    }
                    first = false;
                }
                consume(LsmTokenType::RParen, "Expected ')'");
                if (match(LsmTokenType::ThinArrow)) {
                    sig += " -> " + std::string(consume(LsmTokenType::Ident, "Expected return type").value);
                }
                methods.push_back(sig);
                match(LsmTokenType::Semicolon);
            } else {
                error("Expected method signature in interface", peek().line, peek().col);
                advance();
            }
        }
        consume(LsmTokenType::RBrace, "Expected '}' after interface body");
        return std::make_unique<InterfaceDeclNode>(std::string(name.value), typeParams, methods, span);
    }

    std::unique_ptr<ASTNode> parseEnumDecl() {
        SourceSpan span = getSpan(peek());
        auto name = consume(LsmTokenType::Ident, "Expected enum name");
        consume(LsmTokenType::LBrace, "Expected '{'");
        std::vector<std::string> variants;
        while (!check(LsmTokenType::RBrace) && !atEnd()) {
            auto v = consume(LsmTokenType::Ident, "Expected enum variant");
            variants.push_back(std::string(v.value));
            match(LsmTokenType::Comma);
        }
        consume(LsmTokenType::RBrace, "Expected '}'");
        return std::make_unique<EnumDeclNode>(std::string(name.value), variants, span);
    }

    std::unique_ptr<ASTNode> parseTypeAlias() {
        SourceSpan span = getSpan(peek());
        auto name = consume(LsmTokenType::Ident, "Expected alias name");
        consume(LsmTokenType::Eq, "Expected '=' in type alias");
        auto target = consume(LsmTokenType::Ident, "Expected target type name");
        match(LsmTokenType::Semicolon);
        return std::make_unique<TypeAliasNode>(std::string(name.value), std::string(target.value), span);
    }

    std::unique_ptr<ASTNode> parseRecDecl() {
        SourceSpan span = getSpan(peek());
        auto name = consume(LsmTokenType::Ident, "Expected record name");
        auto typeParams = parseTypeParams();
        consume(LsmTokenType::LBrace, "Expected '{'");
        std::vector<std::string> fields;
        while (!check(LsmTokenType::RBrace) && !atEnd()) {
            auto f = consume(LsmTokenType::Ident, "Expected field name");
            fields.push_back(std::string(f.value));
            match(LsmTokenType::Semicolon);
        }
        consume(LsmTokenType::RBrace, "Expected '}'");
        return std::make_unique<RecDeclNode>(std::string(name.value), typeParams, fields, span);
    }

    std::unique_ptr<ASTNode> parseArrDecl() {
        SourceSpan span = getSpan(peek());
        auto name = consume(LsmTokenType::Ident, "Expected array name");
        bool isSlice = false;
        int size = -1;

        if (match(LsmTokenType::LBracket)) {
            if (match(LsmTokenType::RBracket)) {
                isSlice = true;
            } else {
                auto sizeTok = consume(LsmTokenType::Int, "Expected array size");
                size = std::stoi(std::string(sizeTok.value));
                consume(LsmTokenType::RBracket, "Expected ']'");
            }
        } else {
            error("Expected '[' for array or slice declaration", peek().line, peek().col);
        }

        std::string elemType = "dynamic";
        if (match(LsmTokenType::Colon)) {
            elemType = std::string(consume(LsmTokenType::Ident, "Expected element type").value);
        }
        match(LsmTokenType::Semicolon);
        return std::make_unique<ArrDeclNode>(std::string(name.value), size, elemType, isSlice, span);
    }

    std::unique_ptr<ASTNode> parseChanDecl() {
        SourceSpan span = getSpan(peek());
        auto name = consume(LsmTokenType::Ident, "Expected channel name");
        bool buffered = match(LsmTokenType::LBracket);
        int capacity = 0;
        if (buffered) {
            auto capTok = consume(LsmTokenType::Int, "Expected integer literal for capacity");
            capacity = std::stoi(std::string(capTok.value));
            consume(LsmTokenType::RBracket, "Expected ']'");
        }
        consume(LsmTokenType::Colon, "Expected ':'");
        auto elemType = consume(LsmTokenType::Ident, "Expected channel element type");
        match(LsmTokenType::Semicolon);
        return std::make_unique<ChanDeclNode>(std::string(name.value), std::string(elemType.value), buffered, capacity, span);
    }
};

#endif 