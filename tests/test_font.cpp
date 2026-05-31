#include <gtest/gtest.h>
#include "ooey/renderer/font_engine.hpp"
#include "ooey/types.hpp"
#include <string>
#include <vector>

using namespace ooey;

TEST(FontEngineTest, GetAvailableFonts) {
    auto fonts = FontEngine::get_available_fonts();
    EXPECT_FALSE(fonts.empty());
    
    bool found_sans = false;
    for (const auto& f : fonts) {
        if (f == "sans-serif" || f == "Liberation Sans" || f == "DejaVu Sans") {
            found_sans = true;
            break;
        }
    }
    EXPECT_TRUE(found_sans);
}

TEST(FontEngineTest, MeasureText) {
    Font f{"sans-serif", 16};
    Size s1 = FontEngine::measure_text("Hello World", f);
    EXPECT_GT(s1.width, 0);
    EXPECT_GT(s1.height, 0);

    Size s2 = FontEngine::measure_text("", f);
    EXPECT_EQ(s2.width, 0);
    EXPECT_GT(s2.height, 0);
}

TEST(FontEngineTest, FontEquality) {
    Font f1{"sans-serif", 16};
    Font f2{"sans-serif", 16};
    Font f3{"serif", 16};
    Font f4{"sans-serif", 14};

    EXPECT_EQ(f1, f2);
    EXPECT_NE(f1, f3);
    EXPECT_NE(f1, f4);
}

#include "ooey/renderer/glyph_atlas.hpp"

TEST(FontEngineTest, GlyphAtlasGeneration) {
    Font font{"sans-serif", 16};
    auto atlas = FontEngine::get_glyph_atlas(font);
    ASSERT_NE(atlas, nullptr);
    ASSERT_NE(atlas->get_image(), nullptr);

    EXPECT_EQ(atlas->get_image()->width(), 512);
    EXPECT_EQ(atlas->get_image()->height(), 512);

    GlyphMetrics metrics;
    // 'A' is a standard printable character and should be packed in the atlas
    bool found_A = atlas->get_glyph_metrics('A', metrics);
    EXPECT_TRUE(found_A);
    EXPECT_GT(metrics.width, 0);
    EXPECT_GT(metrics.height, 0);
    EXPECT_GT(metrics.advance, 0);

    // Space should have an advance but no rasterized width/height in the atlas
    bool found_space = atlas->get_glyph_metrics(' ', metrics);
    EXPECT_TRUE(found_space);
    EXPECT_EQ(metrics.width, 0);
    EXPECT_EQ(metrics.height, 0);
    EXPECT_GT(metrics.advance, 0);
}
