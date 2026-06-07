#include <gtest/gtest.h>
#include "gooey/mvvmc/gooey_node.hpp"
#include "gooey/controls/column.hpp"
#include "gooey/controls/row.hpp"
#include "gooey/controls/grid.hpp"
#include "gooey/controls/flow_layout.hpp"
#include "gooey/controls/button.hpp"
#include "gooey/controls/label.hpp"
#include "gooey/controls/adaptive_stack.hpp"
#include "gooey/controls/scroll_container.hpp"
#include "gooey/controls/canvas_layout.hpp"
#include "gooey/controls/vector_shape_view.hpp"
#include "gooey/controls/rich_text_box.hpp"
#include "gooey/mvvmc/controller.hpp"

using namespace gooey;
using namespace ooey;

TEST(LayoutTest, BaseViewMeasureFixedAndMatchParent) {
    auto view = std::make_shared<GooeyNode>();
    view->set_width(SizePolicy::Fixed, 150.0f);
    view->set_height(SizePolicy::MatchParent);

    Size constraints{200, 300};
    Size measured = view->measure(constraints);

    EXPECT_EQ(measured.width, 150);
    EXPECT_EQ(measured.height, 300);
}

TEST(LayoutTest, BaseViewWrapContent) {
    auto parent = std::make_shared<GooeyNode>();
    parent->set_width(SizePolicy::WrapContent);
    parent->set_height(SizePolicy::WrapContent);
    parent->set_padding(10);

    auto child1 = std::make_shared<GooeyNode>();
    child1->set_width(SizePolicy::Fixed, 50.0f);
    child1->set_height(SizePolicy::Fixed, 30.0f);
    child1->set_margin(5);

    parent->add_child(child1);

    Size constraints{200, 200};
    Size measured = parent->measure(constraints);

    // Padding (10 left + 10 right) + Child fixed width (50) + Child margin (5 left + 5 right) = 80
    EXPECT_EQ(measured.width, 80);
    // Padding (10 top + 10 bottom) + Child fixed height (30) + Child margin (5 top + 5 bottom) = 60
    EXPECT_EQ(measured.height, 60);
}

TEST(LayoutTest, ColumnVerticalLayout) {
    auto col = std::make_shared<Column>();
    col->set_width(SizePolicy::WrapContent);
    col->set_height(SizePolicy::WrapContent);
    col->set_padding(10);

    auto child1 = std::make_shared<GooeyNode>();
    child1->set_width(SizePolicy::Fixed, 100.0f);
    child1->set_height(SizePolicy::Fixed, 40.0f);
    child1->set_margin(5);

    auto child2 = std::make_shared<GooeyNode>();
    child2->set_width(SizePolicy::Fixed, 80.0f);
    child2->set_height(SizePolicy::Fixed, 30.0f);
    child2->set_margin(10);

    col->add_child(child1);
    col->add_child(child2);

    Size constraints{300, 300};
    Size col_measured = col->measure(constraints);

    // Column width is max of children:
    // child1: 100 + 5 + 5 = 110
    // child2: 80 + 10 + 10 = 100
    // col width: max(110, 100) + 10 padding left + 10 padding right = 130
    EXPECT_EQ(col_measured.width, 130);

    // Column height is sum of children:
    // child1 total: 40 + 5 + 5 = 50
    // child2 total: 30 + 10 + 10 = 50
    // col height: 50 + 50 + 10 padding top + 10 padding bottom = 120
    EXPECT_EQ(col_measured.height, 120);

    col->layout(Rect{0, 0, col_measured.width, col_measured.height});

    // Check positions
    // child1: y starts at col.y + col.padding_top + child1.margin_top = 0 + 10 + 5 = 15
    // x: col.x + col.padding_left + child1.margin_left = 0 + 10 + 5 = 15
    EXPECT_EQ(child1->layout_bounds.x, 15);
    EXPECT_EQ(child1->layout_bounds.y, 15);
    EXPECT_EQ(child1->layout_bounds.width, 100);
    EXPECT_EQ(child1->layout_bounds.height, 40);

    // child2: y starts at child1.y + child1.height + child1.margin_bottom + child2.margin_top
    // = 15 + 40 + 5 + 10 = 70
    // x: col.x + col.padding_left + child2.margin_left = 0 + 10 + 10 = 20
    EXPECT_EQ(child2->layout_bounds.x, 20);
    EXPECT_EQ(child2->layout_bounds.y, 70);
    EXPECT_EQ(child2->layout_bounds.width, 80);
    EXPECT_EQ(child2->layout_bounds.height, 30);
}

TEST(LayoutTest, RowHorizontalLayout) {
    auto row = std::make_shared<Row>();
    row->set_width(SizePolicy::WrapContent);
    row->set_height(SizePolicy::WrapContent);
    row->set_padding(5);

    auto child1 = std::make_shared<GooeyNode>();
    child1->set_width(SizePolicy::Fixed, 60.0f);
    child1->set_height(SizePolicy::Fixed, 50.0f);
    child1->set_margin(5);

    auto child2 = std::make_shared<GooeyNode>();
    child2->set_width(SizePolicy::Fixed, 70.0f);
    child2->set_height(SizePolicy::Fixed, 40.0f);
    child2->set_margin(10);

    row->add_child(child1);
    row->add_child(child2);

    Size constraints{300, 300};
    Size row_measured = row->measure(constraints);

    // Row width is sum of children:
    // child1: 60 + 5 + 5 = 70
    // child2: 70 + 10 + 10 = 90
    // row width: 70 + 90 + 5 padding left + 5 padding right = 170
    EXPECT_EQ(row_measured.width, 170);

    // Row height is max of children:
    // child1: 50 + 5 + 5 = 60
    // child2: 40 + 10 + 10 = 60
    // row height: max(60, 60) + 5 padding top + 5 padding bottom = 70
    EXPECT_EQ(row_measured.height, 70);

    row->layout(Rect{10, 20, row_measured.width, row_measured.height});

    // Check positions
    // child1: x starts at row.x + row.padding_left + child1.margin_left = 10 + 5 + 5 = 20
    // y: row.y + row.padding_top + child1.margin_top = 20 + 5 + 5 = 30
    EXPECT_EQ(child1->layout_bounds.x, 20);
    EXPECT_EQ(child1->layout_bounds.y, 30);
    EXPECT_EQ(child1->layout_bounds.width, 60);
    EXPECT_EQ(child1->layout_bounds.height, 50);

    // child2: x starts at child1.x + child1.width + child1.margin_right + child2.margin_left
    // = 20 + 60 + 5 + 10 = 95
    // y: row.y + row.padding_top + child2.margin_top = 20 + 5 + 10 = 35
    EXPECT_EQ(child2->layout_bounds.x, 95);
    EXPECT_EQ(child2->layout_bounds.y, 35);
    EXPECT_EQ(child2->layout_bounds.width, 70);
    EXPECT_EQ(child2->layout_bounds.height, 40);
}

