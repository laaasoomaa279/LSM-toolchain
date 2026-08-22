#ifndef LSM_PARSER_STMT_HPP
#define LSM_PARSER_STMT_HPP

#include "ParserDecl.hpp"

class ParserStmt : public virtual ParserDecl {
public:
    ParserStmt() = default;

protected:
    std::unique_ptr<ASTNode> parseALSMSetReg() {
        SourceSpan span = getSpan(peek());
        consume(LsmTokenType::LParen, "Expected '(' after setreg");
        auto regTok = advance();
        std::string regName = std::string(regTok.value);
        consume(LsmTokenType::Comma, "Expected ',' between register and value");
        auto val = parseExpression();
        consume(LsmTokenType::RParen, "Expected ')' after setreg value");
        match(LsmTokenType::Semicolon);
        return std::make_unique<NativeSetRegNode>(regName, std::move(val), span);
    }

    std::unique_ptr<ASTNode> parseALSMPortOut() {
        SourceSpan span = getSpan(peek());
        consume(LsmTokenType::LParen, "Expected '(' after port_out");
        auto port = parseExpression();
        consume(LsmTokenType::Comma, "Expected ',' between port and value");
        auto val = parseExpression();
        consume(LsmTokenType::RParen, "Expected ')' after port_out arguments");
        match(LsmTokenType::Semicolon);
        return std::make_unique<NativePortOutNode>(std::move(port), std::move(val), span);
    }

    std::unique_ptr<ASTNode> parseALSMMmioWrite() {
        SourceSpan span = getSpan(peek());
        consume(LsmTokenType::LParen, "Expected '(' after mmio_write");
        auto addr = parseExpression();
        consume(LsmTokenType::Comma, "Expected ',' between address and value");
        auto val = parseExpression();
        consume(LsmTokenType::RParen, "Expected ')' after mmio_write arguments");
        match(LsmTokenType::Semicolon);
        return std::make_unique<NativeMmioWriteNode>(std::move(addr), std::move(val), span);
    }

    std::unique_ptr<BlockNode> parseBlock() override {
        SourceSpan span = getSpan(peek());
        consume(LsmTokenType::LBrace, "Expected '{' to start block");
        auto block = std::make_unique<BlockNode>(span);
        while (!check(LsmTokenType::RBrace) && !atEnd()) {
            std::vector<std::unique_ptr<AttributeNode>> attrs;
            while (isAttributeLookahead()) {
                auto attr = parseAttributes();
                if (attr) attrs.push_back(std::unique_ptr<AttributeNode>(static_cast<AttributeNode*>(attr.release())));
            }
            auto stmt = parseStatement();
            if (stmt) {
                if (!attrs.empty()) applyAttributes(stmt, attrs);
                block->stmts.push_back(std::move(stmt));
            }
        }
        consume(LsmTokenType::RBrace, "Expected '}' to close block");
        return block;
    }

    std::unique_ptr<ASTNode> parseAttributes() {
        SourceSpan span = getSpan(peek());
        consume(LsmTokenType::Hash, "Expected '#' for attribute");
        consume(LsmTokenType::LBracket, "Expected '[' after '#'");
        auto name = consume(LsmTokenType::Ident, "Expected attribute name");
        std::vector<std::string> args;
        if (match(LsmTokenType::LParen)) {
            if (!check(LsmTokenType::RParen)) {
                do {
                    auto arg = consume(LsmTokenType::String, "Expected attribute argument string");
                    args.push_back(std::string(arg.value));
                } while (match(LsmTokenType::Comma));
            }
            consume(LsmTokenType::RParen, "Expected ')' after attribute arguments");
        }
        consume(LsmTokenType::RBracket, "Expected ']' after attribute");
        return std::make_unique<AttributeNode>(std::string(name.value), args, span);
    }

    void applyAttributes(std::unique_ptr<ASTNode>& node, const std::vector<std::unique_ptr<AttributeNode>>& attrs) {
        if (!node) return;
        for (const auto& attr : attrs) {
            if (node->type == ASTNodeType::FuncDecl) {
                auto func = static_cast<FuncDeclNode*>(node.get());
                if (attr->name == "inline") func->isInline = true;
                else if (attr->name == "naked") func->isNaked = true;
                else if (attr->name == "interrupt") func->isInterrupt = true;
                else if (attr->name == "no_mangle") func->isNoMangle = true;
                else if (attr->name == "section" && !attr->args.empty()) {
                    func->section = attr->args[0];
                } else {
                    error("Unknown attribute '" + attr->name + "' applied to function", peek().line, 0);
                }
            } else if (node->type == ASTNodeType::RecDecl) {
                auto rec = static_cast<RecDeclNode*>(node.get());
                if (attr->name == "align" && !attr->args.empty()) {
                    rec->alignment = std::stoi(attr->args[0]);
                } else {
                    error("Unknown attribute '" + attr->name + "' applied to record", peek().line, 0);
                }
            }
        }
    }

