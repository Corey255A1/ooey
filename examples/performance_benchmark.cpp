#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <cmath>
#include <fstream>
#include <algorithm>

#include "ooey/ooey.hpp"
#include "gooey/application.hpp"
#include "ooey/platform.hpp"
#include "gooey/mvvmc/view.hpp"
#include "gooey/controls/column.hpp"
#include "gooey/controls/row.hpp"
#include "gooey/controls/grid.hpp"
#include "gooey/controls/button.hpp"
#include "gooey/controls/label.hpp"
#include "gooey/controls/image_control.hpp"
#include "gooey/controls/list_control.hpp"
#include "ooey/renderer/image.hpp"
#include "ooey/renderer/primitives/line_primitive.hpp"
#include "ooey/renderer/primitives/circle_primitive.hpp"
#include "ooey/renderer/primitives/curve_primitive.hpp"
#include "ooey/renderer/primitives/rect_primitive.hpp"
#include "ooey/renderer/primitives/rounded_rect_primitive.hpp"

#ifdef OOEY_BUILD_WAYLAND
#include "ooey/platform/wayland/window_backend.hpp"
#include "ooey/platform/wayland/egl_window_backend.hpp"
#include "ooey/platform/wayland/vulkan_window_backend.hpp"
#endif

#ifdef OOEY_BUILD_X11
#include "ooey/platform/x11/window_backend.hpp"
#endif

using namespace ooey;
using namespace gooey;
using namespace gooey::controls;

// Helper to write a 64x64 BMP file with a gradient pattern to test image rendering
static void create_benchmark_bmp_pattern(const std::string& path) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return;

    #pragma pack(push, 1)
    struct BMPFileHeader {
        uint16_t file_type{0x4D42}; // BM
        uint32_t file_size{14 + 40 + 64 * 64 * 4};
        uint16_t reserved1{0};
        uint16_t reserved2{0};
        uint32_t offset_data{14 + 40};
    };

    struct BMPInfoHeader {
        uint32_t size{40};
        int32_t width{64};
        int32_t height{64};
        uint16_t planes{1};
        uint16_t bit_count{32};
        uint32_t compression{0};
        uint32_t size_image{64 * 64 * 4};
        int32_t x_pixels_per_meter{0};
        int32_t y_pixels_per_meter{0};
        uint32_t colors_used{0};
        uint32_t colors_important{0};
    };
    #pragma pack(pop)

    BMPFileHeader fh;
    BMPInfoHeader ih;

    f.write(reinterpret_cast<const char*>(&fh), sizeof(fh));
    f.write(reinterpret_cast<const char*>(&ih), sizeof(ih));

    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 64; ++x) {
            uint8_t r = static_cast<uint8_t>(x * 4);
            uint8_t g = static_cast<uint8_t>(y * 4);
            uint8_t b = 128;
            uint8_t a = static_cast<uint8_t>(128 + y * 2); // Alpha gradient
            // BGRA layout in BMP
            f.put(b);
            f.put(g);
            f.put(r);
            f.put(a);
        }
    }
}

// A view that draws a large number of moving geometric primitives
class BenchmarkGeometryView : public View {
public:
    BenchmarkGeometryView(int count) : count_(count) {}

    void draw(IRenderTarget& target) const override {
        // Draw the base view children first
        View::draw(target);

        // Render dynamic geometry based on our frame count (which advances slightly)
        float t = frame_count_ * 0.05f;
        frame_count_++;

        for (int i = 0; i < count_; ++i) {
            int seed = i * 17;
            float angle = seed + t;
            float r = 50.0f + 30.0f * std::sin(angle * 0.5f);
            float cx = 400.0f + 300.0f * std::cos(angle);
            float cy = 300.0f + 200.0f * std::sin(angle * 1.3f);

            // Alternate between drawing types to exercise the backends' dynamic drawing
            int type = i % 4;
            Color color{
                static_cast<uint8_t>((seed % 128) + 127),
                static_cast<uint8_t>(((seed * 3) % 128) + 127),
                static_cast<uint8_t>(((seed * 7) % 128) + 127),
                180
            };

            if (type == 0) {
                // Circle
                CirclePrimitive circ(Point{static_cast<int>(cx), static_cast<int>(cy)}, static_cast<int>(r), color, Color{255, 255, 255}, 1.0f);
                circ.draw(target);
            } else if (type == 1) {
                // Line
                float x2 = cx + r * std::cos(angle * 2.0f);
                float y2 = cy + r * std::sin(angle * 2.0f);
                LinePrimitive line(Point{static_cast<int>(cx), static_cast<int>(cy)}, Point{static_cast<int>(x2), static_cast<int>(y2)}, color, 2.0f);
                line.draw(target);
            } else if (type == 2) {
                // Rounded Rect
                Rect rect{static_cast<int>(cx - r), static_cast<int>(cy - r), static_cast<int>(r * 2), static_cast<int>(r * 2)};
                RoundedRectPrimitive rr(rect, 8, color, Color{255, 255, 255}, 1.0f);
                rr.draw(target);
            } else {
                // Curve
                Point p0{static_cast<int>(cx - r), static_cast<int>(cy)};
                Point p1{static_cast<int>(cx), static_cast<int>(cy - r * 2)};
                Point p2{static_cast<int>(cx + r), static_cast<int>(cy)};
                CurvePrimitive curve(p0, p1, p2, color, 2.0f);
                curve.draw(target);
            }
        }
    }

private:
    int count_;
    mutable int frame_count_{0};
};

