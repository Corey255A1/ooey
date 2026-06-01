#pragma once

#include "gooey/controls/column.hpp"
#include "ooey/types.hpp"

class StyledPanel : public gooey::controls::Column {
public:
    explicit StyledPanel();

    void draw(ooey::IRenderTarget& target) const override;
    void apply_style(const gooey::mvvmc::Style& style) override;

private:
    ooey::Color fill_color_{35, 35, 40};
    ooey::Color stroke_color_{0, 0, 0, 0};
    float stroke_thickness_{0.0f};
    int corner_radius_{8};
};
