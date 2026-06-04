#include "tooey/lexer.hpp"
#include "tooey/parser.hpp"
#include "tooey/codegen.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <set>
#include <map>

// Recursive helper to gather keys from AST
void gather_localization_keys(const std::shared_ptr<tooey::AstNode>& node, std::vector<std::string>& keys) {
    if (!node) return;
    for (const auto& prop : node->properties) {
        if (prop.second.type == tooey::PropertyType::Localization) {
            keys.push_back(prop.second.rawData);
        }
    }
    for (const auto& child : node->children) {
        gather_localization_keys(child, keys);
    }
}

// Extract built-in keys from localization.hpp
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

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file.ooey> <output_dir> [<class_name> <view_model_class>]\n";
        return 1;
    }

    std::string input_path = argv[1];
    std::string output_dir = argv[2];

    std::filesystem::path in_path(input_path);
    if (!std::filesystem::exists(in_path)) {
        std::cerr << "Error: Input file does not exist: " << input_path << "\n";
        return 1;
    }

    std::string class_name = (argc >= 4) ? argv[3] : in_path.stem().string();
    std::string vm_class = (argc >= 5) ? argv[4] : class_name + "ViewModel";

    std::ifstream ifs(input_path);
    if (!ifs.is_open()) {
        std::cerr << "Error: Could not open input file: " << input_path << "\n";
        return 1;
    }

    std::string source((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();

    // Directory containing the input file is scanned for reusable component definitions
    std::string current_dir = in_path.parent_path().string();
    if (current_dir.empty()) {
        current_dir = ".";
    }

    try {
        auto tokens = tooey::Lexer::tokenize(source);
        auto ast = tooey::Parser::parse(tokens, current_dir);

        // Handle build-time localization extraction and verification
        std::vector<std::string> local_keys;
        gather_localization_keys(ast, local_keys);
        if (!local_keys.empty()) {
            std::set<std::string> builtin = get_builtin_keys(current_dir);
            for (const auto& k : local_keys) {
                if (builtin.find(k) == builtin.end()) {
                    std::cerr << "Warning: Localization key '" << k << "' is missing from built-in translation dictionaries.\n";
                }
            }

            // Output master translation template
            std::filesystem::path locale_dir = std::filesystem::path(output_dir) / "locales";
            std::filesystem::create_directories(locale_dir);
            std::filesystem::path locale_file = locale_dir / "en_US.json";

            // Load existing template if any to merge
            std::map<std::string, std::string> merged_keys;
            if (std::filesystem::exists(locale_file)) {
                std::ifstream t_ifs(locale_file);
                if (t_ifs.is_open()) {
                    std::string line;
                    while (std::getline(t_ifs, line)) {
                        size_t p1 = line.find("\"");
                        if (p1 != std::string::npos) {
                            size_t p2 = line.find("\"", p1 + 1);
                            if (p2 != std::string::npos) {
                                std::string key = line.substr(p1 + 1, p2 - p1 - 1);
                                size_t p3 = line.find("\"", p2 + 1);
                                if (p3 != std::string::npos) {
                                    size_t p4 = line.find("\"", p3 + 1);
                                    if (p4 != std::string::npos) {
                                        std::string val = line.substr(p3 + 1, p4 - p3 - 1);
                                        merged_keys[key] = val;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Add new keys
            for (const auto& k : local_keys) {
                if (merged_keys.find(k) == merged_keys.end()) {
                    merged_keys[k] = k;
                }
            }

            // Write out the merged JSON
            std::ofstream t_ofs(locale_file);
            if (t_ofs.is_open()) {
                t_ofs << "{\n";
                bool first = true;
                for (const auto& pair : merged_keys) {
                    if (!first) t_ofs << ",\n";
                    t_ofs << "    \"" << pair.first << "\": \"" << pair.second << "\"";
                    first = false;
                }
                t_ofs << "\n}\n";
                t_ofs.close();
                std::cout << "Extracted and wrote localization template to: " << locale_file << "\n";
            }
        }

        auto result = tooey::CodeGenerator::generate(ast, class_name, vm_class);

        std::filesystem::path out_path(output_dir);
        std::filesystem::create_directories(out_path);

        std::filesystem::path header_file = out_path / (class_name + ".hpp");
        std::filesystem::path source_file = out_path / (class_name + ".cpp");

        std::ofstream h_ofs(header_file);
        if (!h_ofs.is_open()) {
            std::cerr << "Error: Could not write header file: " << header_file << "\n";
            return 1;
        }
        h_ofs << result.header;
        h_ofs.close();

        std::ofstream s_ofs(source_file);
        if (!s_ofs.is_open()) {
            std::cerr << "Error: Could not write source file: " << source_file << "\n";
            return 1;
        }
        s_ofs << result.source;
        s_ofs.close();

        std::cout << "Generated: " << header_file << " and " << source_file << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Compilation failed: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
