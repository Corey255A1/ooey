#pragma once

#include "gooey/controls/rich_text_box.hpp"

namespace gooey::controls {
    using namespace ooey;

class CodeEditor : public RichTextBox {
public:
    CodeEditor(Rect bounds, Font font, Color text_color, Color bg_color);
};

} // namespace gooey::controls
namespace gooey {
    using namespace ooey;
using gooey::controls::CodeEditor;
}
