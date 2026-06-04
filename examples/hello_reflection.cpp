#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cstdio>
#include "ooey/ooey.hpp"
#include "gooey/application.hpp"
#include "ooey/platform.hpp"
#include "gooey/mvvmc/gooey_node.hpp"
#include "gooey/mvvmc/theme.hpp"
#include "gooey/controls/column.hpp"
#include "gooey/controls/row.hpp"
#include "gooey/controls/button.hpp"
#include "gooey/controls/label.hpp"
#include "gooey/controls/text_box.hpp"
#include "gooey/controls/scrollbar.hpp"
#include "gooey/controls/scroll_container.hpp"
#include "ooey/renderer/primitives/rect_primitive.hpp"
#include "ooey/renderer/primitives/rounded_rect_primitive.hpp"

using namespace ooey;
using namespace gooey;
using namespace gooey::controls;

// ---------------------------------------------------------
// 1. Custom Controls
// ---------------------------------------------------------

// CheckBox Control
class CheckBox : public GooeyElement, public mvvmc::IInteractive {
public:
    CheckBox(Rect bounds, std::string text, bool initial_checked = false)
        : bounds_(bounds), text_(std::move(text)) {
        checked.set(initial_checked);
        width = {SizePolicy::Fixed, static_cast<float>(bounds.width)};
        height = {SizePolicy::Fixed, static_cast<float>(bounds.height)};
        is_absolute = false;
        absolute_bounds = bounds;

        box_bg_ = std::make_shared<RoundedRectPrimitive>(Rect{0, 0, 0, 0}, 4, Color{30, 30, 35}, Color{150, 150, 155}, 1.5f);
        checked_indicator_ = std::make_shared<RectPrimitive>(Rect{0, 0, 0, 0}, Color{0, 120, 215});
    }

    Property<bool> checked{false};

    void draw(IRenderTarget& target) const override {
        if (box_bg_) {
            box_bg_->draw(target);
        }
        if (checked.get() && checked_indicator_) {
            checked_indicator_->draw(target);
        }

        // Draw label text
        Font font{"sans-serif", 14};
        Size ts = target.measure_text(text_, font);
        int tx = bounds_.x + 18 + 8;
        int ty = bounds_.y + (bounds_.height - ts.height) / 2;
        target.draw_text(text_, font, Point{tx, ty}, Color{220, 220, 220});
    }

    [[nodiscard]] Rect bounds() const override { return bounds_; }

    bool on_pointer_event(const Pointer& e) override {
        if (e.state == PointerState::Pressed) {
            bool hit = (e.x >= bounds_.x && e.x <= bounds_.x + bounds_.width &&
                        e.y >= bounds_.y && e.y <= bounds_.y + bounds_.height);
            if (hit) {
                checked.set(!checked.get());
                return true;
            }
        }
        return false;
    }

    bool on_key_event(const KeyEvent&) override { return false; }

protected:
    Size do_measure(Size constraints) override {
        int w = resolve_width(constraints.width, absolute_bounds.width);
        int h = resolve_height(constraints.height, absolute_bounds.height);
        return Size{w, h};
    }

    void do_layout(Rect bounds) override {
        bounds_ = bounds;
        GooeyElement::do_layout(bounds);
        
        int box_size = 18;
        int by = bounds_.y + (bounds_.height - box_size) / 2;
        if (box_bg_) {
            box_bg_->set_rect(Rect{bounds_.x, by, box_size, box_size});
        }
        if (checked_indicator_) {
            checked_indicator_->set_rect(Rect{bounds_.x + 4, by + 4, box_size - 8, box_size - 8});
        }
    }

private:
    Rect bounds_;
    std::string text_;
    std::shared_ptr<RoundedRectPrimitive> box_bg_;
    std::shared_ptr<RectPrimitive> checked_indicator_;
};

