#pragma once

#include "gooey/mvvmc/view.hpp"
#include "gooey/mvvmc/i_interactive.hpp"
#include "gooey/controls/scrollbar.hpp"
#include "ooey/renderer/font_engine.hpp"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <functional>

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

class RichTextBox : public View, public IInteractive {
public:
    RichTextBox(Rect bounds, Font font, Color text_color, Color bg_color);

    Rect bounds() const override;

    void set_text(const std::string& text);
    std::string get_text() const;

    void set_font(const Font& font);
    const Font& get_font() const;

    void set_syntax_highlighter(std::shared_ptr<ISyntaxHighlighter> highlighter);
    
    bool on_pointer_event(const Pointer& e) override;
    bool on_key_event(const KeyEvent& e) override;
    bool on_text_event(const TextEvent& e) override;

    void draw(ooey::IRenderTarget& target) const override;

    std::string get_selected_text() const;
    void insert_text(const std::string& text);
    bool has_selection() const { return has_selection_; }
    void clear_selection() { has_selection_ = false; }

    std::function<void(const std::string&)> on_text_changed;

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
    
    Color bg_color = Color{30, 30, 30};
    Color line_num_bg = Color{38, 38, 38};
    Color line_num_color = Color{120, 120, 120};
    Color divider_color = Color{60, 60, 60};
    Color cursor_color = Color{220, 220, 220};
    Color selection_color = Color{51, 153, 255, 100};

    bool show_line_numbers{false};

protected:
    Size do_measure(Size constraints) override;
    void do_layout(Rect bounds) override;

    void update_line_states();
    void sync_scrollbar();
    void make_cursor_visible(int visible_lines);
    void get_selection_ordered(int& start_line, int& start_col, int& end_line, int& end_col) const;

    Rect bounds_;
    Font font_;
    
    std::vector<std::string> lines_;
    std::vector<int> line_start_states_;
    
    int cursor_line_{0};
    int cursor_col_{0};
    int scroll_line_{0};

    int anchor_line_{0};
    int anchor_col_{0};
    bool has_selection_{false};
    bool dragging_selection_{false};
    bool shift_pressed_{false};

    std::shared_ptr<ISyntaxHighlighter> highlighter_;
    std::shared_ptr<ScrollBar> scrollbar_;
};

} // namespace gooey::controls
namespace gooey {
    using namespace ooey;
using gooey::controls::RichTextBox;
using gooey::controls::TokenType;
using gooey::controls::HighlightedToken;
using gooey::controls::ISyntaxHighlighter;
using gooey::controls::CppSyntaxHighlighter;
}