    std::unique_ptr<ASTNode> parseCompileTimeIf() override {
        SourceSpan span = getSpan(peek());
        consume(LsmTokenType::At, "Expected '@' before 'if'");
        consume(LsmTokenType::If, "Expected 'if' after '@'");
        auto cond = parseExpression();
        auto thenBranch = parseBlock();
        std::unique_ptr<ASTNode> elseBranch = nullptr;
        if (match(LsmTokenType::At) && check(LsmTokenType::Else)) {
            advance();
            if (check(LsmTokenType::If)) {
                advance();
                elseBranch = parseCompileTimeIf();
            } else {
                elseBranch = parseBlock();
            }
        }
        return std::make_unique<CompileTimeIfNode>(std::move(cond), std::move(thenBranch), std::move(elseBranch), span);
    }

    std::unique_ptr<ASTNode> parseRegisterVar() override {
        SourceSpan span = getSpan(peek());
        consume(LsmTokenType::Ident, "Expected 'register' keyword");
        auto reg = consume(LsmTokenType::Ident, "Expected register name");
        consume(LsmTokenType::Colon, "Expected ':' after register name");
        auto type = consume(LsmTokenType::Ident, "Expected register type");
        match(LsmTokenType::Semicolon);
        return std::make_unique<RegisterVarNode>(std::string(reg.value), "", std::string(type.value), span);
    }

    std::unique_ptr<ASTNode> parseIfStmt() {
        SourceSpan span = getSpan(peek());
        auto cond = parseExpression();
        auto thenBranch = parseBlock();
        std::unique_ptr<ASTNode> elseBranch = nullptr;
        if (match(LsmTokenType::Else)) {
            if (match(LsmTokenType::If)) elseBranch = parseIfStmt();
            else elseBranch = parseBlock();
        }
        return std::make_unique<IfNode>(std::move(cond), std::move(thenBranch), std::move(elseBranch), span);
    }

    std::unique_ptr<ASTNode> parseWhileStmt() {
        SourceSpan span = getSpan(peek());
        auto cond = parseExpression();
        auto body = parseBlock();
        return std::make_unique<WhileNode>(std::move(cond), std::move(body), span);
    }

    std::unique_ptr<ASTNode> parseForStmt() {
        SourceSpan span = getSpan(peek());
        auto var = consume(LsmTokenType::Ident, "Expected loop variable");
        consume(LsmTokenType::In, "Expected 'in' in for loop");
        auto iterable = parseExpression();
        auto body = parseBlock();
        return std::make_unique<ForNode>(std::string(var.value), std::move(iterable), std::move(body), span);
    }

    std::unique_ptr<ASTNode> parseSwitchStmt() {
        SourceSpan span = getSpan(peek());
        consume(LsmTokenType::LParen, "Expected '(' after 'switch'");
        auto expr = parseExpression();
        consume(LsmTokenType::RParen, "Expected ')' after switch expression");
        consume(LsmTokenType::LBrace, "Expected '{' to start cases");
        std::vector<SwitchCase> cases;

        while (!check(LsmTokenType::RBrace) && !atEnd()) {
            if (match(LsmTokenType::Case)) {
                auto val = parseExpression();
                consume(LsmTokenType::Colon, "Expected ':' after case value");
                auto body = parseBlock();
                cases.push_back({std::move(val), std::move(body)});
            } else if (match(LsmTokenType::Default)) {
                consume(LsmTokenType::Colon, "Expected ':' after default");
                auto body = parseBlock();
                cases.push_back({nullptr, std::move(body)});
            } else {
                error("Expected 'case' or 'default'", peek().line, peek().col);
                advance();
            }
        }
        consume(LsmTokenType::RBrace, "Expected '}'");
        return std::make_unique<SwitchNode>(std::move(expr), std::move(cases), span);
    }

