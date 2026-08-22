#ifndef LSM_PARSER_HPP
#define LSM_PARSER_HPP

#include "modules/ParserStmt.hpp"

class Parser : public ParserStmt {
public:
    explicit Parser(std::vector<Token> tokens);
    std::unique_ptr<ProgramNode> parseProgram();
};

#endif 