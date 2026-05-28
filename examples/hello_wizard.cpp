#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <cmath>
#include "ooey/ooey.hpp"
#include "ooey/application.hpp"
#include "ooey/platform/platform.hpp"
#include "ooey/mvvmc/view.hpp"
#include "ooey/controls/button.hpp"
#include "ooey/controls/label.hpp"
#include "ooey/controls/list_control.hpp"
#include "ooey/renderer/primitives/line_primitive.hpp"
#include "ooey/renderer/primitives/circle_primitive.hpp"
#include "ooey/renderer/primitives/rounded_rect_primitive.hpp"
#include "ooey/renderer/primitives/sinusoid_primitive.hpp"
#include "ooey/mvvmc/navigation_coordinator.hpp"

// Forward declarations of ViewModels
class Page1ViewModel;
class Page2ViewModel;
class Page3ViewModel;
class Page4ViewModel;
class Page5ViewModel;

// ---------------------------------------------------------
// Page 1: Welcome & Start Button
// ---------------------------------------------------------
class Page1ViewModel : public ooey::PageViewModelBase {
public:
    explicit Page1ViewModel(std::shared_ptr<ooey::NavigationCoordinator> coordinator)
        : coordinator_(coordinator) {}

    std::string get_title() const override { return "Page 1: Welcome"; }

    void on_start_clicked();

private:
    std::shared_ptr<ooey::NavigationCoordinator> coordinator_;
};

// ---------------------------------------------------------
// Page 2: Animal List
// ---------------------------------------------------------
class Page2ViewModel : public ooey::PageViewModelBase {
public:
    explicit Page2ViewModel(std::shared_ptr<ooey::NavigationCoordinator> coordinator)
        : coordinator_(coordinator) {
        animals_ = {"Dog", "Cat", "Elephant", "Tiger", "Lion", "Zebra", "Giraffe"};
    }

    std::string get_title() const override { return "Page 2: Animal Selection"; }

    const std::vector<std::string>& get_animals() const { return animals_; }

    ooey::Property<int> selected_animal_index{-1};

private:
    std::shared_ptr<ooey::NavigationCoordinator> coordinator_;
    std::vector<std::string> animals_;
};

// ---------------------------------------------------------
// Page 3: Fancy Clock & Branch Buttons
// ---------------------------------------------------------
class Page3ViewModel : public ooey::PageViewModelBase {
public:
    explicit Page3ViewModel(std::shared_ptr<ooey::NavigationCoordinator> coordinator)
        : coordinator_(coordinator) {}

    std::string get_title() const override { return "Page 3: Fancy Clock"; }

    ooey::Property<float> hour_angle{0.0f};
    ooey::Property<float> minute_angle{0.0f};
    ooey::Property<float> second_angle{0.0f};
    ooey::Property<std::string> digital_time{"00:00:00"};

    void update(float dt) override {
        auto now = std::chrono::system_clock::now();
        auto time_c = std::chrono::system_clock::to_time_t(now);
        struct tm parts{};
#if defined(_WIN32)
        localtime_s(&parts, &time_c);
#else
        localtime_r(&time_c, &parts);
#endif
        int hour = parts.tm_hour;
        int min = parts.tm_min;
        int sec = parts.tm_sec;

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        float smooth_sec = static_cast<float>(sec) + static_cast<float>(ms.count()) / 1000.0f;
        float smooth_min = static_cast<float>(min) + smooth_sec / 60.0f;
        float smooth_hour = static_cast<float>(hour % 12) + smooth_min / 60.0f;

        constexpr float PI = 3.14159265f;
        second_angle.set(smooth_sec * (2.0f * PI / 60.0f));
        minute_angle.set(smooth_min * (2.0f * PI / 60.0f));
        hour_angle.set(smooth_hour * (2.0f * PI / 12.0f));

        char format_buf[32];
        std::snprintf(format_buf, sizeof(format_buf), "%02d:%02d:%02d", hour, min, sec);
        digital_time.set(format_buf);
    }

