#pragma once

#include "gooey/mvvmc/gooey_node.hpp"
#include "gooey/mvvmc/i_interactive.hpp"
#include "ooey/renderer/primitives/rect_primitive.hpp"
#include "ooey/renderer/primitives/line_primitive.hpp"
#include "ooey/renderer/primitives/text_primitive.hpp"
#include <vector>
#include <string>
#include <functional>
#include <memory>

namespace gooey::controls {

struct MenuItem {
    std::string label;
    std::string shortcut;
    bool is_separator{false};
    bool is_checkbox{false};
    bool checked{false};
    std::vector<MenuItem> subitems;
    std::function<void()> action;
};

class Menu : public GooeyNode, public mvvmc::IInteractive, public std::enable_shared_from_this<Menu> {
public:
    Menu(const std::vector<MenuItem>& items);

    Rect bounds() const override { return bounds_; }

    void draw(ooey::IRenderTarget& target) const override;
    bool on_pointer_event(const ooey::Pointer& e) override;
    bool on_key_event(const ooey::KeyEvent& e) override;

    void set_absolute_position(Point pos) {
        absolute_pos_ = pos;
        invalidate_layout();
    }

    void apply_style(const mvvmc::Style& style) override;

    void close();
    bool is_open() const { return is_open_; }

    // Close all open submenus recursively
    void close_submenus();

    std::function<void()> on_close;
    std::function<void(int direction)> on_navigate_sibling;

protected:
    Size do_measure(Size constraints) override;
    void do_layout(Rect bounds) override;

private:
    std::vector<MenuItem> items_;
    Rect bounds_{0, 0, 0, 0};
    Point absolute_pos_{0, 0};
    bool is_open_{true};
    int hovered_idx_{-1};
    int row_height_{30};
    int width_{150};
    
    std::shared_ptr<Menu> active_submenu_{nullptr};
    std::weak_ptr<Menu> parent_menu_;

    // Styling properties
    Color bg_color_{45, 45, 50};
    Color border_color_{80, 80, 90};
    Color text_color_{240, 240, 240};
    Color hover_bg_color_{0, 120, 215};
    Color hover_text_color_{255, 255, 255};
};

} // namespace gooey::controls
namespace gooey {
using gooey::controls::MenuItem;
using gooey::controls::Menu;
}
