#pragma once

#include "gooey/mvvmc/gooey_node.hpp"
#include "gooey/mvvmc/i_interactive.hpp"
#include "ooey/renderer/primitives/rect_primitive.hpp"
#include "ooey/renderer/primitives/circle_primitive.hpp"
#include "ooey/renderer/primitives/polygon_primitive.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace gooey::controls {
    using namespace ooey;

enum class ShapeInteractionMode {
    None,
    Dragging,
    Resizing
};

class VectorShapeView : public GooeyNode, public IInteractive {
public:
    VectorShapeView();
    virtual ~VectorShapeView() override = default;

    Rect bounds() const override;

    bool on_pointer_event(const Pointer& e) override;
    bool on_key_event(const KeyEvent& e) override { return false; }

    void draw(IRenderTarget& target) const override;

    void set_selected(bool selected);
    bool is_selected() const { return is_selected_; }

    void set_interaction_enabled(bool enabled) { interaction_enabled_ = enabled; }
    bool is_interaction_enabled() const { return interaction_enabled_; }

    virtual void set_fill_color(Color color) = 0;
    virtual Color get_fill_color() const = 0;

    virtual void set_stroke_color(Color color) = 0;
    virtual Color get_stroke_color() const = 0;

    virtual void set_stroke_thickness(float thickness) = 0;
    virtual float get_stroke_thickness() const = 0;

    std::function<void(VectorShapeView*)> on_selected;
    std::function<void(VectorShapeView*)> on_changed;

protected:
    virtual bool hit_test(int px, int py) const = 0;

    bool is_selected_{false};
    bool interaction_enabled_{true};
    ShapeInteractionMode interaction_mode_{ShapeInteractionMode::None};
    int resize_handle_{-1}; // 0: TL, 1: TR, 2: BL, 3: BR
    int last_pointer_x_{0};
    int last_pointer_y_{0};

    // Selection border and handles (not added to children_ to avoid auto layout/hit test interference)
    std::shared_ptr<RectPrimitive> selection_box_;
    std::shared_ptr<RectPrimitive> handles_[4];
};

class CircleShapeView : public VectorShapeView {
public:
    CircleShapeView(Point center, int radius, Color fill_color, Color stroke_color = Color{0,0,0,0}, float stroke_thickness = 0.0f);

    void set_fill_color(Color color) override;
    Color get_fill_color() const override;

    void set_stroke_color(Color color) override;
    Color get_stroke_color() const override;

    void set_stroke_thickness(float thickness) override;
    float get_stroke_thickness() const override;

protected:
    bool hit_test(int px, int py) const override;
    void do_layout(Rect bounds) override;

private:
    std::shared_ptr<CirclePrimitive> circle_;
};

class PolygonShapeView : public VectorShapeView {
public:
    PolygonShapeView(std::vector<Point> points, Color fill_color, Color stroke_color = Color{0,0,0,0}, float stroke_thickness = 0.0f);

    void set_fill_color(Color color) override;
    Color get_fill_color() const override;

    void set_stroke_color(Color color) override;
    Color get_stroke_color() const override;

    void set_stroke_thickness(float thickness) override;
    float get_stroke_thickness() const override;

    std::vector<Point> get_vertices() const;

protected:
    bool hit_test(int px, int py) const override;
    void do_layout(Rect bounds) override;

private:
    std::shared_ptr<PolygonPrimitive> polygon_;
    std::vector<std::pair<float, float>> relative_points_; // normalized relative to bounding box
};

} // namespace gooey::controls

namespace gooey {
using gooey::controls::VectorShapeView;
using gooey::controls::CircleShapeView;
using gooey::controls::PolygonShapeView;
}
