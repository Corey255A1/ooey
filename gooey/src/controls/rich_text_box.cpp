#include "gooey/controls/rich_text_box.hpp"
#include "gooey/application.hpp"
#include "gooey/mvvmc/controller.hpp"
#include "gooey/controls/scroll_container.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace gooey::controls {
    using namespace ooey;

static Size measure_text_logical(const std::string& text, const Font& font) {
    float scale = 1.0f;
    if (gooey::Application::get_instance()) {
        scale = gooey::Application::get_instance()->get_dpi_scale();
    }
    if (scale == 1.0f) {
        return FontEngine::measure_text(text, font);
    }
    Font scaled_font = font;
    scaled_font.size = static_cast<int>(font.size * scale);
    Size physical_size = FontEngine::measure_text(text, scaled_font);
    return Size{
        static_cast<int>(physical_size.width / scale),
        static_cast<int>(physical_size.height / scale)
    };
}

struct StyledSegment {
    std::string text;
    TextFormat format;
};

static std::vector<StyledSegment> split_line_into_segments(
    const std::string& line, 
    const std::vector<FormatRange>& formats, 
    const TextFormat& default_format) {
    
    std::vector<StyledSegment> segments;
    int current_col = 0;
    int line_len = static_cast<int>(line.size());

    auto sorted_formats = formats;
    std::ranges::sort(sorted_formats, 
              [](const FormatRange& a, const FormatRange& b) {
                  return a.start_col < b.start_col;
              });

    for (const auto& fmt : sorted_formats) {
        int start = std::clamp(fmt.start_col, current_col, line_len);
        int end = std::clamp(fmt.end_col, start, line_len);

        if (start > current_col) {
            segments.push_back({line.substr(current_col, start - current_col), default_format});
        }
        if (end > start) {
            segments.push_back({line.substr(start, end - start), fmt.format});
        }
        current_col = end;
    }

    if (current_col < line_len) {
        segments.push_back({line.substr(current_col), default_format});
    }

    if (segments.empty()) {
        segments.push_back({"", default_format});
    }

    return segments;
}

static Geometry make_rect_geometry(const Rect& rect, Color color) {
    Geometry geom;
    auto x1 = static_cast<float>(rect.x);
    auto y1 = static_cast<float>(rect.y);
    auto x2 = static_cast<float>(rect.x + rect.width);
    auto y2 = static_cast<float>(rect.y + rect.height);
    
    geom.vertices = {
        { .x=x1, .y=y1, .color=color },
        { .x=x2, .y=y1, .color=color },
        { .x=x2, .y=y2, .color=color },
        { .x=x1, .y=y2, .color=color }
    };
    geom.indices = { 0, 1, 2, 0, 2, 3 };
    return geom;
}

static Geometry make_squiggle_geometry(int x1, int x2, int y, Color color) {
    Geometry geom;
    geom.type = PrimitiveType::Lines;
    
    int step = 2; // pixel interval for wave peak/valley
    int amp = 1;  // amplitude height
    
    int vertex_index = 0;
    float prev_x = static_cast<float>(x1);
    float prev_y = static_cast<float>(y);
    
    for (int x = x1 + 1; x <= x2; ++x) {
        float next_x = static_cast<float>(x);
        float next_y = static_cast<float>(y + (((x / step) % 2 == 0) ? amp : -amp));
        
        geom.vertices.push_back({ .x=prev_x, .y=prev_y, .color=color });
        geom.vertices.push_back({ .x=next_x, .y=next_y, .color=color });
        
        geom.indices.push_back(vertex_index++);
        geom.indices.push_back(vertex_index++);
        
        prev_x = next_x;
        prev_y = next_y;
    }
    return geom;
}

class RichTextContentView : public GooeyNode, public IInteractive {
public:
    RichTextContentView(RichTextBox& parent) : parent_(parent) {
        is_absolute = true;
    }

    [[nodiscard]] Rect bounds() const override {
        return layout_bounds;
    }

    bool on_pointer_event(const Pointer& e) override {
        return parent_.on_content_pointer_event(e);
    }

    bool on_key_event(const KeyEvent& e) override {
        return parent_.on_content_key_event(e);
    }

    bool on_text_event(const TextEvent& e) override {
        return parent_.on_content_text_event(e);
    }

    void draw(ooey::IRenderTarget& target) const override {
        parent_.draw_content(target);
    }

protected:
    Size do_measure(Size constraints) override {
        return parent_.measure_content(constraints);
    }

    void do_layout(Rect bounds) override {
        layout_bounds = bounds;
        GooeyNode::do_layout(bounds);
    }

private:
    RichTextBox& parent_;
};

