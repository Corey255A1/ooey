#pragma once

#include "ooey/types.hpp"
#include "ooey/input.hpp"
#include "ooey/renderer/i_render_target.hpp"
#include <memory>
#include <string>

namespace ooey {

class IWindowBackend;

enum class ChromeHitTest {
    Client,
    TitleBar,
    CloseButton,
    MinimizeButton,
    BorderTop,
    BorderBottom,
    BorderLeft,
    BorderRight,
    BorderTopLeft,
    BorderTopRight,
    BorderBottomLeft,
    BorderBottomRight
};

class WindowChrome {
public:
    WindowChrome();
    virtual ~WindowChrome() = default;

    void draw(IRenderTarget& target, const Size& window_size) const;
    bool handle_pointer_event(const Pointer& pointer, const Size& window_size, IWindowBackend* backend);

    ChromeHitTest hit_test(int x, int y, const Size& window_size) const;

    // Getters and setters for customization
    void set_title(const std::string& title);
    const std::string& get_title() const;

    void set_title_bar_height(int height);
    int get_title_bar_height() const;

    void set_border_width(int width);
    int get_border_width() const;

    void set_title_bar_color(Color color);
    Color get_title_bar_color() const;

    void set_border_color(Color color);
    Color get_border_color() const;

    void set_title_text_color(Color color);
    Color get_title_text_color() const;

    void set_close_button_color(Color color);
    Color get_close_button_color() const;

    void set_close_button_hover_color(Color color);
    Color get_close_button_hover_color() const;

    void set_close_button_text_color(Color color);
    Color get_close_button_text_color() const;

    void set_minimize_button_color(Color color);
    Color get_minimize_button_color() const;

    void set_minimize_button_hover_color(Color color);
    Color get_minimize_button_hover_color() const;

    void set_minimize_button_text_color(Color color);
    Color get_minimize_button_text_color() const;

private:
    std::string title_{"Window"};
    int title_bar_height_{30};
    int border_width_{4};

    Color title_bar_color_{50, 50, 50, 255};
    Color border_color_{70, 70, 70, 255};
    Color title_text_color_{230, 230, 230, 255};
    Color close_button_color_{60, 60, 60, 255};
    Color close_button_hover_color_{200, 50, 50, 255};
    Color close_button_text_color_{220, 220, 220, 255};
    Color minimize_button_color_{60, 60, 60, 255};
    Color minimize_button_hover_color_{100, 100, 100, 255};
    Color minimize_button_text_color_{220, 220, 220, 255};

    bool close_hovered_{false};
    bool close_pressed_{false};
    bool min_hovered_{false};
    bool min_pressed_{false};
};

class ChromeRenderTarget : public IRenderTarget {
public:
    ChromeRenderTarget(IRenderTarget* target, std::shared_ptr<WindowChrome> chrome, const Size& physical_size);
    ~ChromeRenderTarget() override = default;

    void clear(Color color) override;
    void draw_geometry(const Geometry& geometry) override;
    void draw_image(const Image& image, const Rect& dest_rect) override;
    Size measure_text(const std::string& text, const Font& font) override;
    void draw_text(const std::string& text, const Font& font, const Point& position, Color color) override;
    void resize(int width, int height) override;
    void present() override;

private:
    IRenderTarget* target_;
    std::shared_ptr<WindowChrome> chrome_;
    Size physical_size_;
};

} // namespace ooey
