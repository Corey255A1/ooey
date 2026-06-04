#include "gooey/controls/checkbox.hpp"
#include "ooey/renderer/font_engine.hpp"

namespace gooey::controls {
    using namespace ooey;

CheckBox::CheckBox() : CheckBox(Rect{0, 0, 150, 32}, "") {}

CheckBox::CheckBox(Rect bounds, std::string text, bool initial_checked)
    : bounds_(bounds), text_(std::move(text)), checked_(initial_checked) {
    width = {SizePolicy::WrapContent};
    height = {SizePolicy::Fixed, static_cast<float>(bounds.height)};
    is_absolute = false;
    absolute_bounds = bounds;

    int box_size = 18;
    int by = bounds_.y + (bounds_.height - box_size) / 2;
    box_bg_ = std::make_shared<RoundedRectPrimitive>(Rect{bounds_.x, by, box_size, box_size}, 4, Color{30, 30, 35}, Color{150, 150, 155}, 1.5f);
    checked_indicator_ = std::make_shared<RectPrimitive>(Rect{bounds_.x + 4, by + 4, box_size - 8, box_size - 8}, Color{0, 120, 215});

    if (!text_.empty()) {
        Font font{"sans-serif", 14};
        Size ts = FontEngine::measure_text(text_, font);
        int tx = bounds_.x + 18 + 8;
        int ty = bounds_.y + (bounds_.height - ts.height) / 2;
        label_ = std::make_shared<Label>(text_, font, Point{tx, ty}, Color{220, 220, 220});
        label_->layout(Rect{tx, ty, ts.width, ts.height});
    }
}

void CheckBox::draw(ooey::IRenderTarget& target) const {
    if (box_bg_) {
        box_bg_->draw(target);
    }
    if (checked_ && checked_indicator_) {
        checked_indicator_->draw(target);
    }
    if (label_) {
        label_->draw(target);
    }
}

Rect CheckBox::bounds() const {
    return bounds_;
}

void CheckBox::set_value(bool checked) {
    set_checked(checked);
}

bool CheckBox::get_value() const {
    return is_checked();
}

void CheckBox::set_checked(bool checked) {
    if (checked_ != checked) {
        checked_ = checked;
        if (on_checked_changed) {
            on_checked_changed(checked_);
        }
    }
}

bool CheckBox::is_checked() const {
    return checked_;
}

void CheckBox::set_text(const std::string& text) {
    set_label_text(text);
}

void CheckBox::set_label_text(const std::string& text) {
    text_ = text;
    if (label_) {
        label_->set_text(text);
    } else {
        Font font{"sans-serif", 14};
        label_ = std::make_shared<Label>(text, font, Point{0, 0}, Color{220, 220, 220});
    }
    invalidate_layout();
}

const std::string& CheckBox::get_text() const {
    return text_;
}

bool CheckBox::on_pointer_event(const Pointer& e) {
    if (e.state == PointerState::Pressed) {
        bool hit = (e.x >= bounds_.x && e.x <= bounds_.x + bounds_.width &&
                    e.y >= bounds_.y && e.y <= bounds_.y + bounds_.height);
        if (hit) {
            set_checked(!checked_);
            return true;
        }
    }
    return false;
}

bool CheckBox::on_key_event(const KeyEvent&) {
    return false;
}

Size CheckBox::do_measure(Size constraints) {
    int w = 18;
    if (label_) {
        Size ts = FontEngine::measure_text(text_, label_->get_font());
        w += 8 + ts.width;
    }
    int h = resolve_height(constraints.height, absolute_bounds.height);
    return Size{resolve_width(constraints.width, w), h};
}

void CheckBox::do_layout(Rect bounds) {
    bounds_ = bounds;
    GooeyElement::do_layout(bounds);

    int box_size = 18;
    int by = bounds_.y + (bounds_.height - box_size) / 2;
    if (box_bg_) {
        box_bg_->set_rect(Rect{bounds_.x, by, box_size, box_size});
    }
    if (checked_indicator_) {
        checked_indicator_->set_rect(Rect{bounds_.x + 4, by + 4, box_size - 8, box_size - 8});
    }
    if (label_) {
        Font font = label_->get_font();
        Size ts = FontEngine::measure_text(text_, font);
        int tx = bounds_.x + box_size + 8;
        int ty = bounds_.y + (bounds_.height - ts.height) / 2;
        label_->set_position(Point{tx, ty});
        label_->layout(Rect{tx, ty, ts.width, ts.height});
    }
}

} // namespace gooey::controls
