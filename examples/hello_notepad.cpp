#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include "ooey/ooey.hpp"
#include "gooey/application.hpp"
#include "ooey/platform.hpp"
#include "gooey/mvvmc/view.hpp"
#include "gooey/controls/button.hpp"
#include "gooey/controls/text_box.hpp"
#include "gooey/controls/code_editor.hpp"

static std::string load_file_content(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

static bool save_file_content(const std::string& filepath, const std::string& content) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;
    file << content;
    return true;
}

class NotepadView : public gooey::View {
public:
    NotepadView() {
        ooey::Font default_font{"sans-serif", 14};
        ooey::Font code_font{"monospace", 14};
        
        // 1. File Path Input Box
        file_input_ = std::make_shared<gooey::TextBox>(
            ooey::Rect{50, 20, 430, 30},
            default_font,
            ooey::Color{220, 220, 220}, // Text color (white/gray)
            ooey::Color{40, 40, 42}     // Background color (dark)
        );
        file_input_->set_text("main.cpp");
        
        // 2. Load button
        auto load_btn = std::make_shared<gooey::Button>(
            ooey::Rect{490, 20, 100, 30},
            ooey::Color{50, 50, 55}
        );
        load_btn->set_label_text("Load");
        
        // 3. Save button
        auto save_btn = std::make_shared<gooey::Button>(
            ooey::Rect{600, 20, 100, 30},
            ooey::Color{50, 50, 55}
        );
        save_btn->set_label_text("Save");
        
        // 4. Code editor
        code_editor_ = std::make_shared<gooey::CodeEditor>(
            ooey::Rect{50, 70, 700, 480},
            code_font,
            ooey::Color{220, 220, 220}, // default text color
            ooey::Color{30, 30, 30}     // code editor background
        );
        
        // Enable C++ syntax highlighting
        auto highlighter = std::make_shared<gooey::CppSyntaxHighlighter>();
        code_editor_->set_syntax_highlighter(highlighter);
        
        // Add children
        add_child(file_input_);
        add_child(load_btn);
        add_child(save_btn);
        add_child(code_editor_);
        
        // Action Handlers
        load_btn->on_click = [this]() {
            std::string path = file_input_->get_text();
            std::string content = load_file_content(path);
            code_editor_->set_text(content);
        };
        
        save_btn->on_click = [this]() {
            std::string path = file_input_->get_text();
            std::string content = code_editor_->get_text();
            save_file_content(path, content);
        };
        
        // Put a default hello world program in the editor initially
        code_editor_->set_text(
            "#include <iostream>\n\n"
            "int main() {\n"
            "    // Print a greeting\n"
            "    std::cout << \"Hello from OOEY RichText Code Editor!\" << std::endl;\n"
            "    return 0;\n"
            "}\n"
        );
    }
    
private:
    std::shared_ptr<gooey::TextBox> file_input_;
    std::shared_ptr<gooey::CodeEditor> code_editor_;
};

int main() {
    std::cout << "Starting OOEY Notepad Demo...\n";

    gooey::Application app;

    auto backend = ooey::create_default_window_backend();
    if (!backend || !backend->create({800, 600}, "OOEY RichText Notepad Example")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }
    app.set_window_backend(std::move(backend));

    auto root_view = std::make_shared<NotepadView>();
    app.set_root_view(std::move(root_view));
    app.set_clear_color(ooey::Color{20, 20, 22});

    app.run();

    return 0;
}
