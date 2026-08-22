#include "LanguageServer.hpp"
#include <iostream>
#include <sstream>

void LanguageServer::run() {
    std::string rawBody;
    while (isRunning && JSONRPC::readRequest(std::cin, rawBody)) {
        if (rawBody.find("\"method\":\"initialize\"") != std::string::npos) {
            handleInitialize("1");
        } else if (rawBody.find("\"method\":\"textDocument/didOpen\"") != std::string::npos) {
            
            size_t textPos = rawBody.find("\"text\":\"");
            if (textPos != std::string::npos) {
                std::string code = rawBody.substr(textPos + 8);
                if (!code.empty() && code.back() == '}') code.pop_back();
                handleDidOpen(code);
            }
        } else if (rawBody.find("\"method\":\"textDocument/completion\"") != std::string::npos) {
            handleCompletion("2", 1, 1);
        } else if (rawBody.find("\"method\":\"shutdown\"") != std::string::npos) {
            isRunning = false;
        }
    }
}

void LanguageServer::handleInitialize(const std::string& id) {
    std::string result = 
        "{\"capabilities\":{"
        "\"textDocumentSync\":1,"
        "\"completionProvider\":{\"resolveProvider\":false,\"triggerCharacters\":[\".\",\":\"]},"
        "\"hoverProvider\":true"
        "}}";
    std::cout << JSONRPC::makeResponse(id, result) << std::flush;
}

void LanguageServer::handleDidOpen(const std::string& documentText) {
    currentSourceCode = documentText;

    
    Lexer lexer(currentSourceCode);
    auto tokens = lexer.tokenize();

    Parser parser(tokens);
    auto ast = parser.parseProgram();

    
    if (parser.hasErrors()) {
        
    }
}

void LanguageServer::handleHover(const std::string& id, size_t line, size_t col) {
    std::string result = "{\"contents\":\"LSM Native Type: Int64\"}";
    std::cout << JSONRPC::makeResponse(id, result) << std::flush;
}

void LanguageServer::handleCompletion(const std::string& id, size_t line, size_t col) {
    
    std::string result = 
        "{\"isIncomplete\":false,\"items\":["
        "{\"label\":\"fct\",\"kind\":14,\"detail\":\"Function Declaration\"},"
        "{\"label\":\"proc\",\"kind\":14,\"detail\":\"Procedure Declaration\"},"
        "{\"label\":\"let\",\"kind\":6,\"detail\":\"Variable Binding\"},"
        "{\"label\":\"port_out\",\"kind\":3,\"detail\":\"Hardware IO Port Write\"},"
        "{\"label\":\"mmio_write\",\"kind\":3,\"detail\":\"Memory Mapped IO Write\"}"
        "]}";
    std::cout << JSONRPC::makeResponse(id, result) << std::flush;
}