TEST(LayoutTest, GridLayout2x2) {
    auto grid = std::make_shared<Grid>(2, 2);
    grid->set_width(SizePolicy::Fixed, 200.0f);
    grid->set_height(SizePolicy::Fixed, 100.0f);
    grid->set_padding(10); // 180x80 content area, cells are 90x40

    auto child1 = std::make_shared<GooeyNode>();
    child1->set_margin(5);
    child1->set_align_self(Align::Stretch);

    auto child2 = std::make_shared<GooeyNode>();
    child2->set_margin(5);
    child2->set_align_self(Align::Stretch);

    auto child3 = std::make_shared<GooeyNode>();
    child3->set_margin(5);
    child3->set_align_self(Align::Stretch);

    auto child4 = std::make_shared<GooeyNode>();
    child4->set_margin(5);
    child4->set_align_self(Align::Stretch);

    grid->add_child(child1);
    grid->add_child(child2);
    grid->add_child(child3);
    grid->add_child(child4);

    Size measured = grid->measure(Size{300, 300});
    EXPECT_EQ(measured.width, 200);
    EXPECT_EQ(measured.height, 100);

    grid->layout(Rect{0, 0, 200, 100});

    // Content: x in [10, 190], y in [10, 90]
    // Cell 0,0: x in [10, 100], y in [10, 50]. child1 has margin 5, so layout is x in [15, 95], y in [15, 45]
    // width: 90 - 10 = 80, height: 40 - 10 = 30
    EXPECT_EQ(child1->layout_bounds.x, 15);
    EXPECT_EQ(child1->layout_bounds.y, 15);
    EXPECT_EQ(child1->layout_bounds.width, 80);
    EXPECT_EQ(child1->layout_bounds.height, 30);

    // Cell 0,1: x in [100, 190], y in [10, 50]. child2 has margin 5, so layout is x in [105, 185], y in [15, 45]
    EXPECT_EQ(child2->layout_bounds.x, 105);
    EXPECT_EQ(child2->layout_bounds.y, 15);
    EXPECT_EQ(child2->layout_bounds.width, 80);
    EXPECT_EQ(child2->layout_bounds.height, 30);

    // Cell 1,0: x in [10, 100], y in [50, 90]. child3 has margin 5, so layout is x in [15, 95], y in [55, 85]
    EXPECT_EQ(child3->layout_bounds.x, 15);
    EXPECT_EQ(child3->layout_bounds.y, 55);
    EXPECT_EQ(child3->layout_bounds.width, 80);
    EXPECT_EQ(child3->layout_bounds.height, 30);

    // Cell 1,1: x in [100, 190], y in [50, 90]. child4 has margin 5, so layout is x in [105, 185], y in [55, 85]
    EXPECT_EQ(child4->layout_bounds.x, 105);
    EXPECT_EQ(child4->layout_bounds.y, 55);
    EXPECT_EQ(child4->layout_bounds.width, 80);
    EXPECT_EQ(child4->layout_bounds.height, 30);
}

TEST(LayoutTest, AbsolutePositioningWithinView) {
    auto parent = std::make_shared<GooeyNode>();
    parent->set_width(SizePolicy::WrapContent);
    parent->set_height(SizePolicy::WrapContent);
    parent->set_padding(10);

    auto child = std::make_shared<GooeyNode>();
    child->set_absolute(true);
    child->set_absolute_bounds(Rect{100, 50, 120, 80});

    parent->add_child(child);

    Size measured = parent->measure(Size{500, 500});

    // Width should encompass child absolute bounds X (100) + width (120) + padding (10 left + 10 right) = 240
    EXPECT_EQ(measured.width, 240);
    // Height should encompass child absolute bounds Y (50) + height (80) + padding (10 top + 10 bottom) = 150
    EXPECT_EQ(measured.height, 150);

    parent->layout(Rect{0, 0, measured.width, measured.height});

    // Check laid-out position is bounds.x + padding_left + absolute_bounds.x = 0 + 10 + 100 = 110
    EXPECT_EQ(child->layout_bounds.x, 110);
    EXPECT_EQ(child->layout_bounds.y, 60);
    EXPECT_EQ(child->layout_bounds.width, 120);
    EXPECT_EQ(child->layout_bounds.height, 80);
}

TEST(LayoutTest, LabelLayoutDynamicAndAbsolute) {
    // 1. Dynamic positioning in Column
    auto col = std::make_shared<Column>();
    col->set_width(SizePolicy::WrapContent);
    col->set_height(SizePolicy::WrapContent);

    auto label1 = std::make_shared<Label>("Hello", Font{"sans-serif", 14}, Point{10, 10}, Color{255, 255, 255});
    label1->set_absolute(false); // opt-into Column's flow
    label1->set_margin(5);

    col->add_child(label1);

    Size size = col->measure(Size{500, 500});
    col->layout(Rect{0, 0, size.width, size.height});

    // Check label positioning under flow layout (margin 5)
    EXPECT_EQ(label1->layout_bounds.x, 5);
    EXPECT_EQ(label1->layout_bounds.y, 5);

    // 2. Absolute positioning
    auto parent = std::make_shared<GooeyNode>();
    auto label2 = std::make_shared<Label>("Hello World", Font{"sans-serif", 14}, Point{50, 60}, Color{255, 255, 255});
    parent->add_child(label2);

    Size p_size = parent->measure(Size{500, 500});
    parent->layout(Rect{0, 0, p_size.width, p_size.height});

    EXPECT_EQ(label2->layout_bounds.x, 50);
    EXPECT_EQ(label2->layout_bounds.y, 60);
}

TEST(LayoutTest, FlowLayoutWrapping) {
    auto flow = std::make_shared<FlowLayout>();
    flow->set_width(SizePolicy::WrapContent);
    flow->set_height(SizePolicy::WrapContent);
    flow->set_padding(10);

    auto child1 = std::make_shared<GooeyNode>();
    child1->set_width(SizePolicy::Fixed, 100.0f);
    child1->set_height(SizePolicy::Fixed, 40.0f);
    child1->set_margin(5);

    auto child2 = std::make_shared<GooeyNode>();
    child2->set_width(SizePolicy::Fixed, 80.0f);
    child2->set_height(SizePolicy::Fixed, 30.0f);
    child2->set_margin(5);

    auto child3 = std::make_shared<GooeyNode>();
    child3->set_width(SizePolicy::Fixed, 70.0f);
    child3->set_height(SizePolicy::Fixed, 50.0f);
    child3->set_margin(5);

    flow->add_child(child1);
    flow->add_child(child2);
    flow->add_child(child3);

    Size measured = flow->measure(Size{220, 500});

    EXPECT_EQ(measured.width, 220);
    EXPECT_EQ(measured.height, 130);

    flow->layout(Rect{0, 0, measured.width, measured.height});

    EXPECT_EQ(child1->layout_bounds.x, 15);
    EXPECT_EQ(child1->layout_bounds.y, 15);

    EXPECT_EQ(child2->layout_bounds.x, 125);
    EXPECT_EQ(child2->layout_bounds.y, 15);

    EXPECT_EQ(child3->layout_bounds.x, 15);
    EXPECT_EQ(child3->layout_bounds.y, 65);
}

#include "gooey/controls/text_box.hpp"

