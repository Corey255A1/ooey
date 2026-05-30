#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "ooey/ooey.hpp"
#include "gooey/application.hpp"
#include "ooey/platform.hpp"
#include "gooey/mvvmc/view.hpp"
#include "gooey/mvvmc/theme.hpp"
#include "gooey/controls/column.hpp"
#include "gooey/controls/row.hpp"
#include "gooey/controls/grid.hpp"
#include "gooey/controls/flow_layout.hpp"
#include "gooey/controls/button.hpp"
#include "gooey/controls/label.hpp"
#include "gooey/controls/text_box.hpp"
#include "ooey/renderer/primitives/rect_primitive.hpp"
#include "ooey/renderer/primitives/rounded_rect_primitive.hpp"

using namespace ooey;
using namespace gooey;
using namespace gooey::controls;

// ---------------------------------------------------------
// 1. The ViewModel (Logic & State)
// ---------------------------------------------------------
class LayoutViewModel {
private:
    int theme_index_{0};
    std::shared_ptr<ThemeManager> theme_manager_;

public:
    explicit LayoutViewModel(std::shared_ptr<ThemeManager> theme_manager)
        : theme_manager_(std::move(theme_manager)) {}

    // Properties that the View will observe
    Property<std::string> greeting_text{"Enter your name below to get started!"};
    Property<int> click_count{0};
    Property<std::string> system_status{"CPU Load: 42%\nRAM usage: 5.4 GB / 16 GB"};
    
    // Commands/Methods triggered by the UI
    void set_name(const std::string& name) {
        if (name.empty()) {
            greeting_text.set("Enter your name below to get started!");
        } else {
            greeting_text.set("Hello, " + name + "! Welcome to the OOEY MVVMC Dashboard.");
        }
    }

    void cycle_theme() {
        theme_index_ = (theme_index_ + 1) % 3;
        if (theme_manager_) {
            if (theme_index_ == 0) {
                theme_manager_->set_active_theme("dark");
            } else if (theme_index_ == 1) {
                theme_manager_->set_active_theme("light");
            } else {
                theme_manager_->set_active_theme("cyberpunk");
            }
            auto theme = theme_manager_->active_theme.get();
            std::cout << "ViewModel: Cycled active theme to " << (theme ? theme->name : "null") << "\n";
        }
    }

    void increment_counter() {
        int current = click_count.get();
        click_count.set(current + 1);
        system_status.set("Total Clicks: " + std::to_string(current + 1) + "\nVM uptime is healthy.");
    }
};

// ---------------------------------------------------------
// 2. Custom Card component with a drawn background
// ---------------------------------------------------------
class CardView : public Column {
public:
    explicit CardView(Color bg_color) : bg_color_(bg_color) {
        set_padding(15);
        set_margin(5);
        set_align_self(Align::Stretch);
    }

    void draw(IRenderTarget& target) const override {
        // Draw card background based on active styling parameters
        if (corner_radius_ > 0 || stroke_thickness_ > 0.0f) {
            RoundedRectPrimitive(layout_bounds, corner_radius_, bg_color_, stroke_color_, stroke_thickness_).draw(target);
        } else {
            RectPrimitive(layout_bounds, bg_color_).draw(target);
        }
        // Draw children on top of background
        Column::draw(target);
    }

    void apply_style(const Style& style) override {
        bg_color_ = style.fill_color;
        stroke_color_ = style.stroke_color;
        stroke_thickness_ = style.stroke_thickness;
        corner_radius_ = style.corner_radius;
        Column::apply_style(style);
    }

private:
    Color bg_color_;
    Color stroke_color_{0, 0, 0, 0};
    float stroke_thickness_{0.0f};
    int corner_radius_{0};
};