// Tree Structure and Inspector Control
struct InspectorNode {
    std::string name;
    std::string full_path;
    PropertyBase* property{nullptr};
    std::map<std::string, std::shared_ptr<InspectorNode>> children;
    bool expanded{true};
};

class PropertyTreeInspector : public Column {
public:
    PropertyTreeInspector() {
        set_padding(10);
        align_self = Align::Stretch;
        set_width(SizePolicy::MatchParent);
        set_height(SizePolicy::WrapContent);
    }

    void inspect(ViewModel& vm) {
        root_node_ = std::make_shared<InspectorNode>();
        
        // Build the hierarchical tree from the model's property paths
        for (const auto& path : vm.get_property_paths()) {
            std::shared_ptr<InspectorNode> curr = root_node_;
            size_t prev = 0;
            size_t pos = 0;
            while ((pos = path.find('.', prev)) != std::string::npos) {
                std::string part = path.substr(prev, pos - prev);
                if (curr->children.find(part) == curr->children.end()) {
                    auto node = std::make_shared<InspectorNode>();
                    node->name = part;
                    curr->children[part] = node;
                }
                curr = curr->children[part];
                prev = pos + 1;
            }
            std::string part = path.substr(prev);
            if (curr->children.find(part) == curr->children.end()) {
                auto node = std::make_shared<InspectorNode>();
                node->name = part;
                node->full_path = path;
                node->property = vm.resolve_path(path);
                curr->children[part] = node;
            }
        }

        rebuild_ui();
    }

    void draw(IRenderTarget& target) const override {
        // Draw dark sidebar background with subtle border
        RectPrimitive bg{layout_bounds, Color{40, 40, 45}, Color{30, 30, 35}, 1.5f};
        bg.draw(target);
        Column::draw(target);
    }

private:
    void rebuild_ui() {
        clear_children();
        value_subscriptions_.clear();

        if (root_node_) {
            for (const auto& kv : root_node_->children) {
                add_node_to_ui(kv.second, 0);
            }
        }
        invalidate_layout();
    }

