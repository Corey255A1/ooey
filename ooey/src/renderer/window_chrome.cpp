#include "ooey/renderer/window_chrome.hpp"
#include "ooey/i_window_backend.hpp"
#include "ooey/renderer/primitives/rect_primitive.hpp"
#include "ooey/renderer/primitives/line_primitive.hpp"
#include "ooey/renderer/primitives/text_primitive.hpp"
#include "ooey/renderer/image.hpp"
#include <algorithm>

namespace ooey {

// --- WindowChrome ---

WindowChrome::WindowChrome() = default;

void WindowChrome::draw(IRenderTarget& target, const Size& window_size) const {
    int w = window_size.width;
    int h = window_size.height;
    int bw = border_width_;
    int th = title_bar_height_;

    // Draw borders (left, right, bottom, top)
    RectPrimitive(Rect{0, 0, bw, h}, border_color_).draw(target);
    RectPrimitive(Rect{w - bw, 0, bw, h}, border_color_).draw(target);
    RectPrimitive(Rect{0, h - bw, w, bw}, border_color_).draw(target);
    RectPrimitive(Rect{0, 0, w, bw}, border_color_).draw(target);

    // Title bar background
    RectPrimitive(Rect{bw, bw, w - 2 * bw, th}, title_bar_color_).draw(target);

    // Title Text
    Font font{"sans-serif", 14, FontWeight::Bold};
    int text_y = bw + (th - 14) / 2;
    TextPrimitive(title_, font, Point{bw + 10, text_y}, title_text_color_).draw(target);

    // Minimize Button background
    int min_x = w - bw - 60;
    Color min_bg = min_pressed_ ? Color{40, 40, 40, 255} : (min_hovered_ ? minimize_button_hover_color_ : minimize_button_color_);
    RectPrimitive(Rect{min_x, bw, 30, th}, min_bg).draw(target);

    // Minimize Symbol (horizontal line)
    int min_cy = bw + th / 2;
    LinePrimitive(Point{min_x + 10, min_cy}, Point{min_x + 20, min_cy}, minimize_button_text_color_, 2.0f).draw(target);

    // Close Button background
    int close_x = w - bw - 30;
    Color close_bg = close_pressed_ ? Color{150, 40, 40, 255} : (close_hovered_ ? close_button_hover_color_ : close_button_color_);
    RectPrimitive(Rect{close_x, bw, 30, th}, close_bg).draw(target);

    // Close Symbol (X)
    int close_cy = bw + th / 2;
    int close_cx = close_x + 15;
    LinePrimitive(Point{close_cx - 5, close_cy - 5}, Point{close_cx + 5, close_cy + 5}, close_button_text_color_, 2.0f).draw(target);
    LinePrimitive(Point{close_cx - 5, close_cy + 5}, Point{close_cx + 5, close_cy - 5}, close_button_text_color_, 2.0f).draw(target);
}

ChromeHitTest WindowChrome::hit_test(int x, int y, const Size& window_size) const {
    int w = window_size.width;
    int h = window_size.height;
    int bw = border_width_;
    int th = title_bar_height_;

    int cs = 12; // corner active size
    int bs = std::max(bw, 6); // border active hit size

    // Corners
    if (x < cs && y < cs) return ChromeHitTest::BorderTopLeft;
    if (x >= w - cs && y < cs) return ChromeHitTest::BorderTopRight;
    if (x < cs && y >= h - cs) return ChromeHitTest::BorderBottomLeft;
    if (x >= w - cs && y >= h - cs) return ChromeHitTest::BorderBottomRight;

    // Edges
    if (y < bs) return ChromeHitTest::BorderTop;
    if (y >= h - bs) return ChromeHitTest::BorderBottom;
    if (x < bs) return ChromeHitTest::BorderLeft;
    if (x >= w - bs) return ChromeHitTest::BorderRight;

    // Title bar
    if (y < bw + th && x >= bw && x < w - bw) {
        int close_x = w - bw - 30;
        if (x >= close_x) {
            return ChromeHitTest::CloseButton;
        }
        int min_x = w - bw - 60;
        if (x >= min_x) {
            return ChromeHitTest::MinimizeButton;
        }
        return ChromeHitTest::TitleBar;
    }

    return ChromeHitTest::Client;
}

bool WindowChrome::handle_pointer_event(const Pointer& pointer, const Size& window_size, IWindowBackend* backend) {
    if (!backend) {
        return false;
    }

    int x = pointer.x;
    int y = pointer.y;

    ChromeHitTest hit = hit_test(x, y, window_size);

    if (pointer.state == PointerState::Moved) {
        close_hovered_ = (hit == ChromeHitTest::CloseButton);
        min_hovered_ = (hit == ChromeHitTest::MinimizeButton);
        return (hit != ChromeHitTest::Client);
    }

    if (pointer.state == PointerState::Pressed) {
        if (hit == ChromeHitTest::CloseButton) {
            close_pressed_ = true;
            return true;
        }
        if (hit == ChromeHitTest::MinimizeButton) {
            min_pressed_ = true;
            return true;
        }
        if (hit == ChromeHitTest::TitleBar) {
            backend->start_interactive_move();
            return true;
        }

        WindowResizeEdge edge = WindowResizeEdge::None;
        switch (hit) {
            case ChromeHitTest::BorderTop:          edge = WindowResizeEdge::Top; break;
            case ChromeHitTest::BorderBottom:       edge = WindowResizeEdge::Bottom; break;
            case ChromeHitTest::BorderLeft:         edge = WindowResizeEdge::Left; break;
            case ChromeHitTest::BorderRight:        edge = WindowResizeEdge::Right; break;
            case ChromeHitTest::BorderTopLeft:      edge = WindowResizeEdge::TopLeft; break;
            case ChromeHitTest::BorderTopRight:     edge = WindowResizeEdge::TopRight; break;
            case ChromeHitTest::BorderBottomLeft:   edge = WindowResizeEdge::BottomLeft; break;
            case ChromeHitTest::BorderBottomRight:  edge = WindowResizeEdge::BottomRight; break;
            default: break;
        }
        if (edge != WindowResizeEdge::None) {
            backend->start_interactive_resize(edge);
            return true;
        }

        return (hit != ChromeHitTest::Client);
    }

    if (pointer.state == PointerState::Released) {
        bool consumed = false;
        if (close_pressed_) {
            close_pressed_ = false;
            if (hit == ChromeHitTest::CloseButton) {
                backend->request_close();
            }
            consumed = true;
        }
        if (min_pressed_) {
            min_pressed_ = false;
            consumed = true;
        }
        return consumed || (hit != ChromeHitTest::Client);
    }

    return false;
}

void WindowChrome::set_title(const std::string& title) { title_ = title; }
const std::string& WindowChrome::get_title() const { return title_; }

void WindowChrome::set_title_bar_height(int height) { title_bar_height_ = height; }
int WindowChrome::get_title_bar_height() const { return title_bar_height_; }

void WindowChrome::set_border_width(int width) { border_width_ = width; }
int WindowChrome::get_border_width() const { return border_width_; }

void WindowChrome::set_title_bar_color(Color color) { title_bar_color_ = color; }
Color WindowChrome::get_title_bar_color() const { return title_bar_color_; }

void WindowChrome::set_border_color(Color color) { border_color_ = color; }
Color WindowChrome::get_border_color() const { return border_color_; }

void WindowChrome::set_title_text_color(Color color) { title_text_color_ = color; }
Color WindowChrome::get_title_text_color() const { return title_text_color_; }

void WindowChrome::set_close_button_color(Color color) { close_button_color_ = color; }
Color WindowChrome::get_close_button_color() const { return close_button_color_; }

void WindowChrome::set_close_button_hover_color(Color color) { close_button_hover_color_ = color; }
Color WindowChrome::get_close_button_hover_color() const { return close_button_hover_color_; }

void WindowChrome::set_close_button_text_color(Color color) { close_button_text_color_ = color; }
Color WindowChrome::get_close_button_text_color() const { return close_button_text_color_; }

void WindowChrome::set_minimize_button_color(Color color) { minimize_button_color_ = color; }
Color WindowChrome::get_minimize_button_color() const { return minimize_button_color_; }

void WindowChrome::set_minimize_button_hover_color(Color color) { minimize_button_hover_color_ = color; }
Color WindowChrome::get_minimize_button_hover_color() const { return minimize_button_hover_color_; }

void WindowChrome::set_minimize_button_text_color(Color color) { minimize_button_text_color_ = color; }
Color WindowChrome::get_minimize_button_text_color() const { return minimize_button_text_color_; }

// --- ChromeRenderTarget ---

ChromeRenderTarget::ChromeRenderTarget(IRenderTarget* target, std::shared_ptr<WindowChrome> chrome, const Size& physical_size)
    : target_(target), chrome_(std::move(chrome)), physical_size_(physical_size) {}

void ChromeRenderTarget::clear(Color color) {
    target_->clear(color);
}

void ChromeRenderTarget::draw_geometry(const Geometry& geometry) {
    if (!chrome_) {
        target_->draw_geometry(geometry);
        return;
    }
    int dx = chrome_->get_border_width();
    int dy = chrome_->get_border_width() + chrome_->get_title_bar_height();
    Geometry shifted = geometry;
    for (auto& vertex : shifted.vertices) {
        vertex.x += dx;
        vertex.y += dy;
    }
    target_->draw_geometry(shifted);
}

void ChromeRenderTarget::draw_image(const Image& image, const Rect& dest_rect) {
    if (!chrome_) {
        target_->draw_image(image, dest_rect);
        return;
    }
    int dx = chrome_->get_border_width();
    int dy = chrome_->get_border_width() + chrome_->get_title_bar_height();
    Rect shifted = dest_rect;
    shifted.x += dx;
    shifted.y += dy;
    target_->draw_image(image, shifted);
}

Size ChromeRenderTarget::measure_text(const std::string& text, const Font& font) {
    return target_->measure_text(text, font);
}

void ChromeRenderTarget::draw_text(const std::string& text, const Font& font, const Point& position, Color color) {
    if (!chrome_) {
        target_->draw_text(text, font, position, color);
        return;
    }
    int dx = chrome_->get_border_width();
    int dy = chrome_->get_border_width() + chrome_->get_title_bar_height();
    target_->draw_text(text, font, Point{position.x + dx, position.y + dy}, color);
}

void ChromeRenderTarget::resize(int width, int height) {
    physical_size_ = Size{width, height};
    target_->resize(width, height);
}

void ChromeRenderTarget::present() {
    if (chrome_) {
        chrome_->draw(*target_, physical_size_);
    }
    target_->present();
}

} // namespace ooey
