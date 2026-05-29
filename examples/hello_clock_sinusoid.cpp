#include <iostream>
#include <memory>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include "ooey/ooey.hpp"
#include "gooey/application.hpp"
#include "ooey/platform.hpp"
#include "gooey/mvvmc/view.hpp"
#include "gooey/controls/label.hpp"
#include "ooey/renderer/primitives/line_primitive.hpp"
#include "ooey/renderer/primitives/circle_primitive.hpp"
#include "ooey/renderer/primitives/rounded_rect_primitive.hpp"
#include "ooey/renderer/primitives/sinusoid_primitive.hpp"
#include "gooey/mvvmc/property.hpp"

// ---------------------------------------------------------
// 1. The ViewModel (Logic & State)
// ---------------------------------------------------------
class ClockSinusoidViewModel {
public:
    // Properties observed by the View
    gooey::Property<float> hour_angle{0.0f};
    gooey::Property<float> minute_angle{0.0f};
    gooey::Property<float> second_angle{0.0f};
    gooey::Property<std::string> digital_time{"00:00:00"};
    gooey::Property<float> sinusoid_phase{0.0f};

    void update(float dt) {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm* parts = std::localtime(&now_c);

        auto duration = now.time_since_epoch();
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

        float sec = static_cast<float>(parts->tm_sec) + static_cast<float>(milliseconds) / 1000.0f;
        float min = static_cast<float>(parts->tm_min) + sec / 60.0f;
        float hour = static_cast<float>(parts->tm_hour % 12) + min / 60.0f;

        second_angle.set(sec * (2.0f * M_PI / 60.0f));
        minute_angle.set(min * (2.0f * M_PI / 60.0f));
        hour_angle.set(hour * (2.0f * M_PI / 12.0f));

        std::stringstream ss;
        ss << std::setw(2) << std::setfill('0') << parts->tm_hour << ":"
           << std::setw(2) << std::setfill('0') << parts->tm_min << ":"
           << std::setw(2) << std::setfill('0') << parts->tm_sec;
        digital_time.set(ss.str());

        // Increment phase for scrolling sinusoid wave
        sinusoid_phase.set(sinusoid_phase.get() + dt * 3.0f);
    }
};

