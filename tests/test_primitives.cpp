#include <gtest/gtest.h>
#include "ooey/i_window_backend.hpp"
#include "ooey/types.hpp"
#include "ooey/renderer/geometry.hpp"
#include "ooey/renderer/i_render_target.hpp"
#include "ooey/renderer/primitives/line_primitive.hpp"
#include "ooey/renderer/primitives/rect_primitive.hpp"
#include "ooey/renderer/primitives/circle_primitive.hpp"
#include "ooey/renderer/primitives/rounded_rect_primitive.hpp"
#include "ooey/renderer/primitives/polygon_primitive.hpp"
#include "ooey/renderer/primitives/curve_primitive.hpp"
#include "ooey/renderer/primitives/sinusoid_primitive.hpp"
#include <vector>

class MockRenderTarget : public ooey::IRenderTarget {
public:
    mutable std::vector<ooey::Geometry> geometries;

    void clear(ooey::Color /*color*/) override {}

    void draw_geometry(const ooey::Geometry& geometry) override {
        geometries.push_back(geometry);
    }

    ooey::Size measure_text(const std::string& /*text*/, const ooey::Font& /*font*/) override {
        return {0, 0};
    }

    void draw_text(const std::string& /*text*/, const ooey::Font& /*font*/, const ooey::Point& /*position*/, ooey::Color /*color*/) override {}

    void present() override {}
};

TEST(PrimitivesTest, LinePrimitiveThickness) {
    MockRenderTarget target;

    // Thin line
    ooey::LinePrimitive thin_line({10, 20}, {100, 200}, {255, 0, 0}, 1.0f);
    thin_line.draw(target);

    ASSERT_EQ(target.geometries.size(), 1);
    EXPECT_EQ(target.geometries[0].type, ooey::PrimitiveType::Lines);
    EXPECT_EQ(target.geometries[0].vertices.size(), 2);
    EXPECT_EQ(target.geometries[0].indices.size(), 2);

    target.geometries.clear();

    // Thick line
    ooey::LinePrimitive thick_line({10, 20}, {100, 200}, {255, 0, 0}, 5.0f);
    thick_line.draw(target);

    ASSERT_EQ(target.geometries.size(), 1);
    EXPECT_EQ(target.geometries[0].type, ooey::PrimitiveType::Triangles);
    EXPECT_EQ(target.geometries[0].vertices.size(), 4);
    EXPECT_EQ(target.geometries[0].indices.size(), 6);
}

TEST(PrimitivesTest, RectPrimitiveFillAndStroke) {
    MockRenderTarget target;

    // Fill only
    ooey::RectPrimitive fill_rect({10, 10, 100, 100}, {255, 0, 0});
    fill_rect.draw(target);

    ASSERT_EQ(target.geometries.size(), 1);
    EXPECT_EQ(target.geometries[0].vertices.size(), 4);
    EXPECT_EQ(target.geometries[0].indices.size(), 6);

    target.geometries.clear();

    // Stroke only
    ooey::RectPrimitive stroke_rect({10, 10, 100, 100}, {0, 0, 0, 0}, {0, 0, 255}, 4.0f);
    stroke_rect.draw(target);

    ASSERT_EQ(target.geometries.size(), 1);
    // 4 edges * 4 vertices = 16 vertices
    EXPECT_EQ(target.geometries[0].vertices.size(), 16);
    // 4 edges * 6 indices = 24 indices
    EXPECT_EQ(target.geometries[0].indices.size(), 24);

    target.geometries.clear();

    // Fill + Stroke
    ooey::RectPrimitive complex_rect({10, 10, 100, 100}, {255, 0, 0}, {0, 0, 255}, 4.0f);
    complex_rect.draw(target);

    ASSERT_EQ(target.geometries.size(), 1);
    EXPECT_EQ(target.geometries[0].vertices.size(), 20); // 4 (fill) + 16 (stroke)
    EXPECT_EQ(target.geometries[0].indices.size(), 30);  // 6 (fill) + 24 (stroke)
}

TEST(PrimitivesTest, CirclePrimitiveRendering) {
    MockRenderTarget target;

    // Fill + Stroke
    ooey::CirclePrimitive circle({50, 50}, 20, {255, 0, 0}, {0, 255, 0}, 3.0f);
    circle.draw(target);

    ASSERT_EQ(target.geometries.size(), 1);
    // Fill: 1 center + 64 perimeter = 65 vertices
    // Stroke: 64 edges * 4 vertices = 256 vertices
    // Total = 321 vertices
    EXPECT_EQ(target.geometries[0].vertices.size(), 321);
}

