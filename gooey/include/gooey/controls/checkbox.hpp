#pragma once

#include "gooey/mvvmc/gooey_element.hpp"
#include "gooey/mvvmc/i_interactive.hpp"
#include "ooey/renderer/primitives/rounded_rect_primitive.hpp"
#include "ooey/renderer/primitives/rect_primitive.hpp"
#include "gooey/controls/label.hpp"
#include <functional>
#include <memory>
#include <string>

namespace gooey::controls {
    using namespace ooey;

class CheckBox : public mvvmc::GooeyElement, public IInteractive {
public:
    static std::shared_ptr<CheckBox> create() {
        return std::make_shared<CheckBox>();
    }
    static std::shared_ptr<CheckBox> create(Rect bounds, std::string text, bool initial_checked = false) {
        return std::make_shared<CheckBox>(bounds, text, initial_checked);
    }
    static std::shared_ptr<CheckBox> create(LocalizedString text, bool initial_checked = false) {
        auto cb = std::make_shared<CheckBox>();
        cb->set_localized_text(std::move(text));
        cb->set_checked(initial_checked);
        return cb;
    }

    CheckBox();
    CheckBox(Rect bounds, std::string text, bool initial_checked = false);

    void draw(ooey::IRenderTarget& target) const override;
    Rect bounds() const override;

    // Value bindings compatibility
    void set_value(bool checked);
    bool get_value() const;

    void set_checked(bool checked);
    bool is_checked() const;

    void set_text(const std::string& text);
    void set_label_text(const std::string& text);
    const std::string& get_text() const;

    void set_localized_text(LocalizedString text) {
        localized_key_ = std::move(text);
        bind(LocalizationManager::get().active_locale, [this](const std::string&) {
            this->set_text(LocalizationManager::get().translate(localized_key_.key));
        });
    }

    bool on_pointer_event(const Pointer& e) override;
    bool on_key_event(const KeyEvent& e) override;

    std::function<void(bool)> on_checked_changed;

    template <typename Target>
    void set_on_checked_changed(const std::shared_ptr<Target>& target, void (Target::*member_func)(bool)) {
        std::weak_ptr<Target> weak_target = target;
        on_checked_changed = [weak_target, member_func](bool checked) {
            if (auto locked = weak_target.lock()) {
                (locked.get()->*member_func)(checked);
            }
        };
    }

    template <typename Target>
    void set_on_checked_changed(std::weak_ptr<Target> target, void (Target::*member_func)(bool)) {
        on_checked_changed = [target = std::move(target), member_func](bool checked) {
            if (auto locked = target.lock()) {
                (locked.get()->*member_func)(checked);
            }
        };
    }

    template <typename Target, typename Func>
    void set_on_checked_changed_weak(const std::shared_ptr<Target>& target, Func&& callback) {
        std::weak_ptr<Target> weak_target = target;
        on_checked_changed = [weak_target, callback = std::forward<Func>(callback)](bool checked) {
            if (auto locked = weak_target.lock()) {
                callback(locked.get(), checked);
            }
        };
    }

    template <typename Target, typename Func>
    void set_on_checked_changed_weak(std::weak_ptr<Target> target, Func&& callback) {
        on_checked_changed = [target = std::move(target), callback = std::forward<Func>(callback)](bool checked) {
            if (auto locked = target.lock()) {
                callback(locked.get(), checked);
            }
        };
    }

protected:
    Size do_measure(Size constraints) override;
    void do_layout(Rect bounds) override;

private:
    Rect bounds_;
    std::string text_;
    bool checked_{false};
    std::shared_ptr<RoundedRectPrimitive> box_bg_;
    std::shared_ptr<RectPrimitive> checked_indicator_;
    std::shared_ptr<Label> label_;
    LocalizedString localized_key_{""};
};

} // namespace gooey::controls
namespace gooey {
    using namespace ooey;
using gooey::controls::CheckBox;
}