// ---------------------------------------------------------
// 3. The View (UI Layout Composition & Bindings)
// ---------------------------------------------------------
class LayoutView : public Column {
public:
    explicit LayoutView(std::shared_ptr<LayoutViewModel> view_model) : view_model_(std::move(view_model)) {
        // Root column styling
        set_width(SizePolicy::MatchParent);
        set_height(SizePolicy::MatchParent);
        set_padding(25);

        // Header Section
        auto header = std::make_shared<Column>();
        header->set_width(SizePolicy::MatchParent);
        header->set_height(SizePolicy::WrapContent);
        header->set_margin(0, 0, 0, 15);

        auto title = std::make_shared<Label>(
            "MVVMC Dashboard Demo",
            Font{"sans-serif", 28, FontWeight::Bold},
            Point{0, 0},
            Color{255, 255, 255}
        );
        title->set_margin(0, 0, 0, 5);
        title->set_style_name("title-text");
        header->add_child(title);

        auto subtitle = std::make_shared<Label>(
            "Resize this window to watch the responsive layout adapt under dynamic themes.",
            Font{"sans-serif", 14},
            Point{0, 0},
            Color{150, 150, 160}
        );
        subtitle->set_margin(0, 0, 0, 15);
        subtitle->set_style_name("subtitle-text");
        header->add_child(subtitle);

        add_child(header);

        // Buttons Flow Panel
        auto button_row = std::make_shared<FlowLayout>();
        button_row->set_width(SizePolicy::MatchParent);
        button_row->set_height(SizePolicy::WrapContent);
        button_row->set_margin(0, 0, 0, 20);

        auto cycle_theme_btn = std::make_shared<Button>(
            Rect{0, 0, 160, 40},
            Color{0, 120, 215},
            Color{0, 0, 0, 0},
            0.0f,
            8,
            "Cycle Theme",
            Color{255, 255, 255}
        );
        cycle_theme_btn->set_margin(0, 5, 10, 5);
        cycle_theme_btn->set_style_name("primary-btn");
        button_row->add_child(cycle_theme_btn);

        auto counter_btn = std::make_shared<Button>(
            Rect{0, 0, 160, 40},
            Color{50, 50, 55},
            Color{80, 80, 90},
            1.5f,
            8,
            "Clicks: 0",
            Color{220, 220, 230}
        );
        counter_btn->set_margin(0, 5, 10, 5);
        counter_btn->set_style_name("secondary-btn");
        button_row->add_child(counter_btn);

        add_child(button_row);

        // Grid of Cards
        auto grid = std::make_shared<Grid>(2, 2);
        grid->set_width(SizePolicy::MatchParent);
        grid->set_height(SizePolicy::Fixed, 200.0f);
        grid->set_margin(0, 0, 0, 20);

        auto card1 = std::make_shared<CardView>(Color{35, 35, 40});
        card1->set_style_name("card-bg");
        auto card1_title = std::make_shared<Label>("Theme Status", Font{"sans-serif", 16, FontWeight::Bold}, Point{0, 0}, Color{0, 180, 240});
        card1_title->set_margin(0, 0, 0, 5);
        card1_title->set_style_name("card-title-accent");
        card1->add_child(card1_title);
        auto card1_lbl = std::make_shared<Label>("Dynamic card styling accent", Font{"sans-serif", 12}, Point{0, 0}, Color{180, 180, 190});
        card1_lbl->set_style_name("card-muted-text");
        card1->add_child(card1_lbl);
        grid->add_child(card1);

        auto card2 = std::make_shared<CardView>(Color{35, 35, 40});
        card2->set_style_name("card-bg");
        auto card2_title = std::make_shared<Label>("Console Output", Font{"sans-serif", 16, FontWeight::Bold}, Point{0, 0}, Color{0, 200, 100});
        card2_title->set_margin(0, 0, 0, 5);
        card2_title->set_style_name("card-title-accent-green");
        card2->add_child(card2_title);
        auto card2_lbl = std::make_shared<Label>("CPU Load: 42%\nRAM usage: 5.4 GB / 16 GB", Font{"sans-serif", 12}, Point{0, 0}, Color{180, 180, 190});
        card2_lbl->set_style_name("card-muted-text");
        card2->add_child(card2_lbl);
        grid->add_child(card2);

        auto card3 = std::make_shared<CardView>(Color{35, 35, 40});
        card3->set_style_name("card-bg");
        auto card3_title = std::make_shared<Label>("Interactive Console", Font{"sans-serif", 16, FontWeight::Bold}, Point{0, 0}, Color{235, 160, 0});
        card3_title->set_margin(0, 0, 0, 5);
        card3_title->set_style_name("card-title-accent-orange");
        card3->add_child(card3_title);
        auto card3_lbl = std::make_shared<Label>("Layout positions auto flow", Font{"sans-serif", 12}, Point{0, 0}, Color{180, 180, 190});
        card3_lbl->set_style_name("card-muted-text");
        card3->add_child(card3_lbl);
        grid->add_child(card3);

        auto card4 = std::make_shared<CardView>(Color{35, 35, 40});
        card4->set_style_name("card-bg");
        auto card4_title = std::make_shared<Label>("System Details", Font{"sans-serif", 16, FontWeight::Bold}, Point{0, 0}, Color{180, 100, 240});
        card4_title->set_margin(0, 0, 0, 5);
        card4_title->set_style_name("card-title-accent-purple");
        card4->add_child(card4_title);
        auto card4_lbl = std::make_shared<Label>("Mode: Retained MVVMC\nTheme: Cycle Modern", Font{"sans-serif", 12}, Point{0, 0}, Color{180, 180, 190});
        card4_lbl->set_style_name("card-muted-text");
        card4->add_child(card4_lbl);
        grid->add_child(card4);

        add_child(grid);

        // Footer Input Flow
        auto footer_row = std::make_shared<Row>();
        footer_row->set_width(SizePolicy::MatchParent);
        footer_row->set_height(SizePolicy::WrapContent);
        footer_row->set_margin(0, 0, 0, 10);

        auto name_lbl = std::make_shared<Label>("Enter Name:", Font{"sans-serif", 16}, Point{0, 0}, Color{200, 200, 200});
        name_lbl->set_margin(0, 5, 10, 0);
        name_lbl->set_style_name("default-text");
        footer_row->add_child(name_lbl);

        auto name_box = std::make_shared<TextBox>(
            Rect{0, 0, 250, 36},
            Font{"sans-serif", 16},
            Color{255, 255, 255},
            Color{35, 35, 40}
        );
        name_box->set_style_name("text-input");
        footer_row->add_child(name_box);
        add_child(footer_row);

        auto greeting_lbl = std::make_shared<Label>("", Font{"sans-serif", 16, FontWeight::Bold}, Point{0, 0}, Color{0, 200, 100});
        greeting_lbl->set_width(SizePolicy::MatchParent);
        greeting_lbl->set_margin(0, 5, 0, 15);
        greeting_lbl->set_style_name("card-title-accent-green");
        add_child(greeting_lbl);

        // -- Data Bindings (ViewModel -> UI) --
        bind(view_model_->click_count, [counter_btn](const int& count) {
            counter_btn->set_label_text("Clicks: " + std::to_string(count));
        });

        bind(view_model_->greeting_text, [greeting_lbl](const std::string& text) {
            greeting_lbl->set_text(text);
        });

        bind(view_model_->system_status, [card2_lbl](const std::string& status) {
            card2_lbl->set_text(status);
        });

        // -- Input bindings (UI -> ViewModel) --
        auto vm = view_model_;
        cycle_theme_btn->on_click = [vm]() {
            vm->cycle_theme();
        };

        counter_btn->on_click = [vm]() {
            vm->increment_counter();
        };

        name_box->on_text_changed = [vm](const std::string& name) {
            vm->set_name(name);
        };
    }

private:
    std::shared_ptr<LayoutViewModel> view_model_;
};