RichTextBox::RichTextBox()
    : RichTextBox(Rect{0, 0, 100, 100}, Font{}, Color{220, 220, 220}, Color{30, 30, 30}) {}

RichTextBox::RichTextBox(Rect bounds, Font font, Color text_color, Color bg_color)
    : bounds_(bounds), font_(font), lines_{""}, line_formats_{std::vector<FormatRange>{}} {
    width = {SizePolicy::Fixed, static_cast<float>(bounds.width)};
    height = {SizePolicy::Fixed, static_cast<float>(bounds.height)};
    is_absolute = true;
    absolute_bounds = bounds;

    this->default_text_color = text_color;
    this->bg_color = bg_color;

    scroll_container_ = std::make_shared<ScrollContainer>();
    content_view_ = std::make_shared<RichTextContentView>(*this);
    scroll_container_->set_child(content_view_);
    add_child(scroll_container_);
}

Rect RichTextBox::bounds() const {
    return bounds_;
}

void RichTextBox::set_text(const std::string& text) {
    auto normalize_newlines = [](const std::string& s) {
        std::string res;
        res.reserve(s.size());
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '\r') {
                if (i + 1 < s.size() && s[i + 1] == '\n') {
                    res += '\n';
                    i++;
                } else {
                    res += '\n';
                }
            } else {
                res += s[i];
            }
        }
        return res;
    };

    if (get_text() == normalize_newlines(text)) {
        return;
    }

    int old_cursor_line = cursor_line_;
    int old_cursor_col = cursor_col_;
    int old_anchor_line = anchor_line_;
    int old_anchor_col = anchor_col_;
    bool old_has_selection = has_selection_;
    int old_scroll_x = scroll_container_ ? scroll_container_->get_scroll_offset_x() : 0;
    int old_scroll_y = scroll_container_ ? scroll_container_->get_scroll_offset_y() : 0;

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

    cursor_line_ = std::max(0, std::min(old_cursor_line, static_cast<int>(lines_.size()) - 1));
    cursor_col_ = std::max(0, std::min(old_cursor_col, static_cast<int>(lines_[cursor_line_].size())));
    anchor_line_ = std::max(0, std::min(old_anchor_line, static_cast<int>(lines_.size()) - 1));
    anchor_col_ = std::max(0, std::min(old_anchor_col, static_cast<int>(lines_[anchor_line_].size())));
    has_selection_ = old_has_selection;

    line_formats_.clear();
    line_formats_.resize(lines_.size());

    update_formatting();
    
    if (scroll_container_) {
        scroll_container_->set_scroll_offset_x(old_scroll_x);
        scroll_container_->set_scroll_offset_y(old_scroll_y);
    }
    
    scroll_cursor_into_view();
    invalidate_content_layout();
    if (on_text_changed) {
        on_text_changed(get_text());
    }
}

std::string RichTextBox::get_text() const {
    std::string text = "";
    for (size_t i = 0; i < lines_.size(); ++i) {
        text += lines_[i];
        if (i + 1 < lines_.size()) {
            text += "\n";
        }
    }
    return text;
}

void RichTextBox::set_font(const Font& font) {
    font_ = font;
    invalidate_content_layout();
}

const Font& RichTextBox::get_font() const {
    return font_;
}

void RichTextBox::clear_formats() {
    for (auto& formats : line_formats_) {
        formats.clear();
    }
    invalidate_content_layout();
}

void RichTextBox::clear_line_formats(int line_idx) {
    if (line_idx >= 0 && line_idx < static_cast<int>(line_formats_.size())) {
        line_formats_[line_idx].clear();
        invalidate_content_layout();
    }
}

void RichTextBox::apply_format(int line_idx, int start_col, int end_col, const TextFormat& format) {
    if (line_idx >= 0 && line_idx < static_cast<int>(line_formats_.size())) {
        line_formats_[line_idx].push_back({start_col, end_col, format});
        invalidate_content_layout();
    }
}

void RichTextBox::set_line_formats(int line_idx, const std::vector<FormatRange>& formats) {
    if (line_idx >= 0 && line_idx < static_cast<int>(line_formats_.size())) {
        line_formats_[line_idx] = formats;
        invalidate_content_layout();
    }
}

const std::vector<FormatRange>& RichTextBox::get_line_formats(int line_idx) const {
    static const std::vector<FormatRange> empty_formats;
    if (line_idx >= 0 && line_idx < static_cast<int>(line_formats_.size())) {
        return line_formats_[line_idx];
    }
    return empty_formats;
}