TEST(PrimitivesTest, RoundedRectPrimitiveRendering) {
    MockRenderTarget target;

    ooey::RoundedRectPrimitive rounded_rect({10, 10, 100, 100}, 10, {255, 0, 0}, {0, 0, 255}, 2.0f);
    rounded_rect.draw(target);

    ASSERT_EQ(target.geometries.size(), 1);
    // Check that it generated some geometry
    EXPECT_GT(target.geometries[0].vertices.size(), 0);
}

TEST(PrimitivesTest, PolygonPrimitiveRendering) {
    MockRenderTarget target;

    std::vector<ooey::Point> points = {{10, 10}, {50, 10}, {30, 40}};
    ooey::PolygonPrimitive poly(points, {255, 0, 0}, {0, 255, 0}, 2.0f);
    poly.draw(target);

    ASSERT_EQ(target.geometries.size(), 1);
    // Fill: 3 vertices. Stroke: 3 edges * 4 = 12 vertices. Total: 15 vertices.
    EXPECT_EQ(target.geometries[0].vertices.size(), 15);
}

TEST(PrimitivesTest, CurvePrimitiveBezier) {
    MockRenderTarget target;

    // Quadratic curve
    ooey::CurvePrimitive quad_curve({10, 10}, {50, 100}, {90, 10}, {255, 0, 0}, 3.0f);
    quad_curve.draw(target);

    ASSERT_EQ(target.geometries.size(), 1);
    // 30 segments * 4 vertices = 120 vertices
    EXPECT_EQ(target.geometries[0].vertices.size(), 120);

    target.geometries.clear();

    // Cubic curve
    ooey::CurvePrimitive cubic_curve({10, 10}, {30, 100}, {70, -100}, {90, 10}, {255, 0, 0}, 3.0f);
    cubic_curve.draw(target);

    ASSERT_EQ(target.geometries.size(), 1);
    EXPECT_EQ(target.geometries[0].vertices.size(), 120);
}

TEST(PrimitivesTest, SinusoidPrimitiveRendering) {
    MockRenderTarget target;

    // Thin sinusoid
    ooey::SinusoidPrimitive thin_sine({10, 100}, {210, 100}, 20.0f, 2.0f, 0.0f, {255, 0, 0}, 1.0f);
    thin_sine.draw(target);

    ASSERT_EQ(target.geometries.size(), 1);
    EXPECT_EQ(target.geometries[0].type, ooey::PrimitiveType::Lines);
    // 100 segments * 2 vertices = 200 vertices
    EXPECT_EQ(target.geometries[0].vertices.size(), 200);

    target.geometries.clear();

    // Thick sinusoid
    ooey::SinusoidPrimitive thick_sine({10, 100}, {210, 100}, 20.0f, 2.0f, 0.0f, {255, 0, 0}, 4.0f);
    thick_sine.draw(target);

    ASSERT_EQ(target.geometries.size(), 1);
    EXPECT_EQ(target.geometries[0].type, ooey::PrimitiveType::Triangles);
    // 100 segments * 4 vertices = 400 vertices
    EXPECT_EQ(target.geometries[0].vertices.size(), 400);
}

