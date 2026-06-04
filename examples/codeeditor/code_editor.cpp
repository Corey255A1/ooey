#include "code_editor.hpp"
#include <cctype>
#include <utility>

namespace gooey::controls {
    using namespace ooey;

// ---------------------------------------------------------
// CppSyntaxHighlighter Implementation
// ---------------------------------------------------------

const std::unordered_set<std::string> CppSyntaxHighlighter::keywords_ = {
    "alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel", "atomic_commit",
    "atomic_noexcept", "auto", "bitand", "bitor", "break", "case", "catch", "class",
    "co_await", "co_return", "co_yield", "compl", "concept", "const", "const_cast",
    "consteval", "constexpr", "constinit", "continue", "decltype", "default", "delete",
    "do", "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false",
    "for", "friend", "goto", "if", "inline", "mutable", "namespace", "new", "noexcept",
    "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected",
    "public", "reflexpr", "register", "reinterpret_cast", "requires", "return",
    "sizeof", "static", "static_assert", "static_cast", "struct", "switch", "synchronized",
    "template", "this", "thread_local", "throw", "true", "try", "typedef", "typeid",
    "typename", "union", "using", "virtual", "volatile", "xor", "xor_eq", "while"
};

const std::unordered_set<std::string> CppSyntaxHighlighter::types_ = {
    "bool", "char", "char8_t", "char16_t", "char32_t", "double", "float",
    "int", "long", "short", "signed", "unsigned", "void", "wchar_t",
    "size_t", "ssize_t", "int8_t", "int16_t", "int32_t", "int64_t",
    "uint8_t", "uint16_t", "uint32_t", "uint64_t", "std::string",
    "std::vector", "std::shared_ptr", "std::unique_ptr", "std::map", "std::set",
    "std::unordered_map", "std::unordered_set"
};

std::vector<HighlightedToken> CppSyntaxHighlighter::highlight(const std::string& line, int start_state, int& out_end_state) {
    std::vector<HighlightedToken> tokens;
    size_t i = 0;
    int state = start_state;
    std::string current_token = "";

    auto emit = [&](TokenType type) {
        if (!current_token.empty()) {
            tokens.push_back({current_token, type});
            current_token.clear();
        }
    };

    while (i < line.size()) {
        if (state == 1) {
            current_token += line[i];
            if (i + 1 < line.size() && line[i] == '*' && line[i+1] == '/') {
                current_token += '/';
                i += 2;
                state = 0;
                emit(TokenType::Comment);
            } else {
                i++;
            }
            continue;
        }

        if (line[i] == '/' && i + 1 < line.size() && line[i+1] == '/') {
            emit(TokenType::Normal);
            current_token = line.substr(i);
            emit(TokenType::Comment);
            break;
        }

        if (line[i] == '/' && i + 1 < line.size() && line[i+1] == '*') {
            emit(TokenType::Normal);
            current_token = "/*";
            i += 2;
            state = 1;
            continue;
        }

        if (line[i] == '"') {
            emit(TokenType::Normal);
            current_token += line[i++];
            while (i < line.size() && line[i] != '"') {
                if (line[i] == '\\' && i + 1 < line.size()) {
                    current_token += line[i++];
                }
                current_token += line[i++];
            }
            if (i < line.size()) {
                current_token += line[i++];
            }
            emit(TokenType::String);
            continue;
        }

        if (line[i] == '\'') {
            emit(TokenType::Normal);
            current_token += line[i++];
            while (i < line.size() && line[i] != '\'') {
                if (line[i] == '\\' && i + 1 < line.size()) {
                    current_token += line[i++];
                }
                current_token += line[i++];
            }
            if (i < line.size()) {
                current_token += line[i++];
            }
            emit(TokenType::String);
            continue;
        }

        if (line[i] == '#' && current_token.empty()) {
            current_token += line[i++];
            while (i < line.size() && std::isalnum(static_cast<unsigned char>(line[i]))) {
                current_token += line[i++];
            }
            emit(TokenType::Preprocessor);
            continue;
        }

        if (std::isalpha(static_cast<unsigned char>(line[i])) || line[i] == '_') {
            emit(TokenType::Normal);
            while (i < line.size() && (std::isalnum(static_cast<unsigned char>(line[i])) || line[i] == '_')) {
                current_token += line[i++];
            }
            if (keywords_.count(current_token)) {
                emit(TokenType::Keyword);
            } else if (types_.count(current_token)) {
                emit(TokenType::Type);
            } else {
                emit(TokenType::Normal);
            }
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(line[i]))) {
            emit(TokenType::Normal);
            while (i < line.size() && (std::isalnum(static_cast<unsigned char>(line[i])) || line[i] == '.')) {
                current_token += line[i++];
            }
            emit(TokenType::Number);
            continue;
        }

        if (std::string("+-*/%=&|^!<>?:~.").find(line[i]) != std::string::npos) {
            emit(TokenType::Normal);
            current_token += line[i++];
            emit(TokenType::Operator);
            continue;
        }

        current_token += line[i++];
    }

    emit(TokenType::Normal);
    out_end_state = state;
    return tokens;
}

// ---------------------------------------------------------
// CodeEditor Implementation
// ---------------------------------------------------------

CodeEditor::CodeEditor(Rect bounds, Font font, Color text_color, Color bg_color)
    : RichTextBox(bounds, font, text_color, bg_color) {
    show_line_numbers = true;
    set_syntax_highlighter(std::make_shared<CppSyntaxHighlighter>());
}

void CodeEditor::set_syntax_highlighter(std::shared_ptr<ISyntaxHighlighter> highlighter) {
    highlighter_ = std::move(highlighter);
    update_formatting();
}

void CodeEditor::update_formatting() {
    int lines_count = get_lines_count();
    if (line_start_states_.size() != static_cast<size_t>(lines_count)) {
        line_start_states_.resize(lines_count, 0);
    }

    clear_formats();

    if (!highlighter_) {
        return;
    }

    int state = 0;
    for (int i = 0; i < lines_count; ++i) {
        line_start_states_[i] = state;
        int next_state = state;

        std::string line_text = get_line_text(i);
        std::vector<HighlightedToken> tokens = highlighter_->highlight(line_text, state, next_state);

        std::vector<FormatRange> formats;
        int col = 0;
        for (const auto& token : tokens) {
            int len = static_cast<int>(token.text.size());
            if (len > 0) {
                TextFormat fmt;
                fmt.color = token_colors.count(token.type) ? token_colors.at(token.type) : default_text_color;
                if (token.type == TokenType::Keyword) {
                    fmt.weight = FontWeight::Bold;
                }
                formats.push_back({col, col + len, fmt});
                col += len;
            }
        }
        set_line_formats(i, formats);
        state = next_state;
    }
}

} // namespace gooey::controls