    std::unique_ptr<ASTNode> parseMatchStmt() {
        SourceSpan span = getSpan(peek());
        auto expr = parseExpression();
        consume(LsmTokenType::LBrace, "Expected '{' to open match patterns");
        std::vector<MatchCase> cases;

        while (!check(LsmTokenType::RBrace) && !atEnd()) {
            MatchCase mc;
            if (match(LsmTokenType::Ok)) {
                mc.patternType = "Ok";
                consume(LsmTokenType::LParen, "Expected '(' after Ok");
                mc.bindVariable = std::string(consume(LsmTokenType::Ident, "Expected binding variable").value);
                consume(LsmTokenType::RParen, "Expected ')'");
            } else if (match(LsmTokenType::Err)) {
                mc.patternType = "Err";
                consume(LsmTokenType::LParen, "Expected '(' after Err");
                mc.bindVariable = std::string(consume(LsmTokenType::Ident, "Expected binding variable").value);
                consume(LsmTokenType::RParen, "Expected ')'");
            } else if (match(LsmTokenType::Some)) {
                mc.patternType = "Some";
                consume(LsmTokenType::LParen, "Expected '(' after Some");
                mc.bindVariable = std::string(consume(LsmTokenType::Ident, "Expected binding variable").value);
                consume(LsmTokenType::RParen, "Expected ')'");
            } else if (match(LsmTokenType::None)) {
                mc.patternType = "None";
            } else {
                mc.patternType = "Literal";
                mc.literalPattern = parseExpression();
            }

            consume(LsmTokenType::FatArrow, "Expected '=>'");
            mc.body = parseBlock();
            cases.push_back(std::move(mc));
        }
        consume(LsmTokenType::RBrace, "Expected '}'");
        return std::make_unique<MatchNode>(std::move(expr), std::move(cases), span);
    }

    std::unique_ptr<ASTNode> parseTryCatchStmt() {
        SourceSpan span = getSpan(peek());
        auto tryBlock = parseBlock();
        consume(LsmTokenType::Catch, "Expected 'catch' after try block");
        consume(LsmTokenType::LParen, "Expected '(' after catch");
        auto errVar = consume(LsmTokenType::Ident, "Expected error variable identifier");
        consume(LsmTokenType::RParen, "Expected ')'");
        auto catchBlock = parseBlock();
        return std::make_unique<TryCatchNode>(std::move(tryBlock), std::string(errVar.value), std::move(catchBlock), span);
    }

