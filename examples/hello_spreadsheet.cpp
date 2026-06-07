#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <any>
#include <sstream>
#include <iomanip>
#include "ooey/ooey.hpp"
#include "gooey/application.hpp"
#include "ooey/platform.hpp"
#include "gooey/mvvmc/gooey_node.hpp"
#include "gooey/mvvmc/controller.hpp"
#include "gooey/controls/button.hpp"
#include "gooey/controls/label.hpp"
#include "gooey/controls/datagrid.hpp"
#include "gooey/controls/row.hpp"
#include "gooey/controls/checkbox.hpp"
#include "gooey/controls/text_box.hpp"
#include "gooey/mvvmc/property.hpp"
#include "ooey/renderer/primitives/rect_primitive.hpp"
#include "ooey/renderer/primitives/line_primitive.hpp"

using namespace gooey;
using namespace gooey::mvvmc;

// 1. Spreadsheet Data Row representation
struct SpreadsheetRow {
    struct CellData {
        std::string value{"0"};
        bool selected{false};
    };
    CellData cells[3]; // Columns A, B, C
};

class SpreadsheetCell : public gooey::GooeyElement, public IInteractive, public std::enable_shared_from_this<SpreadsheetCell> {
public:
    SpreadsheetCell(int row_idx, int col_idx, std::function<void(int, int, bool)> on_select_changed, std::function<void()> on_clear_selection)
        : row_idx_(row_idx), col_idx_(col_idx), on_select_changed_(on_select_changed), on_clear_selection_(on_clear_selection) {
        
        textbox_ = std::make_shared<gooey::controls::TextBox>();
        textbox_->set_margin(0);
    }

    Rect bounds() const override {
        return layout_bounds;
    }

    void set_text(const std::string& text) {
        textbox_->set_text(text);
    }

    std::string get_text() const {
        return textbox_->get_text();
    }

    void set_row_index(int idx) {
        row_idx_ = idx;
    }

    void set_row_data(std::shared_ptr<SpreadsheetRow> row_data) {
        row_data_ = row_data;
    }

    void set_selected(bool selected) {
        if (is_selected_ != selected) {
            is_selected_ = selected;
            if (is_selected_) {
                set_style_name("is_selected");
            } else {
                set_style_name("cell");
            }
            if (on_select_changed_) {
                on_select_changed_(row_idx_, col_idx_, selected);
            }
            invalidate_layout();
        }
        if (!is_selected_) {
            is_editing_ = false;
        }
    }

    bool is_selected() const {
        return is_selected_;
    }

    void draw(ooey::IRenderTarget& target) const override {
        if (is_editing_) {
            auto* controller = dynamic_cast<gooey::mvvmc::Controller*>(Application::get_instance()->get_controller());
            if (controller && controller->get_focused_element().get() != this) {
                is_editing_ = false;
            }
        }

        // Draw background selection tint
        if (is_selected_) {
            RectPrimitive sel_bg(layout_bounds, ooey::Color{0, 120, 215, 30});
            sel_bg.draw(target);
        }
        
        if (is_editing_) {
            if (textbox_) {
                textbox_->draw(target);
            }
        } else {
            // Draw clean text, no textbox border/background
            std::string val = textbox_ ? textbox_->get_text() : "";
            Point text_pos{layout_bounds.x + 8, layout_bounds.y + (layout_bounds.height - 14) / 2};
            TextPrimitive txt(val, Font{"sans-serif", 14}, text_pos, Color{240, 240, 240});
            txt.draw(target);
        }

        // Draw selection outline on top of the cell
        if (is_selected_) {
            LinePrimitive top(Point{layout_bounds.x, layout_bounds.y}, Point{layout_bounds.x + layout_bounds.width, layout_bounds.y}, Color{0, 120, 215}, 2.0f);
            LinePrimitive bottom(Point{layout_bounds.x, layout_bounds.y + layout_bounds.height}, Point{layout_bounds.x + layout_bounds.width, layout_bounds.y + layout_bounds.height}, Color{0, 120, 215}, 2.0f);
            LinePrimitive left(Point{layout_bounds.x, layout_bounds.y}, Point{layout_bounds.x, layout_bounds.y + layout_bounds.height}, Color{0, 120, 215}, 2.0f);
            LinePrimitive right(Point{layout_bounds.x + layout_bounds.width, layout_bounds.y}, Point{layout_bounds.x + layout_bounds.width, layout_bounds.y + layout_bounds.height}, Color{0, 120, 215}, 2.0f);
            
            top.draw(target);
            bottom.draw(target);
            left.draw(target);
            right.draw(target);
        }
    }

