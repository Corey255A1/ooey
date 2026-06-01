namespace ooey {}

#include "gooey/controls/code_editor.hpp"
#include "gooey/application.hpp"
#include "gooey/mvvmc/controller.hpp"
#include <cctype>
#include <algorithm>
#include <iostream>

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
    int state = start_state; // 0 = normal, 1 = multiline comment
    std::string current_token = "";

    auto emit = [&](TokenType type) {
        if (!current_token.empty()) {
            tokens.push_back({current_token, type});
            current_token.clear();
        }
    };

    while (i < line.size()) {
        if (state == 1) { // Inside multiline comment
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

        // Single line comment
        if (line[i] == '/' && i + 1 < line.size() && line[i+1] == '/') {
            emit(TokenType::Normal);
            current_token = line.substr(i);
            emit(TokenType::Comment);
            break;
        }

        // Multiline comment start
        if (line[i] == '/' && i + 1 < line.size() && line[i+1] == '*') {
            emit(TokenType::Normal);
            current_token = "/*";
            i += 2;
            state = 1;
            continue;
        }

        // String literal
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

        // Character literal
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

        // Preprocessor directives
        if (line[i] == '#' && current_token.empty()) {
            current_token += line[i++];
            while (i < line.size() && std::isalnum(static_cast<unsigned char>(line[i]))) {
                current_token += line[i++];
            }
            emit(TokenType::Preprocessor);
            continue;
        }

        // Identifiers / Keywords / Types
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

        // Numbers
        if (std::isdigit(static_cast<unsigned char>(line[i]))) {
            emit(TokenType::Normal);
            while (i < line.size() && (std::isalnum(static_cast<unsigned char>(line[i])) || line[i] == '.')) {
                current_token += line[i++];
            }
            emit(TokenType::Number);
            continue;
        }

        // Standard operators
        if (std::string("+-*/%=&|^!<>?:~.").find(line[i]) != std::string::npos) {
            emit(TokenType::Normal);
            current_token += line[i++];
            emit(TokenType::Operator);
            continue;
        }

        // Normal chars (whitespace, braces, semicolons)
        current_token += line[i++];
    }

    emit(TokenType::Normal);
    out_end_state = state;
    return tokens;
}

// ---------------------------------------------------------
// CodeEditor Implementation
// ---------------------------------------------------------

static Geometry make_rect_geometry(const Rect& rect, Color color) {
    Geometry geom;
    geom.type = PrimitiveType::Triangles;
    float x1 = static_cast<float>(rect.x);
    float y1 = static_cast<float>(rect.y);
    float x2 = static_cast<float>(rect.x + rect.width);
    float y2 = static_cast<float>(rect.y + rect.height);
    
    geom.vertices = {
        { x1, y1, color },
        { x2, y1, color },
        { x2, y2, color },
        { x1, y2, color }
    };
    geom.indices = { 0, 1, 2, 0, 2, 3 };
    return geom;
}

CodeEditor::CodeEditor(Rect bounds, Font font, Color text_color, Color bg_color)
    : bounds_(bounds), font_(font), lines_{""}, line_start_states_{0} {
    width = {SizePolicy::Fixed, static_cast<float>(bounds.width)};
    height = {SizePolicy::Fixed, static_cast<float>(bounds.height)};
    is_absolute = true;
    absolute_bounds = bounds;

    token_colors[TokenType::Normal] = text_color;
    this->bg_color = bg_color;

    scrollbar_ = std::make_shared<ScrollBar>(Rect{bounds.x + bounds.width - 12, bounds.y, 12, bounds.height}, ScrollBarOrientation::Vertical);
    scrollbar_->on_value_changed = [this](int val) {
        scroll_line_ = val;
    };
    add_child(scrollbar_);
}

Rect CodeEditor::bounds() const {
    return bounds_;
}

void CodeEditor::set_text(const std::string& text) {
    lines_.clear();
    std::string current_line = "";
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r') {
            if (i + 1 < text.size() && text[i + 1] == '\n') {
                lines_.push_back(current_line);
                current_line.clear();
                i++;
            } else {
                lines_.push_back(current_line);
                current_line.clear();
            }
        } else if (text[i] == '\n') {
            lines_.push_back(current_line);
            current_line.clear();
        } else {
            current_line += text[i];
        }
    }
    lines_.push_back(current_line);
    cursor_line_ = 0;
    cursor_col_ = 0;
    scroll_line_ = 0;
    update_line_states();
    sync_scrollbar();
    invalidate_layout();
    if (on_text_changed) {
        on_text_changed(get_text());
    }
}

std::string CodeEditor::get_text() const {
    std::string text = "";
    for (size_t i = 0; i < lines_.size(); ++i) {
        text += lines_[i];
        if (i + 1 < lines_.size()) {
            text += "\n";
        }
    }
    return text;
}

