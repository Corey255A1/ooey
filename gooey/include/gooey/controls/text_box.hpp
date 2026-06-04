#pragma once

namespace ooey {}


#include "gooey/mvvmc/gooey_element.hpp"
#include "gooey/mvvmc/theme.hpp"
#include "gooey/mvvmc/i_interactive.hpp"
#include "ooey/renderer/primitives/rounded_rect_primitive.hpp"
#include "ooey/renderer/primitives/text_primitive.hpp"
#include <functional>
#include <memory>
#include <string>

namespace gooey::controls {
    using namespace ooey;

class TextBox : public mvvmc::GooeyElement, public IInteractive {
public:
    static std::shared_ptr<TextBox> create() {
        return std::make_shared<TextBox>();
    }
    static std::shared_ptr<TextBox> create(Rect bounds, Font font, Color text_color, Color bg_color) {
        return std::make_shared<TextBox>(bounds, font, text_color, bg_color);
    }
    static std::shared_ptr<TextBox> create(LocalizedString initial_text) {
        auto tb = std::make_shared<TextBox>();
        tb->set_localized_text(std::move(initial_text));
        return tb;
    }

    TextBox();
    TextBox(Rect bounds, Font font, Color text_color, Color bg_color);

    void draw(ooey::IRenderTarget& target) const override;
    Rect bounds() const override;

    void set_text(const std::string& text);
    const std::string& get_text() const;

    void set_localized_text(LocalizedString text) {
        localized_key_ = std::move(text);
        bind(LocalizationManager::get().active_locale, [this](const std::string&) {
            this->set_text(LocalizationManager::get().translate(localized_key_.key));
        });
    }

    void set_font(const Font& font);
    const Font& get_font() const;

    bool on_pointer_event(const Pointer& e) override;
    bool on_key_event(const KeyEvent& e) override;
    bool on_text_event(const TextEvent& e) override;

    std::function<void(const std::string&)> on_text_changed;

    template <typename Target>
    void set_on_text_changed(const std::shared_ptr<Target>& target, void (Target::*member_func)(const std::string&)) {
        std::weak_ptr<Target> weak_target = target;
        on_text_changed = [weak_target, member_func](const std::string& text) {
            if (auto locked = weak_target.lock()) {
                (locked.get()->*member_func)(text);
            }
        };
    }

    template <typename Target>
    void set_on_text_changed(std::weak_ptr<Target> target, void (Target::*member_func)(const std::string&)) {
        on_text_changed = [target = std::move(target), member_func](const std::string& text) {
            if (auto locked = target.lock()) {
                (locked.get()->*member_func)(text);
            }
        };
    }

    template <typename Target, typename Func>
    void set_on_text_changed_weak(const std::shared_ptr<Target>& target, Func&& callback) {
        std::weak_ptr<Target> weak_target = target;
        on_text_changed = [weak_target, callback = std::forward<Func>(callback)](const std::string& text) {
            if (auto locked = weak_target.lock()) {
                callback(locked.get(), text);
            }
        };
    }

    template <typename Target, typename Func>
    void set_on_text_changed_weak(std::weak_ptr<Target> target, Func&& callback) {
        on_text_changed = [target = std::move(target), callback = std::forward<Func>(callback)](const std::string& text) {
            if (auto locked = target.lock()) {
                callback(locked.get(), text);
            }
        };
    }

protected:
    // Layout support
    Size do_measure(Size constraints) override;
    void do_layout(Rect bounds) override;
    void apply_style(const mvvmc::Style& style) override;

private:
    Rect bounds_;
    std::shared_ptr<RoundedRectPrimitive> bg_;
    std::shared_ptr<TextPrimitive> text_primitive_;
    std::string text_;
    bool is_focused_{false};
    mvvmc::Style current_style_;
    LocalizedString localized_key_{""};
};

} // namespace gooey::controls
namespace gooey {
    using namespace ooey;
using gooey::controls::TextBox;
}