    void on_check_this_out_clicked();
    void on_continue_clicked();

private:
    std::shared_ptr<ooey::NavigationCoordinator> coordinator_;
};

// ---------------------------------------------------------
// Page 4: Cool Page (Sinusoid, back only)
// ---------------------------------------------------------
class Page4ViewModel : public ooey::PageViewModelBase {
public:
    explicit Page4ViewModel(std::shared_ptr<ooey::NavigationCoordinator> coordinator)
        : coordinator_(coordinator) {}

    std::string get_title() const override { return "Page 4: Something Cool"; }

    ooey::Property<float> sinusoid_phase{0.0f};

    void update(float dt) override {
        float next_phase = sinusoid_phase.get() + dt * 3.0f;
        sinusoid_phase.set(next_phase);
    }

    void on_back_clicked() {
        coordinator_->go_back();
    }

private:
    std::shared_ptr<ooey::NavigationCoordinator> coordinator_;
};

// ---------------------------------------------------------
// Page 5: End Screen (Fading Text & Exit)
// ---------------------------------------------------------
class Page5ViewModel : public ooey::PageViewModelBase {
public:
    explicit Page5ViewModel(std::shared_ptr<ooey::NavigationCoordinator> coordinator)
        : coordinator_(coordinator) {}

    std::string get_title() const override { return "Page 5: Finished"; }

    ooey::Property<ooey::Color> fading_color{ooey::Color{255, 255, 255}};
    std::function<void()> on_exit_requested;

    void update(float dt) override {
        time_elapsed_ += dt;
        // Smoothly cycle R, G, B colors using HSL-like phase shifting
        float r_val = (std::sin(time_elapsed_ * 2.0f) + 1.0f) * 127.5f;
        float g_val = (std::sin(time_elapsed_ * 2.0f + 2.094395f) + 1.0f) * 127.5f;
        float b_val = (std::sin(time_elapsed_ * 2.0f + 4.188790f) + 1.0f) * 127.5f;

        fading_color.set(ooey::Color{
            static_cast<uint8_t>(r_val),
            static_cast<uint8_t>(g_val),
            static_cast<uint8_t>(b_val)
        });
    }

    void on_exit_clicked() {
        if (on_exit_requested) {
            on_exit_requested();
        }
    }

private:
    std::shared_ptr<ooey::NavigationCoordinator> coordinator_;
    float time_elapsed_{0.0f};
};

// Out-of-line implementations to resolve circular dependency
void Page1ViewModel::on_start_clicked() {
    coordinator_->navigate_to(std::make_shared<Page2ViewModel>(coordinator_));
}

void Page3ViewModel::on_check_this_out_clicked() {
    coordinator_->navigate_to(std::make_shared<Page4ViewModel>(coordinator_));
}

void Page3ViewModel::on_continue_clicked() {
    coordinator_->navigate_to(std::make_shared<Page5ViewModel>(coordinator_));
}

// ---------------------------------------------------------
// Page Views
// ---------------------------------------------------------

class Page1View : public ooey::View {
public:
    explicit Page1View(std::shared_ptr<Page1ViewModel> vm) : vm_(vm) {
        auto msg = std::make_shared<ooey::Label>(
            "Welcome to the MVVMC Wizard!",
            ooey::Font{"sans-serif", 20, ooey::FontWeight::Bold},
            ooey::Point{100, 180},
            ooey::Color{240, 240, 240}
        );
        add_child(std::move(msg));

        auto start_btn = std::make_shared<ooey::Button>(
            ooey::Rect{300, 260, 200, 40},
            ooey::Color{0, 120, 215},
            ooey::Color{0, 0, 0, 0},
            0.0f,
            6,
            "Start Wizard",
            ooey::Color{255, 255, 255}
        );
        start_btn->on_click = [vm]() {
            vm->on_start_clicked();
        };
        add_child(std::move(start_btn));
    }
private:
    std::shared_ptr<Page1ViewModel> vm_;
};