void CodeEditor::set_font(const Font& font) {
    font_ = font;
    invalidate_layout();
}

const Font& CodeEditor::get_font() const {
    return font_;
}

void CodeEditor::set_syntax_highlighter(std::shared_ptr<ISyntaxHighlighter> highlighter) {
    highlighter_ = highlighter;
    update_line_states();
}

void CodeEditor::update_line_states() {
    if (line_start_states_.size() != lines_.size()) {
        line_start_states_.resize(lines_.size(), 0);
    }
    int state = 0;
    for (size_t i = 0; i < lines_.size(); ++i) {
        line_start_states_[i] = state;
        int next_state = state;
        if (highlighter_) {
            highlighter_->highlight(lines_[i], state, next_state);
        }
        state = next_state;
    }
}

void CodeEditor::sync_scrollbar() {
    if (scrollbar_) {
        Size char_size = FontEngine::measure_text("A", font_);
        int visible_lines = std::max(1, bounds_.height / (char_size.height + 4));
        scrollbar_->set_range(0, lines_.size(), visible_lines);
        scrollbar_->set_value(scroll_line_);
    }
}

void CodeEditor::make_cursor_visible(int visible_lines) {
    if (cursor_line_ < scroll_line_) {
        scroll_line_ = cursor_line_;
    } else if (cursor_line_ >= scroll_line_ + visible_lines) {
        scroll_line_ = cursor_line_ - visible_lines + 1;
    }
    sync_scrollbar();
}

bool CodeEditor::on_pointer_event(const Pointer& e) {
    // Route to scrollbar first
    auto children = get_children();
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        auto* child_interactive = dynamic_cast<IInteractive*>(it->get());
        if (child_interactive) {
            Rect cb = child_interactive->bounds();
            if (e.x >= cb.x && e.x <= cb.x + cb.width &&
                e.y >= cb.y && e.y <= cb.y + cb.height) {
                if (child_interactive->on_pointer_event(e)) {
                    return true;
                }
            }
        }
    }

    bool hit = (e.x >= bounds_.x && e.x <= bounds_.x + bounds_.width &&
                e.y >= bounds_.y && e.y <= bounds_.y + bounds_.height);
    
    if (hit && e.state == PointerState::Pressed) {
        Size char_size = FontEngine::measure_text("A", font_);
        int line_h = char_size.height + 4;
        
        int max_line_digits = std::to_string(lines_.size()).size();
        int line_num_width = std::max(40, max_line_digits * char_size.width + 16);
        
        int text_area_x = bounds_.x + line_num_width + 8;
        int text_area_y = bounds_.y + 4;
        
        int clicked_line = scroll_line_ + (e.y - text_area_y) / line_h;
        clicked_line = std::max(0, std::min(clicked_line, static_cast<int>(lines_.size()) - 1));
        
        std::string line = lines_[clicked_line];
        int clicked_col = 0;
        int min_dist = 999999;
        for (size_t col = 0; col <= line.size(); ++col) {
            std::string prefix = line.substr(0, col);
            int char_x = text_area_x + FontEngine::measure_text(prefix, font_).width;
            int dist = std::abs(e.x - char_x);
            if (dist < min_dist) {
                min_dist = dist;
                clicked_col = col;
            }
        }
        
        cursor_line_ = clicked_line;
        cursor_col_ = clicked_col;
        invalidate_layout();
        return true;
    }
    
    return false;
}