int RichTextBox::get_lines_count() const {
    return static_cast<int>(lines_.size());
}

std::string RichTextBox::get_line_text(int line_idx) const {
    if (line_idx >= 0 && line_idx < static_cast<int>(lines_.size())) {
        return lines_[line_idx];
    }
    return "";
}

int RichTextBox::get_scroll_x() const {
    if (scroll_container_) {
        return scroll_container_->get_scroll_offset_x();
    }
    return 0;
}

bool RichTextBox::needs_scroll_x() const {
    return scroll_container_ && scroll_container_->needs_scroll_x();
}

bool RichTextBox::needs_scroll_y() const {
    return scroll_container_ && scroll_container_->needs_scroll_y();
}

void RichTextBox::update_formatting() {
    // Default implementation does nothing
}

void RichTextBox::get_selection_ordered(int& start_line, int& start_col, int& end_line, int& end_col) const {
    if (!has_selection_) {
        start_line = end_line = cursor_line_;
        start_col = end_col = cursor_col_;
        return;
    }
    
    if (anchor_line_ < cursor_line_) {
        start_line = anchor_line_;
        start_col = anchor_col_;
        end_line = cursor_line_;
        end_col = cursor_col_;
    } else if (anchor_line_ > cursor_line_) {
        start_line = cursor_line_;
        start_col = cursor_col_;
        end_line = anchor_line_;
        end_col = anchor_col_;
    } else {
        start_line = anchor_line_;
        end_line = anchor_line_;
        if (anchor_col_ < cursor_col_) {
            start_col = anchor_col_;
            end_col = cursor_col_;
        } else {
            start_col = cursor_col_;
            end_col = anchor_col_;
        }
    }
}

int RichTextBox::get_column_x_offset(int line_idx, int col) const {
    if (line_idx < 0 || line_idx >= static_cast<int>(lines_.size())) return 0;
    const std::string& line = lines_[line_idx];
    const auto& formats = (line_idx < static_cast<int>(line_formats_.size())) ? line_formats_[line_idx] : std::vector<FormatRange>{};
    
    TextFormat default_fmt{.color=default_text_color, .weight=FontWeight::Normal, .style=FontStyle::Normal, .size=font_.size};
    auto segments = split_line_into_segments(line, formats, default_fmt);
    
    int x_offset = 0;
    int current_col = 0;
    
    for (const auto& seg : segments) {
        int seg_len = static_cast<int>(seg.text.size());
        if (current_col + seg_len <= col) {
            Font font = font_;
            font.weight = seg.format.weight;
            font.style = seg.format.style;
            if (seg.format.size > 0) font.size = seg.format.size;
            x_offset += measure_text_logical(seg.text, font).width;
            current_col += seg_len;
        } else {
            int prefix_len = col - current_col;
            if (prefix_len > 0) {
                Font font = font_;
                font.weight = seg.format.weight;
                font.style = seg.format.style;
                if (seg.format.size > 0) font.size = seg.format.size;
                x_offset += measure_text_logical(seg.text.substr(0, prefix_len), font).width;
            }
            break;
        }
    }
    return x_offset;
}

std::string RichTextBox::get_selected_text() const {
    if (!has_selection_) return "";
    
    int start_line, start_col, end_line, end_col;
    get_selection_ordered(start_line, start_col, end_line, end_col);
    
    if (start_line == end_line) {
        return lines_[start_line].substr(start_col, end_col - start_col);
    }
    
    std::string result = "";
    result += lines_[start_line].substr(start_col) + "\n";
    for (int l = start_line + 1; l < end_line; ++l) {
        result += lines_[l] + "\n";
    }
    result += lines_[end_line].substr(0, end_col);
    return result;
}

