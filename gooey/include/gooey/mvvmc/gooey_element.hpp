#pragma once

#include "gooey/mvvmc/i_drawable.hpp"
#include "gooey/mvvmc/subscription_sink.hpp"
#include "gooey/mvvmc/property.hpp"
#include "gooey/mvvmc/layout.hpp"
#include "gooey/mvvmc/localization.hpp"
#include <memory>
#include <string>

namespace gooey::mvvmc {
    using namespace ooey;

    struct Theme;
    class ThemeManager;
    struct Style;
    class GooeyNode;

class GooeyElement : public IDrawable {
public:
    GooeyElement();
    virtual ~GooeyElement() override = default;

    template <typename T>
    void bind(Property<T>& property, typename Property<T>::Listener listener) {
        sink_.add(property.subscribe(std::move(listener)));
    }

    // Bind a property to a member function of a control using std::shared_ptr (converts to weak internally)
    template <typename T, typename Target, typename ClassType>
    void bind(Property<T>& property, const std::shared_ptr<Target>& target, void (ClassType::*member_func)(const T&)) {
        static_assert(std::is_base_of_v<ClassType, Target>, "Target must derive from ClassType");
        std::weak_ptr<Target> weak_target = target;
        sink_.add(property.subscribe([weak_target, member_func](const T& val) {
            if (auto locked = weak_target.lock()) {
                (locked.get()->*member_func)(val);
            }
        }));
    }

    // Bind a property to a member function of a control using std::weak_ptr
    template <typename T, typename Target, typename ClassType>
    void bind(Property<T>& property, std::weak_ptr<Target> target, void (ClassType::*member_func)(const T&)) {
        static_assert(std::is_base_of_v<ClassType, Target>, "Target must derive from ClassType");
        sink_.add(property.subscribe([target = std::move(target), member_func](const T& val) {
            if (auto locked = target.lock()) {
                (locked.get()->*member_func)(val);
            }
        }));
    }

    // Bind a property to a lambda callback targeting a control weakly (using std::shared_ptr)
    template <typename T, typename Target, typename Func>
    void bind_weak(Property<T>& property, const std::shared_ptr<Target>& target, Func&& callback) {
        std::weak_ptr<Target> weak_target = target;
        sink_.add(property.subscribe([weak_target, callback = std::forward<Func>(callback)](const T& val) {
            if (auto locked = weak_target.lock()) {
                callback(locked.get(), val);
            }
        }));
    }

    // Bind a property to a lambda callback targeting a control weakly (using std::weak_ptr)
    template <typename T, typename Target, typename Func>
    void bind_weak(Property<T>& property, std::weak_ptr<Target> target, Func&& callback) {
        sink_.add(property.subscribe([target = std::move(target), callback = std::forward<Func>(callback)](const T& val) {
            if (auto locked = target.lock()) {
                callback(locked.get(), val);
            }
        }));
    }

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

    // Laid-out absolute boundaries
    Rect layout_bounds{0, 0, 0, 0};

    bool is_absolute{false};
    Rect absolute_bounds{0, 0, 0, 0};

    // Builder setters for chaining configuration
    GooeyElement& set_width(SizePolicy policy, float value = 0.0f) { width = {policy, value}; return *this; }
    GooeyElement& set_height(SizePolicy policy, float value = 0.0f) { height = {policy, value}; return *this; }
    GooeyElement& set_margin(int margin) { margin_left = margin_top = margin_right = margin_bottom = margin; return *this; }
    GooeyElement& set_margin(int left, int top, int right, int bottom) { margin_left = left; margin_top = top; margin_right = right; margin_bottom = bottom; return *this; }
    GooeyElement& set_padding(int padding) { padding_left = padding_top = padding_right = padding_bottom = padding; return *this; }
    GooeyElement& set_padding(int left, int top, int right, int bottom) { padding_left = left; padding_top = top; padding_right = right; padding_bottom = bottom; return *this; }
    GooeyElement& set_align_self(Align align) { align_self = align; return *this; }
    GooeyElement& set_absolute(bool absolute) { is_absolute = absolute; return *this; }
    GooeyElement& set_absolute_bounds(Rect bounds) { absolute_bounds = bounds; return *this; }

    // Two-pass reactive layout system
    Size measure(Size constraints);
    void layout(Rect bounds);

    void invalidate_layout();

protected:
    virtual Size do_measure(Size constraints);
    virtual void do_layout(Rect bounds);

public:
    // Helpers to resolve width/height according to policies under parent constraints
    int resolve_width(int constraint_w, int content_w) const;
    int resolve_height(int constraint_h, int content_h) const;

    // Styling and Theme support
    void set_style_name(const std::string& name);
    const std::string& get_style_name() const { return style_name_; }

    virtual void set_theme_manager(std::shared_ptr<ThemeManager> manager);
    std::shared_ptr<ThemeManager> get_theme_manager() const { return theme_manager_.lock(); }
    GooeyNode* get_parent() const { return parent_; }
    void set_parent(GooeyNode* parent) { parent_ = parent; }

    bool is_layout_clean() const { return is_layout_clean_; }
    bool is_measure_clean() const { return is_measure_clean_; }

    virtual void apply_style(const Style& style);

private:
    std::string style_name_;
    std::weak_ptr<ThemeManager> theme_manager_;
    ScopedSubscription theme_subscription_;
    SubscriptionSink sink_;

protected:
    friend class GooeyNode;
    GooeyNode* parent_{nullptr};
    bool is_measure_clean_{false};
    bool is_layout_clean_{false};
    Size measured_size_{0, 0};
    Size last_measure_constraints_{0, 0};
};

template <typename T>
std::weak_ptr<T> weak(const std::shared_ptr<T>& ptr) {
    return std::weak_ptr<T>(ptr);
}

} // namespace gooey::mvvmc
namespace gooey {
    using namespace ooey;
using gooey::mvvmc::GooeyElement;
using gooey::mvvmc::weak;
}