// ---------------------------------------------------------
// 4. Main Entry Point (Bootstrapper)
// ---------------------------------------------------------
int main() {
    std::cout << "Starting OOEY MVVMC Responsive Layout Theme Demo...\n";

    Application app;

    auto backend = create_default_window_backend();
    if (!backend || !backend->create({800, 600}, "OOEY MVVMC Layout Dashboard")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }

    app.set_window_backend(std::move(backend));

    // Create the ThemeManager dynamically
    auto theme_manager = std::make_shared<ThemeManager>();

    // Define custom themes and styles completely decouped from framework code:

    // 1. Dark Theme
    auto dark_theme = std::make_shared<Theme>();
    dark_theme->name = "dark";
    dark_theme->set_style("window", Style{Color{20, 20, 25}});
    dark_theme->set_style("title-text", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{0, 180, 240}});
    dark_theme->set_style("subtitle-text", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{150, 150, 160}});
    dark_theme->set_style("primary-btn", Style{Color{0, 120, 215}, Color{0, 0, 0, 0}, 0.0f, Color{255, 255, 255}, 8});
    dark_theme->set_style("secondary-btn", Style{Color{50, 50, 55}, Color{80, 80, 90}, 1.5f, Color{220, 220, 230}, 8});
    dark_theme->set_style("card-bg", Style{Color{35, 35, 40}, Color{0, 0, 0, 0}, 0.0f, Color{0, 0, 0, 0}, 6});
    dark_theme->set_style("card-title-accent", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{0, 180, 240}});
    dark_theme->set_style("card-title-accent-green", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{0, 200, 100}});
    dark_theme->set_style("card-title-accent-orange", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{235, 160, 0}});
    dark_theme->set_style("card-title-accent-purple", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{180, 100, 240}});
    dark_theme->set_style("card-muted-text", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{180, 180, 190}});
    dark_theme->set_style("text-input", Style{Color{35, 35, 40}, Color{80, 80, 90}, 1.5f, Color{255, 255, 255}, 6});
    dark_theme->set_style("default-text", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{200, 200, 200}});
    theme_manager->add_theme("dark", dark_theme);

    // 2. Light Theme
    auto light_theme = std::make_shared<Theme>();
    light_theme->name = "light";
    light_theme->set_style("window", Style{Color{240, 240, 245}});
    light_theme->set_style("title-text", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{0, 90, 180}});
    light_theme->set_style("subtitle-text", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{100, 100, 110}});
    light_theme->set_style("primary-btn", Style{Color{0, 100, 200}, Color{0, 0, 0, 0}, 0.0f, Color{255, 255, 255}, 8});
    light_theme->set_style("secondary-btn", Style{Color{220, 220, 225}, Color{180, 180, 190}, 1.5f, Color{40, 40, 45}, 8});
    light_theme->set_style("card-bg", Style{Color{255, 255, 255}, Color{210, 210, 215}, 1.0f, Color{0, 0, 0, 0}, 6});
    light_theme->set_style("card-title-accent", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{0, 100, 200}});
    light_theme->set_style("card-title-accent-green", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{0, 150, 50}});
    light_theme->set_style("card-title-accent-orange", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{200, 100, 0}});
    light_theme->set_style("card-title-accent-purple", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{120, 40, 180}});
    light_theme->set_style("card-muted-text", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{80, 80, 90}});
    light_theme->set_style("text-input", Style{Color{255, 255, 255}, Color{180, 180, 190}, 1.5f, Color{0, 0, 0}, 6});
    light_theme->set_style("default-text", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{50, 50, 50}});
    theme_manager->add_theme("light", light_theme);

    // 3. Cyberpunk Theme
    auto cyberpunk_theme = std::make_shared<Theme>();
    cyberpunk_theme->name = "cyberpunk";
    cyberpunk_theme->set_style("window", Style{Color{10, 10, 15}});
    cyberpunk_theme->set_style("title-text", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{255, 0, 128}});
    cyberpunk_theme->set_style("subtitle-text", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{0, 255, 255}});
    cyberpunk_theme->set_style("primary-btn", Style{Color{255, 255, 0}, Color{255, 0, 128}, 2.0f, Color{0, 0, 0}, 0});
    cyberpunk_theme->set_style("secondary-btn", Style{Color{0, 0, 0}, Color{0, 255, 255}, 2.0f, Color{0, 255, 255}, 0});
    cyberpunk_theme->set_style("card-bg", Style{Color{20, 20, 30}, Color{255, 0, 128}, 1.5f, Color{0, 0, 0, 0}, 0});
    cyberpunk_theme->set_style("card-title-accent", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{255, 255, 0}});
    cyberpunk_theme->set_style("card-title-accent-green", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{0, 255, 128}});
    cyberpunk_theme->set_style("card-title-accent-orange", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{255, 128, 0}});
    cyberpunk_theme->set_style("card-title-accent-purple", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{255, 0, 255}});
    cyberpunk_theme->set_style("card-muted-text", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{0, 255, 255}});
    cyberpunk_theme->set_style("text-input", Style{Color{15, 15, 25}, Color{0, 255, 255}, 1.5f, Color{0, 255, 255}, 0});
    cyberpunk_theme->set_style("default-text", Style{Color{0, 0, 0, 0}, Color{0, 0, 0, 0}, 0.0f, Color{255, 255, 0}});
    theme_manager->add_theme("cyberpunk", cyberpunk_theme);

    // Initial theme setting triggers automatic dynamic updates to clear color & all styled nodes
    theme_manager->set_active_theme("dark");

    app.set_theme_manager(theme_manager);

    // Bootstrapping MVVM components
    auto view_model = std::make_shared<LayoutViewModel>(theme_manager);
    auto root_view = std::make_shared<LayoutView>(view_model);

    app.set_root_view(std::move(root_view));

    app.run();

    return 0;
}
