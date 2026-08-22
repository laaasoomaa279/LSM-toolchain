#ifndef LSM_PARSER_EXPR_HPP
#define LSM_PARSER_EXPR_HPP

#include "ParserCore.hpp"

class ParserExpr : public virtual ParserCore {
public:
    ParserExpr() = default;

    std::unique_ptr<ASTNode> parseExpression(Prec minPrec = Prec::Assign) {
        auto left = parsePrefix();
        while (getPrec(peek().type) >= minPrec) {
            left = parseInfix(std::move(left), getPrec(peek().type));
        }
        return left;
    }

protected:
    virtual std::unique_ptr<BlockNode> parseBlock() = 0;
    virtual std::unique_ptr<ASTNode> parseCompileTimeIf() = 0;
    virtual std::unique_ptr<ASTNode> parseRegisterVar() = 0;

    std::vector<std::string> parseTypeArguments() {
        std::vector<std::string> args;
        consume(LsmTokenType::Less, "Expected '<' for generic type arguments");
        do {
            if (check(LsmTokenType::Ident)) {
                std::string typeName = std::string(advance().value);
                if (check(LsmTokenType::Less) && isGenericInvocationLookahead()) {
                    auto nested = parseTypeArguments();
                    typeName += "<";
                    for (size_t i = 0; i < nested.size(); ++i) {
                        if (i > 0) typeName += ", ";
                        typeName += nested[i];
                    }
                    typeName += ">";
                }
                args.push_back(typeName);
            } else {
                consume(LsmTokenType::Ident, "Expected type name argument");
            }
        } while (match(LsmTokenType::Comma));

        if (check(LsmTokenType::Shr)) {
            advance();
            return args;
        }
        consume(LsmTokenType::Greater, "Expected '>' after generic type arguments");
        return args;
    }

    std::unique_ptr<ASTNode> parseBuiltinCall(const std::string& name, SourceSpan span) {
        consume(LsmTokenType::LParen, "Expected '(' after builtin '" + name + "'");

        if (name == "inb" || name == "inw" || name == "inl") {
            auto port = parseExpression();
            consume(LsmTokenType::RParen, "Expected ')' after port argument");
            return std::make_unique<IOPortOpNode>(name, std::move(port), nullptr, span);
        }
        if (name == "outb" || name == "outw" || name == "outl") {
            auto port = parseExpression();
            consume(LsmTokenType::Comma, "Expected ',' between port and value");
            auto value = parseExpression();
            consume(LsmTokenType::RParen, "Expected ')' after arguments");
            return std::make_unique<IOPortOpNode>(name, std::move(port), std::move(value), span);
        }

        
        if (name == "mmio_read8" || name == "mmio_read16" || name == "mmio_read32" || name == "mmio_read64") {
            int sz = (name == "mmio_read8") ? 8 : (name == "mmio_read16") ? 16 : (name == "mmio_read32") ? 32 : 64;
            auto addr = parseExpression();
            consume(LsmTokenType::RParen, "Expected ')' after address");
            return std::make_unique<VolatileLoadNode>(std::move(addr), sz, span);
        }

        if (name == "mmio_write8" || name == "mmio_write16" || name == "mmio_write32" || name == "mmio_write64") {
            int sz = (name == "mmio_write8") ? 8 : (name == "mmio_write16") ? 16 : (name == "mmio_write32") ? 32 : 64;
            auto addr = parseExpression();
            consume(LsmTokenType::Comma, "Expected ',' between address and value");
            auto val = parseExpression();
            consume(LsmTokenType::RParen, "Expected ')' after arguments");
            return std::make_unique<VolatileStoreNode>(std::move(addr), std::move(val), sz, span);
        }

        if (name == "atomic_add" || name == "atomic_sub" || name == "atomic_store") {
            auto addr = parseExpression();
            consume(LsmTokenType::Comma, "Expected ',' after address");
            auto val = parseExpression();
            consume(LsmTokenType::RParen, "Expected ')' after arguments");
            return std::make_unique<AtomicOpNode>(name.substr(8), std::move(addr), std::move(val), nullptr, span);
        }
        if (name == "atomic_cas") {
            auto addr = parseExpression();
            consume(LsmTokenType::Comma, "Expected ',' after address");
            auto expected = parseExpression();
            consume(LsmTokenType::Comma, "Expected ',' after expected value");
            auto newVal = parseExpression();
            consume(LsmTokenType::RParen, "Expected ')' after arguments");
            return std::make_unique<AtomicOpNode>("cas", std::move(addr), std::move(newVal), std::move(expected), span);
        }
        if (name == "atomic_load") {
            auto addr = parseExpression();
            consume(LsmTokenType::RParen, "Expected ')' after address");
            return std::make_unique<AtomicOpNode>("load", std::move(addr), nullptr, nullptr, span);
        }

        if (name == "disable_interrupts" || name == "enable_interrupts" || name == "halt") {
            consume(LsmTokenType::RParen, "Expected '()' after builtin");
            return std::make_unique<InterruptOpNode>(name == "disable_interrupts" ? "disable" :
                                                      name == "enable_interrupts" ? "enable" : "halt", span);
        }

        error("Unknown builtin function: " + name, peek().line, peek().col);
        throw ParseError("Unknown builtin function");
    }