bool CodeEditor::on_key_event(const KeyEvent& e) {
    if (e.state != KeyState::Pressed) return false;

    bool handled = false;
    
    if (e.key_code == 0xFF51 /* Left */) {
        if (cursor_col_ > 0) {
            cursor_col_--;
        } else if (cursor_line_ > 0) {
            cursor_line_--;
            cursor_col_ = lines_[cursor_line_].size();
        }
        handled = true;
    }
    else if (e.key_code == 0xFF53 /* Right */) {
        if (cursor_col_ < static_cast<int>(lines_[cursor_line_].size())) {
            cursor_col_++;
        } else if (cursor_line_ + 1 < static_cast<int>(lines_.size())) {
            cursor_line_++;
            cursor_col_ = 0;
        }
        handled = true;
    }
    else if (e.key_code == 0xFF52 /* Up */) {
        if (cursor_line_ > 0) {
            cursor_line_--;
            cursor_col_ = std::min(cursor_col_, static_cast<int>(lines_[cursor_line_].size()));
        }
        handled = true;
    }
    else if (e.key_code == 0xFF54 /* Down */) {
        if (cursor_line_ + 1 < static_cast<int>(lines_.size())) {
            cursor_line_++;
            cursor_col_ = std::min(cursor_col_, static_cast<int>(lines_[cursor_line_].size()));
        }
        handled = true;
    }
    else if (e.key_code == 0xFF50 /* Home */) {
        cursor_col_ = 0;
        handled = true;
    }
    else if (e.key_code == 0xFF57 /* End */) {
        cursor_col_ = lines_[cursor_line_].size();
        handled = true;
    }
    else if (e.key_code == 0xFF55 /* Page Up */) {
        cursor_line_ = std::max(0, cursor_line_ - 10);
        cursor_col_ = std::min(cursor_col_, static_cast<int>(lines_[cursor_line_].size()));
        handled = true;
    }
    else if (e.key_code == 0xFF56 /* Page Down */) {
        cursor_line_ = std::min(static_cast<int>(lines_.size()) - 1, cursor_line_ + 10);
        cursor_col_ = std::min(cursor_col_, static_cast<int>(lines_[cursor_line_].size()));
        handled = true;
    }
    else if (e.key_code == 0xFF08 /* Backspace */ || e.key_code == 8) {
        if (cursor_col_ > 0) {
            lines_[cursor_line_].erase(cursor_col_ - 1, 1);
            cursor_col_--;
            update_line_states();
            if (on_text_changed) on_text_changed(get_text());
        } else if (cursor_line_ > 0) {
            int old_len = lines_[cursor_line_ - 1].size();
            lines_[cursor_line_ - 1] += lines_[cursor_line_];
            lines_.erase(lines_.begin() + cursor_line_);
            cursor_line_--;
            cursor_col_ = old_len;
            update_line_states();
            sync_scrollbar();
            if (on_text_changed) on_text_changed(get_text());
        }
        handled = true;
    }
    else if (e.key_code == 0xFFFF /* Delete */ || e.key_code == 127) {
        if (cursor_col_ < static_cast<int>(lines_[cursor_line_].size())) {
            lines_[cursor_line_].erase(cursor_col_, 1);
            update_line_states();
            if (on_text_changed) on_text_changed(get_text());
        } else if (cursor_line_ + 1 < static_cast<int>(lines_.size())) {
            lines_[cursor_line_] += lines_[cursor_line_ + 1];
            lines_.erase(lines_.begin() + cursor_line_ + 1);
            update_line_states();
            sync_scrollbar();
            if (on_text_changed) on_text_changed(get_text());
        }
        handled = true;
    }
    else if (e.key_code == 0xFF0D /* Return */ || e.key_code == 13 || e.key_code == 10) {
        std::string suffix = lines_[cursor_line_].substr(cursor_col_);
        lines_[cursor_line_] = lines_[cursor_line_].substr(0, cursor_col_);
        
        std::string indent = "";
        for (char c : lines_[cursor_line_]) {
            if (c == ' ' || c == '\t') indent += c;
            else break;
        }
        
        lines_.insert(lines_.begin() + cursor_line_ + 1, indent + suffix);
        cursor_line_++;
        cursor_col_ = indent.size();
        
        update_line_states();
        sync_scrollbar();
        if (on_text_changed) on_text_changed(get_text());
        handled = true;
    }
    else if (e.key_code == 0xFF09 /* Tab */ || e.key_code == 9) {
        lines_[cursor_line_].insert(cursor_col_, "    ");
        cursor_col_ += 4;
        update_line_states();
        if (on_text_changed) on_text_changed(get_text());
        handled = true;
    }

    if (handled) {
        Size char_size = FontEngine::measure_text("A", font_);
        int visible_lines = std::max(1, bounds_.height / (char_size.height + 4));
        make_cursor_visible(visible_lines);
        invalidate_layout();
        return true;
    }

    return false;
}

bool CodeEditor::on_text_event(const TextEvent& e) {
    if (e.codepoint == 8 || e.codepoint == 127 || e.codepoint == '\n' || e.codepoint == '\r' || e.codepoint == '\t') {
        return true;
    }

    std::string text_to_insert = "";
    if (e.codepoint < 0x80) {
        if (e.codepoint >= 32) {
            text_to_insert += static_cast<char>(e.codepoint);
        }
    } else if (e.codepoint < 0x800) {
        text_to_insert += static_cast<char>(0xC0 | (e.codepoint >> 6));
        text_to_insert += static_cast<char>(0x80 | (e.codepoint & 0x3F));
    } else if (e.codepoint < 0x10000) {
        text_to_insert += static_cast<char>(0xE0 | (e.codepoint >> 12));
        text_to_insert += static_cast<char>(0x80 | ((e.codepoint >> 6) & 0x3F));
        text_to_insert += static_cast<char>(0x80 | (e.codepoint & 0x3F));
    } else {
        text_to_insert += static_cast<char>(0xF0 | (e.codepoint >> 18));
        text_to_insert += static_cast<char>(0x80 | ((e.codepoint >> 12) & 0x3F));
        text_to_insert += static_cast<char>(0x80 | ((e.codepoint >> 6) & 0x3F));
        text_to_insert += static_cast<char>(0x80 | (e.codepoint & 0x3F));
    }

    if (!text_to_insert.empty()) {
        lines_[cursor_line_].insert(cursor_col_, text_to_insert);
        cursor_col_ += text_to_insert.size();
        update_line_states();
        
        Size char_size = FontEngine::measure_text("A", font_);
        int visible_lines = std::max(1, bounds_.height / (char_size.height + 4));
        make_cursor_visible(visible_lines);
        
        invalidate_layout();
        if (on_text_changed) on_text_changed(get_text());
    }
    return true;
}

