#pragma once

#include "gooey/mvvmc/gooey_node.hpp"
#include "gooey/mvvmc/i_interactive.hpp"
#include "gooey/controls/scrollbar.hpp"
#include "ooey/renderer/primitives/rect_primitive.hpp"
#include "ooey/renderer/primitives/rounded_rect_primitive.hpp"
#include "ooey/renderer/primitives/text_primitive.hpp"
#include "ooey/renderer/primitives/line_primitive.hpp"
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <any>

namespace gooey::controls {
    using namespace ooey;

struct DataGridColumn {
    std::string header;
    int width;
    std::function<std::shared_ptr<gooey::mvvmc::GooeyElement>()> cell_factory;
    std::function<void(const std::shared_ptr<gooey::mvvmc::GooeyElement>&, const std::any&, int)> cell_binder;
};

class DataGrid : public GooeyNode, public IInteractive {
public:
    DataGrid(Rect bounds, int row_height, Font font);

    Rect bounds() const override;

    void set_columns(const std::vector<DataGridColumn>& columns);
    const std::vector<DataGridColumn>& get_columns() const { return columns_; }

    void set_rows(const std::vector<std::vector<std::string>>& rows);
    const std::vector<std::vector<std::string>>& get_rows() const { return rows_; }

    void set_items(const std::vector<std::any>& items);
    const std::vector<std::any>& get_items() const { return items_; }
    int get_row_count() const;
    void update_cell_values();

    // separation lines styling
    void set_show_column_lines(bool show) { show_column_lines_ = show; invalidate_layout(); }
    bool get_show_column_lines() const { return show_column_lines_; }

    void set_show_row_lines(bool show) { show_row_lines_ = show; invalidate_layout(); }
    bool get_show_row_lines() const { return show_row_lines_; }

    void set_column_line_thickness(float thickness) { column_line_thickness_ = thickness; invalidate_layout(); }
    float get_column_line_thickness() const { return column_line_thickness_; }

    void set_row_line_thickness(float thickness) { row_line_thickness_ = thickness; invalidate_layout(); }
    float get_row_line_thickness() const { return row_line_thickness_; }

    void set_column_line_color(Color color) { column_line_color_ = color; invalidate_layout(); }
    Color get_column_line_color() const { return column_line_color_; }

    void set_row_line_color(Color color) { row_line_color_ = color; invalidate_layout(); }
    Color get_row_line_color() const { return row_line_color_; }

    void set_column_line_style(LineStyle style) { column_line_style_ = style; invalidate_layout(); }
    LineStyle get_column_line_style() const { return column_line_style_; }

    void set_row_line_style(LineStyle style) { row_line_style_ = style; invalidate_layout(); }
    LineStyle get_row_line_style() const { return row_line_style_; }

    bool on_pointer_event(const Pointer& e) override;
    bool on_key_event(const KeyEvent& e) override;

protected:
    Size do_measure(Size constraints) override;
    void do_layout(Rect bounds) override;
    void apply_style(const mvvmc::Style& style) override;

private:
    void update_layout_elements();
    void update_scroll_ranges();

    Rect bounds_;
    int row_height_;
    Font font_;

    std::vector<DataGridColumn> columns_;
    std::vector<std::vector<std::string>> rows_;

    int scroll_offset_y_{0};
    int scroll_offset_x_{0};

    int visible_rows_count_{0};

    Color bg_color_{30, 30, 35};
    Color header_bg_color_{45, 45, 50};
    Color border_color_{80, 80, 90};
    Color text_color_{240, 240, 240};
    Color header_text_color_{255, 255, 255};
    Color alt_row_bg_color_{35, 35, 40};

    bool show_column_lines_{true};
    bool show_row_lines_{true};
    float column_line_thickness_{1.0f};
    float row_line_thickness_{1.0f};
    Color column_line_color_{80, 80, 90};
    Color row_line_color_{80, 80, 90};
    LineStyle column_line_style_{LineStyle::Solid};
    LineStyle row_line_style_{LineStyle::Solid};

    std::shared_ptr<RoundedRectPrimitive> bg_;
    std::shared_ptr<RectPrimitive> header_bg_;
    std::vector<std::shared_ptr<TextPrimitive>> header_texts_;
    std::vector<std::shared_ptr<LinePrimitive>> header_dividers_;

    std::vector<std::shared_ptr<LinePrimitive>> row_dividers_;
    std::vector<std::shared_ptr<LinePrimitive>> column_dividers_;

    std::vector<std::vector<std::shared_ptr<RectPrimitive>>> cell_bgs_;
    std::vector<std::vector<std::shared_ptr<gooey::mvvmc::GooeyElement>>> cell_elements_;
    std::vector<std::any> items_;

    std::shared_ptr<ScrollBar> v_scroll_;
    std::shared_ptr<ScrollBar> h_scroll_;
};

} // namespace gooey::controls
namespace gooey {
    using namespace ooey;
using gooey::controls::DataGridColumn;
using gooey::controls::DataGrid;
}
