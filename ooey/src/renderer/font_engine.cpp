#include "ooey/renderer/font_engine.hpp"

#ifdef _WIN32
#include "win32_font_backend.hpp"
#else
#include "linux_font_backend.hpp"
#endif

namespace ooey {

std::unique_ptr<IFontBackend> FontEngine::backend_ = nullptr;
bool FontEngine::attempted_init_ = false;

void FontEngine::set_backend(std::unique_ptr<IFontBackend>&& backend) {
    backend_ = std::move(backend);
    attempted_init_ = true;
}

IFontBackend* FontEngine::get_backend() {
    if (!attempted_init_) {
        attempted_init_ = true;
#ifdef _WIN32
        auto win_backend = std::make_unique<Win32FontBackend>();
        if (win_backend->initialize()) {
            backend_ = std::move(win_backend);
        }
#else
        auto linux_backend = std::make_unique<LinuxFontBackend>();
        if (linux_backend->initialize()) {
            backend_ = std::move(linux_backend);
        }
#endif
    }
    return backend_.get();
}

Size FontEngine::measure_text(const std::string& text, const Font& font) {
    IFontBackend* backend = get_backend();
    if (backend && backend->load_font(font)) {
        return backend->measure_text(text, font);
    }
    return BitmapFont::measure_text(text, font.size);
}

std::vector<std::string> FontEngine::get_available_fonts() {
    IFontBackend* backend = get_backend();
    if (backend) {
        return backend->get_available_fonts();
    }
    return {"sans-serif", "serif", "monospace"};
}

} // namespace ooey