    bool on_pointer_event(const Pointer& e) override {
        bool hit = (e.x >= layout_bounds.x && e.x <= layout_bounds.x + layout_bounds.width &&
                    e.y >= layout_bounds.y && e.y <= layout_bounds.y + layout_bounds.height);
        
        if (hit && e.state == PointerState::Pressed) {
            auto& input = Application::get_instance()->get_input_manager();
            // Check left Ctrl (0xffe3) or right Ctrl (0xffe4)
            bool ctrl = input.is_key_pressed(0xffe3) || input.is_key_pressed(0xffe4);

            if (ctrl) {
                set_selected(!is_selected_);
                is_editing_ = false;
                invalidate_layout();
            } else {
                if (!is_selected_) {
                    if (on_clear_selection_) {
                        on_clear_selection_();
                    }
                    set_selected(true);
                    is_editing_ = false;
                    invalidate_layout();
                } else {
                    is_editing_ = true;
                    textbox_->on_pointer_event(e);
                    invalidate_layout();
                }
            }
            return true;
        }

        if (is_editing_) {
            return textbox_->on_pointer_event(e);
        }
        return false;
    }

    bool on_key_event(const KeyEvent& e) override {
        if (is_editing_) {
            if (e.state == KeyState::Pressed) {
                if (e.key_code == 0xFF1B || e.key_code == 27) { // Escape
                    // Revert text
                    if (row_data_) {
                        textbox_->set_text(row_data_->cells[col_idx_].value);
                    }
                    is_editing_ = false;
                    invalidate_layout();
                    
                    // Transfer focus back to the cell
                    auto* controller = dynamic_cast<gooey::mvvmc::Controller*>(Application::get_instance()->get_controller());
                    if (controller) {
                        controller->set_focused_element(shared_from_this());
                    }
                    return true;
                } else if (e.key_code == 0xFF0D || e.key_code == 13 || e.key_code == 10) { // Return
                    is_editing_ = false;
                    invalidate_layout();
                    
                    // Transfer focus back to the cell
                    auto* controller = dynamic_cast<gooey::mvvmc::Controller*>(Application::get_instance()->get_controller());
                    if (controller) {
                        controller->set_focused_element(shared_from_this());
                    }
                    return true;
                }
            }
            return textbox_->on_key_event(e);
        }
        return false;
    }

    bool on_text_event(const TextEvent& e) override {
        if (is_editing_) {
            return textbox_->on_text_event(e);
        }
        return false;
    }

    void set_theme_manager(std::shared_ptr<ThemeManager> manager) override {
        GooeyElement::set_theme_manager(manager);
        if (textbox_) {
            textbox_->set_theme_manager(manager);
        }
    }

    std::shared_ptr<gooey::controls::TextBox> textbox() const {
        return textbox_;
    }

protected:
    void do_layout(Rect bounds) override {
        GooeyElement::do_layout(bounds);
        if (textbox_) {
            textbox_->layout(bounds);
        }
    }

private:
    std::shared_ptr<gooey::controls::TextBox> textbox_;
    int row_idx_;
    int col_idx_;
    bool is_selected_{false};
    mutable bool is_editing_{false};
    std::shared_ptr<SpreadsheetRow> row_data_{nullptr};
    std::function<void(int, int, bool)> on_select_changed_;
    std::function<void()> on_clear_selection_;
};

// 3. Spreadsheet ViewModel (State and Calculation Logic)
class SpreadsheetViewModel {
public:
    SpreadsheetViewModel() {
        // Populate with initial numeric values
        for (int i = 0; i < 8; ++i) {
            auto row = std::make_shared<SpreadsheetRow>();
            row->cells[0].value = std::to_string((i + 1) * 10);
            row->cells[1].value = std::to_string((i + 1) * 5);
            row->cells[2].value = std::to_string(i + 1);
            rows.push_back(row);
        }
        update_sum();
    }

    std::vector<std::shared_ptr<SpreadsheetRow>> rows;
    gooey::Property<std::string> sum_result{"Sum of selected cells: 0.00"};

    void clear_selection() {
        for (auto& row : rows) {
            for (int col = 0; col < 3; ++col) {
                row->cells[col].selected = false;
            }
        }
    }

    void update_sum() {
        double sum = 0.0;
        for (const auto& row : rows) {
            for (int col = 0; col < 3; ++col) {
                if (row->cells[col].selected) {
                    try {
                        sum += std::stod(row->cells[col].value);
                    } catch (...) {
                        // ignore empty or invalid inputs
                    }
                }
            }
        }
        std::stringstream ss;
        ss << "Sum of selected cells: " << std::fixed << std::setprecision(2) << sum;
        sum_result.set(ss.str());
    }

