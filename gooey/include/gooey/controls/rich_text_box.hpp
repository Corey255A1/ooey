#pragma once

#include "gooey/mvvmc/gooey_node.hpp"
#include "gooey/mvvmc/i_interactive.hpp"
#include "gooey/controls/scrollbar.hpp"
#include "ooey/renderer/font_engine.hpp"
#include <vector>
#include <string>
#include <memory>
#include <functional>

namespace gooey::controls {
    using namespace ooey;

struct TextFormat {
    Color color;
    FontWeight weight{FontWeight::Normal};
    FontStyle style{FontStyle::Normal};
    int size{0}; // 0 means default font size

    bool operator==(const TextFormat&) const = default;
};

struct FormatRange {
    int start_col{0};
    int end_col{0};
    TextFormat format;

    bool operator==(const FormatRange&) const = default;
};

struct SquiggleRange {
    int line_idx{0};
    int start_col{0};
    int end_col{0};
    Color color;

    bool operator==(const SquiggleRange&) const = default;
};

class ScrollContainer;
class RichTextContentView;

class RichTextBox : public GooeyNode, public IInteractive {
public:
    RichTextBox();
    RichTextBox(Rect bounds, Font font, Color text_color, Color bg_color);
    virtual ~RichTextBox() override = default;

    Rect bounds() const override;

    void set_text(const std::string& text);
    std::string get_text() const;

    void set_font(const Font& font);
    const Font& get_font() const;

    // Formatting API
    void clear_formats();
    void clear_line_formats(int line_idx);
    void apply_format(int line_idx, int start_col, int end_col, const TextFormat& format);
    void set_line_formats(int line_idx, const std::vector<FormatRange>& formats);
    const std::vector<FormatRange>& get_line_formats(int line_idx) const;

    // Squiggle API
    void add_squiggle(int line_idx, int start_col, int end_col, Color color);
    void clear_squiggles();
    const std::vector<SquiggleRange>& get_squiggles() const;

    // Line/Text inspection APIs
    int get_lines_count() const;
    std::string get_line_text(int line_idx) const;
    int get_scroll_x() const;

    bool on_pointer_event(const Pointer& e) override;
    bool on_key_event(const KeyEvent& e) override;
    bool on_text_event(const TextEvent& e) override;

    void draw(ooey::IRenderTarget& target) const override;

    std::string get_selected_text() const;
    void insert_text(const std::string& text);
    bool has_selection() const { return has_selection_; }
    void clear_selection() { has_selection_ = false; }
    bool needs_scroll_x() const;
    bool needs_scroll_y() const;

    std::function<void(const std::string&)> on_text_changed;
    std::function<void()> on_undo;
    std::function<void()> on_redo;

    Color bg_color = Color{30, 30, 30};
    Color line_num_bg = Color{38, 38, 38};
    Color line_num_color = Color{120, 120, 120};
    Color divider_color = Color{60, 60, 60};
    Color cursor_color = Color{220, 220, 220};
    Color selection_color = Color{51, 153, 255, 100};
    Color default_text_color = Color{220, 220, 220};

    bool show_line_numbers{false};

protected:
    virtual void update_formatting();

    Size do_measure(Size constraints) override;
    void do_layout(Rect bounds) override;

    void get_selection_ordered(int& start_line, int& start_col, int& end_line, int& end_col) const;
    int get_column_x_offset(int line_idx, int col) const;

private:
    friend class RichTextContentView;
    void invalidate_content_layout();
    void scroll_cursor_into_view();
    void draw_content(ooey::IRenderTarget& target) const;
    Size measure_content(Size constraints);
    bool on_content_pointer_event(const Pointer& e);
    bool on_content_key_event(const KeyEvent& e);
    bool on_content_text_event(const TextEvent& e);

    Rect bounds_;
    Font font_;

    std::vector<std::string> lines_;
    std::vector<std::vector<FormatRange>> line_formats_;

    int cursor_line_{0};
    int cursor_col_{0};

    int anchor_line_{0};
    int anchor_col_{0};
    bool has_selection_{false};
    bool dragging_selection_{false};
    bool shift_pressed_{false};
    bool ctrl_pressed_{false};

    std::shared_ptr<ScrollContainer> scroll_container_;
    std::shared_ptr<RichTextContentView> content_view_;

    std::vector<SquiggleRange> squiggles_;
};

} // namespace gooey::controls
namespace gooey {
    using namespace ooey;
using gooey::controls::RichTextBox;
using gooey::controls::TextFormat;
using gooey::controls::FormatRange;
}