// Create a scenario view
std::shared_ptr<View> create_scenario_view(const std::string& scenario, const std::string& bmp_path) {
    if (scenario == "grid") {
        // Massive button grid scenario
        int rows = 20;
        int cols = 20; // 400 buttons
        auto grid = std::make_shared<Grid>(rows, cols);
        grid->set_width(SizePolicy::MatchParent);
        grid->set_height(SizePolicy::MatchParent);

        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                auto btn = std::make_shared<Button>(Rect{0, 0, 10, 10}, Color{30, 80, static_cast<uint8_t>(40 + r * 10)});
                btn->margin_left = 2;
                btn->margin_right = 2;
                btn->margin_top = 2;
                btn->margin_bottom = 2;
                grid->add_child(btn);
            }
        }
        return grid;
    } else if (scenario == "geometry") {
        // Complex geometry scenario: 2000 dynamic shapes
        auto view = std::make_shared<BenchmarkGeometryView>(2000);
        view->set_width(SizePolicy::MatchParent);
        view->set_height(SizePolicy::MatchParent);
        return view;
    } else if (scenario == "image") {
        // Image scaling scenario: 100 scaled image instances
        auto grid = std::make_shared<Grid>(10, 10);
        grid->set_width(SizePolicy::MatchParent);
        grid->set_height(SizePolicy::MatchParent);

        for (int i = 0; i < 100; ++i) {
            auto img_ctrl = std::make_shared<ImageControl>(Rect{0, 0, 60, 60}, bmp_path);
            img_ctrl->margin_left = 4;
            img_ctrl->margin_right = 4;
            img_ctrl->margin_top = 4;
            img_ctrl->margin_bottom = 4;
            grid->add_child(img_ctrl);
        }
        return grid;
    } else if (scenario == "list") {
        // Giant list control scenario
        auto root = std::make_shared<Column>();
        root->set_width(SizePolicy::MatchParent);
        root->set_height(SizePolicy::MatchParent);

        auto list = std::make_shared<ListControl>(
            Rect{50, 50, 700, 500},
            30, // item height
            Font{"sans-serif", 14},
            Color{255, 255, 255},
            Color{45, 45, 50},
            Color{0, 120, 215},
            Color{255, 255, 255}
        );

        std::vector<std::string> items;
        for (int i = 0; i < 5000; ++i) {
            items.push_back("List item number " + std::to_string(i + 1) + " - Performance benchmarking");
        }
        list->set_items(items);

        // Set up list scrolling animation simulation
        struct ScrollSimulation : public View {
            std::shared_ptr<ListControl> list_ref;
            mutable int frames{0};

            ScrollSimulation(std::shared_ptr<ListControl> l) : list_ref(l) {}
            void draw(IRenderTarget& target) const override {
                frames++;
                // Scroll down 1 item every 2 frames
                list_ref->set_selected_index((frames / 2) % 5000);
                View::draw(target);
            }
        };

        auto sim = std::make_shared<ScrollSimulation>(list);
        root->add_child(list);
        root->add_child(sim);
        return root;
    }

    return nullptr;
}

std::unique_ptr<IWindowBackend> create_backend(const std::string& name) {
#ifdef OOEY_BUILD_WAYLAND
    if (std::getenv("WAYLAND_DISPLAY") != nullptr) {
        if (name == "software") {
            return std::make_unique<wayland::WindowBackend>();
        } else if (name == "opengl") {
            return std::make_unique<wayland::EglWindowBackend>();
        } else if (name == "vulkan") {
            return std::make_unique<wayland::VulkanWindowBackend>();
        }
    }
#endif

#ifdef OOEY_BUILD_X11
    if (std::getenv("DISPLAY") != nullptr) {
        if (name == "opengl") {
            return std::make_unique<x11::WindowBackend>();
        }
    }
#endif

    return nullptr;
}

struct BenchmarkResult {
    std::string backend;
    std::string scenario;
    double avg_frame_time_ms;
    double fps;
    bool success;
};

