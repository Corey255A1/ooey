#pragma once

#include "ooey/renderer/i_drawable.hpp"
#include "ooey/types.hpp"
#include "ooey/renderer/geometry.hpp"

namespace ooey {

class SinusoidPrimitive : public IDrawable {
public:
    SinusoidPrimitive(Point start, Point end, float amplitude, float frequency, float phase, Color color, float thickness = 1.0f);

    void set_start(Point start);
    Point get_start() const;

    void set_end(Point end);
    Point get_end() const;

    void set_amplitude(float amplitude);
    float get_amplitude() const;

    void set_frequency(float frequency);
    float get_frequency() const;

    void set_phase(float phase);
    float get_phase() const;

    void set_color(Color color);
    Color get_color() const;

    void set_thickness(float thickness);
    float get_thickness() const;

    void draw(IRenderTarget& target) const override;

    bool is_dirty() const;

private:
    void rebuild_geometry() const;

    Point start_;
    Point end_;
    float amplitude_;
    float frequency_;
    float phase_;
    Color color_;
    float thickness_;

    mutable Geometry cached_geometry_;
    mutable bool is_dirty_{true};
};

} // namespace ooey