void RichTextBox::insert_text(const std::string& text) {
    if (has_selection_) {
        int start_line, start_col, end_line, end_col;
        get_selection_ordered(start_line, start_col, end_line, end_col);
        
        if (start_line == end_line) {
            lines_[start_line].erase(start_col, end_col - start_col);
            line_formats_[start_line].clear();
        } else {
            std::string prefix = lines_[start_line].substr(0, start_col);
            std::string suffix = lines_[end_line].substr(end_col);
            lines_[start_line] = prefix + suffix;
            lines_.erase(lines_.begin() + start_line + 1, lines_.begin() + end_line + 1);
            
            line_formats_[start_line].clear();
            line_formats_.erase(line_formats_.begin() + start_line + 1, line_formats_.begin() + end_line + 1);
        }
        cursor_line_ = start_line;
        cursor_col_ = start_col;
        has_selection_ = false;
    }

    std::string line_suffix = lines_[cursor_line_].substr(cursor_col_);
    lines_[cursor_line_] = lines_[cursor_line_].substr(0, cursor_col_);
    
    std::vector<std::string> insert_lines;
    std::string current_line = "";
    for (char c : text) {
        if (c == '\n') {
            insert_lines.push_back(current_line);
            current_line.clear();
        } else if (c != '\r') {
            current_line += c;
        }
    }
    insert_lines.push_back(current_line);
    
    if (insert_lines.size() == 1) {
        lines_[cursor_line_] += insert_lines[0] + line_suffix;
        line_formats_[cursor_line_].clear();
        cursor_col_ += insert_lines[0].size();
    } else {
        lines_[cursor_line_] += insert_lines[0];
        line_formats_[cursor_line_].clear();
        for (size_t i = 1; i + 1 < insert_lines.size(); ++i) {
            lines_.insert(lines_.begin() + cursor_line_ + i, insert_lines[i]);
            line_formats_.insert(line_formats_.begin() + cursor_line_ + i, std::vector<FormatRange>{});
        }
        lines_.insert(lines_.begin() + cursor_line_ + insert_lines.size() - 1, insert_lines.back() + line_suffix);
        line_formats_.insert(line_formats_.begin() + cursor_line_ + insert_lines.size() - 1, std::vector<FormatRange>{});
        cursor_line_ += insert_lines.size() - 1;
        cursor_col_ = insert_lines.back().size();
    }

    update_formatting();
    scroll_cursor_into_view();
    invalidate_content_layout();
    if (on_text_changed) {
        on_text_changed(get_text());
    }
}

void RichTextBox::invalidate_content_layout() {
    if (content_view_) {
        content_view_->invalidate_layout();
    } else {
        invalidate_layout();
    }
}

void RichTextBox::scroll_cursor_into_view() {
    if (scroll_container_) {
        // Perform initial layout of scroll container if not laid out yet (for tests/direct calls)
        if (scroll_container_->bounds().width == 0 || scroll_container_->bounds().height == 0) {
            Size char_size = measure_text_logical("A", font_);
            int max_line_digits = std::to_string(get_lines_count()).size();
            int line_num_width = show_line_numbers ? std::max(40, static_cast<int>(max_line_digits * char_size.width) + 16) : 0;
            
            scroll_container_->measure(Size{bounds_.width - line_num_width, bounds_.height});
            scroll_container_->layout(Rect{bounds_.x + line_num_width, bounds_.y, bounds_.width - line_num_width, bounds_.height});
        }

        Size char_size = measure_text_logical("A", font_);
        int line_h = char_size.height + 4;
        
        int cx = get_column_x_offset(cursor_line_, cursor_col_);
        int cy = cursor_line_ * line_h;
        
        scroll_container_->scroll_to_visible(Rect{cx, cy, 2, char_size.height});
    }
}

bool RichTextBox::on_pointer_event(const Pointer& e) {
    bool hit = (e.x >= bounds_.x && e.x <= bounds_.x + bounds_.width &&
                e.y >= bounds_.y && e.y <= bounds_.y + bounds_.height);
    if (!hit) return false;
    
    Size char_size = measure_text_logical("A", font_);
    int max_line_digits = std::to_string(get_lines_count()).size();
    int line_num_width = show_line_numbers ? std::max(40, static_cast<int>(max_line_digits * char_size.width) + 16) : 0;
    
    if (show_line_numbers && e.x < bounds_.x + line_num_width) {
        if (e.state == PointerState::Pressed) {
            int line_h = char_size.height + 4;
            int scroll_offset_y = scroll_container_ ? scroll_container_->get_scroll_offset_y() : 0;
            int clicked_line = (e.y - (bounds_.y + 4) + scroll_offset_y) / line_h;
            clicked_line = std::max(0, std::min(clicked_line, static_cast<int>(lines_.size()) - 1));
            
            cursor_line_ = clicked_line;
            cursor_col_ = 0;
            if (!shift_pressed_) {
                anchor_line_ = cursor_line_;
                anchor_col_ = cursor_col_;
                has_selection_ = false;
            } else {
                has_selection_ = true;
            }
            if (Application::get_instance() && Application::get_instance()->get_controller()) {
                dynamic_cast<gooey::mvvmc::Controller*>(Application::get_instance()->get_controller())->set_focused_element(content_view_);
            }
            scroll_cursor_into_view();
            invalidate_content_layout();
            return true;
        }
        return false;
    }
    
    if (content_view_) {
        // Forward click to content view if inside active viewport area
        int viewport_x = bounds_.x + line_num_width;
        int viewport_w = bounds_.width - line_num_width - (scroll_container_->needs_scroll_y() ? 12 : 0);
        int viewport_y = bounds_.y;
        int viewport_h = bounds_.height - (scroll_container_->needs_scroll_x() ? 12 : 0);
        
        if (e.x >= viewport_x && e.x <= viewport_x + viewport_w &&
            e.y >= viewport_y && e.y <= viewport_y + viewport_h) {
            return on_content_pointer_event(e);
        }
    }
    return false;
}

