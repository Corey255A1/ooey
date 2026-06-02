#include "gooey/controls/vector_shape_view.hpp"
#include "ooey/renderer/i_render_target.hpp"
#include <algorithm>
#include <cmath>

namespace {
bool is_point_in_polygon(ooey::Point p, const std::vector<ooey::Point>& poly) {
    int n = poly.size();
    if (n < 3) return false;
    bool inside = false;
    for (int i = 0, j = n - 1; i < n; j = i++) {
        if (((poly[i].y > p.y) != (poly[j].y > p.y)) &&
            (p.x < (poly[j].x - poly[i].x) * (p.y - poly[i].y) / (float)(poly[j].y - poly[i].y) + poly[i].x)) {
            inside = !inside;
        }
    }
    return inside;
}
}

namespace gooey::controls {
    using namespace ooey;

VectorShapeView::VectorShapeView() {
    is_absolute = true;
    
    selection_box_ = std::make_shared<RectPrimitive>(Rect{0,0,0,0}, Color{0,0,0,0}, Color{0, 120, 215, 255}, 1.5f);
    for (int i = 0; i < 4; ++i) {
        handles_[i] = std::make_shared<RectPrimitive>(Rect{0,0,0,0}, Color{255, 255, 255, 255}, Color{0, 120, 215, 255}, 1.5f);
    }
}

Rect VectorShapeView::bounds() const {
    return Rect{
        layout_bounds.x - 8,
        layout_bounds.y - 8,
        layout_bounds.width + 16,
        layout_bounds.height + 16
    };
}

void VectorShapeView::set_selected(bool selected) {
    if (is_selected_ != selected) {
        is_selected_ = selected;
        invalidate_layout();
    }
}

void VectorShapeView::draw(IRenderTarget& target) const {
    View::draw(target);

    if (is_selected_) {
        selection_box_->set_rect(layout_bounds);
        selection_box_->draw(target);

        int hs = 8;
        Rect corners[4] = {
            Rect{layout_bounds.x - hs/2, layout_bounds.y - hs/2, hs, hs}, // TL
            Rect{layout_bounds.x + layout_bounds.width - hs/2, layout_bounds.y - hs/2, hs, hs}, // TR
            Rect{layout_bounds.x - hs/2, layout_bounds.y + layout_bounds.height - hs/2, hs, hs}, // BL
            Rect{layout_bounds.x + layout_bounds.width - hs/2, layout_bounds.y + layout_bounds.height - hs/2, hs, hs} // BR
        };
        for (int i = 0; i < 4; ++i) {
            handles_[i]->set_rect(corners[i]);
            handles_[i]->draw(target);
        }
    }
}

bool VectorShapeView::on_pointer_event(const Pointer& e) {
    if (!interaction_enabled_) {
        return false;
    }

    if (e.state == PointerState::Pressed) {
        if (is_selected_) {
            int hs = 12;
            Rect corners[4] = {
                Rect{layout_bounds.x - hs/2, layout_bounds.y - hs/2, hs, hs}, // TL
                Rect{layout_bounds.x + layout_bounds.width - hs/2, layout_bounds.y - hs/2, hs, hs}, // TR
                Rect{layout_bounds.x - hs/2, layout_bounds.y + layout_bounds.height - hs/2, hs, hs}, // BL
                Rect{layout_bounds.x + layout_bounds.width - hs/2, layout_bounds.y + layout_bounds.height - hs/2, hs, hs} // BR
            };
            for (int i = 0; i < 4; ++i) {
                if (e.x >= corners[i].x && e.x <= corners[i].x + corners[i].width &&
                    e.y >= corners[i].y && e.y <= corners[i].y + corners[i].height) {
                    interaction_mode_ = ShapeInteractionMode::Resizing;
                    resize_handle_ = i;
                    last_pointer_x_ = e.x;
                    last_pointer_y_ = e.y;
                    return true;
                }
            }
        }

        if (hit_test(e.x, e.y)) {
            if (!is_selected_) {
                set_selected(true);
                if (on_selected) {
                    on_selected(this);
                }
            }
            interaction_mode_ = ShapeInteractionMode::Dragging;
            last_pointer_x_ = e.x;
            last_pointer_y_ = e.y;
            return true;
        }

        return false;
    } 
    else if (e.state == PointerState::Moved) {
        if (interaction_mode_ == ShapeInteractionMode::None) {
            return false;
        }

        int dx = e.x - last_pointer_x_;
        int dy = e.y - last_pointer_y_;
        if (dx == 0 && dy == 0) {
            return true;
        }

        if (interaction_mode_ == ShapeInteractionMode::Dragging) {
            absolute_bounds.x += dx;
            absolute_bounds.y += dy;
        } 
        else if (interaction_mode_ == ShapeInteractionMode::Resizing) {
            Rect new_bounds = absolute_bounds;
            if (resize_handle_ == 0) { // TL
                new_bounds.x += dx;
                new_bounds.width -= dx;
                new_bounds.y += dy;
                new_bounds.height -= dy;
            } else if (resize_handle_ == 1) { // TR
                new_bounds.width += dx;
                new_bounds.y += dy;
                new_bounds.height -= dy;
            } else if (resize_handle_ == 2) { // BL
                new_bounds.x += dx;
                new_bounds.width -= dx;
                new_bounds.height += dy;
            } else if (resize_handle_ == 3) { // BR
                new_bounds.width += dx;
                new_bounds.height += dy;
            }

            int min_size = 15;
            if (new_bounds.width < min_size) {
                if (resize_handle_ == 0 || resize_handle_ == 2) {
                    new_bounds.x = absolute_bounds.x + absolute_bounds.width - min_size;
                }
                new_bounds.width = min_size;
            }
            if (new_bounds.height < min_size) {
                if (resize_handle_ == 0 || resize_handle_ == 1) {
                    new_bounds.y = absolute_bounds.y + absolute_bounds.height - min_size;
                }
                new_bounds.height = min_size;
            }

            absolute_bounds = new_bounds;
        }

        last_pointer_x_ = e.x;
        last_pointer_y_ = e.y;
        
        invalidate_layout();
        
        if (on_changed) {
            on_changed(this);
        }
        return true;
    } 
    else if (e.state == PointerState::Released) {
        if (interaction_mode_ != ShapeInteractionMode::None) {
            interaction_mode_ = ShapeInteractionMode::None;
            resize_handle_ = -1;
            return true;
        }
        return false;
    }

    return false;
}

// CircleShapeView Implementation
CircleShapeView::CircleShapeView(Point center, int radius, Color fill_color, Color stroke_color, float stroke_thickness) {
    is_absolute = true;
    absolute_bounds = Rect{center.x - radius, center.y - radius, radius * 2, radius * 2};

    circle_ = std::make_shared<CirclePrimitive>(center, radius, fill_color, stroke_color, stroke_thickness);
    add_child(circle_);
}

void CircleShapeView::set_fill_color(Color color) {
    if (circle_) circle_->set_fill_color(color);
}

Color CircleShapeView::get_fill_color() const {
    return circle_ ? circle_->get_fill_color() : Color{0,0,0,0};
}

void CircleShapeView::set_stroke_color(Color color) {
    if (circle_) circle_->set_stroke_color(color);
}

Color CircleShapeView::get_stroke_color() const {
    return circle_ ? circle_->get_stroke_color() : Color{0,0,0,0};
}

void CircleShapeView::set_stroke_thickness(float thickness) {
    if (circle_) circle_->set_stroke_thickness(thickness);
}

float CircleShapeView::get_stroke_thickness() const {
    return circle_ ? circle_->get_stroke_thickness() : 0.0f;
}

bool CircleShapeView::hit_test(int px, int py) const {
    if (!circle_) return false;
    Point center = circle_->get_center();
    int r = circle_->get_radius();
    int dx = px - center.x;
    int dy = py - center.y;
    return (dx * dx + dy * dy) <= (r * r);
}

void CircleShapeView::do_layout(Rect bounds) {
    layout_bounds = bounds;
    View::do_layout(bounds);

    if (circle_) {
        Point center{bounds.x + bounds.width / 2, bounds.y + bounds.height / 2};
        int radius = std::min(bounds.width, bounds.height) / 2;
        circle_->set_center(center);
        circle_->set_radius(radius);
    }
}

// PolygonShapeView Implementation
PolygonShapeView::PolygonShapeView(std::vector<Point> points, Color fill_color, Color stroke_color, float stroke_thickness) {
    is_absolute = true;
    
    int min_x = 999999, max_x = -999999;
    int min_y = 999999, max_y = -999999;
    for (const auto& pt : points) {
        min_x = std::min(min_x, pt.x);
        max_x = std::max(max_x, pt.x);
        min_y = std::min(min_y, pt.y);
        max_y = std::max(max_y, pt.y);
    }
    int w = max_x - min_x;
    int h = max_y - min_y;
    if (w < 10) w = 10;
    if (h < 10) h = 10;

    absolute_bounds = Rect{min_x, min_y, w, h};

    for (const auto& pt : points) {
        float rx = (pt.x - min_x) / (float)w;
        float ry = (pt.y - min_y) / (float)h;
        relative_points_.push_back({rx, ry});
    }

    polygon_ = std::make_shared<PolygonPrimitive>(points, fill_color, stroke_color, stroke_thickness);
    add_child(polygon_);
}

void PolygonShapeView::set_fill_color(Color color) {
    if (polygon_) polygon_->set_fill_color(color);
}

Color PolygonShapeView::get_fill_color() const {
    return polygon_ ? polygon_->get_fill_color() : Color{0,0,0,0};
}

void PolygonShapeView::set_stroke_color(Color color) {
    if (polygon_) polygon_->set_stroke_color(color);
}

Color PolygonShapeView::get_stroke_color() const {
    return polygon_ ? polygon_->get_stroke_color() : Color{0,0,0,0};
}

void PolygonShapeView::set_stroke_thickness(float thickness) {
    if (polygon_) polygon_->set_stroke_thickness(thickness);
}

float PolygonShapeView::get_stroke_thickness() const {
    return polygon_ ? polygon_->get_stroke_thickness() : 0.0f;
}

std::vector<Point> PolygonShapeView::get_vertices() const {
    std::vector<Point> original_points;
    for (const auto& rp : relative_points_) {
        int cx = absolute_bounds.x + static_cast<int>(rp.first * absolute_bounds.width);
        int cy = absolute_bounds.y + static_cast<int>(rp.second * absolute_bounds.height);
        original_points.push_back(Point{cx, cy});
    }
    return original_points;
}

bool PolygonShapeView::hit_test(int px, int py) const {
    if (!polygon_) return false;
    return is_point_in_polygon(Point{px, py}, polygon_->get_points());
}

void PolygonShapeView::do_layout(Rect bounds) {
    layout_bounds = bounds;
    View::do_layout(bounds);

    if (polygon_) {
        std::vector<Point> screen_points;
        for (const auto& rp : relative_points_) {
            int sx = bounds.x + static_cast<int>(rp.first * bounds.width);
            int sy = bounds.y + static_cast<int>(rp.second * bounds.height);
            screen_points.push_back(Point{sx, sy});
        }
        polygon_->set_points(screen_points);
    }
}

} // namespace gooey::controls
