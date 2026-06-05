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

namespace gooey::mvvmc {

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

        // Common layout sizing properties
        if (prop_name == "width") {
            if (value == "MatchParent") element->set_width(gooey::SizePolicy::MatchParent);
            else if (value == "WrapContent") element->set_width(gooey::SizePolicy::WrapContent);
            else {
                try {
                    element->set_width(gooey::SizePolicy::Fixed, std::stoi(value));
                } catch (...) {}
            }
            return true;
        }
        if (prop_name == "height") {
            if (value == "MatchParent") element->set_height(gooey::SizePolicy::MatchParent);
            else if (value == "WrapContent") element->set_height(gooey::SizePolicy::WrapContent);
            else {
                try {
                    element->set_height(gooey::SizePolicy::Fixed, std::stoi(value));
                } catch (...) {}
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