bool RichTextBox::on_key_event(const KeyEvent& e) {
    return on_content_key_event(e);
}

bool RichTextBox::on_text_event(const TextEvent& e) {
    return on_content_text_event(e);
}

bool RichTextBox::on_content_pointer_event(const Pointer& e) {
    bool hit = (e.x >= content_view_->bounds().x && e.x <= content_view_->bounds().x + content_view_->bounds().width &&
                e.y >= content_view_->bounds().y && e.y <= content_view_->bounds().y + content_view_->bounds().height);
    
    if (hit && e.state == PointerState::Pressed) {
        Size char_size = measure_text_logical("A", font_);
        int line_h = char_size.height + 4;
        
        int text_area_x = content_view_->bounds().x + 8;
        int text_area_y = content_view_->bounds().y + 4;
        
        int clicked_line = (e.y - text_area_y) / line_h;
        clicked_line = std::max(0, std::min(clicked_line, static_cast<int>(lines_.size()) - 1));
        
        std::string line = lines_[clicked_line];
        int clicked_col = 0;
        int min_dist = 999999;
        for (size_t col = 0; col <= line.size(); ++col) {
            int char_x = text_area_x + get_column_x_offset(clicked_line, col);
            int dist = std::abs(e.x - char_x);
            if (dist < min_dist) {
                min_dist = dist;
                clicked_col = col;
            }
        }
        
        cursor_line_ = clicked_line;
        cursor_col_ = clicked_col;

        if (!shift_pressed_) {
            anchor_line_ = cursor_line_;
            anchor_col_ = cursor_col_;
            has_selection_ = false;
        } else {
            has_selection_ = true;
        }

        dragging_selection_ = true;
        scroll_cursor_into_view();
        invalidate_content_layout();
        return true;
    } else if (e.state == PointerState::Moved && dragging_selection_) {
        Size char_size = measure_text_logical("A", font_);
        int line_h = char_size.height + 4;
        
        int text_area_x = content_view_->bounds().x + 8;
        int text_area_y = content_view_->bounds().y + 4;
        
        int clicked_line = (e.y - text_area_y) / line_h;
        clicked_line = std::max(0, std::min(clicked_line, static_cast<int>(lines_.size()) - 1));
        
        std::string line = lines_[clicked_line];
        int clicked_col = 0;
        int min_dist = 999999;
        for (size_t col = 0; col <= line.size(); ++col) {
            int char_x = text_area_x + get_column_x_offset(clicked_line, col);
            int dist = std::abs(e.x - char_x);
            if (dist < min_dist) {
                min_dist = dist;
                clicked_col = col;
            }
        }
        
        cursor_line_ = clicked_line;
        cursor_col_ = clicked_col;

        if (cursor_line_ != anchor_line_ || cursor_col_ != anchor_col_) {
            has_selection_ = true;
        } else {
            has_selection_ = false;
        }
        scroll_cursor_into_view();
        invalidate_content_layout();
        return true;
    } else if (e.state == PointerState::Released) {
        dragging_selection_ = false;
    }
    
    return false;
}