TEST(LayoutTest, MVVMCLayoutDebugging) {
    auto root = std::make_shared<Column>();
    root->set_width(SizePolicy::MatchParent);
    root->set_height(SizePolicy::MatchParent);
    root->set_padding(25);

    auto header = std::make_shared<Column>();
    header->set_width(SizePolicy::MatchParent);
    header->set_height(SizePolicy::WrapContent);
    header->set_margin(0, 0, 0, 15);

    auto title = std::make_shared<Label>("MVVMC Dashboard Demo", Font{"sans-serif", 28, FontWeight::Bold}, Point{0, 0}, Color{255, 255, 255});
    title->set_absolute(false);
    title->set_margin(0, 0, 0, 5);
    header->add_child(title);

    auto subtitle = std::make_shared<Label>("Resize this window...", Font{"sans-serif", 14}, Point{0, 0}, Color{150, 150, 160});
    subtitle->set_absolute(false);
    subtitle->set_margin(0, 0, 0, 15);
    header->add_child(subtitle);
    root->add_child(header);

    auto button_row = std::make_shared<FlowLayout>();
    button_row->set_width(SizePolicy::MatchParent);
    button_row->set_height(SizePolicy::WrapContent);
    button_row->set_margin(0, 0, 0, 20);

    auto cycle_theme_btn = std::make_shared<Button>(Rect{0, 0, 160, 40}, Color{0, 120, 215}, Color{0, 0, 0, 0}, 0.0f, 8, "Cycle Theme", Color{255, 255, 255});
    cycle_theme_btn->set_absolute(false);
    cycle_theme_btn->set_margin(0, 5, 10, 5);
    button_row->add_child(cycle_theme_btn);
    root->add_child(button_row);

    auto grid = std::make_shared<Grid>(2, 2);
    grid->set_width(SizePolicy::MatchParent);
    grid->set_height(SizePolicy::Fixed, 200.0f);
    grid->set_margin(0, 0, 0, 20);
    for (int i = 0; i < 4; ++i) {
        auto card = std::make_shared<GooeyNode>();
        card->set_width(SizePolicy::MatchParent);
        card->set_height(SizePolicy::MatchParent);
        grid->add_child(card);
    }
    root->add_child(grid);

    auto footer_row = std::make_shared<Row>();
    footer_row->set_width(SizePolicy::MatchParent);
    footer_row->set_height(SizePolicy::WrapContent);
    footer_row->set_margin(0, 0, 0, 10);

    auto name_lbl = std::make_shared<Label>("Enter Name:", Font{"sans-serif", 16}, Point{0, 0}, Color{200, 200, 200});
    name_lbl->set_absolute(false);
    name_lbl->set_margin(0, 5, 10, 0);
    footer_row->add_child(name_lbl);

    auto name_box = std::make_shared<TextBox>(Rect{0, 0, 250, 36}, Font{"sans-serif", 16}, Color{255, 255, 255}, Color{35, 35, 40});
    name_box->set_absolute(false);
    footer_row->add_child(name_box);
    root->add_child(footer_row);

    auto greeting_lbl = std::make_shared<Label>("Enter your name below to get started!", Font{"sans-serif", 16, FontWeight::Bold}, Point{0, 0}, Color{0, 200, 100});
    greeting_lbl->set_absolute(false);
    greeting_lbl->set_width(SizePolicy::MatchParent);
    greeting_lbl->set_margin(0, 5, 0, 15);
    root->add_child(greeting_lbl);

    Size size{800, 600};
    root->measure(size);
    root->layout(Rect{0, 0, size.width, size.height});

    std::cout << "[DEBUG] root layout bounds: " << root->layout_bounds.x << ", " << root->layout_bounds.y << ", " << root->layout_bounds.width << ", " << root->layout_bounds.height << "\n";
    std::cout << "[DEBUG] header layout bounds: " << header->layout_bounds.x << ", " << header->layout_bounds.y << ", " << header->layout_bounds.width << ", " << header->layout_bounds.height << "\n";
    std::cout << "[DEBUG] button_row layout bounds: " << button_row->layout_bounds.x << ", " << button_row->layout_bounds.y << ", " << button_row->layout_bounds.width << ", " << button_row->layout_bounds.height << "\n";
    std::cout << "[DEBUG] grid layout bounds: " << grid->layout_bounds.x << ", " << grid->layout_bounds.y << ", " << grid->layout_bounds.width << ", " << grid->layout_bounds.height << "\n";
    std::cout << "[DEBUG] footer_row layout bounds: " << footer_row->layout_bounds.x << ", " << footer_row->layout_bounds.y << ", " << footer_row->layout_bounds.width << ", " << footer_row->layout_bounds.height << "\n";
    std::cout << "[DEBUG] name_lbl layout bounds: " << name_lbl->layout_bounds.x << ", " << name_lbl->layout_bounds.y << ", " << name_lbl->layout_bounds.width << ", " << name_lbl->layout_bounds.height << "\n";
    std::cout << "[DEBUG] name_box layout bounds: " << name_box->layout_bounds.x << ", " << name_box->layout_bounds.y << ", " << name_box->layout_bounds.width << ", " << name_box->layout_bounds.height << "\n";
    std::cout << "[DEBUG] greeting_lbl layout bounds: " << greeting_lbl->layout_bounds.x << ", " << greeting_lbl->layout_bounds.y << ", " << greeting_lbl->layout_bounds.width << ", " << greeting_lbl->layout_bounds.height << "\n";
}

class CountingView : public GooeyNode {
public:
    int measure_count{0};
    int layout_count{0};

protected:
    Size do_measure(Size constraints) override {
        measure_count++;
        return GooeyNode::do_measure(constraints);
    }
    void do_layout(Rect bounds) override {
        layout_count++;
        GooeyNode::do_layout(bounds);
    }
};

TEST(LayoutTest, MeasureAndLayoutCaching) {
    auto view = std::make_shared<CountingView>();
    view->set_width(SizePolicy::Fixed, 100.0f);
    view->set_height(SizePolicy::Fixed, 50.0f);

    Size constraints{200, 200};
    Rect bounds{0, 0, 100, 50};

    // First pass - should measure and layout
    view->measure(constraints);
    view->layout(bounds);
    EXPECT_EQ(view->measure_count, 1);
    EXPECT_EQ(view->layout_count, 1);

    // Second pass with same inputs - should use cache
    view->measure(constraints);
    view->layout(bounds);
    EXPECT_EQ(view->measure_count, 1);
    EXPECT_EQ(view->layout_count, 1);

    // Invalidation - should reset cache
    view->invalidate_layout();
    view->measure(constraints);
    view->layout(bounds);
    EXPECT_EQ(view->measure_count, 2);
    EXPECT_EQ(view->layout_count, 2);
}

#include "gooey/controls/scrollbar.hpp"
#include "gooey/controls/datagrid.hpp"

TEST(LayoutTest, ScrollBarRangeAndValue) {
    ScrollBar scroll(Rect{0, 0, 100, 10}, ScrollBarOrientation::Horizontal);
    scroll.set_range(0, 100, 20);
    EXPECT_EQ(scroll.get_value(), 0);

    scroll.set_value(50);
    EXPECT_EQ(scroll.get_value(), 50);

    // Value should clamp to max_val - page_size = 80
    scroll.set_value(90);
    EXPECT_EQ(scroll.get_value(), 80);
}

