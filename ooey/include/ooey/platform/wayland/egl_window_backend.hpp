#pragma once

#include "ooey/platform/wayland/window_backend.hpp"

namespace ooey::wayland {

class EglWindowBackend : public WindowBackend {
public:
    EglWindowBackend();
    ~EglWindowBackend() override;

protected:
    bool init_graphics_context() override;
    void cleanup_graphics_context() override;
    void recreate_render_target(int width, int height) override;

private:
    bool create_egl_display();
    bool choose_egl_config();
    bool create_egl_context();

    void* egl_display_{nullptr};
    void* egl_context_{nullptr};
    void* egl_surface_{nullptr};
    void* egl_config_{nullptr};
    struct wl_egl_window* egl_window_{nullptr};
};

} // namespace ooey::wayland