class Page2View : public ooey::View {
public:
    explicit Page2View(std::shared_ptr<Page2ViewModel> vm) : vm_(vm) {
        auto msg = std::make_shared<ooey::Label>(
            "Please select an animal from the list:",
            ooey::Font{"sans-serif", 16},
            ooey::Point{100, 140},
            ooey::Color{200, 200, 200}
        );
        add_child(std::move(msg));

        auto list = std::make_shared<ooey::ListControl>(
            ooey::Rect{100, 180, 350, 250},
            50,
            ooey::Font{"sans-serif", 16},
            ooey::Color{200, 200, 200},
            ooey::Color{45, 45, 50},
            ooey::Color{0, 120, 215},
            ooey::Color{255, 255, 255}
        );
        list->set_items(vm->get_animals());
        
        auto selection_info = std::make_shared<ooey::Label>(
            "Selected: None",
            ooey::Font{"sans-serif", 16},
            ooey::Point{480, 250},
            ooey::Color{0, 200, 100}
        );

        auto active_vm = vm_;
        list->on_selected_changed = [active_vm, selection_info, vm](int index) {
            active_vm->selected_animal_index.set(index);
            selection_info->set_text("Selected: " + vm->get_animals()[index]);
        };

        add_child(list);
        add_child(selection_info);
    }
private:
    std::shared_ptr<Page2ViewModel> vm_;
};

class Page3View : public ooey::View {
public:
    explicit Page3View(std::shared_ptr<Page3ViewModel> vm) : vm_(vm) {
        float cx = 300.0f;
        float cy = 250.0f;
        float radius = 70.0f;

        // Face outline
        auto clock_face = std::make_shared<ooey::CirclePrimitive>(
            ooey::Point{static_cast<int>(cx), static_cast<int>(cy)},
            static_cast<int>(radius),
            ooey::Color{20, 20, 22},
            ooey::Color{100, 100, 110},
            3.0f
        );
        add_child(std::move(clock_face));

        // Dial
        auto center_dial = std::make_shared<ooey::CirclePrimitive>(
            ooey::Point{static_cast<int>(cx), static_cast<int>(cy)},
            5,
            ooey::Color{0, 120, 215},
            ooey::Color{0, 0, 0, 0},
            0.0f
        );

        // Hands
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

        bind(vm->hour_angle, [hour_hand, cx, cy, len_h](float angle) {
            int ex = static_cast<int>(cx + len_h * std::sin(angle));
            int ey = static_cast<int>(cy - len_h * std::cos(angle));
            hour_hand->set_end({ex, ey});
        });

        bind(vm->minute_angle, [minute_hand, cx, cy, len_m](float angle) {
            int ex = static_cast<int>(cx + len_m * std::sin(angle));
            int ey = static_cast<int>(cy - len_m * std::cos(angle));
            minute_hand->set_end({ex, ey});
        });

        bind(vm->second_angle, [second_hand, cx, cy, len_s](float angle) {
            int ex = static_cast<int>(cx + len_s * std::sin(angle));
            int ey = static_cast<int>(cy - len_s * std::cos(angle));
            second_hand->set_end({ex, ey});
        });

        add_child(hour_hand);
        add_child(minute_hand);
        add_child(second_hand);
        add_child(std::move(center_dial));

        auto digital_label = std::make_shared<ooey::Label>(
            "00:00:00",
            ooey::Font{"sans-serif", 20, ooey::FontWeight::Bold},
            ooey::Point{250, 350},
            ooey::Color{255, 255, 255}
        );
        bind(vm->digital_time, [digital_label, cx](const std::string& time_str) {
            digital_label->set_text(time_str);
            int text_width = 16 * 5;
            digital_label->set_position({static_cast<int>(cx) - text_width / 2, 350});
        });
        add_child(digital_label);

        // Buttons
        auto check_out_btn = std::make_shared<ooey::Button>(
            ooey::Rect{100, 440, 220, 40},
            ooey::Color{45, 45, 50},
            ooey::Color{100, 100, 110},
            1.5f,
            6,
            "Check this out",
            ooey::Color{240, 240, 240}
        );
        check_out_btn->on_click = [vm]() {
            vm->on_check_this_out_clicked();
        };
        add_child(std::move(check_out_btn));

        auto continue_btn = std::make_shared<ooey::Button>(
            ooey::Rect{480, 440, 220, 40},
            ooey::Color{0, 120, 215},
            ooey::Color{0, 0, 0, 0},
            0.0f,
            6,
            "Continue",
            ooey::Color{255, 255, 255}
        );
        continue_btn->on_click = [vm]() {
            vm->on_continue_clicked();
        };
        add_child(std::move(continue_btn));
    }
private:
    std::shared_ptr<Page3ViewModel> vm_;
};