TEST(LayoutTest, DataGridVirtualizationAndSetup) {
    Font font{"sans-serif", 12};
    DataGrid grid(Rect{0, 0, 400, 300}, 20, font);

    std::vector<DataGridColumn> cols = {
        {.header="Col 1", .width=100},
        {.header="Col 2", .width=150},
        {.header="Col 3", .width=150}
    };
    grid.set_columns(cols);

    std::vector<std::vector<std::string>> rows = {
        {"A1", "B1", "C1"},
        {"A2", "B2", "C2"},
        {"A3", "B3", "C3"},
        {"A4", "B4", "C4"},
        {"A5", "B5", "C5"}
    };
    grid.set_rows(rows);

    EXPECT_EQ(grid.get_columns().size(), 3);
    EXPECT_EQ(grid.get_rows().size(), 5);

    // Grid measure/layout should execute fine
    Size measured = grid.measure(Size{400, 300});
    EXPECT_EQ(measured.width, 400);
    EXPECT_EQ(measured.height, 300);

    grid.layout(Rect{0, 0, 400, 300});

    // Verify separation line properties and setters/getters
    grid.set_show_column_lines(false);
    EXPECT_FALSE(grid.get_show_column_lines());

    grid.set_show_row_lines(false);
    EXPECT_FALSE(grid.get_show_row_lines());

    grid.set_column_line_thickness(2.5f);
    EXPECT_FLOAT_EQ(grid.get_column_line_thickness(), 2.5f);

    grid.set_row_line_thickness(3.0f);
    EXPECT_FLOAT_EQ(grid.get_row_line_thickness(), 3.0f);

    Color col_color{255, 0, 0, 255};
    grid.set_column_line_color(col_color);
    EXPECT_EQ(grid.get_column_line_color(), col_color);

    Color row_color{0, 255, 0, 255};
    grid.set_row_line_color(row_color);
    EXPECT_EQ(grid.get_row_line_color(), row_color);

    grid.set_column_line_style(LineStyle::Dashed);
    EXPECT_EQ(grid.get_column_line_style(), LineStyle::Dashed);

    grid.set_row_line_style(LineStyle::Dotted);
    EXPECT_EQ(grid.get_row_line_style(), LineStyle::Dotted);
}

#include "gooey/controls/checkbox.hpp"

TEST(LayoutTest, DataGridCustomCellsAndItems) {
    Font font{"sans-serif", 12};
    DataGrid grid(Rect{0, 0, 400, 300}, 20, font);

    struct MyTask {
        std::string name;
        bool done;
    };

    std::vector<MyTask> tasks = {
        {"Task A", false},
        {"Task B", true},
        {"Task C", false}
    };

    std::vector<std::any> items;
    for (const auto& t : tasks) {
        items.push_back(t);
    }

    std::vector<DataGridColumn> cols = {
        {
            .header="Task Name",
            .width=200,
            .cell_factory=nullptr,
            .cell_binder=[](const std::shared_ptr<gooey::mvvmc::GooeyElement>& el, const std::any& item, int idx) {
                auto lbl = std::dynamic_pointer_cast<Label>(el);
                ASSERT_NE(lbl, nullptr);
                if (item.has_value()) {
                    auto t = std::any_cast<MyTask>(item);
                    lbl->set_text(t.name);
                } else {
                    lbl->set_text("");
                }
            }
        },
        {
            .header="Done",
            .width=100,
            .cell_factory=[]() {
                return std::make_shared<CheckBox>();
            },
            .cell_binder=[](const std::shared_ptr<gooey::mvvmc::GooeyElement>& el, const std::any& item, int idx) {
                auto cb = std::dynamic_pointer_cast<CheckBox>(el);
                ASSERT_NE(cb, nullptr);
                if (item.has_value()) {
                    auto t = std::any_cast<MyTask>(item);
                    cb->set_checked(t.done);
                } else {
                    cb->set_checked(false);
                }
            }
        }
    };

    grid.set_columns(cols);
    grid.set_items(items);

    EXPECT_EQ(grid.get_row_count(), 3);

    grid.layout(Rect{0, 0, 400, 300});

    auto children = grid.get_children();
    bool found_label = false;
    bool found_checkbox = false;
    for (const auto& child : children) {
        if (std::dynamic_pointer_cast<Label>(child)) {
            found_label = true;
        } else if (std::dynamic_pointer_cast<CheckBox>(child)) {
            found_checkbox = true;
        }
    }
    EXPECT_TRUE(found_label);
    EXPECT_TRUE(found_checkbox);
}

TEST(LayoutTest, AdaptiveStackHorizontal) {
    auto stack = std::make_shared<AdaptiveStack>();
    stack->set_breakpoint(600);
    stack->set_width(SizePolicy::MatchParent);
    stack->set_height(SizePolicy::MatchParent);
    stack->set_padding(10);

    auto child1 = std::make_shared<GooeyNode>();
    child1->set_width(SizePolicy::Fixed, 100.0f);
    child1->set_height(SizePolicy::Fixed, 50.0f);
    child1->set_margin(5);

    auto child2 = std::make_shared<GooeyNode>();
    child2->set_width(SizePolicy::Fixed, 150.0f);
    child2->set_height(SizePolicy::Fixed, 60.0f);
    child2->set_margin(5);

    stack->add_child(child1);
    stack->add_child(child2);

    // Measure with width > breakpoint (800 > 600) -> Horizontal/Row mode
    Size constraints{800, 300};
    Size measured = stack->measure(constraints);

    // Width: padding (20) + child1 (100 + 10) + child2 (150 + 10) = 290
    EXPECT_EQ(measured.width, 800); // MatchParent
    EXPECT_EQ(measured.height, 300); // MatchParent

    stack->layout(Rect{0, 0, 800, 300});

    // Check horizontal positions
    EXPECT_EQ(child1->layout_bounds.x, 15); // padding(10) + margin(5)
    EXPECT_EQ(child2->layout_bounds.x, 125); // child1.x(15) + child1.width(100) + child1.margin_right(5) + child2.margin_left(5)
    EXPECT_EQ(child1->layout_bounds.y, 15);
    EXPECT_EQ(child2->layout_bounds.y, 15);
}

TEST(LayoutTest, AdaptiveStackVertical) {
    auto stack = std::make_shared<AdaptiveStack>();
    stack->set_breakpoint(600);
    stack->set_width(SizePolicy::MatchParent);
    stack->set_height(SizePolicy::MatchParent);
    stack->set_padding(10);
    stack->set_stretch_when_vertical(true);

    auto child1 = std::make_shared<GooeyNode>();
    child1->set_width(SizePolicy::Fixed, 100.0f);
    child1->set_height(SizePolicy::Fixed, 50.0f);
    child1->set_margin(5);

    auto child2 = std::make_shared<GooeyNode>();
    child2->set_width(SizePolicy::Fixed, 150.0f);
    child2->set_height(SizePolicy::MatchParent); // vertical flex height child
    child2->set_margin(5);

    stack->add_child(child1);
    stack->add_child(child2);

    // Measure with width <= breakpoint (400 <= 600) -> Vertical/Column mode
    Size constraints{400, 300};
    Size measured = stack->measure(constraints);

    EXPECT_EQ(measured.width, 400); // MatchParent
    EXPECT_EQ(measured.height, 300); // MatchParent

    stack->layout(Rect{0, 0, 400, 300});

    // Check vertical stack positioning and stretching
    // Width should stretch to content width: 400 - padding(20) - margins(10) = 370
    EXPECT_EQ(child1->layout_bounds.width, 370);
    EXPECT_EQ(child2->layout_bounds.width, 370);

    // Check positions
    EXPECT_EQ(child1->layout_bounds.x, 15);
    EXPECT_EQ(child2->layout_bounds.x, 15);
    EXPECT_EQ(child1->layout_bounds.y, 15);
    // child1 height total: 50 + 10 = 60
    // child2 y: child1.y(15) + child1.height(50) + child1.margin_bottom(5) + child2.margin_top(5) = 75
    EXPECT_EQ(child2->layout_bounds.y, 75);

    // Remaining height: 300 - padding(20) - child1 total(60) = 220
    // child2 is MatchParent height, so it should get allocated the remaining height minus its margins (10) = 210
    EXPECT_EQ(child2->layout_bounds.height, 210);
}

