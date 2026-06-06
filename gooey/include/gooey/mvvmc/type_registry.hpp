#pragma once

#include "gooey/mvvmc/gooey_element.hpp"
#include "gooey/mvvmc/gooey_node.hpp"
#include "gooey/controls/button.hpp"
#include "gooey/controls/checkbox.hpp"
#include "gooey/controls/label.hpp"
#include "gooey/controls/text_box.hpp"
#include "gooey/controls/column.hpp"
#include "gooey/controls/row.hpp"
#include "gooey/controls/grid.hpp"
#include "gooey/controls/flow_layout.hpp"
#include "gooey/controls/scrollbar.hpp"
#include "gooey/controls/scroll_container.hpp"
#include "gooey/controls/list_control.hpp"

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <algorithm>
#include <sstream>
#include <vector>

namespace gooey::mvvmc {

inline void parse_sizing_value(const std::string& val_str, gooey::SizePolicy& out_policy, float& out_value) {
    if (val_str == "MatchParent") {
        out_policy = gooey::SizePolicy::MatchParent;
        out_value = 0.0f;
    } else if (val_str == "WrapContent") {
        out_policy = gooey::SizePolicy::WrapContent;
        out_value = 0.0f;
    } else if (!val_str.empty() && val_str.back() == '%') {
        out_policy = gooey::SizePolicy::Percentage;
        try {
            out_value = std::stof(val_str.substr(0, val_str.size() - 1));
        } catch (...) { out_value = 0.0f; }
    } else if (!val_str.empty() && val_str.back() == '*') {
        out_policy = gooey::SizePolicy::Flex;
        if (val_str.size() == 1) {
            out_value = 1.0f;
        } else {
            try {
                out_value = std::stof(val_str.substr(0, val_str.size() - 1));
            } catch (...) { out_value = 1.0f; }
        }
    } else {
        out_policy = gooey::SizePolicy::Fixed;
        try {
            out_value = std::stof(val_str);
        } catch (...) { out_value = 0.0f; }
    }
}

inline gooey::Align parse_align_enum(const std::string& val_str) {
    if (val_str == "Inherit") return gooey::Align::Inherit;
    if (val_str == "Start") return gooey::Align::Start;
    if (val_str == "Center") return gooey::Align::Center;
    if (val_str == "End") return gooey::Align::End;
    if (val_str == "Stretch") return gooey::Align::Stretch;
    return gooey::Align::Start;
}

inline gooey::Justify parse_justify_enum(const std::string& val_str) {
    if (val_str == "Start") return gooey::Justify::Start;
    if (val_str == "Center") return gooey::Justify::Center;
    if (val_str == "End") return gooey::Justify::End;
    if (val_str == "SpaceBetween") return gooey::Justify::SpaceBetween;
    if (val_str == "SpaceAround") return gooey::Justify::SpaceAround;
    if (val_str == "SpaceEvenly") return gooey::Justify::SpaceEvenly;
    return gooey::Justify::Start;
}

inline void parse_spacing_shorthand(const std::string& val_str, int& left, int& top, int& right, int& bottom) {
    std::vector<int> vals;
    std::stringstream ss(val_str);
    std::string token;
    while (ss >> token) {
        try {
            vals.push_back(std::stoi(token));
        } catch (...) {}
    }
    if (vals.empty()) return;
    if (vals.size() == 1) {
        left = top = right = bottom = vals[0];
    } else if (vals.size() == 2) {
        top = bottom = vals[0];
        left = right = vals[1];
    } else if (vals.size() >= 4) {
        top = vals[0];
        right = vals[1];
        bottom = vals[2];
        left = vals[3];
    }
}

class TypeRegistry {
public:
    using Factory = std::function<std::shared_ptr<GooeyElement>()>;
    using PropertySetter = std::function<void(const std::shared_ptr<GooeyElement>&, const std::string&)>;

    static TypeRegistry& get_instance() {
        static TypeRegistry instance;
        return instance;
    }

    void register_type(const std::string& type_name, Factory factory) {
        factories_[type_name] = factory;
    }

    void register_property(const std::string& type_name, const std::string& prop_name, PropertySetter setter) {
        properties_[type_name][prop_name] = setter;
    }

    std::shared_ptr<GooeyElement> create(const std::string& type_name) {
        auto it = factories_.find(type_name);
        if (it != factories_.end()) {
            return it->second();
        }
        return nullptr;
    }

