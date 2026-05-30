#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "ooey/ooey.hpp"
#include "gooey/application.hpp"
#include "ooey/platform.hpp"
#include "gooey/mvvmc/view.hpp"
#include "gooey/controls/column.hpp"
#include "gooey/controls/row.hpp"
#include "gooey/controls/grid.hpp"
#include "gooey/controls/flow_layout.hpp"
#include "gooey/controls/button.hpp"
#include "gooey/controls/label.hpp"
#include "gooey/controls/text_box.hpp"
#include "ooey/renderer/primitives/rect_primitive.hpp"

using namespace ooey;
using namespace gooey;

// ---------------------------------------------------------
// 1. The ViewModel (Logic & State)
// ---------------------------------------------------------
class LayoutViewModel {
private:
    size_t theme_index_{0};
    std::vector<Color> themes_{
        Color{0, 120, 215},   // Blue
        Color{0, 180, 100},   // Green
        Color{180, 100, 240},  // Purple
        Color{235, 160, 0},    // Orange
        Color{200, 50, 50}     // Red
    };

public:
    // Properties that the View will observe
    Property<std::string> greeting_text{"Enter your name below to get started!"};
    Property<int> click_count{0};
    Property<Color> theme_color{Color{0, 120, 215}};
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
        theme_index_ = (theme_index_ + 1) % themes_.size();
        theme_color.set(themes_[theme_index_]);
        std::cout << "ViewModel: Cycled to theme index " << theme_index_ << "\n";
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

    void set_bg_color(Color color) {
        bg_color_ = color;
    }

    void draw(IRenderTarget& target) const override {
        // Draw the background card rectangle covering laid-out boundaries
        RectPrimitive(layout_bounds, bg_color_).draw(target);
        // Draw children on top of background
        Column::draw(target);
    }

private:
    Color bg_color_;
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
        header->add_child(title);

        auto subtitle = std::make_shared<Label>(
            "Resize this window to watch the responsive layout adapt under dynamic bindings.",
            Font{"sans-serif", 14},
            Point{0, 0},
            Color{150, 150, 160}
        );
        subtitle->set_margin(0, 0, 0, 15);
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
        button_row->add_child(counter_btn);

        add_child(button_row);

        // Grid of Cards
        auto grid = std::make_shared<Grid>(2, 2);
        grid->set_width(SizePolicy::MatchParent);
        grid->set_height(SizePolicy::Fixed, 200.0f);
        grid->set_margin(0, 0, 0, 20);

        auto card1 = std::make_shared<CardView>(Color{35, 35, 40});
        auto card1_title = std::make_shared<Label>("Theme Status", Font{"sans-serif", 16, FontWeight::Bold}, Point{0, 0}, Color{0, 180, 240});
        card1_title->set_margin(0, 0, 0, 5);
        card1->add_child(card1_title);
        auto card1_lbl = std::make_shared<Label>("Dynamic card styling accent", Font{"sans-serif", 12}, Point{0, 0}, Color{180, 180, 190});
        card1->add_child(card1_lbl);
        grid->add_child(card1);

        auto card2 = std::make_shared<CardView>(Color{35, 35, 40});
        auto card2_title = std::make_shared<Label>("Console Output", Font{"sans-serif", 16, FontWeight::Bold}, Point{0, 0}, Color{0, 200, 100});
        card2_title->set_margin(0, 0, 0, 5);
        card2->add_child(card2_title);
        auto card2_lbl = std::make_shared<Label>("CPU Load: 42%\nRAM usage: 5.4 GB / 16 GB", Font{"sans-serif", 12}, Point{0, 0}, Color{180, 180, 190});
        card2->add_child(card2_lbl);
        grid->add_child(card2);

        auto card3 = std::make_shared<CardView>(Color{35, 35, 40});
        auto card3_title = std::make_shared<Label>("Interactive Console", Font{"sans-serif", 16, FontWeight::Bold}, Point{0, 0}, Color{235, 160, 0});
        card3_title->set_margin(0, 0, 0, 5);
        card3->add_child(card3_title);
        auto card3_lbl = std::make_shared<Label>("Layout positions auto flow", Font{"sans-serif", 12}, Point{0, 0}, Color{180, 180, 190});
        card3->add_child(card3_lbl);
        grid->add_child(card3);

        auto card4 = std::make_shared<CardView>(Color{35, 35, 40});
        auto card4_title = std::make_shared<Label>("System Details", Font{"sans-serif", 16, FontWeight::Bold}, Point{0, 0}, Color{180, 100, 240});
        card4_title->set_margin(0, 0, 0, 5);
        card4->add_child(card4_title);
        auto card4_lbl = std::make_shared<Label>("Mode: Retained MVVMC\nTheme: Cycle Modern", Font{"sans-serif", 12}, Point{0, 0}, Color{180, 180, 190});
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
        footer_row->add_child(name_lbl);

        auto name_box = std::make_shared<TextBox>(
            Rect{0, 0, 250, 36},
            Font{"sans-serif", 16},
            Color{255, 255, 255},
            Color{35, 35, 40}
        );
        footer_row->add_child(name_box);
        add_child(footer_row);

        auto greeting_lbl = std::make_shared<Label>("", Font{"sans-serif", 16, FontWeight::Bold}, Point{0, 0}, Color{0, 200, 100});
        greeting_lbl->set_width(SizePolicy::MatchParent);
        greeting_lbl->set_margin(0, 5, 0, 15);
        add_child(greeting_lbl);

        // -- Data Bindings (ViewModel -> UI) --
        bind(view_model_->click_count, [counter_btn](const int& count) {
            counter_btn->set_label_text("Clicks: " + std::to_string(count));
        });

        bind(view_model_->greeting_text, [greeting_lbl](const std::string& text) {
            greeting_lbl->set_text(text);
        });

        bind(view_model_->theme_color, [cycle_theme_btn, card1_title, card1](const Color& c) {
            cycle_theme_btn->set_fill_color(c);
            card1_title->set_color(c);
            card1->set_bg_color(Color{static_cast<uint8_t>(35 + c.r / 6),
                                      static_cast<uint8_t>(35 + c.g / 6),
                                      static_cast<uint8_t>(40 + c.b / 6)});
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
    std::cout << "Starting OOEY MVVMC Responsive Layout Demo...\n";

    Application app;

    auto backend = create_default_window_backend();
    if (!backend || !backend->create({800, 600}, "OOEY MVVMC Layout Dashboard")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }

    app.set_window_backend(std::move(backend));
    app.set_clear_color(Color{24, 24, 28});

    // Bootstrapping MVVM components
    auto view_model = std::make_shared<LayoutViewModel>();
    auto root_view = std::make_shared<LayoutView>(view_model);

    app.set_root_view(std::move(root_view));

    app.run();

    return 0;
}