TEST(LayoutTest, ScrollContainerNoScroll) {
    auto scroll = std::make_shared<ScrollContainer>();
    scroll->set_width(SizePolicy::MatchParent);
    scroll->set_height(SizePolicy::MatchParent);
    scroll->set_padding(10);

    auto child = std::make_shared<GooeyNode>();
    child->set_width(SizePolicy::MatchParent);
    child->set_height(SizePolicy::Fixed, 100.0f);
    scroll->set_child(child);

    Size constraints{400, 300};
    Size measured = scroll->measure(constraints);
    EXPECT_EQ(measured.width, 400);
    EXPECT_EQ(measured.height, 300);

    scroll->layout(Rect{0, 0, 400, 300});
    // With no scroll needed, child occupies the full height (offset y = 0)
    EXPECT_EQ(child->layout_bounds.x, 10); // padding
    EXPECT_EQ(child->layout_bounds.y, 10);
    EXPECT_EQ(child->layout_bounds.width, 380); // 400 - padding(20)
    EXPECT_EQ(child->layout_bounds.height, 100);
}

TEST(LayoutTest, ScrollContainerWithScrollAndDrag) {
    auto scroll = std::make_shared<ScrollContainer>();
    scroll->set_width(SizePolicy::MatchParent);
    scroll->set_height(SizePolicy::MatchParent);
    scroll->set_padding(10);

    auto child = std::make_shared<GooeyNode>();
    child->set_width(SizePolicy::MatchParent);
    child->set_height(SizePolicy::Fixed, 600.0f);
    scroll->set_child(child);

    Size constraints{400, 300};
    Size measured = scroll->measure(constraints);
    EXPECT_EQ(measured.width, 400);
    EXPECT_EQ(measured.height, 300);

    scroll->layout(Rect{0, 0, 400, 300});

    // Content height (600) > viewport height (300 - padding(20) = 280) -> Scroll is needed.
    // Child width should subtract scrollbar width (12px): 380 - 12 = 368
    EXPECT_EQ(child->layout_bounds.width, 368);
    EXPECT_EQ(child->layout_bounds.height, 600);
    EXPECT_EQ(scroll->get_scroll_offset_y(), 0);

    // Simulated drag scroll pointer events
    // 1. Pressed inside container
    scroll->on_pointer_event(Pointer{.id=0, .x=200, .y=150, .state=PointerState::Pressed});
    // 2. Dragged up by 50px (e.y = 100) -> dy = -50 -> scroll offset increases by 50
    scroll->on_pointer_event(Pointer{.id=0, .x=200, .y=100, .state=PointerState::Moved});
    EXPECT_EQ(scroll->get_scroll_offset_y(), 50);

    // Apply layout again with new scroll offset
    scroll->layout(Rect{0, 0, 400, 300});
    // Child is offset upwards: y = padding(10) - scroll_offset(50) = -40
    EXPECT_EQ(child->layout_bounds.y, -40);

    // 3. Drag release
    scroll->on_pointer_event(Pointer{.id=0, .x=200, .y=100, .state=PointerState::Released});
}

TEST(LayoutTest, ScrollContainerControllerInterception) {
    using namespace gooey::mvvmc;
    using namespace gooey::controls;

    InputManager input_manager;
    auto scroll = std::make_shared<ScrollContainer>();
    scroll->set_width(SizePolicy::MatchParent);
    scroll->set_height(SizePolicy::MatchParent);

    auto button = std::make_shared<Button>(Rect{10, 10, 100, 40}, Color{255, 255, 255});
    button->set_width(SizePolicy::Fixed, 100);
    button->set_height(SizePolicy::Fixed, 40);

    auto content = std::make_shared<GooeyNode>();
    content->set_width(SizePolicy::MatchParent);
    content->set_height(SizePolicy::Fixed, 600.0f);
    content->add_child(button);

    scroll->set_child(content);

    scroll->measure(Size{400, 300});
    scroll->layout(Rect{0, 0, 400, 300});

    EXPECT_EQ(button->layout_bounds.x, 10);
    EXPECT_EQ(button->layout_bounds.y, 10);

    Controller controller(input_manager, scroll);

    // 1. Press on the button
    input_manager.push_pointer_event(Pointer{.id=0, .x=50, .y=30, .state=PointerState::Pressed});
    controller.process_events();
    input_manager.update();

    // 2. Drag pointer vertically by 20 pixels (from y=30 to y=10)
    input_manager.push_pointer_event(Pointer{.id=0, .x=50, .y=10, .state=PointerState::Moved});
    controller.process_events();
    input_manager.update();

    // The scroll container should intercept the gesture, cancel button press, and drag scroll by 20 pixels
    EXPECT_EQ(scroll->get_scroll_offset_y(), 20);

    // 3. Release pointer
    input_manager.push_pointer_event(Pointer{.id=0, .x=50, .y=10, .state=PointerState::Released});
    controller.process_events();
    input_manager.update();

    // Offset should remain at 20
    EXPECT_EQ(scroll->get_scroll_offset_y(), 20);
}

class ClipSpyRenderTarget : public ooey::IRenderTarget {
public:
    std::vector<Rect> clips;
    std::vector<Rect> geometry_draws;
    std::vector<std::string> text_draws;
    std::vector<int> text_font_sizes;

    void clear(Color) override {}
    void draw_geometry(const Geometry& geometry) override {
        if (!geometry.vertices.empty()) {
            int min_x = geometry.vertices[0].x;
            int min_y = geometry.vertices[0].y;
            int max_x = geometry.vertices[0].x;
            int max_y = geometry.vertices[0].y;
            for (const auto& v : geometry.vertices) {
                min_x = std::min(min_x, (int)v.x);
                min_y = std::min(min_y, (int)v.y);
                max_x = std::max(max_x, (int)v.x);
                max_y = std::max(max_y, (int)v.y);
            }
            geometry_draws.emplace_back(min_x, min_y, max_x - min_x, max_y - min_y);
        }
    }
    void draw_image(const Image&, const Rect&) override {}
    Size measure_text(const std::string& text, const Font& font) override {
        // Mock measure: 10 pixels wide per char, 15 pixels high
        return Size{static_cast<int>(text.length() * 10), 15};
    }
    void draw_text(const std::string& text, const Font& font, const Point& position, Color color) override {
        text_draws.push_back(text);
        text_font_sizes.push_back(font.size);
    }
    void push_clip(const Rect& rect) override {
        clips.push_back(rect);
        clips_history.push_back(rect);
    }
    void pop_clip() override {
        if (!clips.empty()) {
            clips.pop_back();
        }
    }
    void present() override {}

    std::vector<Rect> clips_history;
};

TEST(LayoutTest, ViewClippingStack) {
    using namespace gooey::mvvmc;

    ClipSpyRenderTarget target;
    auto parent = std::make_shared<GooeyNode>();
    parent->set_clip_children(true);
    parent->layout(Rect{10, 20, 100, 100});

    parent->draw(target);

    // Parent has clip_children, so push_clip is called with layout bounds
    ASSERT_FALSE(target.clips_history.empty());
    EXPECT_EQ(target.clips_history[0].x, 10);
    EXPECT_EQ(target.clips_history[0].y, 20);
    EXPECT_EQ(target.clips_history[0].width, 100);
    EXPECT_EQ(target.clips_history[0].height, 100);
}

TEST(LayoutTest, TextOverflowClipped) {
    using namespace gooey::controls;

    ClipSpyRenderTarget target;
    Label lbl("Hello Clipping", Font("sans-serif", 12), Point{0,0}, Color{255,255,255});
    lbl.set_overflow(TextOverflow::Clipped);
    lbl.layout(Rect{5, 10, 50, 20});

    lbl.draw(target);

    // Clipped text should push/pop clip around layout bounds
    ASSERT_FALSE(target.clips_history.empty());
    EXPECT_EQ(target.clips_history[0].x, 5);
    EXPECT_EQ(target.clips_history[0].y, 10);
    EXPECT_EQ(target.clips_history[0].width, 50);
    EXPECT_EQ(target.clips_history[0].height, 20);
}