BenchmarkResult run_single_benchmark(const std::string& backend_name, const std::string& scenario_name, int frames_to_run, const std::string& bmp_path) {
    std::cout << "[BENCHMARK] Running " << backend_name << " backend on scenario '" << scenario_name << "' for " << frames_to_run << " frames...\n";

    auto backend = create_backend(backend_name);
    if (!backend) {
        std::cerr << "[BENCHMARK] Warning: Could not create backend: " << backend_name << "\n";
        return {backend_name, scenario_name, 0.0, 0.0, false};
    }

    try {
        Application app;
        if (!backend->create({800, 600}, "OOEY Performance Benchmark")) {
            std::cerr << "[BENCHMARK] Warning: Failed to create window backend: " << backend_name << "\n";
            return {backend_name, scenario_name, 0.0, 0.0, false};
        }

        app.set_window_backend(std::move(backend));
        app.set_clear_color(Color{25, 25, 28});

        auto root = create_scenario_view(scenario_name, bmp_path);
        if (!root) {
            std::cerr << "[BENCHMARK] Warning: Scenario not found: " << scenario_name << "\n";
            return {backend_name, scenario_name, 0.0, 0.0, false};
        }
        app.set_root_view(std::move(root));

        int frame_count = 0;
        std::vector<double> frame_times;
        auto last_frame_time = std::chrono::high_resolution_clock::now();

        app.set_after_render_callback([&](IRenderTarget*) {
            auto now = std::chrono::high_resolution_clock::now();
            double duration = std::chrono::duration<double, std::milli>(now - last_frame_time).count();
            // Ignore first frame to avoid initialization artifacts
            if (frame_count > 0) {
                frame_times.push_back(duration);
            }
            last_frame_time = now;
            frame_count++;
            if (frame_count >= frames_to_run) {
                app.quit();
            }
        });

        auto start = std::chrono::high_resolution_clock::now();
        app.run();
        auto end = std::chrono::high_resolution_clock::now();

        double total_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        double avg_frame_time = 0.0;
        if (!frame_times.empty()) {
            double sum = 0.0;
            for (double t : frame_times) sum += t;
            avg_frame_time = sum / frame_times.size();
        }
        double fps = frame_count / (total_time_ms / 1000.0);

        std::cout << "[BENCHMARK] Result: " << avg_frame_time << " ms/frame (" << fps << " FPS)\n";
        return {backend_name, scenario_name, avg_frame_time, fps, true};

    } catch (const std::exception& e) {
        std::cerr << "[BENCHMARK] Error: Exception during run: " << e.what() << "\n";
        return {backend_name, scenario_name, 0.0, 0.0, false};
    } catch (...) {
        std::cerr << "[BENCHMARK] Error: Unknown exception during run\n";
        return {backend_name, scenario_name, 0.0, 0.0, false};
    }
}

int main(int argc, char* argv[]) {
    std::string backend_arg = "all";
    std::string scenario_arg = "all";
    int frames_arg = 100;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--backend" && i + 1 < argc) {
            backend_arg = argv[++i];
        } else if (arg == "--scenario" && i + 1 < argc) {
            scenario_arg = argv[++i];
        } else if (arg == "--frames" && i + 1 < argc) {
            frames_arg = std::stoi(argv[++i]);
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n"
                      << "Options:\n"
                      << "  --backend [opengl|vulkan|software|all]  (default: all)\n"
                      << "  --scenario [grid|geometry|image|list|all]  (default: all)\n"
                      << "  --frames <count>                          (default: 100)\n";
            return 0;
        }
    }

    std::string bmp_path = "benchmark_temp_image.bmp";
    create_benchmark_bmp_pattern(bmp_path);

    std::vector<std::string> backends;
    if (backend_arg == "all") {
        backends = {"software", "opengl", "vulkan"};
    } else {
        backends = {backend_arg};
    }

    std::vector<std::string> scenarios;
    if (scenario_arg == "all") {
        scenarios = {"grid", "geometry", "image", "list"};
    } else {
        scenarios = {scenario_arg};
    }

    std::vector<BenchmarkResult> results;
    for (const auto& backend : backends) {
        for (const auto& scenario : scenarios) {
            auto result = run_single_benchmark(backend, scenario, frames_arg, bmp_path);
            results.push_back(result);
        }
    }

    std::remove(bmp_path.c_str());

    // Print final results table
    std::cout << "\n========================================================================\n";
    std::cout << "                      OOEY PERFORMANCE BENCHMARK\n";
    std::cout << "========================================================================\n";
    std::cout << "  Backend    |  Scenario  |  Avg Frame Time (ms)  |  Est. FPS  |  Status\n";
    std::cout << "-------------+------------+-----------------------+------------+--------\n";
    for (const auto& r : results) {
        printf("  %-10s |  %-9s |  ", r.backend.c_str(), r.scenario.c_str());
        if (r.success) {
            printf("%21.2f |  %9.1f |  PASSED\n", r.avg_frame_time_ms, r.fps);
        } else {
            printf("%21s |  %9s |  FAILED/UNSUPPORTED\n", "N/A", "N/A");
        }
    }
    std::cout << "========================================================================\n";

    return 0;
}
