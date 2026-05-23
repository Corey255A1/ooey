#pragma once

#include "ooey/i_window_backend.hpp"

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

namespace ooey {

class WaylandWindowBackend : public IWindowBackend {
public:
    WaylandWindowBackend();
    ~WaylandWindowBackend() override;

    bool create(const Size& size, const char* title) override;
    void destroy() override;
    bool poll_events() override;
    void poll_input() override;
    IRenderTarget* get_render_target() override;

    void set_input_manager(InputManager* manager);

    // Handlers invoked by generated/static listeners
    void handle_xdg_surface_configure(uint32_t serial);
    void handle_xdg_toplevel_configure(int32_t width, int32_t height);

private:
    // Opaque pointers to Wayland objects kept private in the cpp implementation
    wl_display* display_{};
    wl_registry* registry_{};
    wl_compositor* compositor_{};
    wl_shm* shm_{};
    wl_surface* surface_{};
    xdg_surface* xdg_surface_{};
    xdg_toplevel* xdg_toplevel_{};
    wl_seat* seat_{};

    bool waiting_for_configure_{false};
    int pending_width_{};
    int pending_height_{};

    std::unique_ptr<IRenderTarget> render_target_;
    InputManager* input_manager_{nullptr};
    // Keep listener contexts so we can update them when the input manager is set later
    void* pointer_data_{nullptr};
    void* keyboard_data_{nullptr};
    wl_pointer* pointer_obj_{};
    wl_keyboard* keyboard_obj_{};

    int width_{};
    int height_{};
    std::string title_;
};

} // namespace ooey
