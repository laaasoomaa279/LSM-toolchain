#ifndef LSM_MODULE_LOADER_HPP
#define LSM_MODULE_LOADER_HPP

#include "../frontend/lexer/Lexer.hpp"
#include "../frontend/parser/Parser.hpp"
#include "../frontend/ast/AST.hpp"
#include "../core/Result.hpp"

#include <string>
#include <vector>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <memory>

class ModuleLoader {
private:
    std::unordered_set<std::string> loadedModules;
    std::unordered_set<std::string> visitingModules; 
    std::vector<std::string> searchPaths;

    std::string readFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return "";
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    std::string resolvePath(const std::string& moduleName) {
        std::ifstream direct(moduleName);
        if (direct.is_open()) return moduleName;

        for (const auto& basePath : searchPaths) {
            std::string candidate = basePath + "/" + moduleName;
            std::ifstream test(candidate);
            if (test.is_open()) return candidate;

            if (moduleName.find(".lsm") == std::string::npos) {
                candidate += ".lsm";
                std::ifstream testWithExt(candidate);
                if (testWithExt.is_open()) return candidate;
            }
        }
        return moduleName;
    }

public:
    ModuleLoader() {
        searchPaths.push_back(".");
        searchPaths.push_back("libs");
        searchPaths.push_back("libs/std");
    }

    void addSearchPath(const std::string& path) {
        searchPaths.push_back(path);
    }

    Result<void, std::string> loadAndMerge(const std::string& rootFilePath, std::unique_ptr<ProgramNode>& rootProgram) {
        std::string actualPath = resolvePath(rootFilePath);
        
        if (loadedModules.count(actualPath)) {
            return Result<void, std::string>::Ok();
        }

        
        if (visitingModules.count(actualPath)) {
            return Result<void, std::string>::Err("ModuleLoader: Circular import detected -> " + actualPath);
        }

        std::string source = readFile(actualPath);
        if (source.empty()) {
            return Result<void, std::string>::Err("ModuleLoader: Failed to open or empty file -> " + actualPath);
        }

        visitingModules.insert(actualPath);

        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        if (lexer.hasErrors()) {
            return Result<void, std::string>::Err("ModuleLoader: Lexer error while processing " + actualPath);
        }

        Parser parser(tokens);
        auto parsedProg = parser.parseProgram();
        if (parser.hasErrors() || !parsedProg) {
            return Result<void, std::string>::Err("ModuleLoader: Syntax error in " + actualPath);
        }

        for (auto it = parsedProg->stmts.begin(); it != parsedProg->stmts.end();) {
            if ((*it)->type == ASTNodeType::Import) {
                auto impNode = static_cast<ImportNode*>(it->get());
                if (!impNode->isCHeader) {
                    auto res = loadAndMerge(impNode->path, rootProgram);
                    if (res.isErr()) return res;
                }
                it = parsedProg->stmts.erase(it);
                continue;
            }
            ++it;
        }

        for (auto& stmt : parsedProg->stmts) {
            rootProgram->stmts.push_back(std::move(stmt));
        }

        visitingModules.erase(actualPath);
        loadedModules.insert(actualPath);

        return Result<void, std::string>::Ok();
    }
};

#endif 