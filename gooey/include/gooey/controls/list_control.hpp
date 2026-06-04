#pragma once

namespace ooey {}


#include "gooey/mvvmc/gooey_node.hpp"
#include "gooey/mvvmc/i_interactive.hpp"
#include "ooey/renderer/primitives/rounded_rect_primitive.hpp"
#include "ooey/renderer/primitives/rect_primitive.hpp"
#include "ooey/renderer/primitives/text_primitive.hpp"
#include <vector>
#include <string>
#include <memory>
#include <functional>

namespace gooey::controls {
    using namespace ooey;

class ListControl : public GooeyNode, public IInteractive {
public:
    ListControl();
    ListControl(Rect bounds, int item_height, Font font, Color text_color, Color bg_color, Color highlight_bg_color, Color highlight_text_color);

    Rect bounds() const override;

    void set_items(const std::vector<std::string>& items);
    const std::vector<std::string>& get_items() const;

    void set_item_views(const std::vector<std::shared_ptr<GooeyElement>>& views);
    const std::vector<std::shared_ptr<GooeyElement>>& get_item_views() const;

    void set_item_height(int height);
    int get_item_height() const;

    void set_selected_index(int index);
    int get_selected_index() const;

    void select_next();
    void select_previous();

    void set_stylize_items(bool stylize);
    bool get_stylize_items() const;

    bool on_pointer_event(const Pointer& e) override;
    bool on_key_event(const KeyEvent& e) override;

    std::function<void(int)> on_selected_changed;

protected:
    // Layout support
    Size do_measure(Size constraints) override;
    void do_layout(Rect bounds) override;
    void apply_style(const mvvmc::Style& style) override;

private:
    void update_children();

    Rect bounds_;
    int item_height_;
    int visible_count_;
    Font font_;
    Color text_color_;
    Color bg_color_;
    Color highlight_bg_color_;
    Color highlight_text_color_;

    std::vector<std::string> items_;
    std::vector<std::shared_ptr<GooeyElement>> item_views_;
    int selected_index_{0};
    int scroll_offset_{0};
    bool stylize_items_{false};

    std::shared_ptr<RoundedRectPrimitive> bg_;
    std::vector<std::shared_ptr<RectPrimitive>> item_bgs_;
    std::vector<std::shared_ptr<RoundedRectPrimitive>> item_checkbox_bgs_;
    std::vector<std::shared_ptr<RectPrimitive>> item_checkbox_checks_;
    std::vector<std::shared_ptr<TextPrimitive>> item_texts_;
};

} // namespace gooey::controls

namespace gooey {
    using namespace ooey;
using gooey::controls::ListControl;
}