bool RichTextBox::on_content_key_event(const KeyEvent& e) {
    if (e.key_code == 0xFFE1 /* Left Shift */ || e.key_code == 0xFFE2 /* Right Shift */) {
        shift_pressed_ = (e.state == KeyState::Pressed);
        return true;
    }
    if (e.key_code == 0xFFE3 /* Left Ctrl */ || e.key_code == 0xFFE4 /* Right Ctrl */) {
        ctrl_pressed_ = (e.state == KeyState::Pressed);
        return true;
    }

    if (e.state != KeyState::Pressed) return false;

    if (ctrl_pressed_) {
        if (e.key_code == 'z' || e.key_code == 'Z') {
            if (shift_pressed_) {
                if (on_redo) on_redo();
            } else {
                if (on_undo) on_undo();
            }
            return true;
        }
        if (e.key_code == 'y' || e.key_code == 'Y') {
            if (on_redo) on_redo();
            return true;
        }
    }

    bool handled = false;

    auto move_cursor = [&](const std::function<void()>& move_fn) {
        if (shift_pressed_) {
            if (!has_selection_) {
                anchor_line_ = cursor_line_;
                anchor_col_ = cursor_col_;
                has_selection_ = true;
            }
            move_fn();
        } else {
            has_selection_ = false;
            move_fn();
        }
    };
    
    if (e.key_code == 0xFF51 /* Left */) {
        move_cursor([&]() {
            if (cursor_col_ > 0) {
                cursor_col_--;
            } else if (cursor_line_ > 0) {
                cursor_line_--;
                cursor_col_ = lines_[cursor_line_].size();
            }
        });
        handled = true;
    }
    else if (e.key_code == 0xFF53 /* Right */) {
        move_cursor([&]() {
            if (cursor_col_ < static_cast<int>(lines_[cursor_line_].size())) {
                cursor_col_++;
            } else if (cursor_line_ + 1 < static_cast<int>(lines_.size())) {
                cursor_line_++;
                cursor_col_ = 0;
            }
        });
        handled = true;
    }
    else if (e.key_code == 0xFF52 /* Up */) {
        move_cursor([&]() {
            if (cursor_line_ > 0) {
                cursor_line_--;
                cursor_col_ = std::min(cursor_col_, static_cast<int>(lines_[cursor_line_].size()));
            }
        });
        handled = true;
    }
    else if (e.key_code == 0xFF54 /* Down */) {
        move_cursor([&]() {
            if (cursor_line_ + 1 < static_cast<int>(lines_.size())) {
                cursor_line_++;
                cursor_col_ = std::min(cursor_col_, static_cast<int>(lines_[cursor_line_].size()));
            }
        });
        handled = true;
    }
    else if (e.key_code == 0xFF50 /* Home */) {
        move_cursor([&]() {
            cursor_col_ = 0;
        });
        handled = true;
    }
    else if (e.key_code == 0xFF57 /* End */) {
        move_cursor([&]() {
            cursor_col_ = lines_[cursor_line_].size();
        });
        handled = true;
    }
    else if (e.key_code == 0xFF55 /* Page Up */) {
        move_cursor([&]() {
            cursor_line_ = std::max(0, cursor_line_ - 10);
            cursor_col_ = std::min(cursor_col_, static_cast<int>(lines_[cursor_line_].size()));
        });
        handled = true;
    }
    else if (e.key_code == 0xFF56 /* Page Down */) {
        move_cursor([&]() {
            cursor_line_ = std::min(static_cast<int>(lines_.size()) - 1, cursor_line_ + 10);
            cursor_col_ = std::min(cursor_col_, static_cast<int>(lines_[cursor_line_].size()));
        });
        handled = true;
    }
    else if (e.key_code == 0xFF08 /* Backspace */ || e.key_code == 8) {
        if (has_selection_) {
            insert_text("");
        } else {
            if (cursor_col_ > 0) {
                lines_[cursor_line_].erase(cursor_col_ - 1, 1);
                line_formats_[cursor_line_].clear();
                cursor_col_--;
                update_formatting();
                if (on_text_changed) on_text_changed(get_text());
            } else if (cursor_line_ > 0) {
                int old_len = lines_[cursor_line_ - 1].size();
                lines_[cursor_line_ - 1] += lines_[cursor_line_];
                lines_.erase(lines_.begin() + cursor_line_);
                line_formats_.erase(line_formats_.begin() + cursor_line_);
                cursor_line_--;
                cursor_col_ = old_len;
                line_formats_[cursor_line_].clear();
                update_formatting();
                if (on_text_changed) on_text_changed(get_text());
            }
        }
        handled = true;
    }
    else if (e.key_code == 0xFFFF /* Delete */ || e.key_code == 127) {
        if (has_selection_) {
            insert_text("");
        } else {
            if (cursor_col_ < static_cast<int>(lines_[cursor_line_].size())) {
                lines_[cursor_line_].erase(cursor_col_, 1);
                line_formats_[cursor_line_].clear();
                update_formatting();
                if (on_text_changed) on_text_changed(get_text());
            } else if (cursor_line_ + 1 < static_cast<int>(lines_.size())) {
                lines_[cursor_line_] += lines_[cursor_line_ + 1];
                lines_.erase(lines_.begin() + cursor_line_ + 1);
                line_formats_.erase(line_formats_.begin() + cursor_line_ + 1);
                line_formats_[cursor_line_].clear();
                update_formatting();
                if (on_text_changed) on_text_changed(get_text());
            }
        }
        handled = true;
    }
    else if (e.key_code == 0xFF0D /* Return */ || e.key_code == 13 || e.key_code == 10) {
        if (has_selection_) {
            insert_text("");
        }
        std::string suffix = lines_[cursor_line_].substr(cursor_col_);
        lines_[cursor_line_] = lines_[cursor_line_].substr(0, cursor_col_);
        
        std::string indent = "";
        for (char c : lines_[cursor_line_]) {
            if (c == ' ' || c == '\t') indent += c;
            else break;
        }
        
        lines_.insert(lines_.begin() + cursor_line_ + 1, indent + suffix);
        line_formats_.insert(line_formats_.begin() + cursor_line_ + 1, std::vector<FormatRange>{});
        line_formats_[cursor_line_].clear();
        cursor_line_++;
        cursor_col_ = indent.size();
        
        update_formatting();
        if (on_text_changed) on_text_changed(get_text());
        handled = true;
    }
    else if (e.key_code == 0xFF09 /* Tab */ || e.key_code == 9) {
        insert_text("    ");
        handled = true;
    }

    if (handled) {
        scroll_cursor_into_view();
        invalidate_content_layout();
        return true;
    }

    return false;
}