TEST(PrimitivesTest, GeometryCachingBehavior) {
    MockRenderTarget target;

    // 1. LinePrimitive
    {
        ooey::LinePrimitive line({0, 0}, {10, 10}, {255, 0, 0});
        EXPECT_TRUE(line.is_dirty());
        line.draw(target);
        EXPECT_FALSE(line.is_dirty());
        line.draw(target);
        EXPECT_FALSE(line.is_dirty());
        line.set_start({0, 0}); // same value
        EXPECT_FALSE(line.is_dirty());
        line.set_start({5, 5}); // different value
        EXPECT_TRUE(line.is_dirty());
        line.draw(target);
        EXPECT_FALSE(line.is_dirty());
    }

    // 2. CirclePrimitive
    {
        ooey::CirclePrimitive circle({0, 0}, 10, {255, 0, 0});
        EXPECT_TRUE(circle.is_dirty());
        circle.draw(target);
        EXPECT_FALSE(circle.is_dirty());
        circle.draw(target);
        EXPECT_FALSE(circle.is_dirty());
        circle.set_radius(10); // same value
        EXPECT_FALSE(circle.is_dirty());
        circle.set_radius(15); // different value
        EXPECT_TRUE(circle.is_dirty());
        circle.draw(target);
        EXPECT_FALSE(circle.is_dirty());
    }

    // 3. CurvePrimitive (Quadratic)
    {
        ooey::CurvePrimitive curve({0, 0}, {5, 10}, {10, 0}, {255, 0, 0});
        EXPECT_TRUE(curve.is_dirty());
        curve.draw(target);
        EXPECT_FALSE(curve.is_dirty());
        curve.draw(target);
        EXPECT_FALSE(curve.is_dirty());
        curve.set_p0({0, 0}); // same value
        EXPECT_FALSE(curve.is_dirty());
        curve.set_p0({1, 1}); // different value
        EXPECT_TRUE(curve.is_dirty());
        curve.draw(target);
        EXPECT_FALSE(curve.is_dirty());
    }

    // 4. PolygonPrimitive
    {
        std::vector<ooey::Point> points = {{0, 0}, {10, 0}, {5, 10}};
        ooey::PolygonPrimitive poly(points, {255, 0, 0});
        EXPECT_TRUE(poly.is_dirty());
        poly.draw(target);
        EXPECT_FALSE(poly.is_dirty());
        poly.draw(target);
        EXPECT_FALSE(poly.is_dirty());
        poly.set_points(points); // same value
        EXPECT_FALSE(poly.is_dirty());
        poly.set_points({{0, 0}, {20, 0}, {10, 20}}); // different value
        EXPECT_TRUE(poly.is_dirty());
        poly.draw(target);
        EXPECT_FALSE(poly.is_dirty());
    }

    // 5. RectPrimitive
    {
        ooey::RectPrimitive rect({0, 0, 10, 10}, {255, 0, 0});
        EXPECT_TRUE(rect.is_dirty());
        rect.draw(target);
        EXPECT_FALSE(rect.is_dirty());
        rect.draw(target);
        EXPECT_FALSE(rect.is_dirty());
        rect.set_rect({0, 0, 10, 10}); // same value
        EXPECT_FALSE(rect.is_dirty());
        rect.set_rect({0, 0, 20, 20}); // different value
        EXPECT_TRUE(rect.is_dirty());
        rect.draw(target);
        EXPECT_FALSE(rect.is_dirty());
    }

    // 6. RoundedRectPrimitive
    {
        ooey::RoundedRectPrimitive rrect({0, 0, 10, 10}, 2, {255, 0, 0});
        EXPECT_TRUE(rrect.is_dirty());
        rrect.draw(target);
        EXPECT_FALSE(rrect.is_dirty());
        rrect.draw(target);
        EXPECT_FALSE(rrect.is_dirty());
        rrect.set_corner_radius(2); // same value
        EXPECT_FALSE(rrect.is_dirty());
        rrect.set_corner_radius(4); // different value
        EXPECT_TRUE(rrect.is_dirty());
        rrect.draw(target);
        EXPECT_FALSE(rrect.is_dirty());
    }

    // 7. SinusoidPrimitive
    {
        ooey::SinusoidPrimitive sine({0, 0}, {10, 0}, 2.0f, 1.0f, 0.0f, {255, 0, 0});
        EXPECT_TRUE(sine.is_dirty());
        sine.draw(target);
        EXPECT_FALSE(sine.is_dirty());
        sine.draw(target);
        EXPECT_FALSE(sine.is_dirty());
        sine.set_amplitude(2.0f); // same value
        EXPECT_FALSE(sine.is_dirty());
        sine.set_amplitude(4.0f); // different value
        EXPECT_TRUE(sine.is_dirty());
        sine.draw(target);
        EXPECT_FALSE(sine.is_dirty());
    }
}

#include "ooey/renderer/window_chrome.hpp"

