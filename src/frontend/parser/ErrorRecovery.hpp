#ifndef LSM_ERROR_RECOVERY_HPP
#define LSM_ERROR_RECOVERY_HPP

#include "../Token.hpp"
#include <vector>
#include <string>
#include <iostream>

struct DiagnosticError {
    std::string message;
    size_t line;
    size_t col;
};

class ErrorRecoveryHandler {
private:
    std::vector<DiagnosticError> errors;

public:
    void reportError(const std::string& msg, size_t line, size_t col) {
        errors.push_back({msg, line, col});
        std::cerr << "\033[1;31m[Syntax Diagnostic] Line " << line << ", Col " << col << ": " << msg << "\033[0m\n";
    }

    bool hasErrors() const { return !errors.empty(); }
    const std::vector<DiagnosticError>& getDiagnostics() const { return errors; }

    
    void synchronize(const std::vector<Token>& tokens, size_t& pos) {
        if (pos < tokens.size()) pos++;
        while (pos < tokens.size() && tokens[pos].type != LsmTokenType::Eof) {
            if (tokens[pos - 1].type == LsmTokenType::Semicolon || tokens[pos - 1].type == LsmTokenType::RBrace) {
                return;
            }
            switch (tokens[pos].type) {
                case LsmTokenType::Fct:
                case LsmTokenType::Proc:
                case LsmTokenType::Let:
                case LsmTokenType::If:
                case LsmTokenType::While:
                case LsmTokenType::For:
                case LsmTokenType::Return:
                    return;
                default:
                    break;
            }
            pos++;
        }
    }
};

#endif 