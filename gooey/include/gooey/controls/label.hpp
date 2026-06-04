#pragma once

namespace ooey {}


#include "gooey/mvvmc/gooey_element.hpp"
#include "gooey/mvvmc/theme.hpp"
#include "ooey/renderer/primitives/text_primitive.hpp"
#include <memory>
#include <string>

namespace gooey::controls {
    using namespace ooey;

class Label : public mvvmc::GooeyElement {
public:
    static std::shared_ptr<Label> create() {
        return std::make_shared<Label>();
    }
    static std::shared_ptr<Label> create(std::string text, Font font, Point position, Color color) {
        return std::make_shared<Label>(std::move(text), font, position, color);
    }
    static std::shared_ptr<Label> create(LocalizedString text, Font font, Point position, Color color) {
        auto lbl = std::make_shared<Label>("", font, position, color);
        lbl->set_localized_text(std::move(text));
        return lbl;
    }

    void set_localized_text(LocalizedString text) {
        localized_key_ = std::move(text);
        bind(LocalizationManager::get().active_locale, [this](const std::string&) {
            this->set_text(LocalizationManager::get().translate(localized_key_.key));
        });
    }

    Label();
    Label(std::string text, Font font, Point position, Color color);

    void set_text(const std::string& text);
    const std::string& get_text() const;

    void set_font(const Font& font);
    const Font& get_font() const;

    void set_color(Color color);
    Color get_color() const;

    void set_position(Point position);
    Point get_position() const;

    TextOverflow get_overflow_policy() const { return overflow_; }
    void set_overflow_policy(TextOverflow overflow) { overflow_ = overflow; invalidate_layout(); }
    Label& set_overflow(TextOverflow overflow) { overflow_ = overflow; invalidate_layout(); return *this; }

    void draw(ooey::IRenderTarget& target) const override;

protected:
    // Layout support
    Size do_measure(Size constraints) override;
    void do_layout(Rect bounds) override;
    void apply_style(const mvvmc::Style& style) override;

private:
    std::vector<std::string> wrap_text(const std::string& text, const Font& font, int max_width, ooey::IRenderTarget* target = nullptr) const;
    void wrap_single_line(const std::string& line, const Font& font, int max_width, std::vector<std::string>& out_lines, ooey::IRenderTarget* target = nullptr) const;

    std::shared_ptr<TextPrimitive> text_primitive_;
    TextOverflow overflow_{TextOverflow::None};
    LocalizedString localized_key_{""};
};

} // namespace gooey::controls
namespace gooey {
    using namespace ooey;
using gooey::controls::Label;
}
