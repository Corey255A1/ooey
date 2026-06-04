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
    static std::shared_ptr<ListControl> create() {
        return std::make_shared<ListControl>();
    }
    static std::shared_ptr<ListControl> create(Rect bounds, int item_height, Font font, Color text_color, Color bg_color, Color highlight_bg_color, Color highlight_text_color) {
        return std::make_shared<ListControl>(bounds, item_height, font, text_color, bg_color, highlight_bg_color, highlight_text_color);
    }

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

    template <typename Target>
    void set_on_selected_changed(const std::shared_ptr<Target>& target, void (Target::*member_func)(int)) {
        std::weak_ptr<Target> weak_target = target;
        on_selected_changed = [weak_target, member_func](int val) {
            if (auto locked = weak_target.lock()) {
                (locked.get()->*member_func)(val);
            }
        };
    }

    template <typename Target>
    void set_on_selected_changed(std::weak_ptr<Target> target, void (Target::*member_func)(int)) {
        on_selected_changed = [target = std::move(target), member_func](int val) {
            if (auto locked = target.lock()) {
                (locked.get()->*member_func)(val);
            }
        };
    }

    template <typename Target, typename Func>
    void set_on_selected_changed_weak(const std::shared_ptr<Target>& target, Func&& callback) {
        std::weak_ptr<Target> weak_target = target;
        on_selected_changed = [weak_target, callback = std::forward<Func>(callback)](int val) {
            if (auto locked = weak_target.lock()) {
                callback(locked.get(), val);
            }
        };
    }

    template <typename Target, typename Func>
    void set_on_selected_changed_weak(std::weak_ptr<Target> target, Func&& callback) {
        on_selected_changed = [target = std::move(target), callback = std::forward<Func>(callback)](int val) {
            if (auto locked = target.lock()) {
                callback(locked.get(), val);
            }
        };
    }

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