    std::unique_ptr<ASTNode> parseStatement() {
        SourceSpan span = getSpan(peek());

        if (match(LsmTokenType::Unsafe)) {
            insideUnsafe = true;
            auto block = parseBlock();
            insideUnsafe = false;
            return std::make_unique<UnsafeBlockNode>(std::move(block), span);
        }

        if (match(LsmTokenType::SetReg)) return parseALSMSetReg();
        if (match(LsmTokenType::PortOut)) return parseALSMPortOut();
        if (match(LsmTokenType::MmioWrite)) return parseALSMMmioWrite();

        if (match(LsmTokenType::CpuHalt)) {
            match(LsmTokenType::LParen); match(LsmTokenType::RParen); match(LsmTokenType::Semicolon);
            return std::make_unique<NativeHaltNode>(span);
        }
        if (match(LsmTokenType::CpuCli)) {
            match(LsmTokenType::LParen); match(LsmTokenType::RParen); match(LsmTokenType::Semicolon);
            return std::make_unique<NativeCliNode>(span);
        }
        if (match(LsmTokenType::CpuSti)) {
            match(LsmTokenType::LParen); match(LsmTokenType::RParen); match(LsmTokenType::Semicolon);
            return std::make_unique<NativeStiNode>(span);
        }

        if (match(LsmTokenType::Asm)) {
            consume(LsmTokenType::LParen, "Expected '(' after 'asm'");
            auto code = consume(LsmTokenType::String, "Expected string literal for inline ASM");
            consume(LsmTokenType::RParen, "Expected ')' after ASM string");
            match(LsmTokenType::Semicolon);
            return std::make_unique<InlineAsmNode>(std::string(code.value), span);
        }

        if (match(LsmTokenType::Volatile)) {
            consume(LsmTokenType::LParen, "Expected '(' after 'volatile'");
            auto addr = parseExpression();
            if (match(LsmTokenType::Comma)) {
                auto val = parseExpression();
                consume(LsmTokenType::RParen, "Expected ')' after volatile value");
                match(LsmTokenType::Semicolon);
                
                return std::make_unique<VolatileStoreNode>(std::move(addr), std::move(val), 8, span);
            }
            consume(LsmTokenType::RParen, "Expected ')' after volatile address");
            match(LsmTokenType::Semicolon);
            
            return std::make_unique<ExprStmtNode>(std::make_unique<VolatileLoadNode>(std::move(addr), 8, span), span);
        }

        if (match(LsmTokenType::Extern)) return parseExternFuncDecl();

        if (match(LsmTokenType::Let)) {
            auto name = consume(LsmTokenType::Ident, "Expected variable name after 'let'");
            std::string explicitType = "";
            int64_t arraySize = 0;
            bool isArrayDecl = false;

            if (match(LsmTokenType::Colon)) {
                if (match(LsmTokenType::LBracket)) {
                    isArrayDecl = true;
                    explicitType = std::string(consume(LsmTokenType::Ident, "Expected element type in array").value);
                    consume(LsmTokenType::Semicolon, "Expected ';' between type and array size");
                    auto sizeTok = consume(LsmTokenType::Int, "Expected array size integer");
                    arraySize = std::stoll(std::string(sizeTok.value));
                    consume(LsmTokenType::RBracket, "Expected ']' after array type");
                } else {
                    explicitType = std::string(consume(LsmTokenType::Ident, "Expected explicit type").value);
                }
            }

            if (isArrayDecl) {
                match(LsmTokenType::Semicolon);
                auto allocNode = std::make_unique<ArrayAllocNode>(explicitType, arraySize, span);
                return std::make_unique<VarAssignNode>(std::string(name.value), "[" + explicitType + "]", std::move(allocNode), "=", span);
            }

            if (match(LsmTokenType::Eq)) {
                auto val = parseExpression();
                match(LsmTokenType::Semicolon);
                return std::make_unique<VarAssignNode>(std::string(name.value), explicitType, std::move(val), "=", span);
            } else {
                std::unique_ptr<ASTNode> defaultVal;
                if (explicitType == "Int") defaultVal = std::make_unique<IntLitNode>(0, span);
                else if (explicitType == "Float") defaultVal = std::make_unique<FloatLitNode>(0.0, span);
                else if (explicitType == "String") defaultVal = std::make_unique<StringLitNode>("", span);
                else defaultVal = std::make_unique<NilLitNode>(span);
                match(LsmTokenType::Semicolon);
                return std::make_unique<VarAssignNode>(std::string(name.value), explicitType, std::move(defaultVal), "=", span);
            }
        }

        if (match(LsmTokenType::Import)) {
            auto path = consume(LsmTokenType::String, "Expected module path string");
            match(LsmTokenType::Semicolon);
            std::string pStr(path.value);
            bool isC = (endsWith(pStr, ".h") || endsWith(pStr, ".hpp") || pStr.find(".h") != std::string::npos);
            return std::make_unique<ImportNode>(pStr, isC, span);
        }

        if (match(LsmTokenType::Fct) || match(LsmTokenType::Proc)) {
            return parseFuncDecl(prev().type == LsmTokenType::Proc);
        }

        if (match(LsmTokenType::Class)) return parseClassDecl();
        if (match(LsmTokenType::Interface)) return parseInterfaceDecl();
        if (match(LsmTokenType::Enum)) return parseEnumDecl();
        if (match(LsmTokenType::Type)) return parseTypeAlias();
        if (match(LsmTokenType::Rec)) return parseRecDecl();
        if (match(LsmTokenType::Array)) return parseArrDecl();
        if (match(LsmTokenType::Chan)) return parseChanDecl();

        if (match(LsmTokenType::If)) return parseIfStmt();
        if (match(LsmTokenType::While)) return parseWhileStmt();
        if (match(LsmTokenType::For)) return parseForStmt();
        if (match(LsmTokenType::Switch)) return parseSwitchStmt();
        if (match(LsmTokenType::Match)) return parseMatchStmt();
        if (match(LsmTokenType::Try)) return parseTryCatchStmt();

        if (match(LsmTokenType::Go)) {
            auto call = parseExpression();
            match(LsmTokenType::Semicolon);
            return std::make_unique<GoStmtNode>(std::move(call), span);
        }
        if (match(LsmTokenType::Defer)) {
            auto call = parseExpression();
            match(LsmTokenType::Semicolon);
            return std::make_unique<DeferNode>(std::move(call), span);
        }

        if (match(LsmTokenType::Break)) { match(LsmTokenType::Semicolon); return std::make_unique<BreakNode>(span); }
        if (match(LsmTokenType::Continue)) { match(LsmTokenType::Semicolon); return std::make_unique<ContinueNode>(span); }

        if (match(LsmTokenType::Return)) {
            std::unique_ptr<ASTNode> expr = nullptr;
            if (!check(LsmTokenType::Semicolon) && !check(LsmTokenType::RBrace) && !atEnd()) {
                expr = parseExpression();
            }
            match(LsmTokenType::Semicolon);
            return std::make_unique<ReturnNode>(std::move(expr), span);
        }

        if (match(LsmTokenType::Throw)) {
            auto expr = parseExpression();
            match(LsmTokenType::Semicolon);
            return std::make_unique<ThrowNode>(std::move(expr), span);
        }

        if (match(LsmTokenType::Print)) {
            consume(LsmTokenType::LParen, "Expected '(' after 'print'");
            auto expr = parseExpression();
            consume(LsmTokenType::RParen, "Expected ')' after print argument");
            match(LsmTokenType::Semicolon);
            return std::make_unique<PrintNode>(std::move(expr), span);
        }

        auto expr = parseExpression();
        match(LsmTokenType::Semicolon);
        return std::make_unique<ExprStmtNode>(std::move(expr), span);
    }
};

#endif 