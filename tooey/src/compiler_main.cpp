#include "tooey/lexer.hpp"
#include "tooey/parser.hpp"
#include "tooey/codegen.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>

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
