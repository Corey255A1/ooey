#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include "ooey/ooey.hpp"
#include "gooey/application.hpp"
#include "ooey/platform.hpp"
#include "gooey/mvvmc/gooey_node.hpp"
#include "gooey/controls/button.hpp"
#include "gooey/controls/text_box.hpp"
#include "code_editor.hpp"

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

class NotepadView : public gooey::GooeyNode {
public:
    NotepadView() {
        ooey::Font default_font{"sans-serif", 14};
        ooey::Font code_font{"monospace", 14};
        
        // 1. File Path Input Box
        file_input_ = std::make_shared<gooey::TextBox>(
            ooey::Rect{50, 20, 390, 30},
            default_font,
            ooey::Color{220, 220, 220}, // Text color
            ooey::Color{40, 40, 42}     // Background color
        );
        file_input_->set_text("main.cpp");
        
        // 2. Load button
        load_btn_ = std::make_shared<gooey::Button>(
            ooey::Rect{450, 20, 70, 30},
            ooey::Color{50, 50, 55}
        );
        load_btn_->set_label_text("Load");
        
        // 3. Save button
        save_btn_ = std::make_shared<gooey::Button>(
            ooey::Rect{530, 20, 70, 30},
            ooey::Color{50, 50, 55}
        );
        save_btn_->set_label_text("Save");

        // 4. Copy button
        copy_btn_ = std::make_shared<gooey::Button>(
            ooey::Rect{610, 20, 70, 30},
            ooey::Color{50, 50, 55}
        );
        copy_btn_->set_label_text("Copy");

        // 5. Paste button
        paste_btn_ = std::make_shared<gooey::Button>(
            ooey::Rect{690, 20, 70, 30},
            ooey::Color{50, 50, 55}
        );
        paste_btn_->set_label_text("Paste");
        
        // 6. Code editor
        code_editor_ = std::make_shared<gooey::controls::CodeEditor>(
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
        add_child(load_btn_);
        add_child(save_btn_);
        add_child(copy_btn_);
        add_child(paste_btn_);
        add_child(code_editor_);
        
        // Action Handlers
        load_btn_->on_click = [this]() {
            std::string path = file_input_->get_text();
            std::string content = load_file_content(path);
            code_editor_->set_text(content);
        };
        
        save_btn_->on_click = [this]() {
            std::string path = file_input_->get_text();
            std::string content = code_editor_->get_text();
            save_file_content(path, content);
        };

        copy_btn_->on_click = [this]() {
            clipboard_ = code_editor_->get_selected_text();
            std::cout << "[Notepad] Copied selected text: \"" << clipboard_ << "\"\n";
        };

        paste_btn_->on_click = [this]() {
            if (!clipboard_.empty()) {
                code_editor_->insert_text(clipboard_);
            }
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

protected:
    ooey::Size do_measure(ooey::Size constraints) override {
        // Parent constraints are the window client dimensions
        int left = 50;
        int right = constraints.width - 50;
        int btn_w = 70;
        int btn_h = 30;
        int gap = 10;
        int load_x = right - btn_w - gap - btn_w - gap - btn_w - gap - btn_w;
        int input_w = std::max(0, load_x - gap - left);
        
        if (load_btn_) load_btn_->measure(ooey::Size{btn_w, btn_h});
        if (save_btn_) save_btn_->measure(ooey::Size{btn_w, btn_h});
        if (copy_btn_) copy_btn_->measure(ooey::Size{btn_w, btn_h});
        if (paste_btn_) paste_btn_->measure(ooey::Size{btn_w, btn_h});
        if (file_input_) file_input_->measure(ooey::Size{input_w, btn_h});

        int editor_y = 70;
        int editor_w = std::max(0, right - left);
        int editor_h = std::max(0, constraints.height - 50 - editor_y);
        if (code_editor_) code_editor_->measure(ooey::Size{editor_w, editor_h});

        return constraints;
    }

    void do_layout(ooey::Rect bounds) override {
        int left = bounds.x + 50;
        int right = bounds.x + bounds.width - 50;

        int btn_w = 70;
        int btn_h = 30;
        int gap = 10;

        int paste_x = right - btn_w;
        int copy_x = paste_x - gap - btn_w;
        int save_x = copy_x - gap - btn_w;
        int load_x = save_x - gap - btn_w;

        int toolbar_y = bounds.y + 20;

        if (load_btn_) {
            load_btn_->layout(ooey::Rect{load_x, toolbar_y, btn_w, btn_h});
        }
        if (save_btn_) {
            save_btn_->layout(ooey::Rect{save_x, toolbar_y, btn_w, btn_h});
        }
        if (copy_btn_) {
            copy_btn_->layout(ooey::Rect{copy_x, toolbar_y, btn_w, btn_h});
        }
        if (paste_btn_) {
            paste_btn_->layout(ooey::Rect{paste_x, toolbar_y, btn_w, btn_h});
        }

        int input_w = std::max(0, load_x - gap - left);
        if (file_input_) {
            file_input_->layout(ooey::Rect{left, toolbar_y, input_w, btn_h});
        }

        int editor_y = bounds.y + 70;
        int editor_w = std::max(0, right - left);
        int editor_h = std::max(0, bounds.y + bounds.height - 50 - editor_y);
        if (code_editor_) {
            code_editor_->layout(ooey::Rect{left, editor_y, editor_w, editor_h});
        }
    }
    
private:
    std::shared_ptr<gooey::TextBox> file_input_;
    std::shared_ptr<gooey::controls::CodeEditor> code_editor_;
    std::shared_ptr<gooey::Button> load_btn_;
    std::shared_ptr<gooey::Button> save_btn_;
    std::shared_ptr<gooey::Button> copy_btn_;
    std::shared_ptr<gooey::Button> paste_btn_;
    std::string clipboard_;
};

int main() {
    std::cout << "Starting OOEY OoeyCode Demo...\n";

    gooey::Application app;

    auto backend = ooey::create_default_window_backend();
    if (!backend || !backend->create({800, 600}, "OOEY OoeyCode Editor Example")) {
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
