#include "Lexer.hpp"
#include <cctype>
#include <iostream>

Lexer::Lexer(std::string s) : src(std::move(s)) {}

char Lexer::peek() const { return pos < src.size() ? src[pos] : '\0'; }
char Lexer::peekNext() const { return pos + 1 < src.size() ? src[pos + 1] : '\0'; }

char Lexer::advance() {
    char c = peek();
    if (c == '\n') { line++; col = 1; }
    else col++;
    pos++;
    return c;
}

bool Lexer::match(char c) {
    if (peek() == c) { advance(); return true; }
    return false;
}

void Lexer::error(const std::string& msg) {
    std::cerr << "[Lexer Error] Line " << line << ", Col " << col << ": " << msg << "\n";
    err = true;
}

void Lexer::skipWhitespaceAndComments() {
    while (true) {
        char c = peek();
        if (std::isspace(c)) { advance(); continue; }
        if (c == '/' && peekNext() == '/') {
            while (peek() != '\n' && peek() != '\0') advance();
            continue;
        }
        if (c == '/' && peekNext() == '*') {
            advance(); advance();
            bool closed = false;
            while (peek() != '\0') {
                if (peek() == '*' && peekNext() == '/') { advance(); advance(); closed = true; break; }
                advance();
            }
            if (!closed) error("Unterminated multi-line comment");
            continue;
        }
        break;
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    err = false;

    while (pos < src.size()) {
        skipWhitespaceAndComments();
        if (pos >= src.size()) break;

        size_t start = pos, lineStart = line, colStart = col;
        char c = peek();

        if (std::isalpha(c) || c == '_') {
            while (std::isalnum(peek()) || peek() == '_') advance();
            std::string_view id(&src[start], pos - start);

            LsmTokenType type = LsmTokenType::Ident;

            
            if (id == "fulldev") type = LsmTokenType::FullDev;
            else if (id == "unsafe") type = LsmTokenType::Unsafe;
            else if (id == "asm") type = LsmTokenType::Asm;
            else if (id == "volatile") type = LsmTokenType::Volatile;
            else if (id == "ptr") type = LsmTokenType::Ptr;
            else if (id == "setreg") type = LsmTokenType::SetReg;
            else if (id == "getreg") type = LsmTokenType::GetReg;
            else if (id == "cpu_halt" || id == "halt") type = LsmTokenType::CpuHalt;
            else if (id == "cpu_cli" || id == "cli") type = LsmTokenType::CpuCli;
            else if (id == "cpu_sti" || id == "sti") type = LsmTokenType::CpuSti;
            else if (id == "port_in") type = LsmTokenType::PortIn;
            else if (id == "port_out") type = LsmTokenType::PortOut;
            else if (id == "mmio_read") type = LsmTokenType::MmioRead;
            else if (id == "mmio_write") type = LsmTokenType::MmioWrite;

            
            else if (id == "rax") type = LsmTokenType::RegRax;
            else if (id == "rbx") type = LsmTokenType::RegRbx;
            else if (id == "rcx") type = LsmTokenType::RegRcx;
            else if (id == "rdx") type = LsmTokenType::RegRdx;
            else if (id == "rsi") type = LsmTokenType::RegRsi;
            else if (id == "rdi") type = LsmTokenType::RegRdi;
            else if (id == "rbp") type = LsmTokenType::RegRbp;
            else if (id == "rsp") type = LsmTokenType::RegRsp;
            else if (id == "r8")  type = LsmTokenType::RegR8;
            else if (id == "r9")  type = LsmTokenType::RegR9;
            else if (id == "r10") type = LsmTokenType::RegR10;
            else if (id == "r11") type = LsmTokenType::RegR11;
            else if (id == "r12") type = LsmTokenType::RegR12;
            else if (id == "r13") type = LsmTokenType::RegR13;
            else if (id == "r14") type = LsmTokenType::RegR14;
            else if (id == "r15") type = LsmTokenType::RegR15;

            
            else if (id == "inb") type = LsmTokenType::Inb;
            else if (id == "outb") type = LsmTokenType::Outb;
            else if (id == "inw") type = LsmTokenType::Inw;
            else if (id == "outw") type = LsmTokenType::Outw;
            else if (id == "inl") type = LsmTokenType::Inl;
            else if (id == "outl") type = LsmTokenType::Outl;

            
            else if (id == "atomic_add") type = LsmTokenType::AtomicAdd;
            else if (id == "atomic_sub") type = LsmTokenType::AtomicSub;
            else if (id == "atomic_cas") type = LsmTokenType::AtomicCas;
            else if (id == "atomic_load") type = LsmTokenType::AtomicLoad;
            else if (id == "atomic_store") type = LsmTokenType::AtomicStore;

            
            else if (id == "true") type = LsmTokenType::True;
            else if (id == "false") type = LsmTokenType::False;
            else if (id == "print") type = LsmTokenType::Print;
            else if (id == "input") type = LsmTokenType::Input;
            else if (id == "fct") type = LsmTokenType::Fct;
            else if (id == "proc") type = LsmTokenType::Proc;
            else if (id == "rec") type = LsmTokenType::Rec;
            else if (id == "array") type = LsmTokenType::Array;
            else if (id == "return") type = LsmTokenType::Return;
            else if (id == "if") type = LsmTokenType::If;
            else if (id == "else") type = LsmTokenType::Else;
            else if (id == "while") type = LsmTokenType::While;
            else if (id == "for") type = LsmTokenType::For;
            else if (id == "in") type = LsmTokenType::In;
            else if (id == "to") type = LsmTokenType::To;
            else if (id == "break") type = LsmTokenType::Break;
            else if (id == "continue") type = LsmTokenType::Continue;
            else if (id == "switch") type = LsmTokenType::Switch;
            else if (id == "case") type = LsmTokenType::Case;
            else if (id == "default") type = LsmTokenType::Default;
            else if (id == "class") type = LsmTokenType::Class;
            else if (id == "extends") type = LsmTokenType::Extends;
            else if (id == "this") type = LsmTokenType::This;
            else if (id == "new") type = LsmTokenType::New;
            else if (id == "import") type = LsmTokenType::Import;
            else if (id == "extern") type = LsmTokenType::Extern;
            else if (id == "go") type = LsmTokenType::Go;
            else if (id == "chan") type = LsmTokenType::Chan;
            else if (id == "defer") type = LsmTokenType::Defer;
            else if (id == "interface") type = LsmTokenType::Interface;
            else if (id == "enum") type = LsmTokenType::Enum;
            else if (id == "type") type = LsmTokenType::Type;
            else if (id == "and") type = LsmTokenType::And;
            else if (id == "or") type = LsmTokenType::Or;
            else if (id == "not") type = LsmTokenType::Not;
            else if (id == "try") type = LsmTokenType::Try;
            else if (id == "catch") type = LsmTokenType::Catch;
            else if (id == "throw") type = LsmTokenType::Throw;
            else if (id == "let") type = LsmTokenType::Let;
            else if (id == "nil") type = LsmTokenType::Nil;
            else if (id == "slice") type = LsmTokenType::Slice;
            else if (id == "match") type = LsmTokenType::Match;
            else if (id == "Ok") type = LsmTokenType::Ok;
            else if (id == "Err") type = LsmTokenType::Err;
            else if (id == "Some") type = LsmTokenType::Some;
            else if (id == "None") type = LsmTokenType::None;

            tokens.push_back({type, id, lineStart, colStart, start});
            continue;
        }

        if (std::isdigit(c) || (c == '.' && std::isdigit(peekNext()))) {
            bool hasDot = false;
            if (c == '0' && (peekNext() == 'x' || peekNext() == 'X')) {
                advance(); advance();
                while (std::isxdigit(peek())) advance();
                std::string_view hexNum(&src[start], pos - start);
                tokens.push_back({LsmTokenType::Int, hexNum, lineStart, colStart, start});
                continue;
            }

            while (std::isdigit(peek()) || peek() == '.') {
                if (peek() == '.') {
                    if (peekNext() == '.') break;
                    if (hasDot) { error("Multiple decimal points"); break; }
                    hasDot = true;
                }
                advance();
            }
            std::string_view num(&src[start], pos - start);
            tokens.push_back({hasDot ? LsmTokenType::Float : LsmTokenType::Int, num, lineStart, colStart, start});
            continue;
        }

        if (c == '"') {
            advance();
            bool closed = false;
            while (peek() != '\0' && peek() != '\n') {
                if (peek() == '\\') { advance(); if (peek() != '\0') advance(); continue; }
                if (peek() == '"') { closed = true; break; }
                advance();
            }
            std::string_view str(&src[start + 1], pos - start - 1);
            if (closed) advance();
            else error("Unterminated string literal");
            tokens.push_back({LsmTokenType::String, str, lineStart, colStart, start});
            continue;
        }

        advance();
        switch (c) {
            case ';': tokens.push_back({LsmTokenType::Semicolon, ";", lineStart, colStart, start}); break;
            case '?':
                if (match('.')) tokens.push_back({LsmTokenType::QuestionSafe, "?.", lineStart, colStart, start});
                else tokens.push_back({LsmTokenType::Question, "?", lineStart, colStart, start});
                break;
            case ':': tokens.push_back({LsmTokenType::Colon, ":", lineStart, colStart, start}); break;
            case '=':
                if (match('>')) tokens.push_back({LsmTokenType::FatArrow, "=>", lineStart, colStart, start});
                else if (match('=')) tokens.push_back({LsmTokenType::EqEq, "==", lineStart, colStart, start});
                else tokens.push_back({LsmTokenType::Eq, "=", lineStart, colStart, start});
                break;
            case '-':
                if (match('>')) tokens.push_back({LsmTokenType::ThinArrow, "->", lineStart, colStart, start});
                else if (match('=')) tokens.push_back({LsmTokenType::MinusEq, "-=", lineStart, colStart, start});
                else if (match('-')) tokens.push_back({LsmTokenType::MinusMinus, "--", lineStart, colStart, start});
                else tokens.push_back({LsmTokenType::Minus, "-", lineStart, colStart, start});
                break;
            case '<':
                if (match('-')) tokens.push_back({LsmTokenType::ChanSendRecv, "<-", lineStart, colStart, start});
                else if (match('=')) tokens.push_back({LsmTokenType::LessEq, "<=", lineStart, colStart, start});
                else if (match('<')) tokens.push_back({LsmTokenType::Shl, "<<", lineStart, colStart, start});
                else tokens.push_back({LsmTokenType::Less, "<", lineStart, colStart, start});
                break;
            case '>':
                if (match('=')) tokens.push_back({LsmTokenType::GreaterEq, ">=", lineStart, colStart, start});
                else if (match('>')) tokens.push_back({LsmTokenType::Shr, ">>", lineStart, colStart, start});
                else tokens.push_back({LsmTokenType::Greater, ">", lineStart, colStart, start});
                break;
            case '&':
                if (match('&')) tokens.push_back({LsmTokenType::AmpAmp, "&&", lineStart, colStart, start});
                else tokens.push_back({LsmTokenType::Amp, "&", lineStart, colStart, start});
                break;
            case '|':
                if (match('|')) tokens.push_back({LsmTokenType::PipePipe, "||", lineStart, colStart, start});
                else tokens.push_back({LsmTokenType::Pipe, "|", lineStart, colStart, start});
                break;
            case '!':
                if (match('=')) tokens.push_back({LsmTokenType::NotEq, "!=", lineStart, colStart, start});
                else tokens.push_back({LsmTokenType::Not, "!", lineStart, colStart, start});
                break;
            case '+':
                if (match('=')) tokens.push_back({LsmTokenType::PlusEq, "+=", lineStart, colStart, start});
                else if (match('+')) tokens.push_back({LsmTokenType::PlusPlus, "++", lineStart, colStart, start});
                else tokens.push_back({LsmTokenType::Plus, "+", lineStart, colStart, start});
                break;
            case '*':
                if (match('=')) tokens.push_back({LsmTokenType::StarEq, "*=", lineStart, colStart, start});
                else tokens.push_back({LsmTokenType::Star, "*", lineStart, colStart, start});
                break;
            case '/':
                if (match('=')) tokens.push_back({LsmTokenType::SlashEq, "/=", lineStart, colStart, start});
                else tokens.push_back({LsmTokenType::Slash, "/", lineStart, colStart, start});
                break;
            case '%': tokens.push_back({LsmTokenType::Percent, "%", lineStart, colStart, start}); break;
            case '^': tokens.push_back({LsmTokenType::Caret, "^", lineStart, colStart, start}); break;
            case '~': tokens.push_back({LsmTokenType::Tilde, "~", lineStart, colStart, start}); break;
            case '(': tokens.push_back({LsmTokenType::LParen, "(", lineStart, colStart, start}); break;
            case ')': tokens.push_back({LsmTokenType::RParen, ")", lineStart, colStart, start}); break;
            case '{': tokens.push_back({LsmTokenType::LBrace, "{", lineStart, colStart, start}); break;
            case '}': tokens.push_back({LsmTokenType::RBrace, "}", lineStart, colStart, start}); break;
            case '[': tokens.push_back({LsmTokenType::LBracket, "[", lineStart, colStart, start}); break;
            case ']': tokens.push_back({LsmTokenType::RBracket, "]", lineStart, colStart, start}); break;
            case ',': tokens.push_back({LsmTokenType::Comma, ",", lineStart, colStart, start}); break;
            case '.':
                if (match('.')) {
                    if (match('=')) tokens.push_back({LsmTokenType::RangeInclusive, "..=", lineStart, colStart, start});
                    else tokens.push_back({LsmTokenType::Range, "..", lineStart, colStart, start});
                } else {
                    tokens.push_back({LsmTokenType::Dot, ".", lineStart, colStart, start});
                }
                break;
            case '#': tokens.push_back({LsmTokenType::Hash, "#", lineStart, colStart, start}); break;
            case '@': tokens.push_back({LsmTokenType::At, "@", lineStart, colStart, start}); break;
            default: error("Unexpected character: '" + std::string(1, c) + "'"); break;
        }
    }

    tokens.push_back({LsmTokenType::Eof, "", line, col, pos});
    return tokens;
}