    std::unique_ptr<ASTNode> parseALSMGetReg() {
        SourceSpan span = getSpan(peek());
        consume(LsmTokenType::LParen, "Expected '(' after getreg");
        auto regTok = advance();
        std::string regName = std::string(regTok.value);
        consume(LsmTokenType::RParen, "Expected ')' after register name");
        return std::make_unique<NativeGetRegNode>(regName, span);
    }

    std::unique_ptr<ASTNode> parseALSMPortIn() {
        SourceSpan span = getSpan(peek());
        consume(LsmTokenType::LParen, "Expected '(' after port_in");
        auto port = parseExpression();
        consume(LsmTokenType::RParen, "Expected ')' after port");
        return std::make_unique<NativePortInNode>(std::move(port), span);
    }

    std::unique_ptr<ASTNode> parseALSMMmioRead() {
        SourceSpan span = getSpan(peek());
        consume(LsmTokenType::LParen, "Expected '(' after mmio_read");
        auto addr = parseExpression();
        consume(LsmTokenType::RParen, "Expected ')' after address");
        return std::make_unique<NativeMmioReadNode>(std::move(addr), span);
    }

    std::unique_ptr<ASTNode> parsePrefix() {
        Token tok = peek();
        SourceSpan span = getSpan(tok);

        if (match(LsmTokenType::Int)) {
            std::string s(prev().value);
            int64_t val = 0;
            if (s.size() > 2 && (s[1] == 'x' || s[1] == 'X')) val = std::stoll(s, nullptr, 16);
            else val = std::stoll(s);
            return std::make_unique<IntLitNode>(val, span);
        }
        if (match(LsmTokenType::Float)) {
            return std::make_unique<FloatLitNode>(std::stod(std::string(prev().value)), span);
        }
        if (match(LsmTokenType::String)) {
            return std::make_unique<StringLitNode>(std::string(prev().value), span);
        }
        if (match(LsmTokenType::Nil)) {
            return std::make_unique<NilLitNode>(span);
        }
        if (match(LsmTokenType::Input)) {
            return std::make_unique<InputNode>(span);
        }

        if (peek().type == LsmTokenType::At && pos + 1 < tokens.size() && tokens[pos + 1].value == "offsetof") {
            advance(); advance();
            consume(LsmTokenType::LParen, "Expected '(' after @offsetof");
            auto recTok = consume(LsmTokenType::Ident, "Expected Record type name");
            consume(LsmTokenType::Comma, "Expected ',' after Record name");
            auto fieldTok = consume(LsmTokenType::Ident, "Expected Field name");
            consume(LsmTokenType::RParen, "Expected ')' after Field name");
            return std::make_unique<OffsetOfNode>(std::string(recTok.value), std::string(fieldTok.value), span);
        }

        if (match(LsmTokenType::GetReg)) return parseALSMGetReg();
        if (match(LsmTokenType::PortIn)) return parseALSMPortIn();
        if (match(LsmTokenType::MmioRead)) return parseALSMMmioRead();

        if (match(LsmTokenType::Ident)) {
            std::string name(prev().value);
            if (name == "inb" || name == "outb" || name == "inw" || name == "outw" ||
                name == "inl" || name == "outl" ||
                name == "mmio_read8"  || name == "mmio_read16"  || name == "mmio_read32"  || name == "mmio_read64"  ||
                name == "mmio_write8" || name == "mmio_write16" || name == "mmio_write32" || name == "mmio_write64" ||
                name == "atomic_add" || name == "atomic_sub" || name == "atomic_cas" ||
                name == "atomic_load" || name == "atomic_store" ||
                name == "disable_interrupts" || name == "enable_interrupts" || name == "halt") {
                return parseBuiltinCall(name, span);
            }
            return std::make_unique<VarNode>(name, span);
        }

        if (match(LsmTokenType::Minus) || match(LsmTokenType::Not) || match(LsmTokenType::Tilde)) {
            std::string op = std::string(prev().value);
            auto operand = parseExpression(Prec::Unary);
            return std::make_unique<UnaryNode>(op, std::move(operand), span);
        }
        if (match(LsmTokenType::Amp)) {
            auto expr = parseExpression(Prec::Unary);
            return std::make_unique<PtrAddrNode>(std::move(expr), span);
        }
        if (match(LsmTokenType::Star)) {
            auto expr = parseExpression(Prec::Unary);
            return std::make_unique<PtrDerefNode>(std::move(expr), span);
        }

        if (match(LsmTokenType::ChanSendRecv)) {
            auto ch = parseExpression(Prec::Unary);
            return std::make_unique<ChanOpNode>(std::move(ch), nullptr, false, span);
        }

        if (match(LsmTokenType::Volatile)) {
            consume(LsmTokenType::LParen, "Expected '(' after 'volatile'");
            auto addr = parseExpression();
            consume(LsmTokenType::RParen, "Expected ')' after volatile address");
            return std::make_unique<VolatileLoadNode>(std::move(addr), 8, span);
        }

        if (match(LsmTokenType::Ptr)) {
            consume(LsmTokenType::Less, "Expected '<' after 'ptr'");
            auto targetT = consume(LsmTokenType::Ident, "Expected type name in ptr<...>");
            consume(LsmTokenType::Greater, "Expected '>' after pointer cast target type");
            consume(LsmTokenType::LParen, "Expected '(' for pointer cast target expression");
            auto expr = parseExpression();
            consume(LsmTokenType::RParen, "Expected ')' after pointer cast expression");
            return std::make_unique<PtrCastNode>(std::move(expr), std::string(targetT.value), span);
        }

        if (match(LsmTokenType::Ok)) {
            consume(LsmTokenType::LParen, "Expected '(' after Ok");
            auto val = parseExpression();
            consume(LsmTokenType::RParen, "Expected ')' after Ok payload");
            return std::make_unique<ResultOkNode>(std::move(val), span);
        }
        if (match(LsmTokenType::Err)) {
            consume(LsmTokenType::LParen, "Expected '(' after Err");
            auto val = parseExpression();
            consume(LsmTokenType::RParen, "Expected ')' after Err payload");
            return std::make_unique<ResultErrNode>(std::move(val), span);
        }
        if (match(LsmTokenType::Some)) {
            consume(LsmTokenType::LParen, "Expected '(' after Some");
            auto val = parseExpression();
            consume(LsmTokenType::RParen, "Expected ')' after Some payload");
            return std::make_unique<OptionSomeNode>(std::move(val), span);
        }
        if (match(LsmTokenType::None)) {
            return std::make_unique<OptionNoneNode>(span);
        }

        if (match(LsmTokenType::New)) {
            auto cls = consume(LsmTokenType::Ident, "Expected class name after 'new'");
            std::vector<std::string> typeArgs;
            if (check(LsmTokenType::Less) && isGenericInvocationLookahead()) {
                typeArgs = parseTypeArguments();
            }
            consume(LsmTokenType::LParen, "Expected '(' after class name");
            std::vector<std::unique_ptr<ASTNode>> args;
            if (!check(LsmTokenType::RParen)) {
                do { args.push_back(parseExpression()); } while (match(LsmTokenType::Comma));
            }
            consume(LsmTokenType::RParen, "Expected ')' after constructor arguments");
            return std::make_unique<NewExprNode>(std::string(cls.value), typeArgs, std::move(args), span);
        }

        if (match(LsmTokenType::LParen)) {
            size_t savePos = pos;
            std::vector<std::unique_ptr<ASTNode>> elems;
            bool isMulti = false;

            if (!check(LsmTokenType::RParen)) {
                do {
                    auto expr = parseExpression();
                    elems.push_back(std::move(expr));
                    if (match(LsmTokenType::Comma)) {
                        isMulti = true;
                    } else {
                        break;
                    }
                } while (true);
            }

            if (isMulti) {
                consume(LsmTokenType::RParen, "Expected ')' after multi-return expression");
                return std::make_unique<MultiReturnNode>(std::move(elems), span);
            } else {
                pos = savePos;
                std::vector<std::string> params;
                if (!check(LsmTokenType::RParen)) {
                    while (check(LsmTokenType::Ident)) {
                        params.push_back(std::string(advance().value));
                        if (!match(LsmTokenType::Comma)) break;
                    }
                }
                if (check(LsmTokenType::RParen)) {
                    advance();
                    if (match(LsmTokenType::FatArrow)) {
                        auto body = parseBlock();
                        return std::make_unique<LambdaNode>(params, std::move(body), false, span);
                    }
                }
                pos = savePos;
                auto expr = parseExpression();
                consume(LsmTokenType::RParen, "Expected ')' after grouped expression");
                return expr;
            }
        }

        if (match(LsmTokenType::LBracket)) {
            std::vector<std::unique_ptr<ASTNode>> elems;
            if (!check(LsmTokenType::RBracket)) {
                do { elems.push_back(parseExpression()); } while (match(LsmTokenType::Comma));
            }
            consume(LsmTokenType::RBracket, "Expected ']' after array literal elements");
            return std::make_unique<ArrayLitNode>(std::move(elems), span);
        }

        if (isCompileTimeIfLookahead()) return parseCompileTimeIf();
        if (isRegisterVarLookahead()) return parseRegisterVar();

        error("Unexpected token in expression: '" + std::string(peek().value) + "'", peek().line, peek().col);
        throw ParseError("Prefix expression parsing failed");
    }

