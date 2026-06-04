#include <iostream>
#include <memory>
#include <utility>
#include <utility>
#include <vector>
#include <cmath>
#include <string>
#include "ooey/ooey.hpp"
#include "gooey/application.hpp"
#include "ooey/platform.hpp"
#include "gooey/mvvmc/gooey_node.hpp"
#include "gooey/controls/column.hpp"
#include "gooey/controls/row.hpp"
#include "gooey/controls/button.hpp"
#include "gooey/controls/label.hpp"
#include "gooey/controls/canvas_layout.hpp"
#include "gooey/controls/vector_shape_view.hpp"
#include "ooey/renderer/primitives/rect_primitive.hpp"
#include "ooey/renderer/primitives/rounded_rect_primitive.hpp"
#include "ooey/renderer/primitives/circle_primitive.hpp"
#include "ooey/renderer/primitives/polygon_primitive.hpp"
#include "ooey/renderer/primitives/line_primitive.hpp"

using namespace ooey;
using namespace gooey;
using namespace gooey::controls;

enum class PaintTool {
    Select,
    DrawCircle,
    DrawPolygon
};

std::string get_tool_name(PaintTool tool) {
    switch (tool) {
        case PaintTool::Select: return "Select Shape";
        case PaintTool::DrawCircle: return "Draw Circle";
        case PaintTool::DrawPolygon: return "Draw Polygon";
    }
    return "";
}

// Sidebar panel that draws a background card and acts as a vertical Column layout container
class SidebarPanel : public Column {
public:
    SidebarPanel(int width_px) {
        width = {SizePolicy::Fixed, static_cast<float>(width_px)};
        height = {SizePolicy::MatchParent};
        bg_ = std::make_shared<RoundedRectPrimitive>(Rect{0,0,0,0}, 12, Color{30, 30, 35}, Color{60, 60, 65}, 1.5f);
        add_child(bg_);
    }
protected:
    void do_layout(Rect bounds) override {
        bg_->set_rect(bounds);
        Column::do_layout(bounds);
    }
private:
    std::shared_ptr<RoundedRectPrimitive> bg_;
};

// Canvas container view that wraps CanvasLayout in a styled background frame
class CanvasContainer : public GooeyNode {
public:
    CanvasContainer() {
        width = {SizePolicy::MatchParent};
        height = {SizePolicy::MatchParent};
        bg_ = std::make_shared<RoundedRectPrimitive>(Rect{0,0,0,0}, 12, Color{22, 22, 26}, Color{45, 45, 50}, 1.5f);
        add_child(bg_);
    }
    
    void set_canvas(const std::shared_ptr<CanvasLayout>& canvas) {
        canvas_ = canvas;
        add_child(canvas);
    }
protected:
    void do_layout(Rect bounds) override {
        layout_bounds = bounds;
        bg_->set_rect(bounds);
        if (canvas_) {
            int pad = 5;
            canvas_->is_absolute = true;
            canvas_->set_absolute_bounds(Rect{pad, pad, bounds.width - pad*2, bounds.height - pad*2});
        }
        GooeyNode::do_layout(bounds);
    }
private:
    std::shared_ptr<RoundedRectPrimitive> bg_;
    std::shared_ptr<CanvasLayout> canvas_;
};