void CodeEditor::draw(ooey::IRenderTarget& target) const {
    auto draw_rect = [&](const Rect& r, Color color) {
        target.draw_geometry(make_rect_geometry(r, color));
    };

    draw_rect(bounds_, bg_color);

    Size char_size = target.measure_text("A", font_);
    int char_h = char_size.height;
    int line_h = char_h + 4;

    int max_line_digits = std::to_string(lines_.size()).size();
    int line_num_width = std::max(40, max_line_digits * char_size.width + 16);

    Rect line_num_rect{bounds_.x, bounds_.y, line_num_width, bounds_.height};
    draw_rect(line_num_rect, line_num_bg);

    Rect divider_rect{bounds_.x + line_num_width, bounds_.y, 1, bounds_.height};
    draw_rect(divider_rect, divider_color);

    int text_area_x = bounds_.x + line_num_width + 8;
    int text_area_width = bounds_.width - line_num_width - 20; // 12 scrollbar, 8 padding
    int text_area_y = bounds_.y + 4;
    int text_area_height = bounds_.height - 8;
    
    Rect text_area_bounds{text_area_x, text_area_y, text_area_width, text_area_height};
    
    int visible_lines = bounds_.height / line_h;
    int end_line = std::min(static_cast<int>(lines_.size()), scroll_line_ + visible_lines + 1);
    int y_pos = text_area_y;

    target.push_clip(text_area_bounds);
    for (int l = scroll_line_; l < end_line; ++l) {
        int current_y = y_pos + (l - scroll_line_) * line_h;
        
        std::vector<HighlightedToken> tokens;
        int start_state = line_start_states_[l];
        int next_state = start_state;
        if (highlighter_) {
            tokens = highlighter_->highlight(lines_[l], start_state, next_state);
        } else {
            tokens = {{lines_[l], TokenType::Normal}};
        }
        
        int token_x = text_area_x;
        for (const auto& token : tokens) {
            if (token.text.empty()) continue;
            Color color = token_colors.count(token.type) ? token_colors.at(token.type) : token_colors.at(TokenType::Normal);
            target.draw_text(token.text, font_, Point{token_x, current_y}, color);
            token_x += target.measure_text(token.text, font_).width;
        }

        bool is_focused = (Application::get_instance() && Application::get_instance()->get_controller() &&
                           dynamic_cast<gooey::mvvmc::Controller*>(Application::get_instance()->get_controller())->get_focused_element().get() == this);
        
        if (l == cursor_line_ && is_focused) {
            std::string prefix = lines_[l].substr(0, cursor_col_);
            int cursor_x = text_area_x + target.measure_text(prefix, font_).width;
            Rect cursor_rect{cursor_x, current_y, 2, char_h};
            target.draw_geometry(make_rect_geometry(cursor_rect, cursor_color));
        }
    }
    target.pop_clip();

    target.push_clip(line_num_rect);
    for (int l = scroll_line_; l < end_line; ++l) {
        int current_y = y_pos + (l - scroll_line_) * line_h;
        std::string line_num_str = std::to_string(l + 1);
        int line_num_x = bounds_.x + line_num_width - 8 - target.measure_text(line_num_str, font_).width;
        target.draw_text(line_num_str, font_, Point{line_num_x, current_y}, line_num_color);
    }
    target.pop_clip();

    // Draw scrollbar child view
    View::draw(target);
}

Size CodeEditor::do_measure(Size constraints) {
    int w = resolve_width(constraints.width, absolute_bounds.width);
    int h = resolve_height(constraints.height, absolute_bounds.height);
    return Size{w, h};
}

void CodeEditor::do_layout(Rect bounds) {
    bounds_ = bounds;
    View::do_layout(bounds);
    
    if (scrollbar_) {
        scrollbar_->layout(Rect{bounds.x + bounds.width - 12, bounds.y, 12, bounds.height});
    }
    sync_scrollbar();
}

} // namespace gooey::controls
