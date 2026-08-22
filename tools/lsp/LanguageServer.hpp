#ifndef LSM_TOOLS_LANGUAGE_SERVER_HPP
#define LSM_TOOLS_LANGUAGE_SERVER_HPP

#include "JSONRPC.hpp"
#include "../../src/frontend/lexer/Lexer.hpp"
#include "../../src/frontend/parser/Parser.hpp"
#include <string>
#include <memory>

class LanguageServer {
private:
    bool isRunning = true;
    std::string currentSourceCode;

    void handleInitialize(const std::string& id);
    void handleDidOpen(const std::string& documentText);
    void handleDidChange(const std::string& documentText);
    void handleHover(const std::string& id, size_t line, size_t col);
    void handleCompletion(const std::string& id, size_t line, size_t col);
    
    void publishDiagnostics(const std::vector<DiagnosticError>& errors);

public:
    LanguageServer() = default;
    void run();
};

#endif 