#pragma once

#include "ooey/renderer/font_backend.hpp"
#include <memory>

namespace ooey {

class Win32FontBackend : public IFontBackend {
public:
    Win32FontBackend();
    virtual ~Win32FontBackend() override;

    bool initialize() override;
    bool load_font(const Font& font) override;
    Size measure_text(const std::string& text, const Font& font) override;
    void draw_text(const std::string& text, const Font& font, const Point& position, const DrawCallback& callback) override;
    std::vector<std::string> get_available_fonts() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ooey