bool RichTextBox::on_content_text_event(const TextEvent& e) {
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
        insert_text(text_to_insert);
    }
    return true;
}

Size RichTextBox::measure_content(Size /*constraints*/) {
    // Measure all lines to find the maximum line width and height
    Size char_size = measure_text_logical("A", font_);
    int line_h = char_size.height + 4;
    
    int max_w = 0;
    for (int i = 0; i < static_cast<int>(lines_.size()); ++i) {
        int lw = get_column_x_offset(i, lines_[i].size());
        if (lw > max_w) {
            max_w = lw;
        }
    }
    
    int total_width = max_w + 30; // 30px padding for safety
    int total_height = std::max(1, static_cast<int>(lines_.size())) * line_h + 8;
    return Size{total_width, total_height};
}

void RichTextBox::draw_content(ooey::IRenderTarget& target) const {
    Size char_size = target.measure_text("A", font_);
    int char_h = char_size.height;
    int line_h = char_h + 4;

    int scroll_offset_y = scroll_container_->get_scroll_offset_y();
    int viewport_h = scroll_container_->layout_bounds.height - (scroll_container_->needs_scroll_x() ? 12 : 0);

    int scroll_line = scroll_offset_y / line_h;
    int end_line = std::min(static_cast<int>(lines_.size()), scroll_line + viewport_h / line_h + 2);

    int y_pos = content_view_->bounds().y + 4;

    for (int l = scroll_line; l < end_line; ++l) {
        int current_y = y_pos + l * line_h;

        // Draw selection highlight for this line
        int sel_start_line, sel_start_col, sel_end_line, sel_end_col;
        get_selection_ordered(sel_start_line, sel_start_col, sel_end_line, sel_end_col);

        if (has_selection_ && l >= sel_start_line && l <= sel_end_line) {
            int sel_col_start = 0;
            int sel_col_end = lines_[l].size();

            if (l == sel_start_line) {
                sel_col_start = sel_start_col;
            }
            if (l == sel_end_line) {
                sel_col_end = sel_end_col;
            }

            if (sel_col_start < sel_col_end) {
                int sel_x1 = content_view_->bounds().x + 8 + get_column_x_offset(l, sel_col_start);
                int sel_x2 = content_view_->bounds().x + 8 + get_column_x_offset(l, sel_col_end);

                Rect highlight_rect{sel_x1, current_y, sel_x2 - sel_x1, char_h};
                target.draw_geometry(make_rect_geometry(highlight_rect, selection_color));
            }
        }

        const auto& formats = (l < static_cast<int>(line_formats_.size())) ? line_formats_[l] : std::vector<FormatRange>{};
        TextFormat default_fmt{.color=default_text_color, .weight=FontWeight::Normal, .style=FontStyle::Normal, .size=font_.size};
        auto segments = split_line_into_segments(lines_[l], formats, default_fmt);

        int token_x = content_view_->bounds().x + 8;
        for (const auto& seg : segments) {
            if (seg.text.empty() && segments.size() > 1) continue;

            Font font = font_;
            font.weight = seg.format.weight;
            font.style = seg.format.style;
            if (seg.format.size > 0) {
                font.size = seg.format.size;
            }

            target.draw_text(seg.text, font, Point{token_x, current_y}, seg.format.color);
            token_x += target.measure_text(seg.text, font).width;
        }

        bool is_focused = false;
        if (Application::get_instance() && Application::get_instance()->get_controller()) {
            auto* ctrl = dynamic_cast<gooey::mvvmc::Controller*>(Application::get_instance()->get_controller());
            if (ctrl) {
                auto focused = ctrl->get_focused_element();
                is_focused = (focused.get() == content_view_.get() || focused.get() == this);
            }
        }

        // Draw squiggles for errors/warnings under text
        for (const auto& sq : squiggles_) {
            if (sq.line_idx == l) {
                int sq_start = std::max(0, sq.start_col);
                int sq_end = std::min(static_cast<int>(lines_[l].size()), sq.end_col);
                if (sq_start < sq_end) {
                    int x1 = content_view_->bounds().x + 8 + get_column_x_offset(l, sq_start);
                    int x2 = content_view_->bounds().x + 8 + get_column_x_offset(l, sq_end);
                    int y = current_y + char_h;
                    target.draw_geometry(make_squiggle_geometry(x1, x2, y, sq.color));
                }
            }
        }

        if (l == cursor_line_ && is_focused) {
            int cursor_x = content_view_->bounds().x + 8 + get_column_x_offset(l, cursor_col_);
            Rect cursor_rect{cursor_x, current_y, 2, char_h};
            target.draw_geometry(make_rect_geometry(cursor_rect, cursor_color));
        }
    }
}

