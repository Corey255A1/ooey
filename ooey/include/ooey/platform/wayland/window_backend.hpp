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
    
    std::unique_ptr<PointerData> pointer_data_;
    std::unique_ptr<KeyboardData> keyboard_data_;
    wl_pointer* pointer_obj_{nullptr};
    wl_keyboard* keyboard_obj_{nullptr};

    int width_{};
    int height_{};
    std::string title_;
};

} // namespace ooey::wayland