    void add_node_to_ui(const std::shared_ptr<InspectorNode>& node, int depth) {
        auto row = std::make_shared<Row>();
        row->set_absolute(false);
        row->set_height(SizePolicy::Fixed, 28.0f);
        row->set_width(SizePolicy::MatchParent);
        row->align_self = Align::Stretch;

        // Indentation spacer label
        if (depth > 0) {
            auto indent = std::make_shared<Label>(std::string(static_cast<size_t>(depth * 3), ' '), Font{"monospace", 14}, Point{0,0}, Color{0,0,0,0});
            indent->set_absolute(false);
            row->add_child(std::move(indent));
        }

        if (!node->children.empty()) {
            // Group node
            std::string btn_txt = node->expanded ? " [-] " : " [+] ";
            auto toggle_btn = std::make_shared<Button>(Rect{0, 0, 32, 20}, Color{60, 60, 65}, Color{90, 90, 95}, 1.0f, 4, btn_txt, Color{220, 220, 220});
            toggle_btn->set_absolute(false);
            toggle_btn->set_width(SizePolicy::Fixed, 32.0f);
            toggle_btn->set_height(SizePolicy::Fixed, 20.0f);
            toggle_btn->set_margin(0, 4, 4, 4);

            toggle_btn->on_click = [this, node]() {
                node->expanded = !node->expanded;
                rebuild_ui();
            };
            row->add_child(std::move(toggle_btn));

            auto name_lbl = std::make_shared<Label>(node->name, Font{"sans-serif", 13, FontWeight::Bold}, Point{0,0}, Color{0, 180, 255});
            name_lbl->set_absolute(false);
            row->add_child(std::move(name_lbl));

            add_child(std::move(row));

            if (node->expanded) {
                for (const auto& kv : node->children) {
                    add_node_to_ui(kv.second, depth + 1);
                }
            }
        } else {
            // Leaf node
            auto bullet = std::make_shared<Label>(" * ", Font{"monospace", 14}, Point{0,0}, Color{120, 120, 120});
            bullet->set_absolute(false);
            row->add_child(std::move(bullet));

            auto name_lbl = std::make_shared<Label>(node->name, Font{"sans-serif", 13}, Point{0,0}, Color{220, 220, 220});
            name_lbl->set_absolute(false);
            row->add_child(std::move(name_lbl));

            auto sep_lbl = std::make_shared<Label>(": ", Font{"sans-serif", 13}, Point{0,0}, Color{140, 140, 140});
            sep_lbl->set_absolute(false);
            row->add_child(std::move(sep_lbl));

            auto val_lbl = std::make_shared<Label>("...", Font{"monospace", 13}, Point{0,0}, Color{255, 230, 100});
            val_lbl->set_absolute(false);

            if (node->property) {
                auto sub = node->property->subscribe_dynamic([val_lbl](const PropertyValue& val) {
                    std::visit([val_lbl](auto&& arg) {
                        using T = std::decay_t<decltype(arg)>;
                        if constexpr (std::is_same_v<T, std::string>) {
                            val_lbl->set_text(arg);
                        } else if constexpr (std::is_same_v<T, int>) {
                            val_lbl->set_text(std::to_string(arg));
                        } else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
                            char buf[32];
                            std::snprintf(buf, sizeof(buf), "%.1f", static_cast<double>(arg));
                            val_lbl->set_text(buf);
                        } else if constexpr (std::is_same_v<T, bool>) {
                            val_lbl->set_text(arg ? "true" : "false");
                        } else if constexpr (std::is_same_v<T, Color>) {
                            char buf[64];
                            std::snprintf(buf, sizeof(buf), "#%02X%02X%02X", arg.r, arg.g, arg.b);
                            val_lbl->set_text(buf);
                        } else if constexpr (std::is_same_v<T, Font>) {
                            char buf[128];
                            std::snprintf(buf, sizeof(buf), "%s %d", arg.family, arg.size);
                            val_lbl->set_text(buf);
                        }
                    }, val);
                });
                value_subscriptions_.push_back(std::move(sub));
            }
            row->add_child(std::move(val_lbl));

            add_child(std::move(row));
        }
    }

    std::shared_ptr<InspectorNode> root_node_;
    std::vector<ScopedSubscription> value_subscriptions_;
};

// ---------------------------------------------------------
// 2. ViewModels
// ---------------------------------------------------------

class SettingsViewModel : public ViewModel {
public:
    Property<std::string> theme{"Sleek Dark"};
    Property<float> volume{75.0f};
    Property<bool> show_notifications{true};

    SettingsViewModel() {
        register_property("theme", &theme);
        register_property("volume", &volume);
        register_property("show_notifications", &show_notifications);
    }
};

class UserViewModel : public ViewModel {
public:
    Property<std::string> username{"Corey"};
    Property<bool> is_admin{true};
    Property<int> login_count{18};

    UserViewModel() {
        register_property("username", &username);
        register_property("is_admin", &is_admin);
        register_property("login_count", &login_count);
    }
};

class MainViewModel : public ViewModel {
public:
    Property<std::string> app_title{"Reflection Tree Demo"};
    UserViewModel user;
    SettingsViewModel settings;

    MainViewModel() {
        register_property("app_title", &app_title);
        register_sub_registry("user", &user);
        register_sub_registry("settings", &settings);
    }
};

// ---------------------------------------------------------
// 3. The Interactive View
// ---------------------------------------------------------