TEST(LayoutTest, TextOverflowShrunk) {
    using namespace gooey::controls;

    ClipSpyRenderTarget target;
    Label lbl("Hello Shrunk", Font("sans-serif", 12), Point{0,0}, Color{255,255,255});
    lbl.set_overflow(TextOverflow::Shrunk);
    // Hello Shrunk length is 12. At 10px per character mock, width is 120.
    // If layout width is 60, scaling factor is 60/120 = 0.5. Font size 12 * 0.5 = 6.
    lbl.layout(Rect{5, 10, 60, 20});

    lbl.draw(target);

    ASSERT_FALSE(target.text_font_sizes.empty());
    EXPECT_LE(target.text_font_sizes[0], 6);
}

TEST(LayoutTest, TextOverflowWrapped) {
    using namespace gooey::controls;

    ClipSpyRenderTarget target;
    Label lbl("Hello Wrapped Text Layout", Font("sans-serif", 12), Point{0,0}, Color{255,255,255});
    lbl.set_overflow(TextOverflow::Wrapped);
    // At 10px per char, "Hello Wrapped Text Layout" is 25 chars -> 250px wide.
    // If layout width is 60, it must wrap into multiple lines.
    lbl.layout(Rect{5, 10, 60, 100});

    lbl.draw(target);

    EXPECT_GT(target.text_draws.size(), 1);
}

TEST(LayoutTest, ScrollBarDrawZeroBounds) {
    using namespace gooey::controls;
    ClipSpyRenderTarget target;
    auto scrollbar = std::make_shared<ScrollBar>(Rect{0, 0, 0, 0}, ScrollBarOrientation::Vertical);
    scrollbar->draw(target);
    EXPECT_TRUE(target.geometry_draws.empty());
}

TEST(LayoutTest, ScrollBarLayoutZeroBounds) {
    using namespace gooey::controls;
    ClipSpyRenderTarget target;
    auto scrollbar = std::make_shared<ScrollBar>(Rect{0, 0, 12, 100}, ScrollBarOrientation::Vertical);
    scrollbar->layout(Rect{0, 0, 0, 0});
    scrollbar->draw(target);
    EXPECT_TRUE(target.geometry_draws.empty());
}

TEST(LayoutTest, CanvasAndVectorShapes) {
    using namespace gooey::mvvmc;
    using namespace gooey::controls;

    auto canvas = std::make_shared<CanvasLayout>();
    canvas->set_width(SizePolicy::Fixed, 400);
    canvas->set_height(SizePolicy::Fixed, 400);

    auto circle = std::make_shared<CircleShapeView>(Point{100, 100}, 20, Color{255, 0, 0});
    auto polygon = std::make_shared<PolygonShapeView>(
        std::vector<Point>{Point{200, 200}, Point{250, 200}, Point{225, 250}},
        Color{0, 255, 0}
    );

    canvas->add_child(circle);
    canvas->add_child(polygon);

    canvas->measure(Size{400, 400});
    canvas->layout(Rect{50, 50, 400, 400});

    EXPECT_EQ(circle->layout_bounds.x, 50 + 80); // 50 + (100 - 20)
    EXPECT_EQ(circle->layout_bounds.y, 50 + 80);
    EXPECT_EQ(circle->layout_bounds.width, 40);
    EXPECT_EQ(circle->layout_bounds.height, 40);

    EXPECT_EQ(polygon->layout_bounds.x, 50 + 200);
    EXPECT_EQ(polygon->layout_bounds.y, 50 + 200);
    EXPECT_EQ(polygon->layout_bounds.width, 50);
    EXPECT_EQ(polygon->layout_bounds.height, 50);

    EXPECT_FALSE(circle->is_selected());
    EXPECT_FALSE(polygon->is_selected());

    InputManager input_manager;
    Controller controller(input_manager, canvas);

    // Click inside circle (center is screen coordinates: 50 + 100 = 150, 50 + 100 = 150)
    input_manager.push_pointer_event(Pointer{.id=0, .x=150, .y=150, .state=PointerState::Pressed});
    controller.process_events();
    input_manager.update();

    EXPECT_TRUE(circle->is_selected());
    EXPECT_FALSE(polygon->is_selected());

    // Drag circle right by 30 pixels (to screen x = 180)
    input_manager.push_pointer_event(Pointer{.id=0, .x=180, .y=150, .state=PointerState::Moved});
    controller.process_events();
    input_manager.update();

    EXPECT_EQ(circle->absolute_bounds.x, 80 + 30);
    EXPECT_EQ(circle->absolute_bounds.y, 80);

    input_manager.push_pointer_event(Pointer{.id=0, .x=180, .y=150, .state=PointerState::Released});
    controller.process_events();
    input_manager.update();

    // In a unit test, we must manually trigger a layout pass to update layout_bounds
    canvas->layout(Rect{50, 50, 400, 400});

    // Click TL handle: TL handle is at top-left of circle layout_bounds
    // layout_bounds for circle is now at: x = 50 + 110 = 160, y = 50 + 80 = 130
    input_manager.push_pointer_event(Pointer{.id=0, .x=160, .y=130, .state=PointerState::Pressed});
    controller.process_events();
    input_manager.update();

    // Drag TL handle top-left by 10px (x=150, y=120)
    input_manager.push_pointer_event(Pointer{.id=0, .x=150, .y=120, .state=PointerState::Moved});
    controller.process_events();
    input_manager.update();

    EXPECT_EQ(circle->absolute_bounds.x, 110 - 10);
    EXPECT_EQ(circle->absolute_bounds.width, 40 + 10);
}

TEST(LayoutTest, RichTextBoxTextFormatting) {
    using namespace gooey::controls;
    
    Font font{"monospace", 14};
    RichTextBox box{Rect{0, 0, 400, 300}, font, Color{255, 255, 255}, Color{30, 30, 30}};
    
    box.set_text("Hello World");
    
    TextFormat red_bold{.color=Color{255, 0, 0}, .weight=FontWeight::Bold, .style=FontStyle::Normal, .size=0};
    TextFormat green_italic{.color=Color{0, 255, 0}, .weight=FontWeight::Normal, .style=FontStyle::Italic, .size=18};
    
    box.apply_format(0, 0, 5, red_bold);
    box.apply_format(0, 6, 11, green_italic);
    
    const auto& formats = box.get_line_formats(0);
    ASSERT_EQ(formats.size(), 2);
    EXPECT_EQ(formats[0].start_col, 0);
    EXPECT_EQ(formats[0].end_col, 5);
    EXPECT_EQ(formats[0].format.color, Color(255, 0, 0));
    EXPECT_EQ(formats[0].format.weight, FontWeight::Bold);
    
    EXPECT_EQ(formats[1].start_col, 6);
    EXPECT_EQ(formats[1].end_col, 11);
    EXPECT_EQ(formats[1].format.color, Color(0, 255, 0));
    EXPECT_EQ(formats[1].format.style, FontStyle::Italic);
    EXPECT_EQ(formats[1].format.size, 18);
}

