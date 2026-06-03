#pragma once

#include <functional>
#include <map>
#include <cstdint>
#include <memory>
#include <variant>
#include <string>
#include <vector>
#include <type_traits>
#include "gooey/mvvmc/scoped_subscription.hpp"
#include "ooey/types.hpp"

namespace gooey {
    void request_render();
}

namespace gooey::mvvmc {
    using namespace ooey;

// Dynamic representation of a property value
using PropertyValue = std::variant<
    int,
    float,
    double,
    bool,
    std::string,
    ooey::Color,
    ooey::Font
>;

// Type-erased base interface for properties to enable dynamic reflection and lookup
class PropertyBase {
public:
    virtual ~PropertyBase() = default;

    virtual PropertyValue get_value() const = 0;
    virtual void set_value(const PropertyValue& val) = 0;
    virtual ScopedSubscription subscribe_dynamic(std::function<void(const PropertyValue&)> listener) = 0;
    virtual std::string get_type_name() const = 0;
};

// Forward declaration of Property
template <typename T>
class Property;

// Registry class to organize properties and nested registries hierarchically
class PropertyRegistry {
public:
    virtual ~PropertyRegistry() = default;

    void register_property(const std::string& name, PropertyBase* prop) {
        properties_[name] = prop;
    }

    void register_sub_registry(const std::string& name, PropertyRegistry* sub_registry) {
        sub_registries_[name] = sub_registry;
    }

    PropertyBase* resolve_path(const std::string& path) const {
        size_t dot_pos = path.find('.');
        if (dot_pos == std::string::npos) {
            auto it = properties_.find(path);
            if (it != properties_.end()) {
                return it->second;
            }
            return nullptr;
        }

        std::string sub_name = path.substr(0, dot_pos);
        std::string sub_path = path.substr(dot_pos + 1);

        auto it = sub_registries_.find(sub_name);
        if (it != sub_registries_.end()) {
            return it->second->resolve_path(sub_path);
        }
        return nullptr;
    }

    std::vector<std::string> get_property_paths() const {
        std::vector<std::string> paths;
        for (const auto& kv : properties_) {
            paths.push_back(kv.first);
        }
        for (const auto& kv : sub_registries_) {
            std::vector<std::string> sub_paths = kv.second->get_property_paths();
            for (const auto& sp : sub_paths) {
                paths.push_back(kv.first + "." + sp);
            }
        }
        return paths;
    }

private:
    std::map<std::string, PropertyBase*> properties_;
    std::map<std::string, PropertyRegistry*> sub_registries_;
};

// Base class for MVVM ViewModels to participate in reflection
class ViewModel : public PropertyRegistry {
public:
    virtual ~ViewModel() override = default;
};

// Coercion helper for PropertyValue variants
template <typename T>
struct PropertyValueConverter {
    static bool convert(const PropertyValue& from, T& to) {
        if (std::holds_alternative<T>(from)) {
            to = std::get<T>(from);
            return true;
        }
        return false;
    }
};

template <>
struct PropertyValueConverter<double> {
    static bool convert(const PropertyValue& from, double& to) {
        if (std::holds_alternative<double>(from)) {
            to = std::get<double>(from);
            return true;
        } else if (std::holds_alternative<float>(from)) {
            to = static_cast<double>(std::get<float>(from));
            return true;
        } else if (std::holds_alternative<int>(from)) {
            to = static_cast<double>(std::get<int>(from));
            return true;
        }
        return false;
    }
};

template <>
struct PropertyValueConverter<float> {
    static bool convert(const PropertyValue& from, float& to) {
        if (std::holds_alternative<float>(from)) {
            to = std::get<float>(from);
            return true;
        } else if (std::holds_alternative<double>(from)) {
            to = static_cast<float>(std::get<double>(from));
            return true;
        } else if (std::holds_alternative<int>(from)) {
            to = static_cast<float>(std::get<int>(from));
            return true;
        }
        return false;
    }
};

template <>
struct PropertyValueConverter<int> {
    static bool convert(const PropertyValue& from, int& to) {
        if (std::holds_alternative<int>(from)) {
            to = std::get<int>(from);
            return true;
        } else if (std::holds_alternative<double>(from)) {
            to = static_cast<int>(std::get<double>(from));
            return true;
        } else if (std::holds_alternative<float>(from)) {
            to = static_cast<int>(std::get<float>(from));
            return true;
        }
        return false;
    }
};

template <typename T>
class Property : public PropertyBase {
public:
    using Listener = std::function<void(const T&)>;

    Property() : alive_flag_(std::make_shared<bool>(true)) {}
    Property(T initial_value) : value_(std::move(initial_value)), alive_flag_(std::make_shared<bool>(true)) {}

    virtual ~Property() override {
        if (alive_flag_) {
            *alive_flag_ = false;
        }
    }

    Property(const Property&) = delete;
    Property& operator=(const Property&) = delete;
    Property(Property&&) = delete;
    Property& operator=(Property&&) = delete;

    ScopedSubscription subscribe(Listener listener) {
        uint32_t id = next_id_++;
        listeners_[id] = std::move(listener);
        listeners_[id](value_); // Initial sync

        std::weak_ptr<bool> weak_alive = alive_flag_;
        return ScopedSubscription([this, id, weak_alive]() {
            if (auto alive = weak_alive.lock()) {
                if (*alive) {
                    this->unsubscribe(id);
                }
            }
        });
    }

    void unsubscribe(uint32_t id) {
        listeners_.erase(id);
    }

    void set(T new_value) {
        value_ = std::move(new_value);
        notify();
        gooey::request_render();
    }

    const T& get() const {
        return value_;
    }

    // PropertyBase overrides
    PropertyValue get_value() const override {
        if constexpr (std::is_constructible_v<PropertyValue, T>) {
            return value_;
        } else {
            return {};
        }
    }

    void set_value(const PropertyValue& val) override {
        if constexpr (std::is_constructible_v<PropertyValue, T>) {
            T converted{};
            if (PropertyValueConverter<T>::convert(val, converted)) {
                set(std::move(converted));
            }
        }
    }

    ScopedSubscription subscribe_dynamic(std::function<void(const PropertyValue&)> listener) override {
        if constexpr (std::is_constructible_v<PropertyValue, T>) {
            return subscribe([listener = std::move(listener)](const T& val) {
                listener(PropertyValue{val});
            });
        } else {
            return {};
        }
    }

    std::string get_type_name() const override {
        if constexpr (std::is_same_v<T, int>) return "int";
        else if constexpr (std::is_same_v<T, float>) return "float";
        else if constexpr (std::is_same_v<T, double>) return "double";
        else if constexpr (std::is_same_v<T, bool>) return "bool";
        else if constexpr (std::is_same_v<T, std::string>) return "string";
        else if constexpr (std::is_same_v<T, ooey::Color>) return "Color";
        else if constexpr (std::is_same_v<T, ooey::Font>) return "Font";
        else return "unknown";
    }

private:
    void notify() {
        for (auto& kv : listeners_) {
            kv.second(value_);
        }
    }

    T value_;
    std::map<uint32_t, Listener> listeners_;
    uint32_t next_id_{0};
    std::shared_ptr<bool> alive_flag_;
};

} // namespace gooey::mvvmc

namespace gooey {
    using namespace ooey;
using gooey::mvvmc::Property;
using gooey::mvvmc::ScopedSubscription;
using gooey::mvvmc::PropertyBase;
using gooey::mvvmc::PropertyValue;
using gooey::mvvmc::PropertyRegistry;
using gooey::mvvmc::ViewModel;
}