class Page4View : public ooey::View {
public:
    explicit Page4View(std::shared_ptr<Page4ViewModel> vm) : vm_(vm) {
        auto label = std::make_shared<ooey::Label>(
            "Page 4: Scrolling Sinusoid!",
            ooey::Font{"sans-serif", 18, ooey::FontWeight::Bold},
            ooey::Point{100, 150},
            ooey::Color{240, 240, 240}
        );
        add_child(std::move(label));

        auto sinusoid = std::make_shared<ooey::SinusoidPrimitive>(
            ooey::Point{100, 280},
            ooey::Point{700, 280},
            40.0f,
            3.0f,
            0.0f,
            ooey::Color{0, 120, 215},
            3.0f
        );
        bind(vm->sinusoid_phase, [sinusoid](float phase) {
            sinusoid->set_phase(phase);
        });
        add_child(sinusoid);

        auto back_btn = std::make_shared<ooey::Button>(
            ooey::Rect{300, 420, 200, 40},
            ooey::Color{45, 45, 50},
            ooey::Color{100, 100, 110},
            1.5f,
            6,
            "Go Back",
            ooey::Color{240, 240, 240}
        );
        back_btn->on_click = [vm]() {
            vm->on_back_clicked();
        };
        add_child(std::move(back_btn));
    }
private:
    std::shared_ptr<Page4ViewModel> vm_;
};

class Page5View : public ooey::View {
public:
    explicit Page5View(std::shared_ptr<Page5ViewModel> vm) : vm_(vm) {
        auto end_label = std::make_shared<ooey::Label>(
            "The End",
            ooey::Font{"sans-serif", 48, ooey::FontWeight::Bold},
            ooey::Point{280, 220},
            ooey::Color{255, 255, 255}
        );
        bind(vm->fading_color, [end_label](ooey::Color c) {
            end_label->set_color(c);
        });
        add_child(end_label);

        auto exit_btn = std::make_shared<ooey::Button>(
            ooey::Rect{300, 400, 200, 40},
            ooey::Color{255, 80, 80},
            ooey::Color{0, 0, 0, 0},
            0.0f,
            6,
            "Exit",
            ooey::Color{255, 255, 255}
        );
        exit_btn->on_click = [vm]() {
            vm->on_exit_clicked();
        };
        add_child(std::move(exit_btn));
    }
private:
    std::shared_ptr<Page5ViewModel> vm_;
};

// ---------------------------------------------------------
// Navigation Shell (Root View)
// ---------------------------------------------------------

