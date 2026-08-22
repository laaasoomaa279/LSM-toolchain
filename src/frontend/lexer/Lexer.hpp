#ifndef LSM_LEXER_HPP
#define LSM_LEXER_HPP

#include "../Token.hpp"
#include <string>
#include <vector>

class Lexer {
public:
    explicit Lexer(std::string src);
    std::vector<Token> tokenize();
    bool hasErrors() const { return err; }

private:
    std::string src;
    size_t pos = 0, line = 1, col = 1;
    bool err = false;

    char peek() const;
    char peekNext() const;
    char advance();
    bool match(char c);
    void skipWhitespaceAndComments();
    void error(const std::string& msg);
};

#endif 