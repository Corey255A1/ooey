#include "view.hpp"
#include "styled_panel.hpp"
#include "gooey/controls/label.hpp"
#include "gooey/controls/button.hpp"
#include "gooey/controls/datagrid.hpp"
#include "gooey/controls/row.hpp"
#include "gooey/controls/adaptive_stack.hpp"
#include "gooey/controls/flow_layout.hpp"

using namespace ooey;
using namespace gooey;
using namespace gooey::controls;

SystemMonitorView::SystemMonitorView(std::shared_ptr<SystemMonitorViewModel> view_model)
    : view_model_(std::move(view_model)) {
    
    set_width(SizePolicy::MatchParent);
    set_height(SizePolicy::MatchParent);
    set_padding(25);
    set_style_name("window");

    auto main_card = std::make_shared<StyledPanel>();
    main_card->set_width(SizePolicy::MatchParent);
    main_card->set_height(SizePolicy::MatchParent);
    main_card->set_padding(20);
    main_card->set_style_name("window-card");
    add_child(main_card);

    auto header = std::make_shared<Column>();
    header->set_width(SizePolicy::MatchParent);
    header->set_height(SizePolicy::WrapContent);
    header->set_margin(0, 0, 0, 15);

    auto title = std::make_shared<Label>(
        "Real-time System Monitor",
        Font{"sans-serif", 24, FontWeight::Bold},
        Point{0, 0},
        Color{255, 255, 255}
    );
    title->set_absolute(false);
    title->set_width(SizePolicy::MatchParent);
    title->set_overflow(TextOverflow::Wrapped);
    title->set_margin(0, 0, 0, 5);
    title->set_style_name("title-text");
    header->add_child(title);

    auto subtitle = std::make_shared<Label>(
        "Dynamic hardware performance monitoring dashboard & running processes list",
        Font{"sans-serif", 13},
        Point{0, 0},
        Color{140, 140, 150}
    );
    subtitle->set_absolute(false);
    subtitle->set_width(SizePolicy::MatchParent);
    subtitle->set_overflow(TextOverflow::Wrapped);
    subtitle->set_margin(0, 0, 0, 10);
    subtitle->set_style_name("subtitle-text");
    header->add_child(subtitle);

    main_card->add_child(header);

    auto metrics_row = std::make_shared<AdaptiveStack>();
    metrics_row->set_breakpoint(740);
    metrics_row->set_width(SizePolicy::MatchParent);
    metrics_row->set_height(SizePolicy::WrapContent);
    metrics_row->set_margin(0, 0, 0, 15);

    // CPU Card
    auto cpu_card = std::make_shared<StyledPanel>();
    cpu_card->set_style_name("card-bg");
    cpu_card->set_width(SizePolicy::Fixed, 235.0f);
    cpu_card->set_height(SizePolicy::Fixed, 140.0f);
    cpu_card->set_margin(0, 5, 10, 5);

    auto cpu_lbl = std::make_shared<Label>("CPU UTILIZATION", Font{"sans-serif", 11, FontWeight::Bold}, Point{0, 0}, Color{0, 180, 240});
    cpu_lbl->set_absolute(false);
    cpu_lbl->set_margin(0, 0, 0, 5);
    cpu_lbl->set_style_name("card-header-cpu");
    cpu_card->add_child(cpu_lbl);

    auto cpu_val = std::make_shared<Label>("0.0 %", Font{"sans-serif", 22, FontWeight::Bold}, Point{0, 0}, Color{230, 230, 235});
    cpu_val->set_absolute(false);
    cpu_val->set_margin(0, 0, 0, 12);
    cpu_val->set_style_name("card-value-cpu");
    bind(view_model_->cpu_text, [cpu_val](const std::string& val) { cpu_val->set_text(val); });
    cpu_card->add_child(cpu_val);

    auto cpu_sec = std::make_shared<Label>("Cores Active", Font{"sans-serif", 12}, Point{0, 0}, Color{150, 150, 160});
    cpu_sec->set_absolute(false);
    cpu_sec->set_style_name("card-desc-text");
    bind(view_model_->cpu_desc, [cpu_sec](const std::string& val) { cpu_sec->set_text(val); });
    cpu_card->add_child(cpu_sec);

    metrics_row->add_child(cpu_card);

    // RAM Card
    auto ram_card = std::make_shared<StyledPanel>();
    ram_card->set_style_name("card-bg");
    ram_card->set_width(SizePolicy::Fixed, 235.0f);
    ram_card->set_height(SizePolicy::Fixed, 140.0f);
    ram_card->set_margin(0, 5, 10, 5);

    auto ram_lbl = std::make_shared<Label>("RAM USAGE", Font{"sans-serif", 11, FontWeight::Bold}, Point{0, 0}, Color{235, 160, 0});
    ram_lbl->set_absolute(false);
    ram_lbl->set_margin(0, 0, 0, 5);
    ram_lbl->set_style_name("card-header-ram");
    ram_card->add_child(ram_lbl);

    auto ram_val = std::make_shared<Label>("0.0 GB / 0.0 GB", Font{"sans-serif", 15, FontWeight::Bold}, Point{0, 0}, Color{230, 230, 235});
    ram_val->set_absolute(false);
    ram_val->set_margin(0, 0, 0, 18);
    ram_val->set_style_name("card-value-ram");
    bind(view_model_->ram_text, [ram_val](const std::string& val) { ram_val->set_text(val); });
    ram_card->add_child(ram_val);

    auto ram_sec = std::make_shared<Label>("0% memory active", Font{"sans-serif", 12}, Point{0, 0}, Color{150, 150, 160});
    ram_sec->set_absolute(false);
    ram_sec->set_style_name("card-desc-text");
    bind(view_model_->ram_desc, [ram_sec](const std::string& val) { ram_sec->set_text(val); });
    ram_card->add_child(ram_sec);

    metrics_row->add_child(ram_card);

    // Disk Card
    auto disk_card = std::make_shared<StyledPanel>();
    disk_card->set_style_name("card-bg");
    disk_card->set_width(SizePolicy::Fixed, 235.0f);
    disk_card->set_height(SizePolicy::Fixed, 140.0f);
    disk_card->set_margin(0, 5, 10, 5);

    auto disk_lbl = std::make_shared<Label>("DISK SPACE", Font{"sans-serif", 11, FontWeight::Bold}, Point{0, 0}, Color{180, 100, 240});
    disk_lbl->set_absolute(false);
    disk_lbl->set_margin(0, 0, 0, 5);
    disk_lbl->set_style_name("card-header-disk");
    disk_card->add_child(disk_lbl);

    auto disk_val = std::make_shared<Label>("0.0 GB / 0.0 GB", Font{"sans-serif", 15, FontWeight::Bold}, Point{0, 0}, Color{230, 230, 235});
    disk_val->set_absolute(false);
    disk_val->set_margin(0, 0, 0, 18);
    disk_val->set_style_name("card-value-disk");
    bind(view_model_->disk_text, [disk_val](const std::string& val) { disk_val->set_text(val); });
    disk_card->add_child(disk_val);

    auto disk_sec = std::make_shared<Label>("0% disk active", Font{"sans-serif", 12}, Point{0, 0}, Color{150, 150, 160});
    disk_sec->set_absolute(false);
    disk_sec->set_style_name("card-desc-text");
    bind(view_model_->disk_desc, [disk_sec](const std::string& val) { disk_sec->set_text(val); });
    disk_card->add_child(disk_sec);

    metrics_row->add_child(disk_card);

    main_card->add_child(metrics_row);

    // Bottom Row
    auto bottom_row = std::make_shared<AdaptiveStack>();
    bottom_row->set_breakpoint(740);
    bottom_row->set_width(SizePolicy::MatchParent);
    bottom_row->set_height(SizePolicy::WrapContent);
    bottom_row->set_margin(0, 0, 0, 10);

    // Process DataGrid Panel
    auto grid_container = std::make_shared<Column>();
    grid_container->set_width(SizePolicy::MatchParent);
    grid_container->set_height(SizePolicy::WrapContent);
    grid_container->set_margin(0, 5, 10, 5);

    auto list_lbl = std::make_shared<Label>(
        "Top System Processes (RSS Memory Sorted)",
        Font{"sans-serif", 13, FontWeight::Bold},
        Point{0, 0},
        Color{180, 180, 195}
    );
    list_lbl->set_absolute(false);
    list_lbl->set_margin(0, 0, 0, 5);
    list_lbl->set_style_name("section-header");
    grid_container->add_child(list_lbl);

    auto proc_grid = std::make_shared<DataGrid>(
        Rect{0, 0, 510, 240},
        26, // row height
        Font{"monospace", 12}
    );
    proc_grid->set_absolute(false);
    proc_grid->set_width(SizePolicy::MatchParent);
    proc_grid->set_height(SizePolicy::WrapContent);
    proc_grid->set_style_name("list-box");

    // Define columns matching standard process specs: PID, Name, CPU, RAM, State
    proc_grid->set_columns({
        {.header="PID", .width=65},
        {.header="Process Name", .width=160},
        {.header="CPU %", .width=80},
        {.header="Memory", .width=110},
        {.header="State", .width=60}
    });

    bind(view_model_->process_rows, [proc_grid](const std::vector<std::vector<std::string>>& rows) {
        proc_grid->set_rows(rows);
    });

    grid_container->add_child(proc_grid);
    bottom_row->add_child(grid_container);

    // Right column: Theme Selection
    auto theme_card = std::make_shared<StyledPanel>();
    theme_card->set_style_name("card-bg");
    theme_card->set_width(SizePolicy::Fixed, 220.0f);
    theme_card->set_height(SizePolicy::WrapContent);
    theme_card->set_align_self(Align::Stretch);
    theme_card->set_margin(0, 5, 10, 5);

    auto theme_lbl = std::make_shared<Label>(
        "DASHBOARD THEME",
        Font{"sans-serif", 11, FontWeight::Bold},
        Point{0, 0},
        Color{150, 150, 165}
    );
    theme_lbl->set_absolute(false);
    theme_lbl->set_margin(0, 0, 0, 12);
    theme_lbl->set_style_name("theme-header");
    theme_card->add_child(theme_lbl);

    auto button_flow = std::make_shared<FlowLayout>();
    button_flow->set_width(SizePolicy::MatchParent);
    button_flow->set_height(SizePolicy::WrapContent);

    auto btn_dark = std::make_shared<Button>(Rect{0, 0, 160, 34}, Color{45, 45, 52});
    btn_dark->set_absolute(false);
    btn_dark->set_margin(0, 0, 8, 8);
    btn_dark->set_label_text("Dark Mode");
    btn_dark->set_style_name("btn-dark");
    btn_dark->on_click = [this]() { view_model_->set_theme("dark"); };
    button_flow->add_child(btn_dark);

    auto btn_light = std::make_shared<Button>(Rect{0, 0, 160, 34}, Color{45, 45, 52});
    btn_light->set_absolute(false);
    btn_light->set_margin(0, 0, 8, 8);
    btn_light->set_label_text("Light Clean");
    btn_light->set_style_name("btn-light");
    btn_light->on_click = [this]() { view_model_->set_theme("light"); };
    button_flow->add_child(btn_light);

    auto btn_hacker = std::make_shared<Button>(Rect{0, 0, 160, 34}, Color{45, 45, 52});
    btn_hacker->set_absolute(false);
    btn_hacker->set_margin(0, 0, 8, 8);
    btn_hacker->set_label_text("Hacker Green");
    btn_hacker->set_style_name("btn-hacker");
    btn_hacker->on_click = [this]() { view_model_->set_theme("hacker"); };
    button_flow->add_child(btn_hacker);

    auto btn_lofi = std::make_shared<Button>(Rect{0, 0, 160, 34}, Color{45, 45, 52});
    btn_lofi->set_absolute(false);
    btn_lofi->set_margin(0, 0, 8, 8);
    btn_lofi->set_label_text("Soft Lofi");
    btn_lofi->set_style_name("btn-lofi");
    btn_lofi->on_click = [this]() { view_model_->set_theme("lofi"); };
    button_flow->add_child(btn_lofi);

    theme_card->add_child(button_flow);
    bottom_row->add_child(theme_card);
    main_card->add_child(bottom_row);

    auto footnote = std::make_shared<Label>(
        "Note: Processes grid view is fully virtualized and refreshed once per second. Responsive DataGrid layout enabled.",
        Font{"sans-serif", 11},
        Point{0, 0},
        Color{110, 110, 120}
    );
    footnote->set_absolute(false);
    footnote->set_width(SizePolicy::MatchParent);
    footnote->set_overflow(TextOverflow::Wrapped);
    footnote->set_style_name("footnote-text");
    main_card->add_child(footnote);
}
