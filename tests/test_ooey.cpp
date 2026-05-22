#include <gtest/gtest.h>
#include "ooey/ooey.hpp"

TEST(OoeyCore, VersionCheck) {
    EXPECT_EQ(ooey::get_version(), "0.1.0");
}