TEST(LayoutTest, RichTextBoxResizeLayout) {
    using namespace gooey::controls;
    
    Font font{"monospace", 14};
    RichTextBox box{Rect{0, 0, 400, 300}, font, Color{255, 255, 255}, Color{30, 30, 30}};
    
    EXPECT_EQ(box.bounds().width, 400);
    EXPECT_EQ(box.bounds().height, 300);
    
    box.layout(Rect{10, 10, 800, 600});
    
    EXPECT_EQ(box.bounds().x, 10);
    EXPECT_EQ(box.bounds().y, 10);
    EXPECT_EQ(box.bounds().width, 800);
    EXPECT_EQ(box.bounds().height, 600);
}

TEST(LayoutTest, RichTextBoxHorizontalScrolling) {
    using namespace gooey::controls;
    
    Font font{"monospace", 14};
    RichTextBox box{Rect{0, 0, 100, 100}, font, Color{255, 255, 255}, Color{30, 30, 30}};
    
    EXPECT_EQ(box.get_scroll_x(), 0);
    
    // Type a very long string so it goes past the 100px width limit
    box.insert_text("This is an extremely long line of text that exceeds the bounds of the rich text box");
    
    // We expect make_cursor_visible to have updated scroll_x_ to be > 0
    EXPECT_GT(box.get_scroll_x(), 0);
}

TEST(LayoutTest, RichTextBoxScrollbarVisibility) {
    using namespace gooey::controls;
    
    Font font{"monospace", 10};
    RichTextBox box{Rect{0, 0, 100, 100}, font, Color{255, 255, 255}, Color{30, 30, 30}};
    
    box.measure(Size{100, 100});
    box.layout(Rect{0, 0, 100, 100});
    
    EXPECT_FALSE(box.needs_scroll_x());
    EXPECT_FALSE(box.needs_scroll_y());
    
    // Add multiple lines to trigger vertical scrollbar
    std::string multi_line = "";
    for (int i = 0; i < 20; ++i) {
        multi_line += "line " + std::to_string(i) + "\n";
    }
    box.set_text(multi_line);
    
    box.measure(Size{100, 100});
    box.layout(Rect{0, 0, 100, 100});
    
    EXPECT_TRUE(box.needs_scroll_y());
    
    // Set a very long single line to trigger horizontal scrollbar
    box.set_text("This is a very long line of text that should trigger the horizontal scrollbar because it overflows the viewport width bounds.");
    
    box.measure(Size{100, 100});
    box.layout(Rect{0, 0, 100, 100});
    
    EXPECT_TRUE(box.needs_scroll_x());
}

TEST(LayoutTest, ProportionalFlexAndAlignment) {
    auto row = std::make_shared<Row>();
    row->set_width(SizePolicy::Fixed, 300.0f);
    row->set_height(SizePolicy::Fixed, 100.0f);
    row->set_padding(10);
    row->set_spacing(10);
    row->set_align_items(Align::Center);

    auto child1 = std::make_shared<GooeyNode>();
    child1->set_width(SizePolicy::Flex, 1.0f);
    child1->set_height(SizePolicy::Fixed, 40.0f);

    auto child2 = std::make_shared<GooeyNode>();
    child2->set_width(SizePolicy::Flex, 2.0f);
    child2->set_height(SizePolicy::Fixed, 60.0f);

    row->add_child(child1);
    row->add_child(child2);

    // Measure and layout
    Size measured = row->measure(Size{300, 100});
    EXPECT_EQ(measured.width, 300);
    EXPECT_EQ(measured.height, 100);

    row->layout(Rect{0, 0, 300, 100});

    // Verify proportional flex sizing
    // avail_w = 300 - 20 (padding) - 10 (spacing) = 270.
    // child1: 1/3 of 270 = 90.
    // child2: 2/3 of 270 = 180.
    EXPECT_EQ(child1->get_measured_size().width, 90);
    EXPECT_EQ(child2->get_measured_size().width, 180);

    // Verify layout boundaries x-coordinates
    EXPECT_EQ(child1->layout_bounds.x, 10);
    EXPECT_EQ(child2->layout_bounds.x, 110); // 10 (padding) + 90 (child1 width) + 10 (spacing)

    // Verify cross-axis center alignment
    // Row inner content height is 80.
    // child1 (height 40): cy = padding_top(10) + (80 - 40)/2 = 30
    // child2 (height 60): cy = padding_top(10) + (80 - 60)/2 = 20
    EXPECT_EQ(child1->layout_bounds.y, 30);
    EXPECT_EQ(child2->layout_bounds.y, 20);

    // Verify main-axis SpaceBetween justification distribution
    auto row2 = std::make_shared<Row>();
    row2->set_width(SizePolicy::Fixed, 300.0f);
    row2->set_height(SizePolicy::Fixed, 100.0f);
    row2->set_padding(10);
    row2->set_spacing(0);
    row2->set_justify_content(Justify::SpaceBetween);

    auto fixed_child1 = std::make_shared<GooeyNode>();
    fixed_child1->set_width(SizePolicy::Fixed, 50.0f);
    fixed_child1->set_height(SizePolicy::Fixed, 50.0f);

    auto fixed_child2 = std::make_shared<GooeyNode>();
    fixed_child2->set_width(SizePolicy::Fixed, 50.0f);
    fixed_child2->set_height(SizePolicy::Fixed, 50.0f);

    row2->add_child(fixed_child1);
    row2->add_child(fixed_child2);

    row2->measure(Size{300, 100});
    row2->layout(Rect{0, 0, 300, 100});

    // Content area = 280. Total children width = 100.
    // SpaceBetween distribution gap: (280 - 100) / 1 = 180.
    // child1 x starts at 10.
    // child2 x starts at 10 + 50 (child1 width) + 180 (gap) = 240.
    EXPECT_EQ(fixed_child1->layout_bounds.x, 10);
    EXPECT_EQ(fixed_child2->layout_bounds.x, 240);
}

#include "gooey/controls/checkbox.hpp"
#include "gooey/controls/text_box.hpp"
#include "gooey/controls/datagrid.hpp"
#include <any>

struct TestSpreadsheetRow {
    struct CellData {
        std::string value{"0"};
        bool selected{false};
    };
    CellData cells[3];
};

class TestSpreadsheetCell : public gooey::GooeyElement, public IInteractive, public std::enable_shared_from_this<TestSpreadsheetCell> {
public:
    TestSpreadsheetCell(int row_idx, int col_idx, std::function<void(int, int, bool)> on_select_changed, std::function<void()> on_clear_selection)
        : row_idx_(row_idx), col_idx_(col_idx), on_select_changed_(on_select_changed), on_clear_selection_(on_clear_selection) {
        textbox_ = std::make_shared<gooey::controls::TextBox>();
    }

    Rect bounds() const override { return layout_bounds; }
    void set_text(const std::string& text) { textbox_->set_text(text); }
    std::string get_text() const { return textbox_->get_text(); }
    void set_row_index(int idx) { row_idx_ = idx; }
    void set_row_data(std::shared_ptr<TestSpreadsheetRow> row_data) { row_data_ = row_data; }
    void set_selected(bool selected) {
        if (is_selected_ != selected) {
            is_selected_ = selected;
            if (on_select_changed_) on_select_changed_(row_idx_, col_idx_, selected);
            invalidate_layout();
        }
        if (!is_selected_) is_editing_ = false;
    }
    bool is_selected() const { return is_selected_; }
    bool is_editing() const { return is_editing_; }

    void draw(ooey::IRenderTarget& target) const override {}