    bool set_property(const std::shared_ptr<GooeyElement>& element, const std::string& type_name, const std::string& prop_name, const std::string& value) {
        if (!element) return false;

        if (prop_name == "width") {
            gooey::SizePolicy policy;
            float val;
            parse_sizing_value(value, policy, val);
            element->set_width(policy, val);
            return true;
        }
        if (prop_name == "height") {
            gooey::SizePolicy policy;
            float val;
            parse_sizing_value(value, policy, val);
            element->set_height(policy, val);
            return true;
        }
        if (prop_name == "minWidth") {
            try { element->set_min_width(std::stof(value)); } catch(...) {}
            return true;
        }
        if (prop_name == "maxWidth") {
            try { element->set_max_width(std::stof(value)); } catch(...) {}
            return true;
        }
        if (prop_name == "minHeight") {
            try { element->set_min_height(std::stof(value)); } catch(...) {}
            return true;
        }
        if (prop_name == "maxHeight") {
            try { element->set_max_height(std::stof(value)); } catch(...) {}
            return true;
        }
        if (prop_name == "margin") {
            int l=0, t=0, r=0, b=0;
            parse_spacing_shorthand(value, l, t, r, b);
            element->set_margin(l, t, r, b);
            return true;
        }
        if (prop_name == "padding") {
            int l=0, t=0, r=0, b=0;
            parse_spacing_shorthand(value, l, t, r, b);
            element->set_padding(l, t, r, b);
            return true;
        }
        if (prop_name == "marginLeft") {
            try { element->margin_left = std::stoi(value); } catch(...) {}
            return true;
        }
        if (prop_name == "marginTop") {
            try { element->margin_top = std::stoi(value); } catch(...) {}
            return true;
        }
        if (prop_name == "marginRight") {
            try { element->margin_right = std::stoi(value); } catch(...) {}
            return true;
        }
        if (prop_name == "marginBottom") {
            try { element->margin_bottom = std::stoi(value); } catch(...) {}
            return true;
        }
        if (prop_name == "paddingLeft") {
            try { element->padding_left = std::stoi(value); } catch(...) {}
            return true;
        }
        if (prop_name == "paddingTop") {
            try { element->padding_top = std::stoi(value); } catch(...) {}
            return true;
        }
        if (prop_name == "paddingRight") {
            try { element->padding_right = std::stoi(value); } catch(...) {}
            return true;
        }
        if (prop_name == "paddingBottom") {
            try { element->padding_bottom = std::stoi(value); } catch(...) {}
            return true;
        }
        if (prop_name == "alignSelf") {
            element->set_align_self(parse_align_enum(value));
            return true;
        }
        if (prop_name == "alignItems") {
            if (auto node = std::dynamic_pointer_cast<GooeyNode>(element)) {
                node->set_align_items(parse_align_enum(value));
            }
            return true;
        }
        if (prop_name == "justifyContent") {
            if (auto node = std::dynamic_pointer_cast<GooeyNode>(element)) {
                node->set_justify_content(parse_justify_enum(value));
            }
            return true;
        }

        auto it = properties_.find(type_name);
        if (it != properties_.end()) {
            auto prop_it = it->second.find(prop_name);
            if (prop_it != it->second.end()) {
                prop_it->second(element, value);
                return true;
            }
        }
        return false;
    }

    void register_standard_types() {
        register_type("Column", []() { return std::make_shared<gooey::Column>(); });
        register_type("VBox", []() { return std::make_shared<gooey::Column>(); });
        register_type("Row", []() { return std::make_shared<gooey::Row>(); });
        register_type("HBox", []() { return std::make_shared<gooey::Row>(); });
        register_type("Grid", []() { return std::make_shared<gooey::Grid>(2, 2); });
        register_type("FlowLayout", []() { return std::make_shared<gooey::FlowLayout>(); });
        register_type("Button", []() { return std::make_shared<gooey::controls::Button>(); });
        register_type("CheckBox", []() { return std::make_shared<gooey::controls::CheckBox>(); });
        register_type("Label", []() { return std::make_shared<gooey::controls::Label>(); });
        register_type("TextBox", []() { return std::make_shared<gooey::controls::TextBox>(); });
        register_type("ScrollBar", []() { return std::make_shared<gooey::controls::ScrollBar>(ooey::Rect{0, 0, 15, 100}, gooey::controls::ScrollBarOrientation::Vertical); });
        register_type("ScrollContainer", []() { return std::make_shared<gooey::controls::ScrollContainer>(); });
        register_type("ListControl", []() { return std::make_shared<gooey::controls::ListControl>(); });

        // Properties for Button
        register_property("Button", "text", [](const std::shared_ptr<GooeyElement>& el, const std::string& val) {
            if (auto btn = std::dynamic_pointer_cast<gooey::controls::Button>(el)) {
                btn->set_label_text(val);
            }
        });

        // Properties for Label
        register_property("Label", "text", [](const std::shared_ptr<GooeyElement>& el, const std::string& val) {
            if (auto lbl = std::dynamic_pointer_cast<gooey::controls::Label>(el)) {
                lbl->set_text(val);
            }
        });

        // Properties for CheckBox
        register_property("CheckBox", "text", [](const std::shared_ptr<GooeyElement>& el, const std::string& val) {
            if (auto cb = std::dynamic_pointer_cast<gooey::controls::CheckBox>(el)) {
                cb->set_label_text(val);
            }
        });
        register_property("CheckBox", "checked", [](const std::shared_ptr<GooeyElement>& el, const std::string& val) {
            if (auto cb = std::dynamic_pointer_cast<gooey::controls::CheckBox>(el)) {
                cb->set_checked(val == "true");
            }
        });

        // Properties for TextBox
        register_property("TextBox", "text", [](const std::shared_ptr<GooeyElement>& el, const std::string& val) {
            if (auto tb = std::dynamic_pointer_cast<gooey::controls::TextBox>(el)) {
                tb->set_text(val);
            }
        });

        // Spacing
        auto set_spacing_prop = [](const std::shared_ptr<GooeyElement>& el, const std::string& val) {
            try {
                if (auto col = std::dynamic_pointer_cast<gooey::Column>(el)) col->set_spacing(std::stoi(val));
                else if (auto row = std::dynamic_pointer_cast<gooey::Row>(el)) row->set_spacing(std::stoi(val));
            } catch (...) {}
        };
        register_property("Column", "spacing", set_spacing_prop);
        register_property("VBox", "spacing", set_spacing_prop);
        register_property("Row", "spacing", set_spacing_prop);
        register_property("HBox", "spacing", set_spacing_prop);
    }

private:
    TypeRegistry() {
        register_standard_types();
    }

    std::unordered_map<std::string, Factory> factories_;
    std::unordered_map<std::string, std::unordered_map<std::string, PropertySetter>> properties_;
};

} // namespace gooey::mvvmc
namespace gooey {
using gooey::mvvmc::TypeRegistry;
}
