#include <gtest/gtest.h>
#include "gooey/mvvmc/view.hpp"
#include "gooey/controls/column.hpp"
#include "gooey/controls/row.hpp"
#include "gooey/controls/grid.hpp"
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
