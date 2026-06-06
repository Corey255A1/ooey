#include "tooey/linter.hpp"
#include "tooey/lexer.hpp"
#include "tooey/parser.hpp"
#include <sstream>
#include <map>

namespace tooey {

std::set<std::string> Linter::known_types_;
bool Linter::initialized_ = false;

void Linter::init_default_known_types() {
    if (!initialized_) {
        known_types_ = {
            "VBox", "Column", "HBox", "Row", "Grid", "FlowLayout",
            "Button", "CheckBox", "Label", "TextBox", "RichTextBox",
            "ImageControl", "ScrollBar", "ScrollContainer", "ListControl",
            "DataGrid", "AdaptiveStack", "CanvasLayout", "VectorShapeView",
            "Root"
        };
        initialized_ = true;
    }
}

void Linter::register_known_type(const std::string& type_name) {
    init_default_known_types();
    known_types_.insert(type_name);
}

void Linter::reset_known_types() {
    initialized_ = false;
    init_default_known_types();
}

std::vector<Diagnostic> Linter::run_diagnostics(const std::string& dsl) {
    init_default_known_types();
    std::vector<Diagnostic> diagnostics;

    auto tokens = Lexer::tokenize(dsl);

    // 1. Lexer errors (UNKNOWN tokens)
    for (const auto& tok : tokens) {
        if (tok.type == TokenType::UNKNOWN) {
            diagnostics.push_back({
                tok.line - 1,
                tok.column - 1,
                tok.column - 1 + static_cast<int>(tok.text.size()),
                ooey::Color{255, 0, 0, 255} // Red error
            });
        }
    }

    auto ast = Parser::parse(tokens);
    if (ast) {
        // Split input DSL to inspect raw line text for property values
        std::vector<std::string> lines;
        std::string line;
        std::stringstream ss(dsl);
        while (std::getline(ss, line)) {
            lines.push_back(line);
        }

        std::map<std::string, const AstNode*> id_map;

        // Recursive checker helper
        struct AstChecker {
            const std::vector<std::string>& lines;
            std::map<std::string, const AstNode*>& id_map;
            std::vector<Diagnostic>& diagnostics;

            void check(const std::shared_ptr<AstNode>& n) {
                if (!n) return;

                // 2. Duplicate ID check
                if (!n->id.empty()) {
                    auto it = id_map.find(n->id);
                    if (it != id_map.end()) {
                        diagnostics.push_back({
                            n->line - 1,
                            n->column - 1,
                            n->column - 1 + static_cast<int>(n->id.size()) + 4, // " id=" is 4 characters
                            ooey::Color{255, 0, 0, 255} // Red error
                        });
                    } else {
                        id_map[n->id] = n.get();
                    }
                }

                // 3. Unrecognized Control types check
                if (n->nodeType != "Root") {
                    if (known_types_.find(n->nodeType) == known_types_.end()) {
                        diagnostics.push_back({
                            n->line - 1,
                            n->column - 1,
                            n->column - 1 + static_cast<int>(n->nodeType.size()),
                            ooey::Color{255, 165, 0, 255} // Orange warning
                        });
                    }
                }

                // 4. Missing localization check
                for (const auto& prop : n->properties) {
                    if ((prop.first == "text" || prop.first == "title" || prop.first == "label") &&
                        prop.second.type == PropertyType::String) {
                        int line_idx = n->line - 1;
                        if (line_idx >= 0 && line_idx < static_cast<int>(lines.size())) {
                            std::string line_text = lines[line_idx];
                            size_t prop_pos = line_text.find(prop.first);
                            if (prop_pos != std::string::npos) {
                                size_t val_pos = line_text.find(prop.second.rawData, prop_pos);
                                if (val_pos != std::string::npos) {
                                    diagnostics.push_back({
                                        line_idx,
                                        static_cast<int>(val_pos),
                                        static_cast<int>(val_pos + prop.second.rawData.size()),
                                        ooey::Color{255, 165, 0, 255} // Orange warning
                                    });
                                }
                            }
                        }
                    }
                }

                for (const auto& child : n->children) {
                    check(child);
                }
            }
        } checker{lines, id_map, diagnostics};

        checker.check(ast);
    }

    return diagnostics;
}

} // namespace tooey
