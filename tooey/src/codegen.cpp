#include "tooey/codegen.hpp"
#include <sstream>
#include <type_traits>
#include <vector>

namespace tooey {

static std::string to_snake_case(const std::string& s) {
    std::string res = "";
    for (size_t i = 0; i < s.length(); ++i) {
        char c = s[i];
        if (std::isupper(c)) {
            if (i > 0) res += "_";
            res += std::tolower(c);
        } else {
            res += c;
        }
    }
    return res;
}

static std::string generate_template_node(
    const std::shared_ptr<AstNode>& node, 
    std::stringstream& instantiations, 
    std::stringstream& bindings, 
    std::stringstream& hierarchy, 
    const std::string& parent_var, 
    int& unique_id, 
    const std::string& items_prop_name,
    const std::string& view_model_class
) {
    if (!node) return "";

    std::string var_name = node->id;
    if (var_name.empty()) {
        var_name = "item_child_" + std::to_string(++unique_id);
    }

    std::string cpp_type = "gooey::mvvmc::GooeyNode";
    if (node->nodeType == "VBox" || node->nodeType == "Column") {
        cpp_type = "gooey::Column";
    } else if (node->nodeType == "HBox" || node->nodeType == "Row") {
        cpp_type = "gooey::Row";
    } else if (node->nodeType == "Grid") {
        cpp_type = "gooey::Grid";
    } else if (node->nodeType == "FlowLayout") {
        cpp_type = "gooey::FlowLayout";
    } else if (node->nodeType == "Button") {
        cpp_type = "gooey::controls::Button";
    } else if (node->nodeType == "CheckBox") {
        cpp_type = "gooey::controls::CheckBox";
    } else if (node->nodeType == "Label") {
        cpp_type = "gooey::controls::Label";
    } else if (node->nodeType == "TextBox") {
        cpp_type = "gooey::controls::TextBox";
    } else if (node->nodeType == "RichTextBox") {
        cpp_type = "gooey::controls::RichTextBox";
    } else if (node->nodeType == "ImageControl") {
        cpp_type = "gooey::controls::ImageControl";
    } else if (node->nodeType == "ScrollBar") {
        cpp_type = "gooey::controls::ScrollBar";
    } else if (node->nodeType == "ScrollContainer") {
        cpp_type = "gooey::controls::ScrollContainer";
    } else if (node->nodeType == "ListControl") {
        cpp_type = "gooey::controls::ListControl";
    } else if (node->nodeType == "DataGrid") {
        cpp_type = "gooey::controls::DataGrid";
    } else if (node->nodeType == "AdaptiveStack") {
        cpp_type = "gooey::controls::AdaptiveStack";
    } else if (node->nodeType == "CanvasLayout") {
        cpp_type = "gooey::controls::CanvasLayout";
    } else if (node->nodeType == "VectorShapeView") {
        cpp_type = "gooey::controls::VectorShapeView";
    } else {
        cpp_type = node->nodeType;
    }

    instantiations << "                    auto " << var_name << " = std::make_shared<" << cpp_type << ">();\n";
    instantiations << "                    " << var_name << "->set_absolute(false);\n";

    for (const auto& prop : node->properties) {
        std::string key = prop.first;
        const auto& val = prop.second;

        if (val.type == PropertyType::Binding) {
            std::string prefix = items_prop_name + ".";
            if (val.rawData.rfind(prefix, 0) == 0) {
                std::string sub_property = val.rawData.substr(prefix.length());
                bindings << "                    tooey::set_control_value(" << var_name << ", item." << sub_property << ");\n";
                
                bool should_gen_reverse = false;
                std::string cb_field = "";
                std::string param_type = "";
                if (node->nodeType == "TextBox" && key == "text") {
                    should_gen_reverse = true;
                    cb_field = "on_text_changed";
                    param_type = "const std::string&";
                } else if (node->nodeType == "CheckBox" && key == "checked") {
                    should_gen_reverse = true;
                    cb_field = "on_checked_changed";
                    param_type = "bool";
                } else if ((node->nodeType == "Slider" || node->nodeType == "ScrollBar") && key == "value") {
                    should_gen_reverse = true;
                    cb_field = "on_value_changed";
                    param_type = "auto";
                }

                if (should_gen_reverse) {
                    bindings << "                    {\n";
                    bindings << "                        " << var_name << "->" << cb_field << " = [weak_viewModel, i](" << param_type << " newValue) {\n";
                    bindings << "                            if (auto viewModel = weak_viewModel.lock()) {\n";
                    bindings << "                                auto list_val = viewModel->" << items_prop_name << ".get();\n";
                    bindings << "                                list_val[i]." << sub_property << " = newValue;\n";
                    bindings << "                                viewModel->" << items_prop_name << ".set(list_val);\n";
                    bindings << "                            }\n";
                    bindings << "                        };\n";
                    bindings << "                    }\n";
                }
            } else {
                bindings << "                    " << var_name << " <<= viewModel->" << val.rawData << ";\n";
                
                bool should_gen_reverse = false;
                std::string cb_field = "";
                std::string param_type = "";
                if (node->nodeType == "TextBox" && key == "text") {
                    should_gen_reverse = true;
                    cb_field = "on_text_changed";
                    param_type = "const std::string&";
                } else if (node->nodeType == "CheckBox" && key == "checked") {
                    should_gen_reverse = true;
                    cb_field = "on_checked_changed";
                    param_type = "bool";
                } else if ((node->nodeType == "Slider" || node->nodeType == "ScrollBar") && key == "value") {
                    should_gen_reverse = true;
                    cb_field = "on_value_changed";
                    param_type = "auto";
                }

                if (should_gen_reverse) {
                    bindings << "                    {\n";
                    bindings << "                        " << var_name << "->" << cb_field << " = [weak_viewModel](" << param_type << " newValue) {\n";
                    bindings << "                            if (auto viewModel = weak_viewModel.lock()) {\n";
                    bindings << "                                viewModel->" << val.rawData << ".set(newValue);\n";
                    bindings << "                            }\n";
                    bindings << "                        };\n";
                    bindings << "                    }\n";
                }
            }
        }
        else if (val.type == PropertyType::Signal) {
            std::string cb_name = "on_click";
            if (node->nodeType == "Slider" || node->nodeType == "ScrollBar") {
                cb_name = "on_value_changed";
            } else if (node->nodeType == "TextBox") {
                cb_name = "on_text_changed";
            } else if (node->nodeType == "CheckBox") {
                cb_name = "on_checked_changed";
            } else if (node->nodeType == "ListControl") {
                cb_name = "on_selected_changed";
            }

            bindings << "                    {\n";
            bindings << "                        " << var_name << "->" << cb_name << " = [weak_viewModel, i](" << (cb_name == "on_click" ? "" : "auto newValue") << ") {\n";
            bindings << "                            if (auto viewModel = weak_viewModel.lock()) {\n";
            
            std::string call_expr = val.rawData;
            size_t idx_pos = call_expr.find("index");
            while (idx_pos != std::string::npos) {
                call_expr.replace(idx_pos, 5, "i");
                idx_pos = call_expr.find("index");
            }
            
            if (call_expr.find('(') != std::string::npos) {
                bindings << "                                viewModel->" << call_expr << ";\n";
            } else {
                if (cb_name == "on_click") {
                    bindings << "                                viewModel->" << call_expr << "();\n";
                } else {
                    bindings << "                                viewModel->" << call_expr << ".set(newValue);\n";
                }
            }
            bindings << "                            }\n";
            bindings << "                        };\n";
            bindings << "                    }\n";
        }
        else {
            std::string val_repr = val.rawData;
            if (val.type == PropertyType::String) {
                val_repr = "\"" + val_repr + "\"";
            }

            if (key == "text") {
                if (node->nodeType == "Button") {
                    instantiations << "                    " << var_name << "->set_label_text(" << val_repr << ");\n";
                } else if (node->nodeType == "Label" || node->nodeType == "TextBox") {
                    instantiations << "                    " << var_name << "->set_text(" << val_repr << ");\n";
                } else if (node->nodeType == "CheckBox") {
                    instantiations << "                    " << var_name << "->set_label_text(" << val_repr << ");\n";
                }
            } else if (key == "checked") {
                instantiations << "                    " << var_name << "->set_checked(" << val_repr << ");\n";
            } else if (key == "width") {
                if (val_repr == "\"MatchParent\"" || val_repr == "MatchParent") {
                    instantiations << "                    " << var_name << "->set_width(gooey::SizePolicy::MatchParent);\n";
                } else if (val_repr == "\"WrapContent\"" || val_repr == "WrapContent") {
                    instantiations << "                    " << var_name << "->set_width(gooey::SizePolicy::WrapContent);\n";
                } else {
                    instantiations << "                    " << var_name << "->set_width(gooey::SizePolicy::Fixed, " << val_repr << ");\n";
                }
            } else if (key == "height") {
                if (val_repr == "\"MatchParent\"" || val_repr == "MatchParent") {
                    instantiations << "                    " << var_name << "->set_height(gooey::SizePolicy::MatchParent);\n";
                } else if (val_repr == "\"WrapContent\"" || val_repr == "WrapContent") {
                    instantiations << "                    " << var_name << "->set_height(gooey::SizePolicy::WrapContent);\n";
                } else {
                    instantiations << "                    " << var_name << "->set_height(gooey::SizePolicy::Fixed, " << val_repr << ");\n";
                }
            } else if (key == "spacing") {
                instantiations << "                    " << var_name << "->set_spacing(" << val_repr << ");\n";
            } else {
                instantiations << "                    // " << key << ": " << val_repr << "\n";
            }
        }
    }

    if (!parent_var.empty()) {
        hierarchy << "                    " << parent_var << "->add_child(" << var_name << ");\n";
    }

    for (const auto& child : node->children) {
        generate_template_node(child, instantiations, bindings, hierarchy, var_name, unique_id, items_prop_name, view_model_class);
    }

    return var_name;
}

static void generate_node(
    const std::shared_ptr<AstNode>& node, 
    std::stringstream& instantiations, 
    std::stringstream& bindings, 
    std::stringstream& hierarchy, 
    const std::string& parent_var, 
    int& unique_id, 
    std::vector<std::string>& member_declarations,
    const std::string& view_model_class
) {
    if (!node) return;

    if (node->nodeType == "Root") {
        for (const auto& child : node->children) {
            generate_node(child, instantiations, bindings, hierarchy, "this", unique_id, member_declarations, view_model_class);
        }
        return;
    }

    std::string var_name = node->id;
    bool is_member = !var_name.empty();
    if (var_name.empty()) {
        var_name = "child_" + std::to_string(++unique_id);
    }

    // Resolve element class names to GUI framework class names
    std::string cpp_type = "gooey::mvvmc::GooeyNode";
    if (node->nodeType == "VBox" || node->nodeType == "Column") {
        cpp_type = "gooey::Column";
    } else if (node->nodeType == "HBox" || node->nodeType == "Row") {
        cpp_type = "gooey::Row";
    } else if (node->nodeType == "Grid") {
        cpp_type = "gooey::Grid";
    } else if (node->nodeType == "FlowLayout") {
        cpp_type = "gooey::FlowLayout";
    } else if (node->nodeType == "Button") {
        cpp_type = "gooey::controls::Button";
    } else if (node->nodeType == "CheckBox") {
        cpp_type = "gooey::controls::CheckBox";
    } else if (node->nodeType == "Label") {
        cpp_type = "gooey::controls::Label";
    } else if (node->nodeType == "TextBox") {
        cpp_type = "gooey::controls::TextBox";
    } else if (node->nodeType == "RichTextBox") {
        cpp_type = "gooey::controls::RichTextBox";
    } else if (node->nodeType == "ImageControl") {
        cpp_type = "gooey::controls::ImageControl";
    } else if (node->nodeType == "ScrollBar") {
        cpp_type = "gooey::controls::ScrollBar";
    } else if (node->nodeType == "ScrollContainer") {
        cpp_type = "gooey::controls::ScrollContainer";
    } else if (node->nodeType == "ListControl") {
        cpp_type = "gooey::controls::ListControl";
    } else if (node->nodeType == "DataGrid") {
        cpp_type = "gooey::controls::DataGrid";
    } else if (node->nodeType == "AdaptiveStack") {
        cpp_type = "gooey::controls::AdaptiveStack";
    } else if (node->nodeType == "CanvasLayout") {
        cpp_type = "gooey::controls::CanvasLayout";
    } else if (node->nodeType == "VectorShapeView") {
        cpp_type = "gooey::controls::VectorShapeView";
    } else {
        cpp_type = node->nodeType; // Custom subclass component
    }

    if (is_member) {
        member_declarations.push_back("    std::shared_ptr<" + cpp_type + "> " + var_name + ";");
        if (node->isCustomComponent) {
            instantiations << "        " << var_name << " = std::make_shared<" << cpp_type << ">(viewModel->get_" << to_snake_case(node->nodeType) << "_view_model());\n";
        } else {
            instantiations << "        " << var_name << " = std::make_shared<" << cpp_type << ">();\n";
        }
        instantiations << "        " << var_name << "->set_absolute(false);\n";
    } else {
        if (node->isCustomComponent) {
            instantiations << "        auto " << var_name << " = std::make_shared<" << cpp_type << ">(viewModel->get_" << to_snake_case(node->nodeType) << "_view_model());\n";
        } else {
            instantiations << "        auto " << var_name << " = std::make_shared<" << cpp_type << ">();\n";
        }
        instantiations << "        " << var_name << "->set_absolute(false);\n";
    }

    // Process properties & bindings
    for (const auto& prop : node->properties) {
        std::string key = prop.first;
        const auto& val = prop.second;

        if (val.type == PropertyType::Binding) {
            // E.g. @binding.strokeWidth
            bindings << "        " << var_name << " <<= viewModel->" << val.rawData << ";\n";
            
            // Generate reverse binding for interactive controls
            bool should_gen_reverse = false;
            std::string cb_field = "";
            std::string param_type = "";
            if (node->nodeType == "TextBox" && key == "text") {
                should_gen_reverse = true;
                cb_field = "on_text_changed";
                param_type = "const std::string&";
            } else if (node->nodeType == "CheckBox" && key == "checked") {
                should_gen_reverse = true;
                cb_field = "on_checked_changed";
                param_type = "bool";
            } else if ((node->nodeType == "Slider" || node->nodeType == "ScrollBar") && key == "value") {
                should_gen_reverse = true;
                cb_field = "on_value_changed";
                param_type = "auto";
            }

            if (should_gen_reverse) {
                bindings << "        {\n";
                bindings << "            std::weak_ptr<" << view_model_class << "> weak_viewModel = viewModel;\n";
                bindings << "            " << var_name << "->" << cb_field << " = [weak_viewModel](" << param_type << " newValue) {\n";
                bindings << "                if (auto viewModel = weak_viewModel.lock()) {\n";
                				bindings << "                    viewModel->" << val.rawData << ".set(newValue);\n";
                bindings << "                }\n";
                bindings << "            };\n";
                bindings << "        }\n";
            }
        }
        else if (val.type == PropertyType::Signal) {
            // E.g. @signal.execute
            std::string cb_name = "on_click";
            if (node->nodeType == "Slider" || node->nodeType == "ScrollBar") {
                cb_name = "on_value_changed";
            } else if (node->nodeType == "TextBox") {
                cb_name = "on_text_changed";
            } else if (node->nodeType == "CheckBox") {
                cb_name = "on_checked_changed";
            } else if (node->nodeType == "ListControl") {
                cb_name = "on_selected_changed";
            }

            bindings << "        {\n";
            bindings << "            std::weak_ptr<" << view_model_class << "> weak_viewModel = viewModel;\n";
            bindings << "            " << var_name << "->" << cb_name << " = [weak_viewModel](" << (cb_name == "on_click" ? "" : "auto newValue") << ") {\n";
            bindings << "                if (auto viewModel = weak_viewModel.lock()) {\n";
            if (val.rawData.find('(') != std::string::npos) {
                bindings << "                    viewModel->" << val.rawData << ";\n";
            } else {
                if (cb_name == "on_click") {
                    bindings << "                    viewModel->" << val.rawData << "();\n";
                } else {
                    bindings << "                    viewModel->" << val.rawData << ".set(newValue);\n";
                }
            }
            bindings << "                }\n";
            bindings << "            };\n";
            bindings << "        }\n";
        }
        else {
            // Raw values (String, Number, Boolean, Theme)
            std::string val_repr = val.rawData;
            if (val.type == PropertyType::String) {
                val_repr = "\"" + val_repr + "\"";
            }

            if (key == "text") {
                if (node->nodeType == "Button") {
                    instantiations << "        " << var_name << "->set_label_text(" << val_repr << ");\n";
                } else if (node->nodeType == "Label" || node->nodeType == "TextBox") {
                    instantiations << "        " << var_name << "->set_text(" << val_repr << ");\n";
                } else if (node->nodeType == "CheckBox") {
                    instantiations << "        " << var_name << "->set_label_text(" << val_repr << ");\n";
                }
            } else if (key == "checked") {
                instantiations << "        " << var_name << "->set_checked(" << val_repr << ");\n";
            } else if (key == "width") {
                if (val_repr == "\"MatchParent\"" || val_repr == "MatchParent") {
                    instantiations << "        " << var_name << "->set_width(gooey::SizePolicy::MatchParent);\n";
                } else if (val_repr == "\"WrapContent\"" || val_repr == "WrapContent") {
                    instantiations << "        " << var_name << "->set_width(gooey::SizePolicy::WrapContent);\n";
                } else {
                    instantiations << "        " << var_name << "->set_width(gooey::SizePolicy::Fixed, " << val_repr << ");\n";
                }
            } else if (key == "height") {
                if (val_repr == "\"MatchParent\"" || val_repr == "MatchParent") {
                    instantiations << "        " << var_name << "->set_height(gooey::SizePolicy::MatchParent);\n";
                } else if (val_repr == "\"WrapContent\"" || val_repr == "WrapContent") {
                    instantiations << "        " << var_name << "->set_height(gooey::SizePolicy::WrapContent);\n";
                } else {
                    instantiations << "        " << var_name << "->set_height(gooey::SizePolicy::Fixed, " << val_repr << ");\n";
                }
            } else if (key == "spacing") {
                instantiations << "        " << var_name << "->set_spacing(" << val_repr << ");\n";
            } else {
                instantiations << "        // " << key << ": " << val_repr << "\n";
            }
        }
    }

    // Hierarchy additions
    if (parent_var == "this") {
        hierarchy << "        this->add_child(" << var_name << ");\n";
    } else {
        hierarchy << "        " << parent_var << "->add_child(" << var_name << ");\n";
    }

    bool is_list_with_template = (node->nodeType == "ListControl" && !node->children.empty());
    std::string items_prop_name = "";
    if (is_list_with_template) {
        auto items_it = node->properties.find("items");
        if (items_it != node->properties.end() && items_it->second.type == PropertyType::Binding) {
            items_prop_name = items_it->second.rawData;
        }
    }

    if (is_list_with_template && !items_prop_name.empty()) {
        auto template_root = node->children[0];
        int item_height = 40;
        auto height_it = template_root->properties.find("height");
        if (height_it != template_root->properties.end() && height_it->second.type != PropertyType::Binding) {
            try {
                item_height = std::stoi(height_it->second.rawData);
            } catch (...) {}
        }
        instantiations << "        " << var_name << "->set_item_height(" << item_height << ");\n";

        bindings << "        {\n";
        bindings << "            std::weak_ptr<" << view_model_class << "> weak_viewModel = viewModel;\n";
        bindings << "            std::weak_ptr<decltype(" << var_name << ")::element_type> weak_" << var_name << " = " << var_name << ";\n";
        bindings << "            using ItemListType = decltype(static_cast<" << view_model_class << "*>(nullptr)->" << items_prop_name << ".get());\n";
        bindings << "            using ItemType = typename std::decay_t<ItemListType>::value_type;\n";
        bindings << "            " << var_name << "->bind(viewModel->" << items_prop_name << ", [weak_viewModel, weak_" << var_name << "](const ItemListType& items) {\n";
        bindings << "                std::vector<std::shared_ptr<gooey::mvvmc::GooeyElement>> views;\n";
        bindings << "                for (size_t i = 0; i < items.size(); ++i) {\n";
        bindings << "                    const auto& item = items[i];\n";
        
        int template_unique_id = 0;
        std::stringstream template_inst;
        std::stringstream template_bind;
        std::stringstream template_hier;
        std::string template_root_var = generate_template_node(
            template_root, 
            template_inst, 
            template_bind, 
            template_hier, 
            "", 
            template_unique_id, 
            items_prop_name, 
            view_model_class
        );
        
        bindings << template_inst.str();
        bindings << template_bind.str();
        bindings << template_hier.str();
        
        bindings << "                    views.push_back(" << template_root_var << ");\n";
        bindings << "                }\n";
        bindings << "                if (auto " << var_name << "_locked = weak_" << var_name << ".lock()) {\n";
        bindings << "                    " << var_name << "_locked->set_item_views(views);\n";
        bindings << "                }\n";
        bindings << "            });\n";
        bindings << "        }\n";
    } else {
        for (const auto& child : node->children) {
            generate_node(child, instantiations, bindings, hierarchy, var_name, unique_id, member_declarations, view_model_class);
        }
    }
}

CodegenResult CodeGenerator::generate(
    const std::shared_ptr<AstNode>& ast, 
    const std::string& class_name, 
    const std::string& view_model_class
) {
    std::stringstream instantiations;
    std::stringstream bindings;
    std::stringstream hierarchy;
    std::vector<std::string> member_declarations;
    int unique_id = 0;

    generate_node(ast, instantiations, bindings, hierarchy, "this", unique_id, member_declarations, view_model_class);

    std::string root_base_type = "gooey::Column";
    if (ast && !ast->children.empty()) {
        std::string first_child_type = ast->children[0]->nodeType;
        if (first_child_type == "HBox" || first_child_type == "Row") {
            root_base_type = "gooey::Row";
        }
    }

    // 1. Generate Header
    std::stringstream h_ss;
    h_ss << "#pragma once\n\n";
    h_ss << "#include \"gooey/mvvmc/gooey_node.hpp\"\n";
    h_ss << "#include \"gooey/controls/button.hpp\"\n";
    h_ss << "#include \"gooey/controls/checkbox.hpp\"\n";
    h_ss << "#include \"gooey/controls/label.hpp\"\n";
    h_ss << "#include \"gooey/controls/text_box.hpp\"\n";
    h_ss << "#include \"gooey/controls/rich_text_box.hpp\"\n";
    h_ss << "#include \"gooey/controls/image_control.hpp\"\n";
    h_ss << "#include \"gooey/controls/scrollbar.hpp\"\n";
    h_ss << "#include \"gooey/controls/list_control.hpp\"\n";
    h_ss << "#include \"gooey/controls/datagrid.hpp\"\n";
    h_ss << "#include \"gooey/controls/adaptive_stack.hpp\"\n";
    h_ss << "#include \"gooey/controls/scroll_container.hpp\"\n";
    h_ss << "#include \"gooey/controls/canvas_layout.hpp\"\n";
    h_ss << "#include \"gooey/controls/vector_shape_view.hpp\"\n";
    h_ss << "#include \"gooey/controls/column.hpp\"\n";
    h_ss << "#include \"gooey/controls/row.hpp\"\n";
    h_ss << "#include \"gooey/controls/grid.hpp\"\n";
    h_ss << "#include \"gooey/controls/flow_layout.hpp\"\n";
    if (ast) {
        for (const auto& inc : ast->customIncludes) {
            h_ss << "#include \"" << inc << ".hpp\"\n";
        }
    }
    h_ss << "#include <memory>\n\n";
    h_ss << "class " << view_model_class << ";\n\n";
    h_ss << "class " << class_name << " : public " << root_base_type << " {\n";
    h_ss << "public:\n";
    h_ss << "    explicit " << class_name << "(std::shared_ptr<" << view_model_class << "> viewModel);\n\n";
    for (const auto& decl : member_declarations) {
        h_ss << decl << "\n";
    }
    h_ss << "};\n";

    // 2. Generate C++ Source
    std::stringstream cpp_ss;
    cpp_ss << "#include \"" << class_name << ".hpp\"\n";
    cpp_ss << "#include \"" << view_model_class << ".hpp\"\n";
    cpp_ss << "#include \"tooey/binding.hpp\"\n\n";
    cpp_ss << "using namespace tooey;\n\n";
    cpp_ss << class_name << "::" << class_name << "(std::shared_ptr<" << view_model_class << "> viewModel) {\n";
    cpp_ss << "        set_width(gooey::SizePolicy::MatchParent);\n";
    cpp_ss << "        set_height(gooey::SizePolicy::MatchParent);\n\n";
    cpp_ss << "        // 1. Instantiations\n";
    cpp_ss << instantiations.str() << "\n";
    cpp_ss << "        // 2. Bindings\n";
    cpp_ss << bindings.str() << "\n";
    cpp_ss << "        // 3. Hierarchy\n";
    cpp_ss << hierarchy.str();
    cpp_ss << "}\n";

    CodegenResult result;
    result.header = h_ss.str();
    result.source = cpp_ss.str();
    return result;
}

} // namespace tooey
