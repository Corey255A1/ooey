#include <gtest/gtest.h>
#include "ooey/types.hpp"
#include "ooey/renderer/geometry.hpp"
#include "ooey/renderer/i_render_target.hpp"
#include "gooey/renderer/primitives/line_primitive.hpp"
#include "gooey/renderer/primitives/rect_primitive.hpp"
#include "gooey/renderer/primitives/circle_primitive.hpp"
#include "gooey/renderer/primitives/rounded_rect_primitive.hpp"
#include "gooey/renderer/primitives/polygon_primitive.hpp"
#include "gooey/renderer/primitives/curve_primitive.hpp"
#include "gooey/renderer/primitives/sinusoid_primitive.hpp"
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
    gooey::LinePrimitive thin_line({10, 20}, {100, 200}, {255, 0, 0}, 1.0f);
    thin_line.draw(target);

    ASSERT_EQ(target.geometries.size(), 1);
    EXPECT_EQ(target.geometries[0].type, ooey::PrimitiveType::Lines);
    EXPECT_EQ(target.geometries[0].vertices.size(), 2);
    EXPECT_EQ(target.geometries[0].indices.size(), 2);

    target.geometries.clear();

    // Thick line
    gooey::LinePrimitive thick_line({10, 20}, {100, 200}, {255, 0, 0}, 5.0f);
    thick_line.draw(target);

    ASSERT_EQ(target.geometries.size(), 1);
    EXPECT_EQ(target.geometries[0].type, ooey::PrimitiveType::Triangles);
    EXPECT_EQ(target.geometries[0].vertices.size(), 4);
    EXPECT_EQ(target.geometries[0].indices.size(), 6);
}

TEST(PrimitivesTest, RectPrimitiveFillAndStroke) {
    MockRenderTarget target;

    // Fill only
    gooey::RectPrimitive fill_rect({10, 10, 100, 100}, {255, 0, 0});
    fill_rect.draw(target);

    ASSERT_EQ(target.geometries.size(), 1);
    EXPECT_EQ(target.geometries[0].vertices.size(), 4);
    EXPECT_EQ(target.geometries[0].indices.size(), 6);

    target.geometries.clear();

    // Stroke only
    gooey::RectPrimitive stroke_rect({10, 10, 100, 100}, {0, 0, 0, 0}, {0, 0, 255}, 4.0f);
    stroke_rect.draw(target);

    ASSERT_EQ(target.geometries.size(), 1);
    // 4 edges * 4 vertices = 16 vertices
    EXPECT_EQ(target.geometries[0].vertices.size(), 16);
    // 4 edges * 6 indices = 24 indices
    EXPECT_EQ(target.geometries[0].indices.size(), 24);

    target.geometries.clear();

    // Fill + Stroke
    gooey::RectPrimitive complex_rect({10, 10, 100, 100}, {255, 0, 0}, {0, 0, 255}, 4.0f);
    complex_rect.draw(target);

    ASSERT_EQ(target.geometries.size(), 1);
    EXPECT_EQ(target.geometries[0].vertices.size(), 20); // 4 (fill) + 16 (stroke)
    EXPECT_EQ(target.geometries[0].indices.size(), 30);  // 6 (fill) + 24 (stroke)
}

TEST(PrimitivesTest, CirclePrimitiveRendering) {
    MockRenderTarget target;

    // Fill + Stroke
    gooey::CirclePrimitive circle({50, 50}, 20, {255, 0, 0}, {0, 255, 0}, 3.0f);
    circle.draw(target);

    ASSERT_EQ(target.geometries.size(), 1);
    // Fill: 1 center + 64 perimeter = 65 vertices
    // Stroke: 64 edges * 4 vertices = 256 vertices
    // Total = 321 vertices
    EXPECT_EQ(target.geometries[0].vertices.size(), 321);
}

TEST(PrimitivesTest, RoundedRectPrimitiveRendering) {
    MockRenderTarget target;

    gooey::RoundedRectPrimitive rounded_rect({10, 10, 100, 100}, 10, {255, 0, 0}, {0, 0, 255}, 2.0f);
    rounded_rect.draw(target);

    ASSERT_EQ(target.geometries.size(), 1);
    // Check that it generated some geometry
    EXPECT_GT(target.geometries[0].vertices.size(), 0);
}

TEST(PrimitivesTest, PolygonPrimitiveRendering) {
    MockRenderTarget target;

    std::vector<ooey::Point> points = {{10, 10}, {50, 10}, {30, 40}};
    gooey::PolygonPrimitive poly(points, {255, 0, 0}, {0, 255, 0}, 2.0f);
    poly.draw(target);

    ASSERT_EQ(target.geometries.size(), 1);
    // Fill: 3 vertices. Stroke: 3 edges * 4 = 12 vertices. Total: 15 vertices.
    EXPECT_EQ(target.geometries[0].vertices.size(), 15);
}

TEST(PrimitivesTest, CurvePrimitiveBezier) {
    MockRenderTarget target;

    // Quadratic curve
    gooey::CurvePrimitive quad_curve({10, 10}, {50, 100}, {90, 10}, {255, 0, 0}, 3.0f);
    quad_curve.draw(target);

    ASSERT_EQ(target.geometries.size(), 1);
    // 30 segments * 4 vertices = 120 vertices
    EXPECT_EQ(target.geometries[0].vertices.size(), 120);

    target.geometries.clear();

    // Cubic curve
    gooey::CurvePrimitive cubic_curve({10, 10}, {30, 100}, {70, -100}, {90, 10}, {255, 0, 0}, 3.0f);
    cubic_curve.draw(target);

    ASSERT_EQ(target.geometries.size(), 1);
    EXPECT_EQ(target.geometries[0].vertices.size(), 120);
}

TEST(PrimitivesTest, SinusoidPrimitiveRendering) {
    MockRenderTarget target;

    // Thin sinusoid
    gooey::SinusoidPrimitive thin_sine({10, 100}, {210, 100}, 20.0f, 2.0f, 0.0f, {255, 0, 0}, 1.0f);
    thin_sine.draw(target);

    ASSERT_EQ(target.geometries.size(), 1);
    EXPECT_EQ(target.geometries[0].type, ooey::PrimitiveType::Lines);
    // 100 segments * 2 vertices = 200 vertices
    EXPECT_EQ(target.geometries[0].vertices.size(), 200);

    target.geometries.clear();

    // Thick sinusoid
    gooey::SinusoidPrimitive thick_sine({10, 100}, {210, 100}, 20.0f, 2.0f, 0.0f, {255, 0, 0}, 4.0f);
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

