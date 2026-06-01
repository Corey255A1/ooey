#include <gtest/gtest.h>
#include "ooey/renderer/i_render_target.hpp"
#include "ooey/renderer/scaled_render_target.hpp"
#include "ooey/renderer/image.hpp"
#include "ooey/input.hpp"
#include "gooey/application.hpp"
#include <cstdlib>
#include <vector>
#include <tuple>
#include <string>

namespace ooey::test {

class SpyRenderTarget : public IRenderTarget {
public:
    mutable std::vector<Geometry> geometries;
    mutable std::vector<std::pair<std::pair<int, int>, Rect>> images;
    mutable std::vector<std::tuple<std::string, Font, Point, Color>> texts;
    mutable std::vector<std::pair<std::string, Font>> text_measures;

    void clear(Color) override {}
    
    void draw_geometry(const Geometry& geometry) override {
        geometries.push_back(geometry);
    }
    
    void draw_geometry(const Geometry& geometry, const void* cache_key, bool is_dirty) override {
        geometries.push_back(geometry);
    }
    
    void draw_image(const Image& image, const Rect& dest_rect) override {
        images.push_back({{image.width(), image.height()}, dest_rect});
    }
    
    Size measure_text(const std::string& text, const Font& font) override {
        text_measures.push_back({text, font});
        // Return physical size: we pretend physical size is twice the font size
        return Size{static_cast<int>(text.length() * font.size), font.size};
    }
    
    void draw_text(const std::string& text, const Font& font, const Point& position, Color color) override {
        texts.push_back({text, font, position, color});
    }

    void push_clip(const Rect& rect) override {
        clips.push_back(rect);
    }

    void pop_clip() override {
        if (!clips.empty()) {
            clips.pop_back();
        }
    }
    
    void present() override {}

    mutable std::vector<Rect> clips;
};

// 1. Test geometry scaling
TEST(DpiScalingTest, GeometryScaling) {
    SpyRenderTarget spy;
    float scale = 2.0f;
    ScaledRenderTarget scaled_target(&spy, scale);

    Geometry geom;
    geom.vertices = {
        Vertex{10.0f, 20.0f, Color{255, 255, 255, 255}},
        Vertex{30.0f, 40.0f, Color{255, 255, 255, 255}}
    };

    scaled_target.draw_geometry(geom);

    ASSERT_EQ(spy.geometries.size(), 1);
    EXPECT_NEAR(spy.geometries[0].vertices[0].x, 20.0f, 1e-5);
    EXPECT_NEAR(spy.geometries[0].vertices[0].y, 40.0f, 1e-5);
    EXPECT_NEAR(spy.geometries[0].vertices[1].x, 60.0f, 1e-5);
    EXPECT_NEAR(spy.geometries[0].vertices[1].y, 80.0f, 1e-5);
}

// 2. Test image destination rect scaling
TEST(DpiScalingTest, ImageRectScaling) {
    SpyRenderTarget spy;
    float scale = 1.5f;
    ScaledRenderTarget scaled_target(&spy, scale);

    Image img(100, 100, {});

    Rect dest_rect{10, 20, 30, 40};
    scaled_target.draw_image(img, dest_rect);

    ASSERT_EQ(spy.images.size(), 1);
    EXPECT_EQ(spy.images[0].first.first, 100);
    EXPECT_EQ(spy.images[0].first.second, 100);
    EXPECT_EQ(spy.images[0].second.x, 15);
    EXPECT_EQ(spy.images[0].second.y, 30);
    EXPECT_EQ(spy.images[0].second.width, 45);
    EXPECT_EQ(spy.images[0].second.height, 60);
}

// 3. Test text rendering and size measurement scaling
TEST(DpiScalingTest, TextAndMeasurementScaling) {
    SpyRenderTarget spy;
    float scale = 2.0f;
    ScaledRenderTarget scaled_target(&spy, scale);

    Font font{"sans-serif", 12};
    
    // Test measure_text:
    // Input font size is 12 (logical).
    // Under scale = 2.0, ScaledRenderTarget queries physical target with font size 24.
    // SpyRenderTarget measure_text returns physical size: width = len(5)*24 = 120, height = 24.
    // ScaledRenderTarget scales it back down by division: logical width = 60, height = 12.
    Size logical_size = scaled_target.measure_text("hello", font);
    EXPECT_EQ(logical_size.width, 60);
    EXPECT_EQ(logical_size.height, 12);
    ASSERT_EQ(spy.text_measures.size(), 1);
    EXPECT_EQ(spy.text_measures[0].second.size, 24);

    // Test draw_text:
    // Input logical position is {10, 20}.
    // Scaled position should be {20, 40}, scaled font size should be 24.
    scaled_target.draw_text("hello", font, Point{10, 20}, Color{255, 0, 0, 255});
    ASSERT_EQ(spy.texts.size(), 1);
    EXPECT_EQ(std::get<0>(spy.texts[0]), "hello");
    EXPECT_EQ(std::get<1>(spy.texts[0]).size, 24);
    EXPECT_EQ(std::get<2>(spy.texts[0]).x, 20);
    EXPECT_EQ(std::get<2>(spy.texts[0]).y, 40);
}

// 4. Test input coordinate scaling
TEST(DpiScalingTest, InputCoordinateScaling) {
    InputManager input;
    input.set_scale(2.5f);

    Pointer p{0, 250, 500, PointerState::Pressed};
    input.push_pointer_event(p);

    const auto& events = input.get_pointer_events();
    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].x, 100);
    EXPECT_EQ(events[0].y, 200);

    const auto& active = input.get_active_pointers();
    ASSERT_EQ(active.size(), 1);
    EXPECT_EQ(active[0].x, 100);
    EXPECT_EQ(active[0].y, 200);
}

// 5. Test Application environment variable override behavior
TEST(DpiScalingTest, EnvironmentVariableScalingOverride) {
    gooey::Application app;
    app.set_dpi_scale_enabled(true);

    // Test default scale
    EXPECT_FLOAT_EQ(app.get_dpi_scale(), 1.0f);

    // Set scale environment variable override
#ifdef _WIN32
    _putenv_s("OOEY_SCALE", "3.5");
#else
    setenv("OOEY_SCALE", "3.5", 1);
#endif

    EXPECT_FLOAT_EQ(app.get_dpi_scale(), 3.5f);

    // Test 'off' string override
#ifdef _WIN32
    _putenv_s("OOEY_SCALE", "off");
#else
    setenv("OOEY_SCALE", "off", 1);
#endif

    EXPECT_FLOAT_EQ(app.get_dpi_scale(), 1.0f);

    // Test disabled DPI aware scale
    app.set_dpi_scale_enabled(false);
    EXPECT_FLOAT_EQ(app.get_dpi_scale(), 1.0f);

    // Clean up
#ifdef _WIN32
    _putenv_s("OOEY_SCALE", "");
#else
    unsetenv("OOEY_SCALE");
#endif
}

} // namespace ooey::test