namespace {

class MockWindowBackend : public ooey::IWindowBackend {
public:
    bool create(const ooey::Size& /*size*/, const char* /*title*/) override { return true; }
    void destroy() override {}
    bool poll_events() override { return true; }
    void poll_input() override {}
    ooey::IRenderTarget* get_render_target() override { return nullptr; }
    void set_input_manager(ooey::InputManager* /*manager*/) override {}
    void set_window_chrome(std::shared_ptr<ooey::WindowChrome> /*chrome*/) override {}
    std::shared_ptr<ooey::WindowChrome> get_window_chrome() const override { return nullptr; }
    void start_interactive_move() override { move_called = true; }
    void start_interactive_resize(ooey::WindowResizeEdge edge) override { resize_called = true; last_edge = edge; }
    void request_close() override { close_called = true; }
    ooey::Size get_size() const override { return ooey::Size{800, 600}; }

    bool move_called{false};
    bool resize_called{false};
    ooey::WindowResizeEdge last_edge{ooey::WindowResizeEdge::None};
    bool close_called{false};
};

} // namespace

TEST(WindowChromeTest, HitTesting) {
    ooey::WindowChrome chrome;
    ooey::Size win_size{800, 600};

    // Assuming default: border_width = 4, title_bar_height = 30
    // Test Top-Left Corner
    EXPECT_EQ(chrome.hit_test(2, 2, win_size), ooey::ChromeHitTest::BorderTopLeft);
    // Test Top Border
    EXPECT_EQ(chrome.hit_test(100, 2, win_size), ooey::ChromeHitTest::BorderTop);
    // Test Title Bar
    EXPECT_EQ(chrome.hit_test(100, 15, win_size), ooey::ChromeHitTest::TitleBar);
    // Test Close Button (right side of title bar)
    // close_x starts at 800 - 4 - 30 = 766
    EXPECT_EQ(chrome.hit_test(780, 15, win_size), ooey::ChromeHitTest::CloseButton);
    // Test Minimize Button (starts at 736)
    EXPECT_EQ(chrome.hit_test(750, 15, win_size), ooey::ChromeHitTest::MinimizeButton);
    // Test Client Area
    EXPECT_EQ(chrome.hit_test(400, 300, win_size), ooey::ChromeHitTest::Client);
}

TEST(WindowChromeTest, HandlePointerEventMoveResizeClose) {
    auto chrome = std::make_shared<ooey::WindowChrome>();
    ooey::Size win_size{800, 600};
    MockWindowBackend backend;

    // 1. Move test (click on title bar)
    ooey::Pointer p_move{0, 100, 15, ooey::PointerState::Pressed};
    EXPECT_TRUE(chrome->handle_pointer_event(p_move, win_size, &backend));
    EXPECT_TRUE(backend.move_called);

    // 2. Resize test (click on top-left corner)
    ooey::Pointer p_resize{0, 2, 2, ooey::PointerState::Pressed};
    EXPECT_TRUE(chrome->handle_pointer_event(p_resize, win_size, &backend));
    EXPECT_TRUE(backend.resize_called);
    EXPECT_EQ(backend.last_edge, ooey::WindowResizeEdge::TopLeft);

    // 3. Close test (press and release on close button)
    ooey::Pointer p_close_press{0, 780, 15, ooey::PointerState::Pressed};
    EXPECT_TRUE(chrome->handle_pointer_event(p_close_press, win_size, &backend));
    EXPECT_FALSE(backend.close_called); // only called on release

    ooey::Pointer p_close_release{0, 780, 15, ooey::PointerState::Released};
    EXPECT_TRUE(chrome->handle_pointer_event(p_close_release, win_size, &backend));
    EXPECT_TRUE(backend.close_called);
}

TEST(ChromeRenderTargetTest, CoordinateShifting) {
    MockRenderTarget mock_target;
    auto chrome = std::make_shared<ooey::WindowChrome>();
    chrome->set_border_width(5);
    chrome->set_title_bar_height(25);
    // client offset: dx = 5, dy = 30

    ooey::ChromeRenderTarget decorated(&mock_target, chrome, {800, 600});

    // Draw some geometry
    ooey::Geometry geom;
    geom.type = ooey::PrimitiveType::Lines;
    geom.vertices.push_back({10.0f, 20.0f, {255, 0, 0}});
    
    decorated.draw_geometry(geom);

    ASSERT_EQ(mock_target.geometries.size(), 1);
    EXPECT_FLOAT_EQ(mock_target.geometries[0].vertices[0].x, 15.0f); // 10 + 5
    EXPECT_FLOAT_EQ(mock_target.geometries[0].vertices[0].y, 50.0f); // 20 + 30
}