class MainView : public Row {
public:
    explicit MainView(std::shared_ptr<MainViewModel> view_model)
        : view_model_(std::move(view_model)) {
        
        std::weak_ptr<MainViewModel> weak_vm = view_model_;

        // Configure main layout: Row splitting Sidebar (320px) and Workspace (MatchParent)
        set_width(SizePolicy::MatchParent);
        set_height(SizePolicy::MatchParent);

        // A. Sidebar Container (ScrollContainer wrapping PropertyTreeInspector)
        auto sidebar_scroll = std::make_shared<ScrollContainer>();
        sidebar_scroll->set_absolute(false);
        sidebar_scroll->set_width(SizePolicy::Fixed, 320.0f);
        sidebar_scroll->set_height(SizePolicy::MatchParent);
        sidebar_scroll->align_self = Align::Stretch;

        auto inspector = std::make_shared<PropertyTreeInspector>();
        inspector->set_absolute(false);
        inspector->inspect(*view_model_);
        sidebar_scroll->set_child(inspector);
        add_child(sidebar_scroll);

        // B. Workspace Editor Container (Column)
        auto workspace = std::make_shared<Column>();
        workspace->set_absolute(false);
        workspace->set_padding(25);
        workspace->set_width(SizePolicy::MatchParent);
        workspace->set_height(SizePolicy::MatchParent);
        workspace->align_self = Align::Stretch;

        // Section Title
        auto title = std::make_shared<Label>("ViewModel Reflection Editor", Font{"sans-serif", 22, FontWeight::Bold}, Point{0,0}, Color{0, 180, 255});
        title->set_absolute(false);
        workspace->add_child(std::move(title));

        // Subtitle
        auto subtitle = std::make_shared<Label>("Modify properties on the right; witness JIT reflection updates in the sidebar tree.", Font{"sans-serif", 13}, Point{0,0}, Color{160, 160, 165});
        subtitle->set_absolute(false);
        subtitle->set_margin(0, 4, 0, 25);
        workspace->add_child(std::move(subtitle));

        // Editor Form Layout
        auto form = std::make_shared<Column>();
        form->set_absolute(false);
        form->align_self = Align::Stretch;
        form->set_width(SizePolicy::MatchParent);
        form->set_height(SizePolicy::WrapContent);

        // Form Row 1: App Title (Text Box)
        auto row1 = std::make_shared<Row>();
        row1->set_absolute(false);
        row1->set_height(SizePolicy::Fixed, 40.0f);
        row1->align_self = Align::Stretch;
        
        auto lbl1 = std::make_shared<Label>("App Title:", Font{"sans-serif", 14}, Point{0,0}, Color{220, 220, 220});
        lbl1->set_absolute(false);
        lbl1->set_width(SizePolicy::Fixed, 130.0f);
        row1->add_child(std::move(lbl1));
        
        auto app_title_box = std::make_shared<TextBox>(Rect{0, 0, 300, 32}, Font{"sans-serif", 14}, Color{255, 255, 255}, Color{45, 45, 50});
        app_title_box->set_absolute(false);
        app_title_box->set_width(SizePolicy::Fixed, 300.0f);
        app_title_box->set_height(SizePolicy::Fixed, 32.0f);
        app_title_box->set_text(view_model_->app_title.get());
        app_title_box->on_text_changed = [weak_vm](const std::string& val) {
            if (auto vm = weak_vm.lock()) {
                vm->app_title.set(val);
            }
        };
        row1->add_child(app_title_box);
        form->add_child(std::move(row1));

        // Form Row 2: User Name (Text Box)
        auto row2 = std::make_shared<Row>();
        row2->set_absolute(false);
        row2->set_height(SizePolicy::Fixed, 40.0f);
        row2->align_self = Align::Stretch;
        
        auto lbl2 = std::make_shared<Label>("Username:", Font{"sans-serif", 14}, Point{0,0}, Color{220, 220, 220});
        lbl2->set_absolute(false);
        lbl2->set_width(SizePolicy::Fixed, 130.0f);
        row2->add_child(std::move(lbl2));
        
        auto username_box = std::make_shared<TextBox>(Rect{0, 0, 300, 32}, Font{"sans-serif", 14}, Color{255, 255, 255}, Color{45, 45, 50});
        username_box->set_absolute(false);
        username_box->set_width(SizePolicy::Fixed, 300.0f);
        username_box->set_height(SizePolicy::Fixed, 32.0f);
        username_box->set_text(view_model_->user.username.get());
        username_box->on_text_changed = [weak_vm](const std::string& val) {
            if (auto vm = weak_vm.lock()) {
                vm->user.username.set(val);
            }
        };
        row2->add_child(username_box);
        form->add_child(std::move(row2));

        // Form Row 3: Admin Privilege (Custom Check Box)
        auto row3 = std::make_shared<Row>();
        row3->set_absolute(false);
        row3->set_height(SizePolicy::Fixed, 40.0f);
        row3->align_self = Align::Stretch;
        
        auto lbl3 = std::make_shared<Label>("Privileges:", Font{"sans-serif", 14}, Point{0,0}, Color{220, 220, 220});
        lbl3->set_absolute(false);
        lbl3->set_width(SizePolicy::Fixed, 130.0f);
        row3->add_child(std::move(lbl3));
        
        auto is_admin_chk = std::make_shared<CheckBox>(Rect{0, 0, 200, 32}, "Enable Admin Mode", view_model_->user.is_admin.get());
        is_admin_chk->set_absolute(false);
        is_admin_chk->set_width(SizePolicy::Fixed, 200.0f);
        is_admin_chk->set_height(SizePolicy::Fixed, 32.0f);
        
        // Two-way MVVM bindings
        bind(is_admin_chk->checked, [weak_vm](bool val) {
            if (auto vm = weak_vm.lock()) {
                if (vm->user.is_admin.get() != val) {
                    vm->user.is_admin.set(val);
                }
            }
        });
        bind(view_model_->user.is_admin, [is_admin_chk](bool val) {
            if (is_admin_chk->checked.get() != val) {
                is_admin_chk->checked.set(val);
            }
        });
        row3->add_child(std::move(is_admin_chk));
        form->add_child(std::move(row3));

        // Form Row 4: Login Count (Button + Count Label)
        auto row4 = std::make_shared<Row>();
        row4->set_absolute(false);
        row4->set_height(SizePolicy::Fixed, 40.0f);
        row4->align_self = Align::Stretch;
        
        auto lbl4 = std::make_shared<Label>("Login Count:", Font{"sans-serif", 14}, Point{0,0}, Color{220, 220, 220});
        lbl4->set_absolute(false);
        lbl4->set_width(SizePolicy::Fixed, 130.0f);
        row4->add_child(std::move(lbl4));
        
        auto count_val_lbl = std::make_shared<Label>("...", Font{"monospace", 14}, Point{0,0}, Color{255, 255, 255});
        count_val_lbl->set_absolute(false);
        count_val_lbl->set_width(SizePolicy::Fixed, 50.0f);
        
        bind(view_model_->user.login_count, [count_val_lbl](int val) {
            count_val_lbl->set_text(std::to_string(val));
        });
        row4->add_child(count_val_lbl);

        auto inc_btn = std::make_shared<Button>(Rect{0, 0, 80, 28}, Color{0, 120, 215}, Color{0, 140, 235}, 1.0f, 6, "Log In (+1)", Color{255, 255, 255});
        inc_btn->set_absolute(false);
        inc_btn->set_width(SizePolicy::Fixed, 90.0f);
        inc_btn->set_height(SizePolicy::Fixed, 28.0f);
        inc_btn->on_click = [weak_vm]() {
            if (auto vm = weak_vm.lock()) {
                vm->user.login_count.set(vm->user.login_count.get() + 1);
            }
        };
        row4->add_child(inc_btn);
        form->add_child(std::move(row4));

        // Form Row 5: Volume Control (ScrollBar)
        auto row5 = std::make_shared<Row>();
        row5->set_absolute(false);
        row5->set_height(SizePolicy::Fixed, 45.0f);
        row5->align_self = Align::Stretch;
        
        auto lbl5 = std::make_shared<Label>("System Volume:", Font{"sans-serif", 14}, Point{0,0}, Color{220, 220, 220});
        lbl5->set_absolute(false);
        lbl5->set_width(SizePolicy::Fixed, 130.0f);
        row5->add_child(std::move(lbl5));

        auto vol_val_lbl = std::make_shared<Label>("...", Font{"monospace", 14}, Point{0,0}, Color{255, 255, 255});
        vol_val_lbl->set_absolute(false);
        vol_val_lbl->set_width(SizePolicy::Fixed, 50.0f);
        row5->add_child(vol_val_lbl);

        auto scroll = std::make_shared<ScrollBar>(Rect{0, 0, 200, 16}, ScrollBarOrientation::Horizontal);
        scroll->set_absolute(false);
        scroll->set_width(SizePolicy::Fixed, 200.0f);
        scroll->set_height(SizePolicy::Fixed, 16.0f);
        scroll->set_margin(0, 14, 0, 14); // vertically center ScrollBar inside row
        scroll->set_range(0, 100, 10);
        scroll->set_value(static_cast<int>(view_model_->settings.volume.get()));

        scroll->on_value_changed = [weak_vm](int val) {
            if (auto vm = weak_vm.lock()) {
                vm->settings.volume.set(static_cast<float>(val));
            }
        };
        bind(view_model_->settings.volume, [scroll, vol_val_lbl](float val) {
            int int_val = static_cast<int>(val);
            vol_val_lbl->set_text(std::to_string(int_val) + "%");
            if (scroll->get_value() != int_val) {
                scroll->set_value(int_val);
            }
        });
        row5->add_child(scroll);
        form->add_child(std::move(row5));

        // Form Row 6: Notifications (Custom Check Box)
        auto row6 = std::make_shared<Row>();
        row6->set_absolute(false);
        row6->set_height(SizePolicy::Fixed, 40.0f);
        row6->align_self = Align::Stretch;
        
        auto lbl6 = std::make_shared<Label>("Notifications:", Font{"sans-serif", 14}, Point{0,0}, Color{220, 220, 220});
        lbl6->set_absolute(false);
        lbl6->set_width(SizePolicy::Fixed, 130.0f);
        row6->add_child(std::move(lbl6));
        
        auto notif_chk = std::make_shared<CheckBox>(Rect{0, 0, 200, 32}, "Show Alerts", view_model_->settings.show_notifications.get());
        notif_chk->set_absolute(false);
        notif_chk->set_width(SizePolicy::Fixed, 200.0f);
        notif_chk->set_height(SizePolicy::Fixed, 32.0f);
        
        // Two-way bindings
        bind(notif_chk->checked, [weak_vm](bool val) {
            if (auto vm = weak_vm.lock()) {
                if (vm->settings.show_notifications.get() != val) {
                    vm->settings.show_notifications.set(val);
                }
            }
        });
        bind(view_model_->settings.show_notifications, [notif_chk](bool val) {
            if (notif_chk->checked.get() != val) {
                notif_chk->checked.set(val);
            }
        });
        row6->add_child(std::move(notif_chk));
        form->add_child(std::move(row6));

        workspace->add_child(std::move(form));
        add_child(std::move(workspace));
    }

private:
    std::shared_ptr<MainViewModel> view_model_;
};

// ---------------------------------------------------------
// 4. Main Bootstrapper
// ---------------------------------------------------------
int main() {
    std::cout << "Starting ViewModel Property Reflection and Inspector Example...\n";

    gooey::Application app;

    auto backend = ooey::create_default_window_backend();
    if (!backend || !backend->create({960, 600}, "OOEY ViewModel Reflection Inspector")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }
    app.set_window_backend(std::move(backend));

    auto view_model = std::make_shared<MainViewModel>();
    auto root_view = std::make_shared<MainView>(view_model);

    app.set_root_view(std::move(root_view));
    app.set_clear_color(ooey::Color{30, 30, 32});

    app.run();

    return 0;
}
