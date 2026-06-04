#pragma once

namespace ooey {}


#include "gooey/mvvmc/gooey_element.hpp"
#include "gooey/mvvmc/theme.hpp"
#include "gooey/mvvmc/i_interactive.hpp"
#include "ooey/renderer/primitives/rounded_rect_primitive.hpp"
#include "gooey/controls/label.hpp"
#include <functional>
#include <memory>
#include <string>

namespace gooey::controls {
    using namespace ooey;

class Button : public mvvmc::GooeyElement, public IInteractive {
public:
    static std::shared_ptr<Button> create() {
        return std::make_shared<Button>();
    }
    static std::shared_ptr<Button> create(Rect bounds, Color color) {
        return std::make_shared<Button>(bounds, color);
    }
    static std::shared_ptr<Button> create(Rect bounds, Color fill_color, Color stroke_color, float stroke_thickness, int corner_radius, const std::string& label_text = "", Color label_color = Color{255, 255, 255}) {
        return std::make_shared<Button>(bounds, fill_color, stroke_color, stroke_thickness, corner_radius, label_text, label_color);
    }
    static std::shared_ptr<Button> create(LocalizedString label_text) {
        auto btn = std::make_shared<Button>();
        btn->set_localized_label_text(std::move(label_text));
        return btn;
    }

    Button();
    Button(Rect bounds, Color color);
    Button(Rect bounds, Color fill_color, Color stroke_color, float stroke_thickness, int corner_radius, const std::string& label_text = "", Color label_color = Color{255, 255, 255});

    void draw(ooey::IRenderTarget& target) const override;
    Rect bounds() const override;

    void set_color(Color color);
    void set_fill_color(Color color);
    void set_stroke_color(Color color);
    void set_stroke_thickness(float thickness);
    void set_corner_radius(int radius);
    void set_label_text(const std::string& text);

    void set_localized_label_text(LocalizedString text) {
        localized_key_ = std::move(text);
        bind(LocalizationManager::get().active_locale, [this](const std::string&) {
            this->set_label_text(LocalizationManager::get().translate(localized_key_.key));
        });
    }

    bool on_pointer_event(const Pointer& e) override;
    bool on_key_event(const KeyEvent& e) override;

    std::function<void()> on_click;

    template <typename Target>
    void set_on_click(const std::shared_ptr<Target>& target, void (Target::*member_func)()) {
        std::weak_ptr<Target> weak_target = target;
        on_click = [weak_target, member_func]() {
            if (auto locked = weak_target.lock()) {
                (locked.get()->*member_func)();
            }
        };
    }

    template <typename Target>
    void set_on_click(std::weak_ptr<Target> target, void (Target::*member_func)()) {
        on_click = [target = std::move(target), member_func]() {
            if (auto locked = target.lock()) {
                (locked.get()->*member_func)();
            }
        };
    }

    template <typename Target, typename Func>
    void set_on_click_weak(const std::shared_ptr<Target>& target, Func&& callback) {
        std::weak_ptr<Target> weak_target = target;
        on_click = [weak_target, callback = std::forward<Func>(callback)]() {
            if (auto locked = weak_target.lock()) {
                callback(locked.get());
            }
        };
    }

    template <typename Target, typename Func>
    void set_on_click_weak(std::weak_ptr<Target> target, Func&& callback) {
        on_click = [target = std::move(target), callback = std::forward<Func>(callback)]() {
            if (auto locked = target.lock()) {
                callback(locked.get());
            }
        };
    }

    void set_theme_manager(std::shared_ptr<ThemeManager> manager) override;

protected:
    // Layout support
    Size do_measure(Size constraints) override;
    void do_layout(Rect bounds) override;
    void apply_style(const mvvmc::Style& style) override;

private:
    Rect bounds_;
    Color color_;
    std::shared_ptr<RoundedRectPrimitive> bg_;
    std::shared_ptr<Label> label_;
    LocalizedString localized_key_{""};
};

} // namespace gooey::controls
namespace gooey {
    using namespace ooey;
using gooey::controls::Button;
}
