#ifndef LSM_PARSER_CORE_HPP
#define LSM_PARSER_CORE_HPP

#include "../../ast/AST.hpp"
#include "../../ast/CST.hpp"
#include "../ErrorRecovery.hpp"
#include "../../Token.hpp"
#include <vector>
#include <memory>
#include <stdexcept>
#include <string>

struct ParseError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

enum class Prec {
    None, Assign, ChanSend, Range, Ternary, Or, And, BitOr, BitXor, BitAnd,
    Equality, Compare, Shift, Term, Factor, Unary, Call, Primary
};

class ParserCore {
protected:
    std::vector<Token> tokens;
    size_t pos = 0;
    bool err = false;
    bool insideUnsafe = false;
    ErrorRecoveryHandler errorHandler;
    std::shared_ptr<CSTNode> cstRoot;

public:
    
    ParserCore() {
        cstRoot = std::make_shared<CSTNode>(CSTNodeKind::SourceFile, "Root");
    }

    explicit ParserCore(std::vector<Token> t) : tokens(std::move(t)) {
        cstRoot = std::make_shared<CSTNode>(CSTNodeKind::SourceFile, "Root");
    }

    void setTokens(std::vector<Token> t) {
        tokens = std::move(t);
        pos = 0;
        err = false;
    }

    std::shared_ptr<CSTNode> getCSTRoot() const { return cstRoot; }
    bool hasErrors() const { return err || errorHandler.hasErrors(); }

protected:
    Token peek() const {
        return pos < tokens.size() ? tokens[pos] : Token{LsmTokenType::Eof, "", 0, 0, 0};
    }

    Token prev() const {
        return pos > 0 ? tokens[pos - 1] : Token{LsmTokenType::Eof, "", 0, 0, 0};
    }

    Token advance() {
        if (!atEnd()) pos++;
        return prev();
    }

    bool atEnd() const { return peek().type == LsmTokenType::Eof; }
    bool check(LsmTokenType type) const { return !atEnd() && peek().type == type; }

    bool match(LsmTokenType type) {
        if (check(type)) { advance(); return true; }
        return false;
    }

    Token consume(LsmTokenType type, const std::string& msg) {
        if (check(type)) return advance();
        error(msg, peek().line, peek().col);
        throw ParseError(msg);
    }

    void error(const std::string& msg, size_t line, size_t col) {
        errorHandler.reportError(msg, line, col);
        err = true;
    }

    SourceSpan getSpan(const Token& tok) {
        return SourceSpan{tok.offset, tok.offset + tok.value.size(), tok.line, tok.col};
    }

    static bool endsWith(const std::string& str, const std::string& suffix) {
        if (str.length() < suffix.length()) return false;
        return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    }

    void synchronize() {
        advance();
        while (!atEnd()) {
            if (prev().type == LsmTokenType::Semicolon || prev().type == LsmTokenType::RBrace) return;
            switch (peek().type) {
                case LsmTokenType::Fct: case LsmTokenType::Proc: case LsmTokenType::Class:
                case LsmTokenType::Interface: case LsmTokenType::Enum: case LsmTokenType::Type:
                case LsmTokenType::Rec: case LsmTokenType::Array: case LsmTokenType::If:
                case LsmTokenType::While: case LsmTokenType::For: case LsmTokenType::Switch:
                case LsmTokenType::Match: case LsmTokenType::Try: case LsmTokenType::Return:
                case LsmTokenType::Let: case LsmTokenType::Go: case LsmTokenType::Chan:
                case LsmTokenType::Defer: case LsmTokenType::Hash: case LsmTokenType::At:
                case LsmTokenType::Extern:
                    return;
                default: break;
            }
            advance();
        }
    }

    Prec getPrec(LsmTokenType type) const {
        switch (type) {
            case LsmTokenType::Eq: case LsmTokenType::PlusEq:
            case LsmTokenType::MinusEq: case LsmTokenType::StarEq:
            case LsmTokenType::SlashEq: return Prec::Assign;
            case LsmTokenType::ChanSendRecv: return Prec::ChanSend;
            case LsmTokenType::Range: case LsmTokenType::RangeInclusive: return Prec::Range;
            case LsmTokenType::Question: return Prec::Ternary;
            case LsmTokenType::PipePipe: return Prec::Or;
            case LsmTokenType::AmpAmp: return Prec::And;
            case LsmTokenType::Pipe: return Prec::BitOr;
            case LsmTokenType::Caret: return Prec::BitXor;
            case LsmTokenType::Amp: return Prec::BitAnd;
            case LsmTokenType::EqEq: case LsmTokenType::NotEq: return Prec::Equality;
            case LsmTokenType::Less: case LsmTokenType::LessEq:
            case LsmTokenType::Greater: case LsmTokenType::GreaterEq: return Prec::Compare;
            case LsmTokenType::Shl: case LsmTokenType::Shr: return Prec::Shift;
            case LsmTokenType::Plus: case LsmTokenType::Minus: return Prec::Term;
            case LsmTokenType::Star: case LsmTokenType::Slash: case LsmTokenType::Percent: return Prec::Factor;
            case LsmTokenType::LParen: case LsmTokenType::LBracket: case LsmTokenType::Dot:
            case LsmTokenType::QuestionSafe: case LsmTokenType::PlusPlus:
            case LsmTokenType::MinusMinus: return Prec::Call;
            default: return Prec::None;
        }
    }

    bool isGenericInvocationLookahead() const {
        if (peek().type != LsmTokenType::Less) return false;
        size_t forward = pos + 1;
        size_t depth = 1;

        while (forward < tokens.size() && tokens[forward].type != LsmTokenType::Eof) {
            LsmTokenType t = tokens[forward].type;
            if (t == LsmTokenType::Less) depth++;
            else if (t == LsmTokenType::Greater) {
                depth--;
                if (depth == 0) { forward++; break; }
            } else if (t == LsmTokenType::Shr) {
                if (depth <= 2) { forward++; break; }
                depth -= 2;
            } else if (t != LsmTokenType::Ident && t != LsmTokenType::Comma && t != LsmTokenType::Ptr) {
                return false;
            }
            forward++;
        }
        return forward < tokens.size() && tokens[forward].type == LsmTokenType::LParen;
    }

    bool isAttributeLookahead() const {
        if (peek().type != LsmTokenType::Hash) return false;
        size_t forward = pos + 1;
        if (forward >= tokens.size() || tokens[forward].type != LsmTokenType::LBracket) return false;
        forward++;
        if (forward >= tokens.size() || tokens[forward].type != LsmTokenType::Ident) return false;
        forward++;
        if (tokens[forward].type == LsmTokenType::LParen) {
            size_t depth = 1;
            forward++;
            while (forward < tokens.size() && depth > 0) {
                if (tokens[forward].type == LsmTokenType::LParen) depth++;
                else if (tokens[forward].type == LsmTokenType::RParen) depth--;
                forward++;
            }
        }
        if (forward >= tokens.size()) return false;
        return tokens[forward].type == LsmTokenType::RBracket;
    }

    bool isCompileTimeIfLookahead() const {
        return peek().type == LsmTokenType::At && 
               pos + 1 < tokens.size() &&
               tokens[pos + 1].type == LsmTokenType::If;
    }

    bool isRegisterVarLookahead() const {
        return peek().type == LsmTokenType::Ident && 
               std::string(peek().value) == "register" &&
               pos + 1 < tokens.size() &&
               tokens[pos + 1].type == LsmTokenType::Ident;
    }
};

#endif 