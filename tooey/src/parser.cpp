#include "tooey/parser.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <filesystem>

namespace tooey {

struct Line {
    int indent = 0;
    std::vector<Token> tokens;
    int line_number = 0;
};

std::vector<Line> group_into_lines(const std::vector<Token>& tokens) {
    std::vector<Line> lines;
    Line current_line;
    bool at_start_of_line = true;

    for (const auto& tok : tokens) {
        if (tok.type == TokenType::END_OF_FILE) {
            if (!current_line.tokens.empty()) {
                lines.push_back(current_line);
            }
            break;
        }

        if (tok.type == TokenType::NEWLINE) {
            if (!current_line.tokens.empty()) {
                lines.push_back(current_line);
                current_line = Line();
            }
            at_start_of_line = true;
            continue;
        }

        if (at_start_of_line) {
            current_line.line_number = tok.line;
            if (tok.type == TokenType::INDENT) {
                current_line.indent = tok.text.length();
                at_start_of_line = false;
                continue;
            }
            at_start_of_line = false;
        }

        current_line.tokens.push_back(tok);
    }
    return lines;
}

bool is_empty_or_comment_only(const Line& line) {
    if (line.tokens.empty()) return true;
    for (const auto& tok : line.tokens) {
        if (tok.type != TokenType::COMMENT) {
            return false;
        }
    }
    return true;
}

std::shared_ptr<AstNode> Parser::parse(const std::vector<Token>& tokens, const std::string& current_directory) {
    auto root = std::make_shared<AstNode>();
    root->nodeType = "Root";
    root->line = 1;
    root->column = 1;

    std::vector<std::string> custom_components;
    std::string search_dir = current_directory.empty() ? "." : current_directory;
    if (std::filesystem::exists(search_dir) && std::filesystem::is_directory(search_dir)) {
        for (const auto& entry : std::filesystem::directory_iterator(search_dir)) {
            if (entry.is_regular_file() && (entry.path().extension() == ".ooey" || entry.path().extension() == ".OOEY")) {
                custom_components.push_back(entry.path().stem().string());
            }
        }
    }

    // Node stack: pair of (NodePtr, Indentation Level)
    std::vector<std::pair<std::shared_ptr<AstNode>, int>> node_stack;
    node_stack.emplace_back(root, -1);

    auto active_scope = [&](int current_indent) -> std::shared_ptr<AstNode> {
        while (node_stack.size() > 1 && node_stack.back().second >= current_indent) {
            node_stack.pop_back();
        }
        return node_stack.back().first;
    };

    std::vector<Line> all_lines = group_into_lines(tokens);

    for (const auto& line : all_lines) {
        if (is_empty_or_comment_only(line)) {
            continue;
        }

        int current_indent = line.indent;
        auto parent_node = active_scope(current_indent);

        const auto& line_toks = line.tokens;
        if (line_toks.empty()) continue;

        // 1. Keyword: use (Imports)
        if (line_toks[0].type == TokenType::USE) {
            std::string import_path = "";
            for (size_t i = 1; i < line_toks.size(); ++i) {
                if (line_toks[i].type == TokenType::COMMENT) break;
                import_path += line_toks[i].text;
            }
            root->properties["imports"].type = PropertyType::String;
            root->properties["imports"].rawData = import_path;
            continue;
        }

        // 2. Native AI block
        if (line_toks[0].type == TokenType::AI_BLOCK) {
            parent_node->aiHint = line_toks[0].text;
            continue;
        }

        // 3. Attribute Assignment: e.g. text: "Submit"
        if (line_toks[0].type == TokenType::ATTR_KEY) {
            std::string key = line_toks[0].text;
            if (line_toks.size() >= 3 && line_toks[1].type == TokenType::COLON) {
                PropertyValue p_val;
                const auto& val_tok = line_toks[2];

                if (val_tok.type == TokenType::STRING) {
                    p_val.type = PropertyType::String;
                    p_val.rawData = val_tok.text;
                } else if (val_tok.type == TokenType::NUMBER) {
                    p_val.type = PropertyType::Number;
                    p_val.rawData = val_tok.text;
                } else if (val_tok.type == TokenType::BOOLEAN) {
                    p_val.type = PropertyType::Boolean;
                    p_val.rawData = val_tok.text;
                } else if (val_tok.type == TokenType::BINDING) {
                    p_val.type = PropertyType::Binding;
                    p_val.rawData = val_tok.text;
                } else if (val_tok.type == TokenType::SIGNAL) {
                    p_val.type = PropertyType::Signal;
                    p_val.rawData = val_tok.text;
                } else if (val_tok.type == TokenType::THEME) {
                    p_val.type = PropertyType::Theme;
                    p_val.rawData = val_tok.text;
                } else if (val_tok.type == TokenType::LBRACKET) {
                    p_val.type = PropertyType::String; // Fallback representation for list values
                    std::string arr_str = "[";
                    size_t idx = 3;
                    while (idx < line_toks.size() && line_toks[idx].type != TokenType::RBRACKET) {
                        arr_str += line_toks[idx].text;
                        idx++;
                    }
                    if (idx < line_toks.size()) {
                        arr_str += "]";
                    }
                    p_val.rawData = arr_str;
                } else {
                    p_val.type = PropertyType::String;
                    p_val.rawData = val_tok.text;
                }

                parent_node->properties[key] = p_val;
            }
            continue;
        }

        // 4. Element Declaration: e.g. Button id=btn text="Submit":
        if (line_toks[0].type == TokenType::ELEMENT) {
            auto new_node = std::make_shared<AstNode>();
            new_node->nodeType = line_toks[0].text;
            new_node->line = line.line_number;
            new_node->column = line_toks[0].column;

            // Check if element is a custom component file in the directory
            std::string elem_type = new_node->nodeType;
            bool is_custom = std::ranges::find(custom_components, elem_type) != custom_components.end();
            if (is_custom) {
                new_node->isCustomComponent = true;
                if (std::find(root->customIncludes.begin(), root->customIncludes.end(), elem_type) == root->customIncludes.end()) {
                    root->customIncludes.push_back(elem_type);
                }
            }

            bool ends_with_colon = false;

            for (size_t i = 1; i < line_toks.size(); ++i) {
                const auto& tok = line_toks[i];
                if (tok.type == TokenType::COMMENT) {
                    break;
                }
                if (tok.type == TokenType::COLON) {
                    ends_with_colon = true;
                    break;
                }
                if (tok.type == TokenType::ID_ASSIGN) {
                    new_node->id = tok.text;
                }
                if (tok.type == TokenType::INLINE_KEY) {
                    std::string key = tok.text;
                    if (i + 2 < line_toks.size() && line_toks[i+1].type == TokenType::EQUAL) {
                        const auto& val_tok = line_toks[i+2];
                        PropertyValue p_val;
                        if (val_tok.type == TokenType::STRING) {
                            p_val.type = PropertyType::String;
                            p_val.rawData = val_tok.text;
                        } else if (val_tok.type == TokenType::NUMBER) {
                            p_val.type = PropertyType::Number;
                            p_val.rawData = val_tok.text;
                        } else if (val_tok.type == TokenType::BOOLEAN) {
                            p_val.type = PropertyType::Boolean;
                            p_val.rawData = val_tok.text;
                        } else if (val_tok.type == TokenType::BINDING) {
                            p_val.type = PropertyType::Binding;
                            p_val.rawData = val_tok.text;
                        } else if (val_tok.type == TokenType::SIGNAL) {
                            p_val.type = PropertyType::Signal;
                            p_val.rawData = val_tok.text;
                        } else if (val_tok.type == TokenType::THEME) {
                            p_val.type = PropertyType::Theme;
                            p_val.rawData = val_tok.text;
                        } else {
                            p_val.type = PropertyType::String;
                            p_val.rawData = val_tok.text;
                        }
                        new_node->properties[key] = p_val;
                        i += 2;
                    }
                }
            }

            parent_node->children.push_back(new_node);

            if (ends_with_colon) {
                node_stack.emplace_back(new_node, current_indent);
            }
            continue;
        }
    }

    return root;
}

} // namespace tooey
