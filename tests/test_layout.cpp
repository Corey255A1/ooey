#include <gtest/gtest.h>
#include "gooey/mvvmc/view.hpp"
#include "gooey/controls/column.hpp"
#include "gooey/controls/row.hpp"
#include "gooey/controls/grid.hpp"
#include "gooey/controls/flow_layout.hpp"
#include "gooey/controls/button.hpp"
#include "gooey/controls/label.hpp"

using namespace gooey;
using namespace ooey;

TEST(LayoutTest, BaseViewMeasureFixedAndMatchParent) {
    auto view = std::make_shared<View>();
    view->set_width(SizePolicy::Fixed, 150.0f);
    view->set_height(SizePolicy::MatchParent);

    Size constraints{200, 300};
    Size measured = view->measure(constraints);

    EXPECT_EQ(measured.width, 150);
    EXPECT_EQ(measured.height, 300);
}

TEST(LayoutTest, BaseViewWrapContent) {
    auto parent = std::make_shared<View>();
    parent->set_width(SizePolicy::WrapContent);
    parent->set_height(SizePolicy::WrapContent);
    parent->set_padding(10);

    auto child1 = std::make_shared<View>();
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

    auto child1 = std::make_shared<View>();
    child1->set_width(SizePolicy::Fixed, 100.0f);
    child1->set_height(SizePolicy::Fixed, 40.0f);
    child1->set_margin(5);

    auto child2 = std::make_shared<View>();
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

    auto child1 = std::make_shared<View>();
    child1->set_width(SizePolicy::Fixed, 60.0f);
    child1->set_height(SizePolicy::Fixed, 50.0f);
    child1->set_margin(5);

    auto child2 = std::make_shared<View>();
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

    auto child1 = std::make_shared<View>();
    child1->set_margin(5);
    child1->set_align_self(Align::Stretch);

    auto child2 = std::make_shared<View>();
    child2->set_margin(5);
    child2->set_align_self(Align::Stretch);

    auto child3 = std::make_shared<View>();
    child3->set_margin(5);
    child3->set_align_self(Align::Stretch);

    auto child4 = std::make_shared<View>();
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
    auto parent = std::make_shared<View>();
    parent->set_width(SizePolicy::WrapContent);
    parent->set_height(SizePolicy::WrapContent);
    parent->set_padding(10);

    auto child = std::make_shared<View>();
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
    auto parent = std::make_shared<View>();
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

    auto child1 = std::make_shared<View>();
    child1->set_width(SizePolicy::Fixed, 100.0f);
    child1->set_height(SizePolicy::Fixed, 40.0f);
    child1->set_margin(5);

    auto child2 = std::make_shared<View>();
    child2->set_width(SizePolicy::Fixed, 80.0f);
    child2->set_height(SizePolicy::Fixed, 30.0f);
    child2->set_margin(5);

    auto child3 = std::make_shared<View>();
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
        auto card = std::make_shared<View>();
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