    bool on_pointer_event(const Pointer& e) override {
        bool hit = (e.x >= layout_bounds.x && e.x <= layout_bounds.x + layout_bounds.width &&
                    e.y >= layout_bounds.y && e.y <= layout_bounds.y + layout_bounds.height);
        if (hit && e.state == PointerState::Pressed) {
            if (!is_selected_) {
                if (on_clear_selection_) on_clear_selection_();
                set_selected(true);
                is_editing_ = false;
                invalidate_layout();
            } else {
                is_editing_ = true;
                original_value_ = textbox_->get_text();
                textbox_->on_pointer_event(e);
                invalidate_layout();
            }
            return true;
        }
        if (is_editing_) return textbox_->on_pointer_event(e);
        return false;
    }

    bool on_key_event(const KeyEvent& e) override {
        if (is_editing_) {
            if (e.state == KeyState::Pressed) {
                if (e.key_code == 0xFF1B || e.key_code == 27) { // Escape
                    textbox_->set_text(original_value_);
                    if (row_data_) {
                        row_data_->cells[col_idx_].value = original_value_;
                    }
                    is_editing_ = false;
                    invalidate_layout();
                    return true;
                } else if (e.key_code == 0xFF0D || e.key_code == 13 || e.key_code == 10) { // Return
                    is_editing_ = false;
                    invalidate_layout();
                    return true;
                }
            }
            return textbox_->on_key_event(e);
        }
        return false;
    }

    bool on_text_event(const TextEvent& e) override {
        if (is_editing_) {
            return textbox_->on_text_event(e);
        }
        return false;
    }

    std::shared_ptr<gooey::controls::TextBox> textbox() const { return textbox_; }

protected:
    void do_layout(Rect bounds) override {
        GooeyElement::do_layout(bounds);
        if (textbox_) textbox_->layout(bounds);
    }

private:
    std::shared_ptr<gooey::controls::TextBox> textbox_;
    int row_idx_;
    int col_idx_;
    bool is_selected_{false};
    mutable bool is_editing_{false};
    std::string original_value_;
    std::shared_ptr<TestSpreadsheetRow> row_data_{nullptr};
    std::function<void(int, int, bool)> on_select_changed_;
    std::function<void()> on_clear_selection_;
};

TEST(LayoutTest, SpreadsheetEditingInteraction) {
    Font font{"sans-serif", 12};
    auto root = std::make_shared<GooeyNode>();
    root->set_width(SizePolicy::Fixed, 500.0f);
    root->set_height(SizePolicy::Fixed, 500.0f);

    InputManager input_manager;
    auto controller = std::make_shared<gooey::mvvmc::Controller>(input_manager, root);

    auto grid = std::make_shared<DataGrid>(Rect{10, 10, 400, 300}, 30, font);
    root->add_child(grid);

    auto rows = std::make_shared<std::vector<std::shared_ptr<TestSpreadsheetRow>>>();
    for (int i = 0; i < 5; ++i) {
        auto r = std::make_shared<TestSpreadsheetRow>();
        r->cells[0].value = std::to_string((i + 1) * 10);
        r->cells[1].value = std::to_string((i + 1) * 5);
        r->cells[2].value = std::to_string(i + 1);
        rows->push_back(r);
    }

    std::vector<DataGridColumn> cols;
    for (int col_idx = 0; col_idx < 3; ++col_idx) {
        DataGridColumn col;
        col.header = std::string(1, 'A' + col_idx);
        col.width = 100;
        col.cell_factory = [rows, col_idx, grid_weak = std::weak_ptr<DataGrid>(grid)]() {
            return std::make_shared<TestSpreadsheetCell>(
                0, col_idx,
                [rows](int r, int c, bool sel) {
                    if (r < static_cast<int>(rows->size())) {
                        (*rows)[r]->cells[c].selected = sel;
                    }
                },
                [rows, grid_weak]() {
                    for (auto& row : *rows) {
                        for (int col = 0; col < 3; ++col) {
                            row->cells[col].selected = false;
                        }
                    }
                    if (auto g = grid_weak.lock()) {
                        g->update_cell_values();
                    }
                }
            );
        };
        col.cell_binder = [col_idx](const std::shared_ptr<GooeyElement>& el, const std::any& item, int row_idx) {
            auto cell = std::dynamic_pointer_cast<TestSpreadsheetCell>(el);
            if (!cell) return;
            cell->set_row_index(row_idx);
            if (item.has_value()) {
                auto row_data = std::any_cast<std::shared_ptr<TestSpreadsheetRow>>(item);
                cell->set_row_data(row_data);
                cell->set_text(row_data->cells[col_idx].value);
                cell->set_selected(row_data->cells[col_idx].selected);
                cell->textbox()->on_text_changed = [row_data, col_idx](const std::string& val) {
                    row_data->cells[col_idx].value = val;
                };
            }
        };
        cols.push_back(col);
    }
    grid->set_columns(cols);

    std::vector<std::any> grid_items;
    for (const auto& r : *rows) {
        grid_items.push_back(r);
    }
    grid->set_items(grid_items);

    root->measure(Size{500, 500});
    root->layout(Rect{0, 0, 500, 500});

    // Find the cell (0, 0) in the grid children
    std::shared_ptr<TestSpreadsheetCell> cell00 = nullptr;
    for (auto& child : grid->get_children()) {
        auto cell = std::dynamic_pointer_cast<TestSpreadsheetCell>(child);
        if (cell) {
            if (!cell00 || (cell->bounds().y < cell00->bounds().y) || 
                (cell->bounds().y == cell00->bounds().y && cell->bounds().x < cell00->bounds().x)) {
                cell00 = cell;
            }
        }
    }
    ASSERT_NE(cell00, nullptr);
    EXPECT_FALSE(cell00->is_selected());
    EXPECT_FALSE(cell00->is_editing());

    // 1. Simulate first click (Select)
    Rect cbounds = cell00->bounds();
    int click_x = cbounds.x + 5;
    int click_y = cbounds.y + 5;

    input_manager.push_pointer_event(Pointer{.id=0, .x=click_x, .y=click_y, .state=PointerState::Pressed});
    controller->process_events();
    EXPECT_TRUE(cell00->is_selected());
    EXPECT_FALSE(cell00->is_editing());
    input_manager.update();

    // Send released event to clean capture
    input_manager.push_pointer_event(Pointer{.id=0, .x=click_x, .y=click_y, .state=PointerState::Released});
    controller->process_events();
    input_manager.update();

    // 2. Simulate second click (Enter Edit Mode)
    input_manager.push_pointer_event(Pointer{.id=0, .x=click_x, .y=click_y, .state=PointerState::Pressed});
    controller->process_events();
    EXPECT_TRUE(cell00->is_selected());
    EXPECT_TRUE(cell00->is_editing());
    EXPECT_EQ(controller->get_focused_element(), cell00);
    input_manager.update();

    input_manager.push_pointer_event(Pointer{.id=0, .x=click_x, .y=click_y, .state=PointerState::Released});
    controller->process_events();
    input_manager.update();

    // 3. Simulate typing "5"
    input_manager.push_text_event(TextEvent{.codepoint='5'});
    controller->process_events();

    // Verify textbox text updated
    EXPECT_EQ(cell00->textbox()->get_text(), "105"); // Initial was 10, appended 5
    EXPECT_EQ((*rows)[0]->cells[0].value, "105");
    input_manager.update();

    // 4. Simulate pressing Escape to cancel
    input_manager.push_key_event(KeyEvent{.key_code=27, .state=KeyState::Pressed});
    controller->process_events();
    EXPECT_FALSE(cell00->is_editing());
    EXPECT_EQ(cell00->textbox()->get_text(), "10"); // reverted
    EXPECT_EQ((*rows)[0]->cells[0].value, "10");
    input_manager.update();
}








