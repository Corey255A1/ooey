#include "gooey/controls/datagrid.hpp"
#include "gooey/controls/label.hpp"
#include <algorithm>

namespace gooey::controls {

DataGrid::DataGrid(Rect bounds, int row_height, Font font)
    : bounds_(bounds), row_height_(row_height), font_(font) {
    width = {SizePolicy::Fixed, static_cast<float>(bounds.width)};
    height = {SizePolicy::Fixed, static_cast<float>(bounds.height)};
    is_absolute = true;
    absolute_bounds = bounds;
    set_style_name("datagrid");

    bg_ = std::make_shared<RoundedRectPrimitive>(bounds_, 6, bg_color_, border_color_, 1.5f);
    add_child(bg_);

    int header_height = row_height_ + 6;
    header_bg_ = std::make_shared<RectPrimitive>(Rect{bounds_.x, bounds_.y, bounds_.width, header_height}, header_bg_color_);
    add_child(header_bg_);

    v_scroll_ = std::make_shared<ScrollBar>(Rect{0, 0, 0, 0}, ScrollBarOrientation::Vertical);
    v_scroll_->on_value_changed = [this](int value) {
        if (scroll_offset_y_ != value) {
            scroll_offset_y_ = value;
            update_cell_values();
            invalidate_layout();
        }
    };
    add_child(v_scroll_);

    h_scroll_ = std::make_shared<ScrollBar>(Rect{0, 0, 0, 0}, ScrollBarOrientation::Horizontal);
    h_scroll_->on_value_changed = [this](int value) {
        if (scroll_offset_x_ != value) {
            scroll_offset_x_ = value;
            update_layout_elements();
            invalidate_layout();
        }
    };
    add_child(h_scroll_);
}

Rect DataGrid::bounds() const {
    return bounds_;
}

void DataGrid::set_columns(const std::vector<DataGridColumn>& columns) {
    columns_ = columns;
    cell_cache_.clear();
    update_layout_elements();
    invalidate_layout();
}

void DataGrid::set_rows(const std::vector<std::vector<std::string>>& rows) {
    rows_ = rows;
    cell_cache_.clear();

    int header_height = row_height_ + 6;
    int viewport_h = bounds_.height - header_height;
    if (h_scroll_ && h_scroll_->bounds().height > 0) {
        viewport_h = std::max(0, viewport_h - 12);
    }
    visible_rows_count_ = row_height_ > 0 ? (viewport_h / row_height_) : 1;
    if (visible_rows_count_ <= 0) visible_rows_count_ = 1;

    if (v_scroll_) {
        v_scroll_->set_range(0, get_row_count(), visible_rows_count_);
    }

    int max_scroll = std::max(0, get_row_count() - visible_rows_count_);
    if (scroll_offset_y_ > max_scroll) {
        scroll_offset_y_ = max_scroll;
        if (v_scroll_) {
            v_scroll_->set_value(scroll_offset_y_);
        }
    }

    update_layout_elements();
    invalidate_layout();
}

void DataGrid::set_items(const std::vector<std::any>& items) {
    items_ = items;
    cell_cache_.clear();

    int header_height = row_height_ + 6;
    int viewport_h = bounds_.height - header_height;
    if (h_scroll_ && h_scroll_->bounds().height > 0) {
        viewport_h = std::max(0, viewport_h - 12);
    }
    visible_rows_count_ = row_height_ > 0 ? (viewport_h / row_height_) : 1;
    if (visible_rows_count_ <= 0) visible_rows_count_ = 1;

    if (v_scroll_) {
        v_scroll_->set_range(0, get_row_count(), visible_rows_count_);
    }

    int max_scroll = std::max(0, get_row_count() - visible_rows_count_);
    if (scroll_offset_y_ > max_scroll) {
        scroll_offset_y_ = max_scroll;
        if (v_scroll_) {
            v_scroll_->set_value(scroll_offset_y_);
        }
    }

    update_layout_elements();
    invalidate_layout();
}

int DataGrid::get_row_count() const {
    if (!items_.empty()) {
        return static_cast<int>(items_.size());
    }
    return static_cast<int>(rows_.size());
}

bool DataGrid::on_pointer_event(const Pointer& e) {
    if (v_scroll_ && v_scroll_->bounds().width > 0) {
        if (v_scroll_->on_pointer_event(e)) {
            return true;
        }
    }

    if (h_scroll_ && h_scroll_->bounds().height > 0) {
        if (h_scroll_->on_pointer_event(e)) {
            return true;
        }
    }

    return false;
}

bool DataGrid::on_key_event(const ooey::KeyEvent& /*e*/) {
    return false;
}

Size DataGrid::do_measure(Size constraints) {
    int w = resolve_width(constraints.width, absolute_bounds.width);
    int h = resolve_height(constraints.height, absolute_bounds.height);
    return Size{w, h};
}

void DataGrid::do_layout(Rect bounds) {
    bounds_ = bounds;
    if (bg_) {
        bg_->set_rect(bounds_);
    }
    int header_height = row_height_ + 6;
    if (header_bg_) {
        header_bg_->set_rect(Rect{bounds_.x, bounds_.y, bounds_.width, header_height});
    }

    update_layout_elements();
}

void DataGrid::apply_style(const mvvmc::Style& style) {
    bg_color_ = style.fill_color;
    text_color_ = style.text_color;
    header_text_color_ = style.text_color;
    if (style.stroke_color.a > 0) {
        border_color_ = style.stroke_color;
        column_line_color_ = style.stroke_color;
        row_line_color_ = style.stroke_color;
    }
    if (style.stroke_thickness > 0) {
        column_line_thickness_ = style.stroke_thickness;
        row_line_thickness_ = style.stroke_thickness;
    }

    header_bg_color_ = Color{
        static_cast<uint8_t>(std::clamp(bg_color_.r + 15, 0, 255)),
        static_cast<uint8_t>(std::clamp(bg_color_.g + 15, 0, 255)),
        static_cast<uint8_t>(std::clamp(bg_color_.b + 20, 0, 255)),
        bg_color_.a
    };
    alt_row_bg_color_ = Color{
        static_cast<uint8_t>(std::clamp(bg_color_.r + 5, 0, 255)),
        static_cast<uint8_t>(std::clamp(bg_color_.g + 5, 0, 255)),
        static_cast<uint8_t>(std::clamp(bg_color_.b + 5, 0, 255)),
        bg_color_.a
    };

    if (bg_) {
        bg_->set_fill_color(bg_color_);
        bg_->set_stroke_color(border_color_);
    }
    if (header_bg_) {
        header_bg_->set_fill_color(header_bg_color_);
    }

    update_layout_elements();
}

void DataGrid::update_layout_elements() {
    clear_children();

    header_texts_.clear();
    header_dividers_.clear();
    column_dividers_.clear();
    row_dividers_.clear();
    cell_bgs_.clear();
    cell_elements_.clear();

    if (columns_.empty()) {
        return;
    }

    int header_height = row_height_ + 6;
    int total_col_width = 0;
    for (const auto& col : columns_) {
        total_col_width += col.width;
    }

    int viewport_w = bounds_.width;
    int viewport_h = bounds_.height - header_height;

    bool need_v = (get_row_count() * row_height_) > viewport_h;
    bool need_h = total_col_width > viewport_w;

    if (need_v) {
        viewport_w = std::max(0, viewport_w - 12);
    }
    if (need_h) {
        viewport_h = std::max(0, viewport_h - 12);
    }
    need_v = (get_row_count() * row_height_) > viewport_h;
    need_h = total_col_width > viewport_w;

    visible_rows_count_ = row_height_ > 0 ? (viewport_h / row_height_) : 1;
    if (visible_rows_count_ <= 0) visible_rows_count_ = 1;

    Rect v_scroll_bounds = need_v ? Rect{bounds_.x + bounds_.width - 12, bounds_.y + header_height, 12, viewport_h} : Rect{0, 0, 0, 0};
    Rect h_scroll_bounds = need_h ? Rect{bounds_.x, bounds_.y + bounds_.height - 12, viewport_w, 12} : Rect{0, 0, 0, 0};

    v_scroll_->set_range(0, get_row_count(), visible_rows_count_);
    v_scroll_->layout(v_scroll_bounds);

    h_scroll_->set_range(0, total_col_width, viewport_w);
    h_scroll_->layout(h_scroll_bounds);

    int col_x_offset = bounds_.x - scroll_offset_x_;

    // Horizontal divider under header
    auto header_sep = std::make_shared<LinePrimitive>(
        Point{bounds_.x, bounds_.y + header_height},
        Point{bounds_.x + viewport_w, bounds_.y + header_height},
        border_color_,
        1.5f
    );
    header_dividers_.push_back(header_sep);

    for (size_t col_idx = 0; col_idx < columns_.size(); ++col_idx) {
        const auto& col = columns_[col_idx];

        if (col_x_offset + col.width < bounds_.x || col_x_offset > bounds_.x + viewport_w) {
            col_x_offset += col.width;
            continue;
        }

        int cell_x = std::max(bounds_.x, col_x_offset);
        int cell_r = std::min(bounds_.x + viewport_w, col_x_offset + col.width);
        int cell_w = cell_r - cell_x;
        if (cell_w <= 0) {
            col_x_offset += col.width;
            continue;
        }

        Point text_pos{col_x_offset + 5, bounds_.y + (header_height - font_.size) / 2};
        if (text_pos.x >= bounds_.x && text_pos.x + 10 < bounds_.x + viewport_w) {
            auto txt = std::make_shared<TextPrimitive>(col.header, font_, text_pos, header_text_color_);
            header_texts_.push_back(txt);
        }

        if (col_idx < columns_.size() - 1) {
            int div_x = col_x_offset + col.width;
            if (div_x >= bounds_.x && div_x <= bounds_.x + viewport_w) {
                // Header vertical divider
                auto h_div = std::make_shared<LinePrimitive>(Point{div_x, bounds_.y}, Point{div_x, bounds_.y + header_height}, border_color_, 1.0f);
                header_dividers_.push_back(h_div);

                // Data vertical divider
                auto col_div = std::make_shared<LinePrimitive>(
                    Point{div_x, bounds_.y + header_height},
                    Point{div_x, bounds_.y + bounds_.height - (need_h ? 12 : 0)},
                    column_line_color_,
                    column_line_thickness_,
                    column_line_style_
                );
                column_dividers_.push_back(col_div);
            }
        }

        col_x_offset += col.width;
    }

    // SOLID Principles: Single Responsibility Principle (SRP)
    // Ensure the persistent cell_cache_ has dimensions matching the total row and column counts.
    // This decouples viewport visibility/culling from actual cell element lifetime management.
    if (cell_cache_.size() != static_cast<size_t>(get_row_count())) {
        cell_cache_.resize(get_row_count());
    }
    for (auto& row_cache : cell_cache_) {
        if (row_cache.size() != columns_.size()) {
            row_cache.resize(columns_.size());
        }
    }

    cell_bgs_.resize(visible_rows_count_);
    cell_elements_.resize(visible_rows_count_);

    for (int r = 0; r < visible_rows_count_; ++r) {
        cell_bgs_[r].clear();
        cell_elements_[r].clear();

        int row_idx = scroll_offset_y_ + r;
        int row_y = bounds_.y + header_height + r * row_height_;
        if (row_y + row_height_ > bounds_.y + bounds_.height - (need_h ? 12 : 0)) {
            break;
        }

        col_x_offset = bounds_.x - scroll_offset_x_;
        for (size_t col_idx = 0; col_idx < columns_.size(); ++col_idx) {
            const auto& col = columns_[col_idx];
            if (col_x_offset + col.width < bounds_.x || col_x_offset > bounds_.x + viewport_w) {
                col_x_offset += col.width;
                continue;
            }

            int cell_x = std::max(bounds_.x, col_x_offset);
            int cell_r = std::min(bounds_.x + viewport_w, col_x_offset + col.width);
            int cell_w = cell_r - cell_x;
            if (cell_w <= 0) {
                col_x_offset += col.width;
                continue;
            }

            Color bg_col = (r % 2 == 0) ? bg_color_ : alt_row_bg_color_;
            Rect cell_rect{cell_x, row_y + 1, cell_w, row_height_ - 2};
            auto cell_bg = std::make_shared<RectPrimitive>(cell_rect, bg_col);
            cell_bgs_[r].push_back(cell_bg);

            // Fetch the cell from cell_cache_ if it was already instantiated for this position.
            // This prevents re-creation of interactive cells across layout invalidation passes,
            // preserving state (focus, cursor position, selection, editing mode) and adhering to LSP/DIP.
            std::shared_ptr<gooey::mvvmc::GooeyElement> cell_el;
            if (row_idx >= 0 && row_idx < static_cast<int>(cell_cache_.size()) &&
                col_idx < cell_cache_[row_idx].size() && cell_cache_[row_idx][col_idx]) {
                cell_el = cell_cache_[row_idx][col_idx];
            } else {
                cell_el = col.cell_factory ? col.cell_factory() : std::make_shared<Label>();
                if (row_idx >= 0 && row_idx < static_cast<int>(cell_cache_.size()) &&
                    col_idx < cell_cache_[row_idx].size()) {
                    cell_cache_[row_idx][col_idx] = cell_el;
                }
            }

            cell_el->set_absolute(true);
            cell_el->set_absolute_bounds(cell_rect);
            if (!col.cell_factory) {
                auto lbl = std::dynamic_pointer_cast<Label>(cell_el);
                if (lbl) {
                    lbl->set_font(font_);
                    lbl->set_color(text_color_);
                    lbl->padding_left = 5;
                }
            }
            cell_el->layout(cell_rect);
            cell_elements_[r].push_back(cell_el);

            col_x_offset += col.width;
        }

        // Horizontal divider under this row
        int div_y = row_y + row_height_;
        if (div_y >= bounds_.y + header_height && div_y <= bounds_.y + bounds_.height - (need_h ? 12 : 0)) {
            auto row_div = std::make_shared<LinePrimitive>(
                Point{bounds_.x, div_y},
                Point{bounds_.x + viewport_w, div_y},
                row_line_color_,
                row_line_thickness_,
                row_line_style_
            );
            row_dividers_.push_back(row_div);
        }
    }

    // Add children in strict z-order
    // 1. General backgrounds
    add_child(bg_);
    add_child(header_bg_);

    // 2. Cell backgrounds
    for (const auto& row_bgs : cell_bgs_) {
        for (const auto& cell_bg : row_bgs) {
            add_child(cell_bg);
        }
    }

    // 3. Grid separation lines (on top of cell bgs)
    if (show_column_lines_) {
        for (const auto& divider : column_dividers_) {
            add_child(divider);
        }
    }
    if (show_row_lines_) {
        for (const auto& divider : row_dividers_) {
            add_child(divider);
        }
    }

    // 4. Header separators/dividers
    for (const auto& divider : header_dividers_) {
        add_child(divider);
    }

    // 5. Custom elements and labels (always on top of cell backgrounds and grid lines)
    for (const auto& txt : header_texts_) {
        add_child(txt);
    }
    for (const auto& row_els : cell_elements_) {
        for (const auto& cell_el : row_els) {
            add_child(cell_el);
        }
    }

    // 6. Interactive scrollbars (on very top)
    add_child(v_scroll_);
    add_child(h_scroll_);

    update_cell_values();
}

void DataGrid::update_cell_values() {
    int header_height = row_height_ + 6;
    int viewport_w = bounds_.width - (v_scroll_->bounds().width > 0 ? 12 : 0);

    for (int r = 0; r < visible_rows_count_; ++r) {
        int row_idx = scroll_offset_y_ + r;
        bool has_row = row_idx < get_row_count();

        size_t cell_idx = 0;
        int col_x_offset = bounds_.x - scroll_offset_x_;
        for (size_t col_idx = 0; col_idx < columns_.size(); ++col_idx) {
            const auto& col = columns_[col_idx];

            if (col_x_offset + col.width < bounds_.x || col_x_offset > bounds_.x + viewport_w) {
                col_x_offset += col.width;
                continue;
            }

            if (r < static_cast<int>(cell_elements_.size()) && cell_idx < cell_elements_[r].size()) {
                auto& cell_el = cell_elements_[r][cell_idx];
                if (has_row) {
                    if (!items_.empty()) {
                        if (col.cell_binder) {
                            col.cell_binder(cell_el, items_[row_idx], row_idx);
                        }
                    } else if (col_idx < rows_[row_idx].size()) {
                        if (col.cell_binder) {
                            col.cell_binder(cell_el, rows_[row_idx][col_idx], row_idx);
                        } else {
                            auto lbl = std::dynamic_pointer_cast<Label>(cell_el);
                            if (lbl) {
                                lbl->set_text(rows_[row_idx][col_idx]);
                            }
                        }
                    }
                } else {
                    if (col.cell_binder) {
                        col.cell_binder(cell_el, std::any(), row_idx);
                    } else {
                        auto lbl = std::dynamic_pointer_cast<Label>(cell_el);
                        if (lbl) {
                            lbl->set_text("");
                        }
                    }
                }
                cell_idx++;
            }

            col_x_offset += col.width;
        }
    }
}

} // namespace gooey::controls
