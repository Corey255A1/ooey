#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>

namespace tooey {

enum class PropertyType : std::uint8_t {
    String,
    Number,
    Boolean,
    Binding,
    Signal,
    Theme,
    Localization
};

inline std::string to_string(PropertyType type) {
    switch (type) {
        case PropertyType::String: return "String";
        case PropertyType::Number: return "Number";
        case PropertyType::Boolean: return "Boolean";
        case PropertyType::Binding: return "Binding";
        case PropertyType::Signal: return "Signal";
        case PropertyType::Theme: return "Theme";
        case PropertyType::Localization: return "Localization";
    }
    return "Unknown";
}

struct PropertyValue {
    PropertyType type;
    std::string rawData;
};

struct AstNode {
    std::string nodeType;   // e.g. Button, Column, VBox
    std::string id;         // Optional component id
    std::string aiHint;     // Optional AI prompt hint
    std::map<std::string, PropertyValue> properties;
    std::vector<std::shared_ptr<AstNode>> children;
    
    // For error reporting and debugging
    int line = 0;
    int column = 0;

    // Support for reusable custom components
    bool isCustomComponent = false;
    std::vector<std::string> customIncludes;
};

} // namespace tooey
