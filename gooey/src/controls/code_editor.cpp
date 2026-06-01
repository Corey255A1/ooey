namespace ooey {}

#include "gooey/controls/code_editor.hpp"

namespace gooey::controls {
    using namespace ooey;

CodeEditor::CodeEditor(Rect bounds, Font font, Color text_color, Color bg_color)
    : RichTextBox(bounds, font, text_color, bg_color) {
    show_line_numbers = true;
    set_syntax_highlighter(std::make_shared<CppSyntaxHighlighter>());
}

} // namespace gooey::controls
