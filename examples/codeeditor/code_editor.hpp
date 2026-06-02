#pragma once

#include "gooey/controls/rich_text_box.hpp"
#include <unordered_set>
#include <unordered_map>
#include <memory>

namespace gooey::controls {
    using namespace ooey;

enum class TokenType {
    Normal,
    Keyword,
    Type,
    Comment,
    String,
    Number,
    Preprocessor,
    Operator
};

struct HighlightedToken {
    std::string text;
    TokenType type;
};

class ISyntaxHighlighter {
public:
    virtual ~ISyntaxHighlighter() = default;
    virtual std::vector<HighlightedToken> highlight(const std::string& line, int start_state, int& out_end_state) = 0;
};

class CppSyntaxHighlighter : public ISyntaxHighlighter {
public:
    std::vector<HighlightedToken> highlight(const std::string& line, int start_state, int& out_end_state) override;
private:
    static const std::unordered_set<std::string> keywords_;
    static const std::unordered_set<std::string> types_;
};

class CodeEditor : public RichTextBox {
public:
    CodeEditor(Rect bounds, Font font, Color text_color, Color bg_color);

    void set_syntax_highlighter(std::shared_ptr<ISyntaxHighlighter> highlighter);

    std::unordered_map<TokenType, Color> token_colors = {
        { TokenType::Normal, Color{220, 220, 220} },
        { TokenType::Keyword, Color{86, 156, 214} },
        { TokenType::Type, Color{78, 201, 176} },
        { TokenType::Comment, Color{106, 153, 85} },
        { TokenType::String, Color{206, 145, 120} },
        { TokenType::Number, Color{181, 206, 168} },
        { TokenType::Preprocessor, Color{189, 99, 197} },
        { TokenType::Operator, Color{180, 180, 180} }
    };

protected:
    void update_formatting() override;

private:
    std::shared_ptr<ISyntaxHighlighter> highlighter_;
    std::vector<int> line_start_states_;
};

} // namespace gooey::controls

namespace gooey {
    using namespace ooey;
using gooey::controls::CodeEditor;
using gooey::controls::TokenType;
using gooey::controls::HighlightedToken;
using gooey::controls::ISyntaxHighlighter;
using gooey::controls::CppSyntaxHighlighter;
}