int main() {
    std::cout << "Starting OOEY Vector Paint Demo...\n";

    gooey::Application app;

    auto backend = ooey::create_default_window_backend();
    if (!backend || !backend->create({950, 700}, "OOEY Vector Paint")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }
    app.set_window_backend(std::move(backend));
    app.set_clear_color(Color{24, 24, 28});

    // --- State Variables ---
    PaintTool active_tool = PaintTool::Select;
    Color current_fill_color{0, 120, 215, 200}; // Default nice translucent blue
    Color current_stroke_color{255, 255, 255, 255}; // Default white outline
    float current_stroke_thickness = 2.0f;

    std::vector<std::shared_ptr<VectorShapeView>> shapes;
    VectorShapeView* selected_shape = nullptr;

    // Drawing-in-progress state
    bool is_drawing = false;
    Point start_point{0,0};
    std::shared_ptr<CircleShapeView> temp_drawing_shape = nullptr;

    // Polygon drawing state
    std::vector<Point> temp_poly_points;
    std::shared_ptr<PolygonPrimitive> temp_poly_preview = nullptr;

    // --- Layout Construction ---
    auto root_row = std::make_shared<Row>();
    root_row->set_width(SizePolicy::MatchParent);
    root_row->set_height(SizePolicy::MatchParent);
    root_row->set_padding(12);

    // 1. Sidebar Setup
    auto sidebar = std::make_shared<SidebarPanel>(220);
    sidebar->set_padding(15, 20, 15, 20);
    sidebar->set_margin(0, 0, 10, 0);

    // Sidebar Title
    auto title = std::make_shared<Label>("Vector Paint", Font{"sans-serif", 20, FontWeight::Bold}, Point{0,0}, Color{255, 255, 255});
    title->set_margin(0, 0, 0, 15);
    sidebar->add_child(title);

    // Tools Header
    auto tools_lbl = std::make_shared<Label>("TOOLS", Font{"sans-serif", 11, FontWeight::Bold}, Point{0,0}, Color{140, 140, 150});
    tools_lbl->set_margin(0, 0, 0, 8);
    sidebar->add_child(tools_lbl);

    // Tool Buttons
    auto select_btn = std::make_shared<Button>(Rect{0, 0, 190, 36}, Color{0, 120, 215}, Color{0,0,0,0}, 0.0f, 6, "Select Shape");
    select_btn->set_margin(0, 0, 0, 6);
    sidebar->add_child(select_btn);

    auto circle_btn = std::make_shared<Button>(Rect{0, 0, 190, 36}, Color{50, 50, 55}, Color{0,0,0,0}, 0.0f, 6, "Draw Circle");
    circle_btn->set_margin(0, 0, 0, 6);
    sidebar->add_child(circle_btn);

    auto polygon_btn = std::make_shared<Button>(Rect{0, 0, 190, 36}, Color{50, 50, 55}, Color{0,0,0,0}, 0.0f, 6, "Draw Polygon");
    polygon_btn->set_margin(0, 0, 0, 15);
    sidebar->add_child(polygon_btn);

    // Actions Header
    auto actions_lbl = std::make_shared<Label>("ACTIONS", Font{"sans-serif", 11, FontWeight::Bold}, Point{0,0}, Color{140, 140, 150});
    actions_lbl->set_margin(0, 0, 0, 8);
    sidebar->add_child(actions_lbl);

    auto finish_poly_btn = std::make_shared<Button>(Rect{0, 0, 190, 36}, Color{40, 140, 80}, Color{0,0,0,0}, 0.0f, 6, "Finish Polygon");
    finish_poly_btn->set_margin(0, 0, 0, 6);
    // Keep it hidden initially by setting size policy to zero (or height 0)
    finish_poly_btn->set_height(SizePolicy::Fixed, 0.0f);
    sidebar->add_child(finish_poly_btn);

    auto delete_btn = std::make_shared<Button>(Rect{0, 0, 190, 36}, Color{180, 50, 50}, Color{0,0,0,0}, 0.0f, 6, "Delete Selected");
    delete_btn->set_margin(0, 0, 0, 6);
    sidebar->add_child(delete_btn);

    auto clear_btn = std::make_shared<Button>(Rect{0, 0, 190, 36}, Color{65, 65, 70}, Color{0,0,0,0}, 0.0f, 6, "Clear Canvas");
    clear_btn->set_margin(0, 0, 0, 15);
    sidebar->add_child(clear_btn);

    // Colors Header
    auto colors_lbl = std::make_shared<Label>("FILL COLOR", Font{"sans-serif", 11, FontWeight::Bold}, Point{0,0}, Color{140, 140, 150});
    colors_lbl->set_margin(0, 0, 0, 8);
    sidebar->add_child(colors_lbl);

    // Fill Colors grid rows
    auto fill_row1 = std::make_shared<Row>();
    fill_row1->set_margin(0, 0, 0, 6);
    auto fill_row2 = std::make_shared<Row>();
    fill_row2->set_margin(0, 0, 0, 15);

    // Stroke Header
    auto stroke_lbl = std::make_shared<Label>("STROKE COLOR", Font{"sans-serif", 11, FontWeight::Bold}, Point{0,0}, Color{140, 140, 150});
    stroke_lbl->set_margin(0, 0, 0, 8);
    sidebar->add_child(stroke_lbl);

    auto stroke_row1 = std::make_shared<Row>();
    stroke_row1->set_margin(0, 0, 0, 6);
    auto stroke_row2 = std::make_shared<Row>();
    stroke_row2->set_margin(0, 0, 0, 15);

    sidebar->add_child(fill_row1);
    sidebar->add_child(fill_row2);
    sidebar->add_child(stroke_row1);
    sidebar->add_child(stroke_row2);

    // Thickness Header
    auto thick_lbl = std::make_shared<Label>("STROKE THICKNESS", Font{"sans-serif", 11, FontWeight::Bold}, Point{0,0}, Color{140, 140, 150});
    thick_lbl->set_margin(0, 0, 0, 8);
    sidebar->add_child(thick_lbl);

    auto thick_row = std::make_shared<Row>();
    thick_row->set_margin(0, 0, 0, 10);
    sidebar->add_child(thick_row);

    root_row->add_child(sidebar);

    // 2. Right Side Workspace Setup
    auto main_work_col = std::make_shared<Column>();
    main_work_col->set_width(SizePolicy::MatchParent);
    main_work_col->set_height(SizePolicy::MatchParent);

    auto canvas_container = std::make_shared<CanvasContainer>();
    canvas_container->set_margin(0, 0, 0, 10);

    auto canvas = std::make_shared<CanvasLayout>();
    canvas_container->set_canvas(canvas);
    main_work_col->add_child(canvas_container);

    // Status Panel at the bottom
    auto status_panel = std::make_shared<Row>();
    status_panel->set_width(SizePolicy::MatchParent);
    status_panel->set_height(SizePolicy::Fixed, 25.0f);
    status_panel->set_padding(5, 0, 5, 0);

    auto status_label = std::make_shared<Label>("Tool: Select Shape | No Selection", Font{"sans-serif", 12}, Point{0,0}, Color{160, 160, 170});
    status_panel->add_child(status_label);
    main_work_col->add_child(status_panel);

    root_row->add_child(main_work_col);

    // --- Callback / Utility functions ---
    auto update_status = [&]() {
        std::string tool_txt = "Tool: " + get_tool_name(active_tool);
        if (selected_shape) {
            auto b = selected_shape->absolute_bounds;
            if (dynamic_cast<CircleShapeView*>(selected_shape)) {
                status_label->set_text(tool_txt + " | Selected: Circle at (" + std::to_string(b.x + b.width/2) + "," + std::to_string(b.y + b.height/2) + ") r=" + std::to_string(b.width/2));
            } else {
                status_label->set_text(tool_txt + " | Selected: Polygon bounds (" + std::to_string(b.x) + "," + std::to_string(b.y) + ", w=" + std::to_string(b.width) + ", h=" + std::to_string(b.height) + ")");
            }
        } else {
            if (active_tool == PaintTool::DrawPolygon && !temp_poly_points.empty()) {
                status_label->set_text(tool_txt + " | Drawing Polygon: " + std::to_string(temp_poly_points.size()) + " points placed. Press Enter or click Finish to complete.");
            } else {
                status_label->set_text(tool_txt + " | No Selection");
            }
        }
    };

    auto update_selection_state = [&](VectorShapeView* shape) {
        if (selected_shape) {
            selected_shape->set_selected(false);
        }
        selected_shape = shape;
        if (selected_shape) {
            selected_shape->set_selected(true);
            
            // Sync current editor attributes to the selected shape's properties
            current_fill_color = selected_shape->get_fill_color();
            current_stroke_color = selected_shape->get_stroke_color();
            current_stroke_thickness = selected_shape->get_stroke_thickness();
        }
        update_status();
    };

    auto set_tool = [&](PaintTool tool) {
        active_tool = tool;

        // Change button styles
        select_btn->set_fill_color(active_tool == PaintTool::Select ? Color{0, 120, 215} : Color{50, 50, 55});
        circle_btn->set_fill_color(active_tool == PaintTool::DrawCircle ? Color{0, 120, 215} : Color{50, 50, 55});
        polygon_btn->set_fill_color(active_tool == PaintTool::DrawPolygon ? Color{0, 120, 215} : Color{50, 50, 55});

        // Toggle polygon finish button visibility
        if (active_tool == PaintTool::DrawPolygon) {
            finish_poly_btn->set_height(SizePolicy::Fixed, 36.0f);
        } else {
            finish_poly_btn->set_height(SizePolicy::Fixed, 0.0f);
            
            // Clean up any polygon preview if switching away
            if (!temp_poly_points.empty()) {
                if (temp_poly_preview) {
                    canvas->remove_child(temp_poly_preview);
                    temp_poly_preview = nullptr;
                }
                temp_poly_points.clear();
            }
        }

        // Toggle shape controls interaction
        for (auto& shape : shapes) {
            shape->set_interaction_enabled(active_tool == PaintTool::Select);
        }

        if (active_tool != PaintTool::Select) {
            update_selection_state(nullptr);
        }
        
        canvas->invalidate_layout();
        update_status();
    };

    // Tool Button bindings
    select_btn->on_click = [&]() { set_tool(PaintTool::Select); };
    circle_btn->on_click = [&]() { set_tool(PaintTool::DrawCircle); };
    polygon_btn->on_click = [&]() { set_tool(PaintTool::DrawPolygon); };

    // Delete binding
    delete_btn->on_click = [&]() {
        if (selected_shape) {
            // Find and remove from canvas & local vector
            for (auto it = shapes.begin(); it != shapes.end(); ++it) {
                if (it->get() == selected_shape) {
                    canvas->remove_child(*it);
                    shapes.erase(it);
                    break;
                }
            }
            update_selection_state(nullptr);
            canvas->invalidate_layout();
        }
    };

    // Clear binding
    clear_btn->on_click = [&]() {
        for (auto& shape : shapes) {
            canvas->remove_child(shape);
        }
        shapes.clear();
        if (temp_poly_preview) {
            canvas->remove_child(temp_poly_preview);
            temp_poly_preview = nullptr;
        }
        temp_poly_points.clear();
        is_drawing = false;
        temp_drawing_shape = nullptr;
        
        update_selection_state(nullptr);
        canvas->invalidate_layout();
    };

    // Finish Polygon binding
    auto finish_polygon = [&]() {
        if (active_tool == PaintTool::DrawPolygon && temp_poly_points.size() >= 3) {
            auto bounds = canvas->bounds();
            std::vector<Point> local_points;
            local_points.reserve(temp_poly_points.size());
for (const auto& pt : temp_poly_points) {
                local_points.emplace_back(pt.x - bounds.x, pt.y - bounds.y);
            }

            auto shape = std::make_shared<PolygonShapeView>(local_points, current_fill_color, current_stroke_color, current_stroke_thickness);
            
            shape->on_selected = [&](VectorShapeView* s) {
                update_selection_state(s);
            };
            shape->on_changed = [&](VectorShapeView*) {
                update_status();
            };

            canvas->add_child(std::move(shape));
            // Add shape to shapes vector (must cast or store base type)
            // Retrieve from children vector to get the shared_ptr back
            auto child_ptr = std::dynamic_pointer_cast<VectorShapeView>(canvas->get_children().back());
            if (child_ptr) {
                shapes.push_back(child_ptr);
                update_selection_state(child_ptr.get());
            }

            // Remove preview
            if (temp_poly_preview) {
                canvas->remove_child(temp_poly_preview);
                temp_poly_preview = nullptr;
            }
            temp_poly_points.clear();
            
            // Automatically switch back to Select tool so they can move it
            set_tool(PaintTool::Select);
        }
    };
    finish_poly_btn->on_click = finish_polygon;

    // --- Fill Color Preset Row Setup ---
    struct ColorPreset {
        Color color;
        std::string name;
    };
    std::vector<ColorPreset> fill_presets = {
        {.color=Color{230, 75, 75, 200}, .name="Red"},
        {.color=Color{40, 180, 100, 200}, .name="Green"},
        {.color=Color{0, 120, 215, 200}, .name="Blue"},
        {.color=Color{240, 195, 48, 200}, .name="Yellow"},
        {.color=Color{26, 188, 156, 200}, .name="Cyan"},
        {.color=Color{155, 89, 182, 200}, .name="Magenta"},
        {.color=Color{255, 255, 255, 200}, .name="White"},
        {.color=Color{0, 0, 0, 0}, .name="None"}
    };

    auto select_fill_color = [&](Color c) {
        current_fill_color = c;
        if (selected_shape) {
            selected_shape->set_fill_color(c);
            canvas->invalidate_layout();
        }
    };

    for (int i = 0; i < 4; ++i) {
        auto preset = fill_presets[i];
        std::shared_ptr<Button> btn;
        if (preset.color.a == 0) {
            btn = std::make_shared<Button>(Rect{0, 0, 42, 32}, Color{45, 45, 50}, Color{80, 80, 90}, 1.0f, 4, preset.name);
        } else {
            // Remove outline by setting stroke color alpha to 0
            btn = std::make_shared<Button>(Rect{0, 0, 42, 32}, Color{preset.color.r, preset.color.g, preset.color.b, 255});
        }
        btn->set_margin(0, 0, 6, 0);
        btn->on_click = [select_fill_color, preset]() { select_fill_color(preset.color); };
        fill_row1->add_child(btn);
    }
    for (int i = 4; i < 8; ++i) {
        auto preset = fill_presets[i];
        std::shared_ptr<Button> btn;
        if (preset.color.a == 0) {
            btn = std::make_shared<Button>(Rect{0, 0, 42, 32}, Color{45, 45, 50}, Color{80, 80, 90}, 1.0f, 4, preset.name);
        } else {
            btn = std::make_shared<Button>(Rect{0, 0, 42, 32}, Color{preset.color.r, preset.color.g, preset.color.b, 255});
        }
        btn->set_margin(0, 0, 6, 0);
        btn->on_click = [select_fill_color, preset]() { select_fill_color(preset.color); };
        fill_row2->add_child(btn);
    }

    // --- Stroke Color Preset Row Setup ---
    std::vector<ColorPreset> stroke_presets = {
        {.color=Color{230, 75, 75, 255}, .name="Red"},
        {.color=Color{40, 180, 100, 255}, .name="Green"},
        {.color=Color{0, 120, 215, 255}, .name="Blue"},
        {.color=Color{240, 195, 48, 255}, .name="Yellow"},
        {.color=Color{26, 188, 156, 255}, .name="Cyan"},
        {.color=Color{155, 89, 182, 255}, .name="Magenta"},
        {.color=Color{255, 255, 255, 255}, .name="White"},
        {.color=Color{0, 0, 0, 0}, .name="None"}
    };

    auto select_stroke_color = [&](Color c) {
        current_stroke_color = c;
        if (selected_shape) {
            selected_shape->set_stroke_color(c);
            canvas->invalidate_layout();
        }
    };

    for (int i = 0; i < 4; ++i) {
        auto preset = stroke_presets[i];
        std::shared_ptr<Button> btn;
        if (preset.color.a == 0) {
            btn = std::make_shared<Button>(Rect{0, 0, 42, 32}, Color{45, 45, 50}, Color{80, 80, 90}, 1.0f, 4, preset.name);
        } else {
            btn = std::make_shared<Button>(Rect{0, 0, 42, 32}, preset.color);
        }
        btn->set_margin(0, 0, 6, 0);
        btn->on_click = [select_stroke_color, preset]() { select_stroke_color(preset.color); };
        stroke_row1->add_child(btn);
    }
    for (int i = 4; i < 8; ++i) {
        auto preset = stroke_presets[i];
        std::shared_ptr<Button> btn;
        if (preset.color.a == 0) {
            btn = std::make_shared<Button>(Rect{0, 0, 42, 32}, Color{45, 45, 50}, Color{80, 80, 90}, 1.0f, 4, preset.name);
        } else {
            btn = std::make_shared<Button>(Rect{0, 0, 42, 32}, preset.color);
        }
        btn->set_margin(0, 0, 6, 0);
        btn->on_click = [select_stroke_color, preset]() { select_stroke_color(preset.color); };
        stroke_row2->add_child(btn);
    }

    // --- Thickness Preset Setup ---
    auto set_stroke_thickness = [&](float thick) {
        current_stroke_thickness = thick;
        if (selected_shape) {
            selected_shape->set_stroke_thickness(thick);
            canvas->invalidate_layout();
        }
    };

    std::vector<int> thick_values = {1, 2, 4, 8};
    for (int val : thick_values) {
        auto btn = std::make_shared<Button>(Rect{0, 0, 42, 32}, Color{50, 50, 55}, Color{80, 80, 90}, 1.0f, 4, std::to_string(val) + "px");
        btn->set_margin(0, 0, 6, 0);
        btn->on_click = [set_stroke_thickness, val]() { set_stroke_thickness(static_cast<float>(val)); };
        thick_row->add_child(btn);
    }

    // --- Canvas Pointer Interaction ---
    canvas->on_canvas_pointer = [&](const Pointer& e) {
        auto bounds = canvas->bounds();

        if (active_tool == PaintTool::Select) {
            if (e.state == PointerState::Pressed) {
                // Clicked empty canvas, clear selection
                update_selection_state(nullptr);
            }
        } 
        else if (active_tool == PaintTool::DrawCircle) {
            if (e.state == PointerState::Pressed) {
                is_drawing = true;
                start_point = Point{e.x, e.y};

                // Map start point to canvas-local coordinates
                int cx = start_point.x - bounds.x;
                int cy = start_point.y - bounds.y;

                auto shape = std::make_shared<CircleShapeView>(
                    Point{cx, cy}, 2, 
                    current_fill_color, current_stroke_color, current_stroke_thickness
                );
                // Temporarily disable interaction so mouse dragging isn't intercepted by the circle view itself
                shape->set_interaction_enabled(false);
                shape->on_selected = [&](VectorShapeView* s) {
                    update_selection_state(s);
                };
                shape->on_changed = [&](VectorShapeView*) {
                    update_status();
                };

                canvas->add_child(std::move(shape));
                // Retrieve the added child as shared_ptr
                temp_drawing_shape = std::dynamic_pointer_cast<CircleShapeView>(canvas->get_children().back());
            } 
            else if (e.state == PointerState::Moved && is_drawing) {
                if (temp_drawing_shape) {
                    // Update circle radius based on current mouse distance
                    int dx = e.x - start_point.x;
                    int dy = e.y - start_point.y;
                    int radius = static_cast<int>(std::sqrt(dx * dx + dy * dy));
                    if (radius < 2) radius = 2;

                    int cx = start_point.x - bounds.x;
                    int cy = start_point.y - bounds.y;

                    // Update bounding box
                    temp_drawing_shape->absolute_bounds = Rect{cx - radius, cy - radius, radius * 2, radius * 2};
                    canvas->invalidate_layout();
                }
            } 
            else if (e.state == PointerState::Released && is_drawing) {
                if (temp_drawing_shape) {
                    // Finalize drawing
                    int dx = e.x - start_point.x;
                    int dy = e.y - start_point.y;
                    int radius = static_cast<int>(std::sqrt(dx * dx + dy * dy));

                    if (radius < 5) {
                        // Discard tiny circle
                        canvas->remove_child(temp_drawing_shape);
                    } else {
                        temp_drawing_shape->set_interaction_enabled(true);
                        shapes.push_back(temp_drawing_shape);
                        update_selection_state(temp_drawing_shape.get());
                    }
                }
                is_drawing = false;
                temp_drawing_shape = nullptr;
                canvas->invalidate_layout();
            }
        } 
        else if (active_tool == PaintTool::DrawPolygon) {
            if (e.state == PointerState::Pressed) {
                // Add vertex in screen coordinates for easy preview lines
                temp_poly_points.emplace_back(e.x, e.y);

                std::vector<Point> preview_pts = temp_poly_points;
                // Add dummy end point that follows cursor
                preview_pts.emplace_back(e.x, e.y);

                if (!temp_poly_preview) {
                    temp_poly_preview = std::make_shared<PolygonPrimitive>(
                        preview_pts,
                        Color{current_fill_color.r, current_fill_color.g, current_fill_color.b, 80},
                        current_stroke_color,
                        current_stroke_thickness
                    );
                    canvas->add_child(temp_poly_preview);
                } else {
                    temp_poly_preview->set_points(preview_pts);
                }
                
                update_status();
                canvas->invalidate_layout();
            } 
            else if (e.state == PointerState::Moved && !temp_poly_points.empty()) {
                if (temp_poly_preview) {
                    std::vector<Point> preview_pts = temp_poly_points;
                    preview_pts.emplace_back(e.x, e.y);
                    temp_poly_preview->set_points(preview_pts);
                    canvas->invalidate_layout();
                }
            }
        }
    };

    // --- Key Events listener for Enter key to finish polygon ---
    // Create a custom controller to intercept Enter keys
    class PaintController : public gooey::mvvmc::IController {
    public:
        PaintController(std::function<void()> finish_cb) : finish_cb_(std::move(std::move(finish_cb))) {}
        void process_events() override {
            // Check for Enter key press to finalize polygon
            auto& input = gooey::Application::get_instance()->get_input_manager();
            for (const auto& ke : input.get_key_events()) {
                if (ke.state == KeyState::Pressed && (ke.key_code == 13 || ke.key_code == 36 || ke.key_code == 66)) { // Enter key codes (depends on backend, x11, wayland etc)
                    finish_cb_();
                }
            }
        }
    private:
        std::function<void()> finish_cb_;
    };
    app.set_controller(std::make_unique<PaintController>(finish_polygon));

    // Provide the UI to the engine
    app.set_root_view(std::move(root_row));
    app.set_clear_color(Color{30, 30, 35});

    // Run application
    app.run();

    return 0;
}