    void on_sum_clicked() {
        update_sum();
    }
};

// 4. Spreadsheet View (Layout Construction and Binding)
class SpreadsheetView : public gooey::GooeyNode {
public:
    SpreadsheetView(std::shared_ptr<SpreadsheetViewModel> view_model)
        : view_model_(std::move(view_model)) {
        
        // 4.1 Title
        auto title = std::make_shared<gooey::controls::Label>(
            "Spreadsheet Calculation Example", 
            ooey::Font{"sans-serif", 18}, 
            ooey::Point{30, 20}, 
            ooey::Color{255, 255, 255}
        );
        add_child(title);

        // 4.2 DataGrid setup
        auto grid = std::make_shared<gooey::controls::DataGrid>(
            ooey::Rect{30, 60, 440, 290}, 
            32, 
            ooey::Font{"sans-serif", 14}
        );

        std::weak_ptr<gooey::controls::DataGrid> weak_grid = grid;

        // Columns definition
        std::vector<gooey::controls::DataGridColumn> cols;
        for (int col_idx = 0; col_idx < 3; ++col_idx) {
            std::string header_name = std::string(1, 'A' + col_idx);
            
            gooey::controls::DataGridColumn col;
            col.header = header_name;
            col.width = 135;
            
            // Factory instantiates SpreadsheetCell
            col.cell_factory = [vm = view_model_, col_idx, weak_grid]() {
                auto cell = std::make_shared<SpreadsheetCell>(
                    0, col_idx,
                    [vm](int r, int c, bool sel) {
                        if (r < static_cast<int>(vm->rows.size())) {
                            vm->rows[r]->cells[c].selected = sel;
                        }
                    },
                    [vm, weak_grid]() {
                        vm->clear_selection();
                        if (auto g = weak_grid.lock()) {
                            g->update_cell_values();
                        }
                    }
                );
                return cell;
            };

            col.cell_binder = [col_idx](const std::shared_ptr<gooey::mvvmc::GooeyElement>& el, const std::any& item, int row_idx) {
                auto cell = std::dynamic_pointer_cast<SpreadsheetCell>(el);
                if (!cell) return;

                cell->set_row_index(row_idx);

                if (item.has_value()) {
                    auto row_data = std::any_cast<std::shared_ptr<SpreadsheetRow>>(item);
                    
                    cell->set_row_data(row_data);
                    cell->set_text(row_data->cells[col_idx].value);
                    cell->set_selected(row_data->cells[col_idx].selected);
                    
                    cell->textbox()->on_text_changed = [row_data, col_idx](const std::string& val) {
                        row_data->cells[col_idx].value = val;
                    };
                } else {
                    cell->set_row_data(nullptr);
                    cell->set_text("");
                    cell->set_selected(false);
                    cell->textbox()->on_text_changed = nullptr;
                }
            };

            cols.push_back(col);
        }

        grid->set_columns(cols);

        // Supply data items to grid
        std::vector<std::any> grid_items;
        for (const auto& r : view_model_->rows) {
            grid_items.push_back(r);
        }
        grid->set_items(grid_items);
        add_child(grid);

        // 4.3 Sum Button
        auto sum_btn = std::make_shared<gooey::Button>(
            ooey::Rect{30, 380, 100, 40}, 
            ooey::Color{0, 120, 215}
        );
        sum_btn->set_label_text("Sum");
        sum_btn->on_click = [vm = view_model_, weak_grid]() {
            vm->on_sum_clicked();
            if (auto g = weak_grid.lock()) {
                g->update_cell_values();
            }
        };
        add_child(sum_btn);

        // 4.4 Sum Result Label
        auto sum_label = std::make_shared<gooey::controls::Label>(
            "Sum of selected cells: 0.00", 
            ooey::Font{"sans-serif", 16}, 
            ooey::Point{150, 390}, 
            ooey::Color{240, 240, 240}
        );
        bind(view_model_->sum_result, [sum_label](const std::string& text) {
            sum_label->set_text(text);
        });
        add_child(sum_label);
    }

private:
    std::shared_ptr<SpreadsheetViewModel> view_model_;
};

int main() {
    std::cout << "Starting OOEY Spreadsheet Example...\n";

    gooey::Application app;

    auto backend = ooey::create_default_window_backend();
    if (!backend || !backend->create({500, 460}, "OOEY Spreadsheet Calculations")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }
    app.set_window_backend(std::move(backend));

    auto view_model = std::make_shared<SpreadsheetViewModel>();
    auto root_view = std::make_shared<SpreadsheetView>(view_model);

    app.set_root_view(std::move(root_view));
    app.set_clear_color(ooey::Color{30, 30, 30});

    app.run();

    return 0;
}