    std::unique_ptr<ASTNode> parseInfix(std::unique_ptr<ASTNode> left, Prec prec) {
        SourceSpan span = getSpan(peek());

        if (isAssignOp(peek().type)) {
            Token op = advance();
            auto rhs = parseExpression(Prec::Assign);

            if (left->type == ASTNodeType::Var) {
                auto v = static_cast<VarNode*>(left.get());
                return std::make_unique<VarAssignNode>(v->name, "", std::move(rhs), std::string(op.value), span);
            }
            if (left->type == ASTNodeType::MemberAccess) {
                auto m = static_cast<MemberAccessNode*>(left.release());
                return std::make_unique<MemberAssignNode>(std::move(m->object), std::move(rhs), std::string(op.value), span);
            }
            if (left->type == ASTNodeType::ArrAccess) {
                auto a = static_cast<ArrAccessNode*>(left.release());
                return std::make_unique<ArrAssignNode>(std::move(a->arr), std::move(a->index), std::move(rhs), std::string(op.value), span);
            }
            error("Invalid target for assignment", peek().line, op.col);
            return left;
        }

        if (match(LsmTokenType::ChanSendRecv)) {
            auto val = parseExpression(Prec::ChanSend);
            return std::make_unique<ChanOpNode>(std::move(left), std::move(val), true, span);
        }

        if (match(LsmTokenType::Range) || match(LsmTokenType::RangeInclusive)) {
            bool inclusive = (prev().type == LsmTokenType::RangeInclusive);
            auto end = parseExpression(Prec::Range);
            return std::make_unique<RangeNode>(std::move(left), std::move(end), inclusive, span);
        }

        if (check(LsmTokenType::Less) && isGenericInvocationLookahead()) {
            auto typeArgs = parseTypeArguments();
            consume(LsmTokenType::LParen, "Expected '(' after generic type arguments");
            std::vector<std::unique_ptr<ASTNode>> args;
            if (!check(LsmTokenType::RParen)) {
                do { args.push_back(parseExpression()); } while (match(LsmTokenType::Comma));
            }
            consume(LsmTokenType::RParen, "Expected ')' after function arguments");
            return std::make_unique<FuncCallNode>(std::move(left), typeArgs, std::move(args), span);
        }

        if (match(LsmTokenType::Question) && !check(LsmTokenType::Dot) && !check(LsmTokenType::LParen)) {
            auto thenExpr = parseExpression();
            consume(LsmTokenType::Colon, "Expected ':' in ternary operator");
            auto elseExpr = parseExpression(Prec::Ternary);
            return std::make_unique<TernaryNode>(std::move(left), std::move(thenExpr), std::move(elseExpr), span);
        }

        if (match(LsmTokenType::QuestionSafe)) {
            return std::make_unique<SafeAccessNode>(std::move(left), span);
        }

        if (match(LsmTokenType::PlusPlus) || match(LsmTokenType::MinusMinus)) {
            std::string op = (prev().type == LsmTokenType::PlusPlus) ? "+=" : "-=";
            if (left->type == ASTNodeType::Var) {
                auto v = static_cast<VarNode*>(left.get());
                return std::make_unique<VarAssignNode>(v->name, "", std::make_unique<IntLitNode>(1, span), op, span);
            }
            error("Invalid target for postfix operator", peek().line, prev().col);
            return left;
        }

        if (match(LsmTokenType::LParen)) {
            std::vector<std::unique_ptr<ASTNode>> args;
            if (!check(LsmTokenType::RParen)) {
                do { args.push_back(parseExpression()); } while (match(LsmTokenType::Comma));
            }
            consume(LsmTokenType::RParen, "Expected ')' after arguments");
            return std::make_unique<FuncCallNode>(std::move(left), std::vector<std::string>{}, std::move(args), span);
        }

        if (match(LsmTokenType::LBracket)) {
            auto idx = parseExpression();
            consume(LsmTokenType::RBracket, "Expected ']' after array index");
            return std::make_unique<ArrAccessNode>(std::move(left), std::move(idx), span);
        }

        if (match(LsmTokenType::Dot)) {
            auto member = consume(LsmTokenType::Ident, "Expected member identifier after '.'");
            std::vector<std::string> typeArgs;
            if (check(LsmTokenType::Less) && isGenericInvocationLookahead()) {
                typeArgs = parseTypeArguments();
            }
            if (match(LsmTokenType::LParen)) {
                std::vector<std::unique_ptr<ASTNode>> args;
                if (!check(LsmTokenType::RParen)) {
                    do { args.push_back(parseExpression()); } while (match(LsmTokenType::Comma));
                }
                consume(LsmTokenType::RParen, "Expected ')' after method arguments");
                return std::make_unique<MethodCallNode>(std::move(left), std::string(member.value), typeArgs, std::move(args), span);
            }
            return std::make_unique<MemberAccessNode>(std::move(left), std::string(member.value), span);
        }

        Token op = advance();
        auto right = parseExpression(static_cast<Prec>(static_cast<int>(prec) + 1));
        return std::make_unique<BinaryNode>(std::string(op.value), std::move(left), std::move(right), span);
    }
};

#endif 