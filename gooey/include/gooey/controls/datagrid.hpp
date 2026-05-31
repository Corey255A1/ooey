#pragma once

#include "gooey/mvvmc/view.hpp"
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

namespace gooey::controls {
    using namespace ooey;

struct DataGridColumn {
    std::string header;
    int width;
};

class DataGrid : public View, public IInteractive {
public:
    DataGrid(Rect bounds, int row_height, Font font);

    Rect bounds() const override;

    void set_columns(const std::vector<DataGridColumn>& columns);
    const std::vector<DataGridColumn>& get_columns() const { return columns_; }

    void set_rows(const std::vector<std::vector<std::string>>& rows);
    const std::vector<std::vector<std::string>>& get_rows() const { return rows_; }

    bool on_pointer_event(const Pointer& e) override;
    bool on_key_event(const KeyEvent& e) override;

protected:
    Size do_measure(Size constraints) override;
    void do_layout(Rect bounds) override;
    void apply_style(const mvvmc::Style& style) override;

private:
    void update_layout_elements();
    void update_scroll_ranges();
    void update_cell_values();

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

    std::shared_ptr<RoundedRectPrimitive> bg_;
    std::shared_ptr<RectPrimitive> header_bg_;
    std::vector<std::shared_ptr<TextPrimitive>> header_texts_;
    std::vector<std::shared_ptr<LinePrimitive>> header_dividers_;

    std::vector<std::vector<std::shared_ptr<RectPrimitive>>> cell_bgs_;
    std::vector<std::vector<std::shared_ptr<TextPrimitive>>> cell_texts_;

    std::shared_ptr<ScrollBar> v_scroll_;
    std::shared_ptr<ScrollBar> h_scroll_;
};

} // namespace gooey::controls
namespace gooey {
    using namespace ooey;
using gooey::controls::DataGridColumn;
using gooey::controls::DataGrid;
}
