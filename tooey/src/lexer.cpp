#include "tooey/lexer.hpp"
#include <cctype>
#include <stdexcept>

namespace tooey {

std::string to_string(TokenType type) {
    switch (type) {
        case TokenType::INDENT: return "INDENT";
        case TokenType::USE: return "USE";
        case TokenType::ELEMENT: return "ELEMENT";
        case TokenType::ATTR_KEY: return "ATTR_KEY";
        case TokenType::INLINE_KEY: return "INLINE_KEY";
        case TokenType::ID_ASSIGN: return "ID_ASSIGN";
        case TokenType::BINDING: return "BINDING";
        case TokenType::SIGNAL: return "SIGNAL";
        case TokenType::THEME: return "THEME";
        case TokenType::LOCALIZATION: return "LOCALIZATION";
        case TokenType::STRING: return "STRING";
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::BOOLEAN: return "BOOLEAN";
        case TokenType::COLON: return "COLON";
        case TokenType::EQUAL: return "EQUAL";
        case TokenType::LBRACKET: return "LBRACKET";
        case TokenType::RBRACKET: return "RBRACKET";
        case TokenType::COMMA: return "COMMA";
        case TokenType::AI_BLOCK: return "AI_BLOCK";
        case TokenType::COMMENT: return "COMMENT";
        case TokenType::NEWLINE: return "NEWLINE";
        case TokenType::UNKNOWN: return "UNKNOWN";
        case TokenType::END_OF_FILE: return "END_OF_FILE";
    }
    return "UNKNOWN";
}

std::vector<Token> Lexer::tokenize(const std::string& source) {
    std::vector<Token> tokens;
    size_t offset = 0;
    int line = 1;
    int col = 1;
    size_t length = source.length();

    auto peek = [&](size_t ahead = 0) -> char {
        if (offset + ahead >= length) return '\0';
        return source[offset + ahead];
    };

    auto consume = [&]() -> char {
        if (offset >= length) return '\0';
        char c = source[offset++];
        if (c == '\n') {
            line++;
            col = 1;
        } else {
            col++;
        }
        return c;
    };

    auto match_str = [&](const std::string& s) -> bool {
        if (offset + s.length() > length) return false;
        for (size_t i = 0; i < s.length(); ++i) {
            if (source[offset + i] != s[i]) return false;
        }
        return true;
    };

    auto consume_str = [&](const std::string& s) {
        for (size_t i = 0; i < s.length(); ++i) {
            consume();
        }
    };

    bool start_of_line = true;

    while (offset < length) {
        // 1. Check for indentation at the start of a line
        if (start_of_line) {
            size_t space_count = 0;
            size_t start_off = offset;
            int start_col = col;
            while (peek() == ' ') {
                space_count++;
                consume();
            }
            if (space_count > 0) {
                Token tok;
                tok.type = TokenType::INDENT;
                tok.text = source.substr(start_off, space_count);
                tok.start_offset = start_off;
                tok.end_offset = offset;
                tok.line = line;
                tok.column = start_col;
                tokens.push_back(tok);
            }
            start_of_line = false;
            continue;
        }

        char c = peek();

        // Newline
        if (c == '\n' || c == '\r') {
            size_t start_off = offset;
            int start_line = line;
            int start_col = col;
            std::string nl_text = "";
            if (c == '\r' && peek(1) == '\n') {
                nl_text = "\r\n";
                consume();
                consume();
            } else {
                nl_text = std::string(1, c);
                consume();
            }
            Token tok;
            tok.type = TokenType::NEWLINE;
            tok.text = nl_text;
            tok.start_offset = start_off;
            tok.end_offset = offset;
            tok.line = start_line;
            tok.column = start_col;
            tokens.push_back(tok);
            start_of_line = true;
            continue;
        }

        // Inline whitespace
        if (c == ' ' || c == '\t') {
            consume();
            continue;
        }

        size_t start_off = offset;
        int start_line = line;
        int start_col = col;

        // Comment
        if (c == '/' && peek(1) == '/') {
            std::string comment_text = "";
            while (peek() != '\0' && peek() != '\n' && peek() != '\r') {
                comment_text += consume();
            }
            Token tok;
            tok.type = TokenType::COMMENT;
            tok.text = comment_text;
            tok.start_offset = start_off;
            tok.end_offset = offset;
            tok.line = start_line;
            tok.column = start_col;
            tokens.push_back(tok);
            continue;
        }

        // AI Block: AI: \s* "[^"]*"
        if (match_str("AI:")) {
            consume_str("AI:");
            while (peek() == ' ' || peek() == '\t') {
                consume();
            }
            if (peek() == '"') {
                consume(); // opening quote
                std::string prompt = "";
                while (peek() != '\0' && peek() != '"') {
                    if (peek() == '\\' && peek(1) == '"') {
                        prompt += "\\\"";
                        consume();
                        consume();
                    } else {
                        prompt += consume();
                    }
                }
                if (peek() == '"') {
                    consume(); // closing quote
                }
                Token tok;
                tok.type = TokenType::AI_BLOCK;
                tok.text = prompt;
                tok.start_offset = start_off;
                tok.end_offset = offset;
                tok.line = start_line;
                tok.column = start_col;
                tokens.push_back(tok);
                continue;
            }
        }

        // id=[a-zA-Z0-9_]+
        if (match_str("id=")) {
            consume_str("id=");
            std::string id_val = "";
            while (std::isalnum(peek()) || peek() == '_') {
                id_val += consume();
            }
            Token tok;
            tok.type = TokenType::ID_ASSIGN;
            tok.text = id_val;
            tok.start_offset = start_off;
            tok.end_offset = offset;
            tok.line = start_line;
            tok.column = start_col;
            tokens.push_back(tok);
            continue;
        }

        // Handles
        if (match_str("@binding.")) {
            consume_str("@binding.");
            std::string bind_val = "";
            while (std::isalnum(peek()) || peek() == '_' || peek() == '.') {
                bind_val += consume();
            }
            Token tok;
            tok.type = TokenType::BINDING;
            tok.text = bind_val;
            tok.start_offset = start_off;
            tok.end_offset = offset;
            tok.line = start_line;
            tok.column = start_col;
            tokens.push_back(tok);
            continue;
        }

        if (match_str("@signal.")) {
            consume_str("@signal.");
            std::string sig_val = "";
            while (std::isalnum(peek()) || peek() == '_') {
                sig_val += consume();
            }
            // Check for optional arguments: ( ... )
            if (peek() == '(') {
                sig_val += consume(); // consume '('
                while (peek() != '\0' && peek() != ')') {
                    sig_val += consume();
                }
                if (peek() == ')') {
                    sig_val += consume(); // consume ')'
                }
            }
            Token tok;
            tok.type = TokenType::SIGNAL;
            tok.text = sig_val;
            tok.start_offset = start_off;
            tok.end_offset = offset;
            tok.line = start_line;
            tok.column = start_col;
            tokens.push_back(tok);
            continue;
        }

        if (match_str("@theme.")) {
            consume_str("@theme.");
            std::string theme_val = "";
            while (std::isalnum(peek()) || peek() == '_' || peek() == '.') {
                theme_val += consume();
            }
            Token tok;
            tok.type = TokenType::THEME;
            tok.text = theme_val;
            tok.start_offset = start_off;
            tok.end_offset = offset;
            tok.line = start_line;
            tok.column = start_col;
            tokens.push_back(tok);
            continue;
        }

        if (match_str("@tr(")) {
            consume_str("@tr(");
            char quote = '\0';
            if (peek() == '"' || peek() == '\'') {
                quote = consume();
            }
            std::string key = "";
            if (quote != '\0') {
                while (peek() != '\0' && peek() != quote) {
                    if (peek() == '\\' && peek(1) != '\0') {
                        key += consume();
                        key += consume();
                    } else {
                        key += consume();
                    }
                }
                if (peek() == quote) {
                    consume();
                }
            } else {
                while (std::isalnum(peek()) || peek() == '_' || peek() == '.') {
                    key += consume();
                }
            }
            if (peek() == ')') {
                consume();
            }
            Token tok;
            tok.type = TokenType::LOCALIZATION;
            tok.text = key;
            tok.start_offset = start_off;
            tok.end_offset = offset;
            tok.line = start_line;
            tok.column = start_col;
            tokens.push_back(tok);
            continue;
        }

        // Element: [A-Z][a-zA-Z0-9_]*
        if (std::isupper(c)) {
            std::string elem_name = "";
            while (std::isalnum(peek()) || peek() == '_') {
                elem_name += consume();
            }
            Token tok;
            tok.type = TokenType::ELEMENT;
            tok.text = elem_name;
            tok.start_offset = start_off;
            tok.end_offset = offset;
            tok.line = start_line;
            tok.column = start_col;
            tokens.push_back(tok);
            continue;
        }

        // String Literal: "[^"]*"
        if (c == '"') {
            consume(); // opening quote
            std::string str_val = "";
            while (peek() != '\0' && peek() != '"') {
                if (peek() == '\\' && peek(1) != '\0') {
                    str_val += consume();
                    str_val += consume();
                } else {
                    str_val += consume();
                }
            }
            if (peek() == '"') {
                consume(); // closing quote
            }
            Token tok;
            tok.type = TokenType::STRING;
            tok.text = str_val;
            tok.start_offset = start_off;
            tok.end_offset = offset;
            tok.line = start_line;
            tok.column = start_col;
            tokens.push_back(tok);
            continue;
        }

        // Numbers: -? [0-9]+ (. [0-9]+)? (px | % | em)?
        if (std::isdigit(c) || (c == '-' && std::isdigit(peek(1)))) {
            std::string num_text = "";
            if (c == '-') {
                num_text += consume();
            }
            while (std::isdigit(peek())) {
                num_text += consume();
            }
            if (peek() == '.' && std::isdigit(peek(1))) {
                num_text += consume(); // '.'
                while (std::isdigit(peek())) {
                    num_text += consume();
                }
            }
            // Check for units
            if (match_str("px")) {
                num_text += "px";
                consume_str("px");
            } else if (match_str("%")) {
                num_text += "%";
                consume_str("%");
            } else if (match_str("em")) {
                num_text += "em";
                consume_str("em");
            }
            Token tok;
            tok.type = TokenType::NUMBER;
            tok.text = num_text;
            tok.start_offset = start_off;
            tok.end_offset = offset;
            tok.line = start_line;
            tok.column = start_col;
            tokens.push_back(tok);
            continue;
        }

        // Special single character tokens
        if (c == ':') {
            consume();
            Token tok;
            tok.type = TokenType::COLON;
            tok.text = ":";
            tok.start_offset = start_off;
            tok.end_offset = offset;
            tok.line = start_line;
            tok.column = start_col;
            tokens.push_back(tok);
            continue;
        }
        if (c == '=') {
            consume();
            Token tok;
            tok.type = TokenType::EQUAL;
            tok.text = "=";
            tok.start_offset = start_off;
            tok.end_offset = offset;
            tok.line = start_line;
            tok.column = start_col;
            tokens.push_back(tok);
            continue;
        }
        if (c == '[') {
            consume();
            Token tok;
            tok.type = TokenType::LBRACKET;
            tok.text = "[";
            tok.start_offset = start_off;
            tok.end_offset = offset;
            tok.line = start_line;
            tok.column = start_col;
            tokens.push_back(tok);
            continue;
        }
        if (c == ']') {
            consume();
            Token tok;
            tok.type = TokenType::RBRACKET;
            tok.text = "]";
            tok.start_offset = start_off;
            tok.end_offset = offset;
            tok.line = start_line;
            tok.column = start_col;
            tokens.push_back(tok);
            continue;
        }
        if (c == ',') {
            consume();
            Token tok;
            tok.type = TokenType::COMMA;
            tok.text = ",";
            tok.start_offset = start_off;
            tok.end_offset = offset;
            tok.line = start_line;
            tok.column = start_col;
            tokens.push_back(tok);
            continue;
        }

        // Lowercase identifiers (use, booleans, attributes/inline keys)
        if (std::islower(c)) {
            std::string ident = "";
            while (std::isalnum(peek()) || peek() == '_' || peek() == '-') {
                ident += consume();
            }

            // Keyword: use
            if (ident == "use") {
                Token tok;
                tok.type = TokenType::USE;
                tok.text = ident;
                tok.start_offset = start_off;
                tok.end_offset = offset;
                tok.line = start_line;
                tok.column = start_col;
                tokens.push_back(tok);
                continue;
            }

            // Boolean
            if (ident == "true" || ident == "false") {
                Token tok;
                tok.type = TokenType::BOOLEAN;
                tok.text = ident;
                tok.start_offset = start_off;
                tok.end_offset = offset;
                tok.line = start_line;
                tok.column = start_col;
                tokens.push_back(tok);
                continue;
            }

            // Check if followed by colon -> ATTR_KEY
            // Or followed by equal -> INLINE_KEY
            size_t temp_off = 0;
            while (peek(temp_off) == ' ' || peek(temp_off) == '\t') {
                temp_off++;
            }
            char next_c = peek(temp_off);

            Token tok;
            tok.text = ident;
            tok.start_offset = start_off;
            tok.end_offset = offset;
            tok.line = start_line;
            tok.column = start_col;

            if (next_c == ':') {
                tok.type = TokenType::ATTR_KEY;
            } else if (next_c == '=') {
                tok.type = TokenType::INLINE_KEY;
            } else {
                tok.type = TokenType::UNKNOWN;
            }
            tokens.push_back(tok);
            continue;
        }

        // Unknown character
        consume();
        Token tok;
        tok.type = TokenType::UNKNOWN;
        tok.text = std::string(1, c);
        tok.start_offset = start_off;
        tok.end_offset = offset;
        tok.line = start_line;
        tok.column = start_col;
        tokens.push_back(tok);
    }

    // End of File Token
    Token tok;
    tok.type = TokenType::END_OF_FILE;
    tok.text = "";
    tok.start_offset = offset;
    tok.end_offset = offset;
    tok.line = line;
    tok.column = col;
    tokens.push_back(tok);

    return tokens;
}

} // namespace tooey