// ---------------------------------------------------------
// 2. The View (UI Construction & MVVM Bindings)
// ---------------------------------------------------------
class ClockSinusoidView : public gooey::View {
public:
    ClockSinusoidView(std::shared_ptr<ClockSinusoidViewModel> view_model)
        : view_model_(std::move(view_model)) {

        // --- Layout Design Elements ---

        // A card-like container for the clock and wave
        auto card = std::make_shared<ooey::RoundedRectPrimitive>(
            ooey::Rect{50, 50, 700, 480},
            16,
            ooey::Color{28, 28, 30},
            ooey::Color{50, 50, 55},
            2.0f
        );
        add_child(std::move(card));

        // --- Clock Widget Layout ---
        float cx = 400.0f;
        float cy = 200.0f;
        float radius = 90.0f;

        // 1. Clock Face Outer Frame
        auto clock_face = std::make_shared<ooey::CirclePrimitive>(
            ooey::Point{static_cast<int>(cx), static_cast<int>(cy)},
            static_cast<int>(radius),
            ooey::Color{20, 20, 22},
            ooey::Color{100, 100, 110},
            3.0f
        );
        add_child(std::move(clock_face));

        // 2. Center Dial Point
        auto center_dial = std::make_shared<ooey::CirclePrimitive>(
            ooey::Point{static_cast<int>(cx), static_cast<int>(cy)},
            5,
            ooey::Color{0, 120, 215},
            ooey::Color{0, 0, 0, 0},
            0.0f
        );

        // 3. Hands
        float len_h = radius * 0.5f;
        float len_m = radius * 0.75f;
        float len_s = radius * 0.85f;

        auto hour_hand = std::make_shared<ooey::LinePrimitive>(
            ooey::Point{static_cast<int>(cx), static_cast<int>(cy)},
            ooey::Point{static_cast<int>(cx), static_cast<int>(cy - len_h)},
            ooey::Color{220, 220, 220},
            4.0f
        );

        auto minute_hand = std::make_shared<ooey::LinePrimitive>(
            ooey::Point{static_cast<int>(cx), static_cast<int>(cy)},
            ooey::Point{static_cast<int>(cx), static_cast<int>(cy - len_m)},
            ooey::Color{160, 160, 160},
            2.5f
        );

        auto second_hand = std::make_shared<ooey::LinePrimitive>(
            ooey::Point{static_cast<int>(cx), static_cast<int>(cy)},
            ooey::Point{static_cast<int>(cx), static_cast<int>(cy - len_s)},
            ooey::Color{255, 80, 80},
            1.2f
        );

        // --- Data Binding: Clock State -> Hands Position ---
        bind(view_model_->hour_angle, [hour_hand, cx, cy, len_h](float angle) {
            int ex = static_cast<int>(cx + len_h * std::sin(angle));
            int ey = static_cast<int>(cy - len_h * std::cos(angle));
            hour_hand->set_end({ex, ey});
        });

        bind(view_model_->minute_angle, [minute_hand, cx, cy, len_m](float angle) {
            int ex = static_cast<int>(cx + len_m * std::sin(angle));
            int ey = static_cast<int>(cy - len_m * std::cos(angle));
            minute_hand->set_end({ex, ey});
        });

        bind(view_model_->second_angle, [second_hand, cx, cy, len_s](float angle) {
            int ex = static_cast<int>(cx + len_s * std::sin(angle));
            int ey = static_cast<int>(cy - len_s * std::cos(angle));
            second_hand->set_end({ex, ey});
        });

        // Add clock layers
        add_child(hour_hand);
        add_child(minute_hand);
        add_child(second_hand);
        add_child(std::move(center_dial));

        // 4. Digital Time Display Label
        auto digital_label = std::make_shared<gooey::Label>(
            "00:00:00",
            ooey::Font{"sans-serif", 20, ooey::FontWeight::Bold},
            ooey::Point{350, 310},
            ooey::Color{255, 255, 255}
        );

        bind(view_model_->digital_time, [digital_label](const std::string& time_str) {
            digital_label->set_text(time_str);
            // Center text on screen dynamically
            int text_width = 16 * 5; // approx width estimation
            digital_label->set_position({400 - text_width / 2, 310});
        });
        add_child(digital_label);

        // --- Sinusoid Wave Widget Layout ---
        auto sinusoid = std::make_shared<ooey::SinusoidPrimitive>(
            ooey::Point{100, 420},          // Start
            ooey::Point{700, 420},          // End
            30.0f,                          // Amplitude
            2.5f,                           // Frequency
            0.0f,                           // Phase
            ooey::Color{0, 120, 215},       // Color
            3.0f                            // Thickness
        );

        // --- Data Binding: Sinusoid State -> Phase Rotation ---
        bind(view_model_->sinusoid_phase, [sinusoid](float phase) {
            sinusoid->set_phase(phase);
        });
        add_child(std::move(sinusoid));
    }

private:
    std::shared_ptr<ClockSinusoidViewModel> view_model_;
};

// ---------------------------------------------------------
// 3. Application Bootstrapper
// ---------------------------------------------------------
int main() {
    std::cout << "Starting OOEY Sinusoid & Clock MVVM Demo...\n";

    gooey::Application app;

    auto backend = ooey::create_default_window_backend();
    if (!backend || !backend->create({800, 600}, "OOEY Sinusoid & Clock MVVM")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }
    app.set_window_backend(std::move(backend));

    auto view_model = std::make_shared<ClockSinusoidViewModel>();
    auto root_view = std::make_shared<ClockSinusoidView>(view_model);

    app.set_root_view(std::move(root_view));
    app.set_clear_color(ooey::Color{15, 15, 17});

    // Time ticker running update loop
    auto last_tick = std::chrono::high_resolution_clock::now();
    app.set_before_render_callback([view_model, &last_tick](ooey::IRenderTarget*) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - last_tick).count();
        last_tick = now;
        view_model->update(dt);
    });

    app.run();

    return 0;
}
