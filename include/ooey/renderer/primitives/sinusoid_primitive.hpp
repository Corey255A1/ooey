#pragma once

#include "ooey/mvvmc/i_drawable.hpp"
#include "ooey/types.hpp"

namespace ooey::renderer {

class SinusoidPrimitive : public IDrawable {
public:
    SinusoidPrimitive(Point start, Point end, float amplitude, float frequency, float phase, Color color, float thickness = 1.0f);

    void set_start(Point start) { start_ = start; }
    Point get_start() const { return start_; }

    void set_end(Point end) { end_ = end; }
    Point get_end() const { return end_; }

    void set_amplitude(float amplitude) { amplitude_ = amplitude; }
    float get_amplitude() const { return amplitude_; }

    void set_frequency(float frequency) { frequency_ = frequency; }
    float get_frequency() const { return frequency_; }

    void set_phase(float phase) { phase_ = phase; }
    float get_phase() const { return phase_; }

    void set_color(Color color) { color_ = color; }
    Color get_color() const { return color_; }

    void set_thickness(float thickness) { thickness_ = thickness; }
    float get_thickness() const { return thickness_; }

    void draw(IRenderTarget& target) const override;

private:
    Point start_;
    Point end_;
    float amplitude_;
    float frequency_;
    float phase_;
    Color color_;
    float thickness_;
};

} // namespace ooey::renderer

namespace ooey {
using renderer::SinusoidPrimitive;
}
