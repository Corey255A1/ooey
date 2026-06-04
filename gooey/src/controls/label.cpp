#include "gooey/controls/label.hpp"
#include "gooey/mvvmc/theme.hpp"
#include "ooey/renderer/font_engine.hpp"

namespace gooey::controls {
    using namespace ooey;


Label::Label() : Label("", Font{"sans-serif", 16}, Point{0,0}, Color{255, 255, 255}) {}

Label::Label(std::string text, Font font, Point position, Color color) {
    text_primitive_ = std::make_shared<TextPrimitive>(std::move(text), font, position, color);

    // Default to absolute positioning using constructor coordinates and text size
    is_absolute = true;
    Size text_size = FontEngine::measure_text(text_primitive_->get_text(), font);
    absolute_bounds = Rect{position.x, position.y, text_size.width, text_size.height};
    width = {SizePolicy::WrapContent};
    height = {SizePolicy::WrapContent};
}

void Label::set_text(const std::string& text) {
    text_primitive_->set_text(text);
    if (is_absolute) {
        Size text_size = FontEngine::measure_text(text, text_primitive_->get_font());
        absolute_bounds.width = text_size.width;
        absolute_bounds.height = text_size.height;
    }
    invalidate_layout();
}


const std::string& Label::get_text() const {
    return text_primitive_->get_text();
}

void Label::set_font(const Font& font) {
    text_primitive_->set_font(font);
    if (is_absolute) {
        Size text_size = FontEngine::measure_text(text_primitive_->get_text(), font);
        absolute_bounds.width = text_size.width;
        absolute_bounds.height = text_size.height;
    }
    invalidate_layout();
}

const Font& Label::get_font() const {
    return text_primitive_->get_font();
}

void Label::set_color(Color color) {
    text_primitive_->set_color(color);
}

Color Label::get_color() const {
    return text_primitive_->get_color();
}

void Label::set_position(Point position) {
    text_primitive_->set_position(position);
    if (is_absolute) {
        absolute_bounds.x = position.x;
        absolute_bounds.y = position.y;
    }
    invalidate_layout();
}

Point Label::get_position() const {
    return text_primitive_->get_position();
}

Size Label::do_measure(Size constraints) {
    if (!text_primitive_) return Size{0, 0};

    const std::string& text = text_primitive_->get_text();
    const Font& font = text_primitive_->get_font();

    if (overflow_ == TextOverflow::Wrapped) {
        int avail_w = constraints.width - padding_left - padding_right;
        if (avail_w < 0) avail_w = 0;

        std::vector<std::string> lines = wrap_text(text, font, avail_w);
        int max_w = 0;
        int total_h = 0;
        int line_h = FontEngine::measure_text("Ay", font).height;
        for (const auto& line : lines) {
            Size sz = FontEngine::measure_text(line, font);
            max_w = std::max(max_w, sz.width);
            total_h += line_h;
        }

        int w = resolve_width(constraints.width, max_w + padding_left + padding_right);
        int h = resolve_height(constraints.height, total_h + padding_top + padding_bottom);
        return Size{w, h};
    }

    Size text_size = FontEngine::measure_text(text, font);
    int w = resolve_width(constraints.width, text_size.width + padding_left + padding_right);
    int h = resolve_height(constraints.height, text_size.height + padding_top + padding_bottom);
    return Size{w, h};
}

void Label::do_layout(Rect bounds) {
    GooeyElement::do_layout(bounds);
    if (text_primitive_) {
        text_primitive_->set_position(Point{bounds.x + padding_left, bounds.y + padding_top});
    }
}

void Label::draw(ooey::IRenderTarget& target) const {
    if (!text_primitive_) return;

    if (layout_bounds.width <= 0 || layout_bounds.height <= 0) {
        return;
    }

    int avail_w = layout_bounds.width - padding_left - padding_right;
    int avail_h = layout_bounds.height - padding_top - padding_bottom;
    if (avail_w <= 0 || avail_h <= 0) return;

    Point pos{layout_bounds.x + padding_left, layout_bounds.y + padding_top};

    if (overflow_ == TextOverflow::Wrapped) {
        std::vector<std::string> lines = wrap_text(get_text(), get_font(), avail_w, &target);
        int line_h = target.measure_text("Ay", get_font()).height;
        int y = pos.y;
        for (const auto& line : lines) {
            target.draw_text(line, get_font(), Point{pos.x, y}, get_color());
            y += line_h;
        }
    } else if (overflow_ == TextOverflow::Shrunk) {
        Size sz = target.measure_text(get_text(), get_font());
        float scale = 1.0f;
        if (sz.width > avail_w) {
            scale = std::min(scale, (float)avail_w / sz.width);
        }
        if (sz.height > avail_h) {
            scale = std::min(scale, (float)avail_h / sz.height);
        }
        Font font = get_font();
        if (scale < 1.0f) {
            font.size = std::max(1, (int)(font.size * scale));
        }
        target.draw_text(get_text(), font, pos, get_color());
    } else if (overflow_ == TextOverflow::Clipped) {
        target.push_clip(layout_bounds);
        target.draw_text(get_text(), get_font(), pos, get_color());
        target.pop_clip();
    } else {
        // TextOverflow::None
        target.draw_text(get_text(), get_font(), pos, get_color());
    }
}

std::vector<std::string> Label::wrap_text(const std::string& text, const Font& font, int max_width, ooey::IRenderTarget* target) const {
    std::vector<std::string> lines;
    if (max_width <= 0 || max_width >= 50000) {
        lines.push_back(text);
        return lines;
    }

    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    while ((pos = text.find('\n', prev)) != std::string::npos) {
        std::string line = text.substr(prev, pos - prev);
        wrap_single_line(line, font, max_width, lines, target);
        prev = pos + 1;
    }
    std::string line = text.substr(prev);
    wrap_single_line(line, font, max_width, lines, target);

    return lines;
}

void Label::wrap_single_line(const std::string& line, const Font& font, int max_width, std::vector<std::string>& out_lines, ooey::IRenderTarget* target) const {
    if (line.empty()) {
        out_lines.push_back("");
        return;
    }
    std::vector<std::string> words;
    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    while ((pos = line.find(' ', prev)) != std::string::npos) {
        words.push_back(line.substr(prev, pos - prev));
        prev = pos + 1;
    }
    words.push_back(line.substr(prev));

    std::string current_line = "";
    for (size_t i = 0; i < words.size(); ++i) {
        const auto& word = words[i];
        std::string test_line = current_line;
        if (!test_line.empty()) {
            test_line += " ";
        }
        test_line += word;

        Size sz = target ? target->measure_text(test_line, font) : FontEngine::measure_text(test_line, font);
        if (sz.width <= max_width) {
            current_line = test_line;
        } else {
            if (!current_line.empty()) {
                out_lines.push_back(current_line);
                current_line = word;
            } else {
                current_line = word;
            }
        }
    }
    if (!current_line.empty()) {
        out_lines.push_back(current_line);
    }
}

void Label::apply_style(const mvvmc::Style& style) {
    set_color(style.text_color);
    GooeyElement::apply_style(style);
}

} // namespace gooey::controls