#pragma once

#include "gooey/mvvmc/gooey_node.hpp"
#include "gooey/mvvmc/i_interactive.hpp"
#include "gooey/controls/menu.hpp"
#include <vector>
#include <string>
#include <memory>

namespace gooey::controls {

struct MenuCategory {
    std::string name;
    std::vector<MenuItem> items;
};

class MenuBar : public GooeyNode, public mvvmc::IInteractive {
public:
    MenuBar();
    MenuBar(const std::vector<MenuCategory>& categories);

    Rect bounds() const override { return bounds_; }

    void set_categories(const std::vector<MenuCategory>& categories);
    const std::vector<MenuCategory>& get_categories() const { return categories_; }

    void set_breakpoint(int breakpoint) { breakpoint_ = breakpoint; invalidate_layout(); }
    int get_breakpoint() const { return breakpoint_; }

    void draw(ooey::IRenderTarget& target) const override;
    bool on_pointer_event(const ooey::Pointer& e) override;
    bool on_key_event(const ooey::KeyEvent& e) override;
    void apply_style(const mvvmc::Style& style) override;

    // MVVM items binding support
    void set_items(const std::vector<MenuCategory>& categories) { set_categories(categories); }

protected:
    Size do_measure(Size constraints) override;
    void do_layout(Rect bounds) override;

private:
    std::vector<MenuCategory> categories_;
    Rect bounds_{0, 0, 0, 0};
    int breakpoint_{600};
    
    // UI state
    int active_category_idx_{-1};
    int hovered_category_idx_{-1};
    bool hamburger_open_{false};
    bool hamburger_hovered_{false};
    
    std::shared_ptr<Menu> active_menu_{nullptr};

    // Rects of horizontal category labels for hit testing
    std::vector<Rect> category_rects_;
    Rect hamburger_rect_{0, 0, 0, 0};

    // Collapsed vertical category items rects (when hamburger is expanded)
    std::vector<Rect> collapsed_category_rects_;

    // Styling properties
    Color bar_bg_color_{30, 30, 35};
    Color border_color_{60, 60, 70};
    Color text_color_{220, 220, 225};
    Color hover_bg_color_{50, 50, 55};
    Color active_bg_color_{0, 120, 215};
    Color active_text_color_{255, 255, 255};

    void open_menu(int idx);
    void close_menu();
};

} // namespace gooey::controls
namespace gooey {
using gooey::controls::MenuCategory;
using gooey::controls::MenuBar;
}