void RichTextBox::draw(ooey::IRenderTarget& target) const {
    // Draw background
    target.draw_geometry(make_rect_geometry(bounds_, bg_color));

    Size char_size = target.measure_text("A", font_);
    int line_h = char_size.height + 4;
    
    int max_line_digits = std::to_string(get_lines_count()).size();
    int line_num_width = show_line_numbers ? std::max(40, static_cast<int>(max_line_digits * char_size.width) + 16) : 0;

    Rect line_num_rect{bounds_.x, bounds_.y, line_num_width, bounds_.height};
    if (show_line_numbers) {
        target.draw_geometry(make_rect_geometry(line_num_rect, line_num_bg));
        Rect divider_rect{bounds_.x + line_num_width, bounds_.y, 1, bounds_.height};
        target.draw_geometry(make_rect_geometry(divider_rect, divider_color));
    }

    if (show_line_numbers) {
        int scroll_offset_y = scroll_container_ ? scroll_container_->get_scroll_offset_y() : 0;
        int viewport_h = bounds_.height - (scroll_container_->needs_scroll_x() ? 12 : 0);
        int scroll_line = scroll_offset_y / line_h;
        int end_line = std::min(get_lines_count(), scroll_line + viewport_h / line_h + 2);
        int y_pos = bounds_.y + 4;

        target.push_clip(line_num_rect);
        for (int l = scroll_line; l < end_line; ++l) {
            int current_y = y_pos + l * line_h - scroll_offset_y;
            std::string line_num_str = std::to_string(l + 1);
            int line_num_x = bounds_.x + line_num_width - 8 - target.measure_text(line_num_str, font_).width;
            target.draw_text(line_num_str, font_, Point{line_num_x, current_y}, line_num_color);
        }
        target.pop_clip();
    }

    GooeyNode::draw(target);
}

Size RichTextBox::do_measure(Size constraints) {
    int w = resolve_width(constraints.width, absolute_bounds.width);
    int h = resolve_height(constraints.height, absolute_bounds.height);

    if (scroll_container_) {
        Size char_size = measure_text_logical("A", font_);
        int max_line_digits = std::to_string(get_lines_count()).size();
        int line_num_width = show_line_numbers ? std::max(40, static_cast<int>(max_line_digits * char_size.width) + 16) : 0;

        int avail_w = std::max(0, w - line_num_width);
        int avail_h = h;
        scroll_container_->measure(Size{avail_w, avail_h});
    }
    return Size{w, h};
}

void RichTextBox::do_layout(Rect bounds) {
    bounds_ = bounds;

    Size char_size = measure_text_logical("A", font_);
    int max_line_digits = std::to_string(get_lines_count()).size();
    int line_num_width = show_line_numbers ? std::max(40, static_cast<int>(max_line_digits * char_size.width) + 16) : 0;

    if (scroll_container_) {
        scroll_container_->layout(Rect{
            bounds.x + line_num_width,
            bounds.y,
            bounds.width - line_num_width,
            bounds.height
        });
    }
}

void RichTextBox::add_squiggle(int line_idx, int start_col, int end_col, Color color) {
    squiggles_.push_back({line_idx, start_col, end_col, color});
    invalidate_content_layout();
}

void RichTextBox::clear_squiggles() {
    squiggles_.clear();
    invalidate_content_layout();
}

const std::vector<SquiggleRange>& RichTextBox::get_squiggles() const {
    return squiggles_;
}

} // namespace gooey::controls
