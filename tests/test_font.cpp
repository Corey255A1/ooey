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
