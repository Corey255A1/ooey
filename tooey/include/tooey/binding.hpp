#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include "gooey/mvvmc/property.hpp"
#include "gooey/mvvmc/gooey_node.hpp"
#include "gooey/mvvmc/gooey_element.hpp"

// SFINAE helpers to check if Control type C has member function set_text, set_label_text, set_value, or set_items
namespace tooey {

template <typename C, typename T, typename = void>
struct has_set_text : std::false_type {};

template <typename C, typename T>
struct has_set_text<C, T, std::void_t<decltype(std::declval<C>().set_text(std::declval<T>()))>> : std::true_type {};

template <typename C, typename T, typename = void>
struct has_set_label_text : std::false_type {};

template <typename C, typename T>
struct has_set_label_text<C, T, std::void_t<decltype(std::declval<C>().set_label_text(std::declval<T>()))>> : std::true_type {};

template <typename C, typename T, typename = void>
struct has_set_value : std::false_type {};

template <typename C, typename T>
struct has_set_value<C, T, std::void_t<decltype(std::declval<C>().set_value(std::declval<T>()))>> : std::true_type {};

template <typename C, typename T, typename = void>
struct has_set_items : std::false_type {};

template <typename C, typename T>
struct has_set_items<C, T, std::void_t<decltype(std::declval<C>().set_items(std::declval<T>()))>> : std::true_type {};

template <typename Control, typename T>
void set_control_value(std::shared_ptr<Control> control, const T& val) {
    if (!control) return;
    if constexpr (has_set_value<Control, T>::value) {
        control->set_value(val);
    } else if constexpr (has_set_items<Control, T>::value) {
        control->set_items(val);
    } else if constexpr (has_set_text<Control, T>::value) {
        control->set_text(val);
    } else if constexpr (has_set_label_text<Control, T>::value) {
        control->set_label_text(val);
    } else if constexpr (has_set_text<Control, std::string>::value) {
        if constexpr (std::is_same_v<T, std::string>) {
            control->set_text(val);
        } else {
            using namespace std;
            control->set_text(to_string(val));
        }
    } else if constexpr (has_set_label_text<Control, std::string>::value) {
        if constexpr (std::is_same_v<T, std::string>) {
            control->set_label_text(val);
        } else {
            using namespace std;
            control->set_label_text(to_string(val));
        }
    }
}

template <typename Control, typename T>
void operator<<=(std::shared_ptr<Control> control, gooey::mvvmc::Property<T>& property) {
    if (!control) return;
    
    // Capture control weakly to break cyclic references between ViewModel properties and View hierarchies
    std::weak_ptr<Control> weak_control = control;
    control->bind(property, [weak_control](const T& val) {
        if (auto ctrl = weak_control.lock()) {
            if constexpr (has_set_value<Control, T>::value) {
                ctrl->set_value(val);
            } else if constexpr (has_set_items<Control, T>::value) {
                ctrl->set_items(val);
            } else if constexpr (has_set_text<Control, T>::value) {
                ctrl->set_text(val);
            } else if constexpr (has_set_label_text<Control, T>::value) {
                ctrl->set_label_text(val);
            } else if constexpr (has_set_text<Control, std::string>::value) {
                if constexpr (std::is_same_v<T, std::string>) {
                    ctrl->set_text(val);
                } else {
                    using namespace std;
                    ctrl->set_text(to_string(val));
                }
            } else if constexpr (has_set_label_text<Control, std::string>::value) {
                if constexpr (std::is_same_v<T, std::string>) {
                    ctrl->set_label_text(val);
                } else {
                    using namespace std;
                    ctrl->set_label_text(to_string(val));
                }
            }
        }
    });
}

inline std::shared_ptr<gooey::mvvmc::GooeyElement> find_element_by_id(const std::shared_ptr<gooey::mvvmc::GooeyElement>& root, const std::string& id) {
    if (!root) return nullptr;
    if (root->id == id) return root;
    if (auto node = std::dynamic_pointer_cast<gooey::mvvmc::GooeyNode>(root)) {
        for (const auto& child_drawable : node->get_children()) {
            if (auto child_element = std::dynamic_pointer_cast<gooey::mvvmc::GooeyElement>(child_drawable)) {
                auto found = find_element_by_id(child_element, id);
                if (found) return found;
            }
        }
    }
    return nullptr;
}

template <typename Control, typename T>
void update_control_value_if_different(std::shared_ptr<Control> control, const T& val) {
    if (!control) return;
    if constexpr (has_set_value<Control, T>::value) {
        if (control->get_value() != val) {
            control->set_value(val);
        }
    } else if constexpr (has_set_items<Control, T>::value) {
        if (control->get_items() != val) {
            control->set_items(val);
        }
    } else if constexpr (has_set_text<Control, T>::value) {
        if (control->get_text() != val) {
            control->set_text(val);
        }
    } else if constexpr (has_set_label_text<Control, T>::value) {
        if (control->get_text() != val) {
            control->set_label_text(val);
        }
    } else if constexpr (has_set_text<Control, std::string>::value) {
        if constexpr (std::is_same_v<T, std::string>) {
            if (control->get_text() != val) {
                control->set_text(val);
            }
        } else {
            using namespace std;
            if (control->get_text() != to_string(val)) {
                control->set_text(to_string(val));
            }
        }
    } else if constexpr (has_set_label_text<Control, std::string>::value) {
        if constexpr (std::is_same_v<T, std::string>) {
            if (control->get_text() != val) {
                control->set_label_text(val);
            }
        } else {
            using namespace std;
            if (control->get_text() != to_string(val)) {
                control->set_label_text(to_string(val));
            }
        }
    }
}

} // namespace tooey

