#include "tooey/lexer.hpp"
#include "tooey/parser.hpp"
#include "tooey/ast.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <set>
#include <map>
#include <string>
#include <vector>

using namespace tooey;

// Core built-in control types
const std::set<std::string> KNOWN_ELEMENTS = {
    "VBox", "Column", "HBox", "Row", "Grid", "FlowLayout",
    "Button", "CheckBox", "Label", "TextBox", "RichTextBox",
    "ImageControl", "ScrollBar", "ScrollContainer", "ListControl",
    "DataGrid", "AdaptiveStack", "CanvasLayout", "VectorShapeView",
    "Root"
};

// Check for duplicate IDs
void check_duplicate_ids(const std::shared_ptr<AstNode>& node, 
                         std::map<std::string, const AstNode*>& id_map,
                         const std::string& filename,
                         int& errors) {
    if (!node) return;
    if (!node->id.empty()) {
        auto it = id_map.find(node->id);
        if (it != id_map.end()) {
            std::cerr << filename << ":" << node->line << ":" << node->column 
                      << ": error: Duplicate element ID '" << node->id << "' defined. "
                      << "Previously defined at line " << it->second->line << ".\n";
            errors++;
        } else {
            id_map[node->id] = node.get();
        }
    }
    for (const auto& child : node->children) {
        check_duplicate_ids(child, id_map, filename, errors);
    }
}

// Check for unrecognized element types
void check_element_types(const std::shared_ptr<AstNode>& node,
                         const std::set<std::string>& custom_components,
                         const std::string& filename,
                         int& warnings) {
    if (!node) return;
    if (node->nodeType != "Root" && KNOWN_ELEMENTS.find(node->nodeType) == KNOWN_ELEMENTS.end()) {
        if (custom_components.find(node->nodeType) == custom_components.end()) {
            std::cerr << filename << ":" << node->line << ":" << node->column
                      << ": warning: Unrecognized element type '" << node->nodeType << "'. "
                      << "Verify if it is a missing custom component.\n";
            warnings++;
        }
    }
    for (const auto& child : node->children) {
        check_element_types(child, custom_components, filename, warnings);
    }
}

// Scans localization.hpp to extract built-in keys
std::set<std::string> get_builtin_keys(const std::string& current_dir) {
    std::set<std::string> keys;
    std::vector<std::string> candidate_paths = {
        "gooey/include/gooey/mvvmc/localization.hpp",
        "../gooey/include/gooey/mvvmc/localization.hpp",
        "../../gooey/include/gooey/mvvmc/localization.hpp",
        current_dir + "/gooey/include/gooey/mvvmc/localization.hpp",
        current_dir + "/../gooey/include/gooey/mvvmc/localization.hpp"
    };
    for (const auto& path : candidate_paths) {
        std::ifstream ifs(path);
        if (ifs.is_open()) {
            std::string line;
            while (std::getline(ifs, line)) {
                size_t p1 = line.find("{\"");
                if (p1 != std::string::npos) {
                    size_t p2 = line.find("\",", p1 + 2);
                    if (p2 != std::string::npos) {
                        keys.insert(line.substr(p1 + 2, p2 - (p1 + 2)));
                    }
                }
            }
            break;
        }
    }
    return keys;
}

// Check for missing localization keys in translation dict
void check_localization_keys(const std::shared_ptr<AstNode>& node,
                             const std::set<std::string>& builtin_keys,
                             const std::string& filename,
                             int& warnings) {
    if (!node) return;
    for (const auto& prop : node->properties) {
        if (prop.second.type == PropertyType::Localization) {
            std::string key = prop.second.rawData;
            if (builtin_keys.find(key) == builtin_keys.end()) {
                std::cerr << filename << ":" << node->line << ":" << node->column
                          << ": warning: Localization key '" << key 
                          << "' is missing from built-in translation dictionaries.\n";
                warnings++;
            }
        }
    }
    for (const auto& child : node->children) {
        check_localization_keys(child, builtin_keys, filename, warnings);
    }
}

// Helper to check for basic syntax issues
void run_syntax_diagnostics(const std::vector<Token>& tokens, const std::string& filename, int& errors) {
    int line_num = 1;
    for (const auto& tok : tokens) {
        if (tok.type == TokenType::UNKNOWN) {
            std::cerr << filename << ":" << tok.line << ":" << tok.column
                      << ": error: Unexpected symbol or invalid syntax: '" << tok.text << "'\n";
            errors++;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file.ooey>\n";
        return 1;
    }

    std::string input_path = argv[1];
    std::filesystem::path file_path(input_path);
    if (!std::filesystem::exists(file_path)) {
        std::cerr << "Error: File does not exist: " << input_path << "\n";
        return 1;
    }

    std::ifstream ifs(input_path);
    if (!ifs.is_open()) {
        std::cerr << "Error: Could not open file: " << input_path << "\n";
        return 1;
    }

    std::string source((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();

    std::string current_dir = file_path.parent_path().string();
    if (current_dir.empty()) {
        current_dir = ".";
    }

    // Identify custom components in the directory
    std::set<std::string> custom_components;
    if (std::filesystem::exists(current_dir) && std::filesystem::is_directory(current_dir)) {
        for (const auto& entry : std::filesystem::directory_iterator(current_dir)) {
            if (entry.is_regular_file() && (entry.path().extension() == ".ooey" || entry.path().extension() == ".OOEY")) {
                custom_components.insert(entry.path().stem().string());
            }
        }
    }

    int errors = 0;
    int warnings = 0;

    try {
        auto tokens = Lexer::tokenize(source);
        run_syntax_diagnostics(tokens, input_path, errors);

        auto ast = Parser::parse(tokens, current_dir);
        if (ast) {
            std::map<std::string, const AstNode*> id_map;
            check_duplicate_ids(ast, id_map, input_path, errors);

            check_element_types(ast, custom_components, input_path, warnings);

            auto builtin_keys = get_builtin_keys(current_dir);
            check_localization_keys(ast, builtin_keys, input_path, warnings);
        } else {
            std::cerr << input_path << ":1:1: error: AST generation failed.\n";
            errors++;
        }
    } catch (const std::exception& e) {
        std::cerr << input_path << ":1:1: error: Lint check failed due to exception: " << e.what() << "\n";
        errors++;
    }

    std::cout << "Lint summary: " << errors << " error(s), " << warnings << " warning(s).\n";

    return (errors > 0) ? 1 : 0;
}