class NavigationShellView : public ooey::View {
public:
    explicit NavigationShellView(std::shared_ptr<ooey::NavigationCoordinator> coordinator)
        : coordinator_(coordinator) {
        
        // Static frame card background
        frame_ = std::make_shared<ooey::RoundedRectPrimitive>(
            ooey::Rect{50, 50, 700, 480},
            16,
            ooey::Color{30, 30, 35},
            ooey::Color{60, 60, 70},
            2.0f
        );
        add_child(frame_);

        // Top navigation bar buttons
        back_btn_ = std::make_shared<ooey::Button>(
            ooey::Rect{70, 60, 100, 36},
            ooey::Color{45, 45, 50},
            ooey::Color{100, 100, 110},
            1.5f,
            6,
            "< Back",
            ooey::Color{240, 240, 240}
        );
        
        forward_btn_ = std::make_shared<ooey::Button>(
            ooey::Rect{180, 60, 100, 36},
            ooey::Color{45, 45, 50},
            ooey::Color{100, 100, 110},
            1.5f,
            6,
            "Forward >",
            ooey::Color{240, 240, 240}
        );

        title_label_ = std::make_shared<ooey::Label>(
            "Title",
            ooey::Font{"sans-serif", 18, ooey::FontWeight::Bold},
            ooey::Point{320, 68},
            ooey::Color{255, 255, 255}
        );

        add_child(back_btn_);
        add_child(forward_btn_);
        add_child(title_label_);

        // Page Content container
        page_container_ = std::make_shared<ooey::View>();
        add_child(page_container_);

        // Set commands
        back_btn_->on_click = [coordinator]() {
            if (coordinator->can_go_back.get()) {
                coordinator->go_back();
            }
        };

        // Forward button command triggers custom wizard logic or forward history navigation
        forward_btn_->on_click = [coordinator, this]() {
            if (coordinator->can_go_forward.get()) {
                coordinator->go_forward();
            } else {
                // Predefined sequential page wizard flow
                auto cur = coordinator->current_viewmodel.get();
                if (std::dynamic_pointer_cast<Page1ViewModel>(cur)) {
                    coordinator->navigate_to(std::make_shared<Page2ViewModel>(coordinator));
                } else if (std::dynamic_pointer_cast<Page2ViewModel>(cur)) {
                    coordinator->navigate_to(std::make_shared<Page3ViewModel>(coordinator));
                } else if (std::dynamic_pointer_cast<Page3ViewModel>(cur)) {
                    coordinator->navigate_to(std::make_shared<Page5ViewModel>(coordinator));
                }
            }
        };

        // Data bindings for enabled/disabled button styling
        bind(coordinator_->can_go_back, [this](bool can) {
            if (can) {
                back_btn_->set_fill_color(ooey::Color{45, 45, 50});
                back_btn_->set_stroke_color(ooey::Color{100, 100, 110});
                back_btn_->set_label_text("< Back");
            } else {
                back_btn_->set_fill_color(ooey::Color{20, 20, 22});
                back_btn_->set_stroke_color(ooey::Color{40, 40, 44});
                back_btn_->set_label_text("");
            }
        });

        // The top forward button acts as "Next >" if we can proceed in the wizard
        bind(coordinator_->current_viewmodel, [this](std::shared_ptr<ooey::PageViewModelBase> vm) {
            rebuild_forward_btn(vm);
        });
        bind(coordinator_->can_go_forward, [this](bool /*can*/) {
            rebuild_forward_btn(coordinator_->current_viewmodel.get());
        });

        // Recreate the active view when the viewmodel changes
        bind(coordinator_->current_viewmodel, [this](std::shared_ptr<ooey::PageViewModelBase> vm) {
            recreate_page_view(vm);
        });
    }

private:
    void rebuild_forward_btn(std::shared_ptr<ooey::PageViewModelBase> vm) {
        if (!vm) return;
        bool has_forward_history = coordinator_->can_go_forward.get();
        bool is_wizard_page = std::dynamic_pointer_cast<Page1ViewModel>(vm) ||
                             std::dynamic_pointer_cast<Page2ViewModel>(vm) ||
                             std::dynamic_pointer_cast<Page3ViewModel>(vm);

        if (has_forward_history || is_wizard_page) {
            forward_btn_->set_fill_color(ooey::Color{45, 45, 50});
            forward_btn_->set_stroke_color(ooey::Color{100, 100, 110});
            forward_btn_->set_label_text("Forward >");
        } else {
            forward_btn_->set_fill_color(ooey::Color{20, 20, 22});
            forward_btn_->set_stroke_color(ooey::Color{40, 40, 44});
            forward_btn_->set_label_text("");
        }
    }

