#pragma once

#include "ooey/i_window_backend.hpp"
#include <memory>
#include <string>
#include <cstdint>

// Forward declare Wayland types to keep header lightweight
struct wl_display;
struct wl_registry;
struct wl_compositor;
struct wl_shm;
struct wl_surface;
struct wl_seat;
struct xdg_wm_base;
struct xdg_surface;
struct xdg_toplevel;
struct wl_pointer;
struct wl_keyboard;
struct wl_buffer;
struct wl_egl_window;

namespace ooey {
class WindowChrome;
class ChromeRenderTarget;
}

namespace ooey::wayland {

struct PointerData;
struct KeyboardData;

class WindowBackend : public IWindowBackend {
public:
    WindowBackend();
    ~WindowBackend() override;

    bool create(const Size& size, const char* title) override;
    void destroy() override;
    bool poll_events() override;
    void poll_input() override;
    IRenderTarget* get_render_target() override;

    void set_input_manager(InputManager* manager) override;

    // Window chrome interface
    void set_window_chrome(std::shared_ptr<WindowChrome> chrome) override;
    std::shared_ptr<WindowChrome> get_window_chrome() const override { return window_chrome_; }
    void start_interactive_move() override;
    void start_interactive_resize(WindowResizeEdge edge) override;
    void request_close() override { should_close_ = true; }

    Size get_window_size() const { return Size{width_, height_}; }

    // Handlers invoked by generated/static listeners
    void handle_xdg_surface_configure(uint32_t serial);
    void handle_xdg_toplevel_configure(int32_t width, int32_t height);

private:
    wl_display* display_{nullptr};
    wl_registry* registry_{nullptr};
    wl_compositor* compositor_{nullptr};
    wl_shm* shm_{nullptr};
    wl_surface* surface_{nullptr};
    xdg_surface* xdg_surface_{nullptr};
    xdg_toplevel* xdg_toplevel_{nullptr};
    wl_seat* seat_{nullptr};

    bool waiting_for_configure_{false};
    int pending_width_{};
    int pending_height_{};

    std::unique_ptr<IRenderTarget> render_target_;
    struct wl_buffer* wl_buffer_{nullptr};
    uint8_t* mapped_data_{nullptr};
    size_t mapped_size_{0};
    bool released_{false};
    InputManager* input_manager_{nullptr};

    void recreate_render_target(int width, int height);
    bool init_egl();
    void cleanup_egl();
    
    std::unique_ptr<PointerData> pointer_data_;
    std::unique_ptr<KeyboardData> keyboard_data_;
    wl_pointer* pointer_obj_{nullptr};
    wl_keyboard* keyboard_obj_{nullptr};

    void* egl_display_{nullptr};
    void* egl_context_{nullptr};
    void* egl_surface_{nullptr};
    void* egl_config_{nullptr};
    wl_egl_window* egl_window_{nullptr};
    bool use_egl_{false};

    int width_{};
    int height_{};
    std::string title_;

    std::shared_ptr<ooey::WindowChrome> window_chrome_;
    std::unique_ptr<ooey::ChromeRenderTarget> decorated_render_target_;
    bool should_close_{false};
};

} // namespace ooey::wayland
