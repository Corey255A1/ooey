#pragma once

#include <string>
#include <vector>

namespace tooey {

enum class TokenType {
    INDENT,
    USE,
    ELEMENT,
    ATTR_KEY,
    INLINE_KEY,
    ID_ASSIGN,
    BINDING,
    SIGNAL,
    THEME,
    LOCALIZATION,
    STRING,
    NUMBER,
    BOOLEAN,
    COLON,
    EQUAL,
    LBRACKET,
    RBRACKET,
    COMMA,
    AI_BLOCK,
    COMMENT,
    NEWLINE,
    UNKNOWN,
    END_OF_FILE
};

std::string to_string(TokenType type);

struct Token {
    TokenType type;
    std::string text;
    size_t start_offset = 0;
    size_t end_offset = 0;
    int line = 0;
    int column = 0;
};

class Lexer {
public:
    static std::vector<Token> tokenize(const std::string& source);
};

} // namespace tooey