    void recreate_page_view(std::shared_ptr<ooey::PageViewModelBase> vm) {
        page_container_->clear_children();
        if (!vm) {
            return;
        }

        title_label_->set_text(vm->get_title());

        std::shared_ptr<ooey::View> page_view = nullptr;
        if (auto p1 = std::dynamic_pointer_cast<Page1ViewModel>(vm)) {
            page_view = std::make_shared<Page1View>(p1);
        } else if (auto p2 = std::dynamic_pointer_cast<Page2ViewModel>(vm)) {
            page_view = std::make_shared<Page2View>(p2);
        } else if (auto p3 = std::dynamic_pointer_cast<Page3ViewModel>(vm)) {
            page_view = std::make_shared<Page3View>(p3);
        } else if (auto p4 = std::dynamic_pointer_cast<Page4ViewModel>(vm)) {
            page_view = std::make_shared<Page4View>(p4);
        } else if (auto p5 = std::dynamic_pointer_cast<Page5ViewModel>(vm)) {
            page_view = std::make_shared<Page5View>(p5);
        }

        if (page_view) {
            page_container_->add_child(std::move(page_view));
        }
    }

    std::shared_ptr<ooey::NavigationCoordinator> coordinator_;
    std::shared_ptr<ooey::RoundedRectPrimitive> frame_;
    std::shared_ptr<ooey::Button> back_btn_;
    std::shared_ptr<ooey::Button> forward_btn_;
    std::shared_ptr<ooey::Label> title_label_;
    std::shared_ptr<ooey::View> page_container_;
};

// ---------------------------------------------------------
// Main Bootstrapper
// ---------------------------------------------------------

int main() {
    std::cout << "Starting OOEY Navigation Wizard Demo...\n";

    ooey::Application app;

    auto backend = ooey::create_default_window_backend();
    if (!backend || !backend->create({800, 600}, "OOEY Page Navigation Wizard")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }
    app.set_window_backend(std::move(backend));

    auto coordinator = std::make_shared<ooey::NavigationCoordinator>();
    auto root_view = std::make_shared<NavigationShellView>(coordinator);

    // Bind exiting app trigger from ViewModel to app quit
    auto page5_on_exit = [&app]() {
        app.quit();
    };

    // Navigate to Page 1 initially
    auto initial_vm = std::make_shared<Page1ViewModel>(coordinator);
    coordinator->navigate_to(initial_vm);

    // Setup color cycle exit hook on Page 5 Viewmodel when created
    coordinator->current_viewmodel.subscribe([page5_on_exit](std::shared_ptr<ooey::PageViewModelBase> vm) {
        if (auto p5 = std::dynamic_pointer_cast<Page5ViewModel>(vm)) {
            p5->on_exit_requested = page5_on_exit;
        }
    });

    app.set_root_view(std::move(root_view));
    app.set_clear_color(ooey::Color{15, 15, 17});

    // Time loop to drive active ViewModel updates (e.g. clock hands, sinusoid scroll, color cycle)
    auto last_tick = std::chrono::high_resolution_clock::now();
    app.set_before_render_callback([coordinator, &last_tick](ooey::IRenderTarget*) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - last_tick).count();
        last_tick = now;

        if (auto active_vm = coordinator->current_viewmodel.get()) {
            active_vm->update(dt);
        }
    });

    app.run();

    return 0;
}
