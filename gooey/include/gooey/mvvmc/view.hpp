#pragma once

namespace ooey {}


#include "gooey/mvvmc/i_drawable.hpp"
#include "gooey/mvvmc/subscription_sink.hpp"
#include "gooey/mvvmc/property.hpp"
#include "gooey/mvvmc/layout.hpp"
#include <vector>
#include <memory>

namespace gooey::mvvmc {
    using namespace ooey;

    struct Theme;
    class ThemeManager;
    struct Style;

class View : public IDrawable {
public:
    View();
    virtual ~View() override = default;

    void add_child(std::shared_ptr<IDrawable>&& child);

    const std::vector<std::shared_ptr<IDrawable>>& get_children() const;

    // Helper to easily bind to a property and manage its lifecycle
    template <typename T>
    void bind(Property<T>& property, typename Property<T>::Listener listener) {
        sink_.add(property.subscribe(std::move(listener)));
    }

    void draw(ooey::IRenderTarget& target) const override;
    void clear_children();

    // Layout configuration
    LayoutLength width{SizePolicy::WrapContent};
    LayoutLength height{SizePolicy::WrapContent};

    int margin_left{0};
    int margin_top{0};
    int margin_right{0};
    int margin_bottom{0};

    int padding_left{0};
    int padding_top{0};
    int padding_right{0};
    int padding_bottom{0};

    Align align_self{Align::Start};

    // Laido-out absolute boundaries
    Rect layout_bounds{0, 0, 0, 0};

    bool is_absolute{false};
    Rect absolute_bounds{0, 0, 0, 0};

    // Builder setters for chaining configuration
    View& set_width(SizePolicy policy, float value = 0.0f) { width = {policy, value}; return *this; }
    View& set_height(SizePolicy policy, float value = 0.0f) { height = {policy, value}; return *this; }
    View& set_margin(int margin) { margin_left = margin_top = margin_right = margin_bottom = margin; return *this; }
    View& set_margin(int left, int top, int right, int bottom) { margin_left = left; margin_top = top; margin_right = right; margin_bottom = bottom; return *this; }
    View& set_padding(int padding) { padding_left = padding_top = padding_right = padding_bottom = padding; return *this; }
    View& set_padding(int left, int top, int right, int bottom) { padding_left = left; padding_top = top; padding_right = right; padding_bottom = bottom; return *this; }
    View& set_align_self(Align align) { align_self = align; return *this; }
    View& set_absolute(bool absolute) { is_absolute = absolute; return *this; }
    View& set_absolute_bounds(Rect bounds) { absolute_bounds = bounds; return *this; }

    // Two-pass reactive layout system
    virtual Size measure(Size constraints);
    virtual void layout(Rect bounds);

    // Helpers to resolve width/height according to policies under parent constraints
    int resolve_width(int constraint_w, int content_w) const;
    int resolve_height(int constraint_h, int content_h) const;

    // Styling and Theme support
    void set_style_name(const std::string& name);
    const std::string& get_style_name() const { return style_name_; }

    virtual void set_theme_manager(std::shared_ptr<ThemeManager> manager);
    std::shared_ptr<ThemeManager> get_theme_manager() const { return theme_manager_.lock(); }

    virtual void apply_style(const Style& style);

private:
    int calculate_content_width(Size child_constraints);
    int calculate_content_height(Size child_constraints);

    std::string style_name_;
    std::weak_ptr<ThemeManager> theme_manager_;
    ScopedSubscription theme_subscription_;
    std::vector<std::shared_ptr<IDrawable>> children_;
    SubscriptionSink sink_;
};

} // namespace gooey::mvvmc
namespace gooey {
    using namespace ooey;
using gooey::mvvmc::View;
}
