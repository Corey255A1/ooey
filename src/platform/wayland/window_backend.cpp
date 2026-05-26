#include "ooey/platform/wayland/window_backend.hpp"
#include "ooey/platform/wayland/render_target.hpp"
#include "ooey/input.hpp"
#include <wayland-client.h>
#include <xdg-shell-client-protocol.h>
#include <xkbcommon/xkbcommon.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <limits.h>

namespace ooey::wayland {

struct PointerData {
    InputManager* input_manager{};
    int last_x{0};
    int last_y{0};
};

struct KeyboardData {
    InputManager* input_manager{};
    xkb_context* xkb_ctx_{nullptr};
    xkb_keymap* keymap_{nullptr};
    xkb_state* xkb_state_{nullptr};

    ~KeyboardData() {
        if (xkb_state_) {
            xkb_state_unref(xkb_state_);
        }
        if (keymap_) {
            xkb_keymap_unref(keymap_);
        }
        if (xkb_ctx_) {
            xkb_context_unref(xkb_ctx_);
        }
    }
};

class WaylandState {
public:
    wl_compositor* compositor{};
    wl_shm* shm{};
    wl_seat* seat{};
    xdg_wm_base* wm_base{};
};

// Forward declarations for listeners
static void xdg_surface_configure(void* data, xdg_surface* surface, uint32_t serial);
static void xdg_toplevel_configure(void* data, xdg_toplevel* toplevel, int32_t width, int32_t height, wl_array* states);

static const xdg_surface_listener g_xdg_surface_listener = {
    xdg_surface_configure
};

static const xdg_toplevel_listener g_xdg_toplevel_listener = {
    xdg_toplevel_configure,
    nullptr // close handler
};

static void registry_global(void* data, wl_registry* registry, uint32_t id, const char* interface, uint32_t version) {
    WaylandState* state = static_cast<WaylandState*>(data);
    if (strcmp(interface, "wl_compositor") == 0) {
        state->compositor = static_cast<wl_compositor*>(wl_registry_bind(registry, id, &wl_compositor_interface, 4));
    } else if (strcmp(interface, "wl_shm") == 0) {
        state->shm = static_cast<wl_shm*>(wl_registry_bind(registry, id, &wl_shm_interface, 1));
    } else if (strcmp(interface, "wl_seat") == 0) {
        state->seat = static_cast<wl_seat*>(wl_registry_bind(registry, id, &wl_seat_interface, 5));
    } else if (strcmp(interface, "xdg_wm_base") == 0) {
        state->wm_base = static_cast<xdg_wm_base*>(wl_registry_bind(registry, id, &xdg_wm_base_interface, 1));
    }
}

static void registry_global_remove(void* /*data*/, wl_registry* /*registry*/, uint32_t /*id*/) {}

static const wl_registry_listener g_registry_listener = {
    registry_global,
    registry_global_remove
};

// xdg_wm_base ping handler
static void xdg_wm_base_ping(void* /*data*/, xdg_wm_base* wm_base, uint32_t serial) {
    xdg_wm_base_pong(wm_base, serial);
}

static const xdg_wm_base_listener g_xdg_wm_base_listener = {
    xdg_wm_base_ping
};

// Pointer and keyboard handling
static void pointer_enter(void* data, wl_pointer* /*wl_pointer*/, uint32_t /*serial*/, wl_surface* /*surface*/, wl_fixed_t sx, wl_fixed_t sy) {
    PointerData* pd = static_cast<PointerData*>(data);
    if (!pd) {
        return;
    }
    int x = wl_fixed_to_int(sx);
    int y = wl_fixed_to_int(sy);
    pd->last_x = x;
    pd->last_y = y;
    if (pd->input_manager) {
        ooey::Pointer p{0, x, y, ooey::PointerState::Moved};
        pd->input_manager->push_pointer_event(p);
    }
}

static void pointer_leave(void* /*data*/, wl_pointer* /*wl_pointer*/, uint32_t /*serial*/, wl_surface* /*surface*/) {}

static void pointer_motion(void* data, wl_pointer* /*wl_pointer*/, uint32_t /*time*/, wl_fixed_t sx, wl_fixed_t sy) {
    PointerData* pd = static_cast<PointerData*>(data);
    if (!pd || !pd->input_manager) {
        return;
    }
    int x = wl_fixed_to_int(sx);
    int y = wl_fixed_to_int(sy);
    pd->last_x = x;
    pd->last_y = y;
    ooey::Pointer p{0, x, y, PointerState::Moved};
    pd->input_manager->push_pointer_event(p);
}

static void pointer_button(void* data, wl_pointer* /*wl_pointer*/, uint32_t /*serial*/, uint32_t /*time*/, uint32_t button, uint32_t state) {
    PointerData* pd = static_cast<PointerData*>(data);
    if (!pd || !pd->input_manager) {
        return;
    }
    // Use last known pointer coordinates
    PointerState st = (state == WL_POINTER_BUTTON_STATE_PRESSED) ? PointerState::Pressed : PointerState::Released;
    ooey::Pointer p{0, pd->last_x, pd->last_y, st};
    pd->input_manager->push_pointer_event(p);
}

static void pointer_axis(void* /*data*/, wl_pointer* /*wl_pointer*/, uint32_t /*time*/, uint32_t /*axis*/, wl_fixed_t /*value*/) {}
static void pointer_frame(void* /*data*/, wl_pointer* /*wl_pointer*/) {}
static void pointer_axis_source(void* /*data*/, wl_pointer* /*wl_pointer*/, uint32_t /*source*/) {}
static void pointer_axis_stop(void* /*data*/, wl_pointer* /*wl_pointer*/, uint32_t /*time*/, uint32_t /*axis*/) {}
static void pointer_axis_discrete(void* /*data*/, wl_pointer* /*wl_pointer*/, uint32_t /*axis*/, int32_t /*discrete*/) {}

static const wl_pointer_listener g_pointer_listener = {
    pointer_enter,
    pointer_leave,
    pointer_motion,
    pointer_button,
    pointer_axis,
    pointer_frame,
    pointer_axis_source,
    pointer_axis_stop,
    pointer_axis_discrete
};

// Keyboard handling with xkbcommon
static void keyboard_keymap(void* data, wl_keyboard* /*wl_keyboard*/, uint32_t format, int fd, uint32_t size) {
    KeyboardData* kd = static_cast<KeyboardData*>(data);
    if (!kd) {
        close(fd);
        return;
    }
    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd);
        return;
    }
    char* map_str = static_cast<char*>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
    if (map_str == MAP_FAILED) {
        close(fd);
        return;
    }
    xkb_keymap_unref(kd->keymap_);
    kd->keymap_ = xkb_keymap_new_from_string(kd->xkb_ctx_, map_str, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(map_str, size);
    close(fd);
    xkb_state_unref(kd->xkb_state_);
    kd->xkb_state_ = xkb_state_new(kd->keymap_);
}

static void keyboard_enter(void* /*data*/, wl_keyboard* /*wl_keyboard*/, uint32_t /*serial*/, wl_surface* /*surface*/, struct wl_array* /*keys*/) {}
static void keyboard_leave(void* /*data*/, wl_keyboard* /*wl_keyboard*/, uint32_t /*serial*/, wl_surface* /*surface*/) {}

static void keyboard_key(void* data, wl_keyboard* /*wl_keyboard*/, uint32_t /*serial*/, uint32_t /*time*/, uint32_t key, uint32_t state) {
    KeyboardData* kd = static_cast<KeyboardData*>(data);
    if (!kd || !kd->input_manager) {
        return;
    }
    KeyState ks = (state == WL_KEYBOARD_KEY_STATE_PRESSED) ? KeyState::Pressed : KeyState::Released;
    if (kd->keymap_ && kd->xkb_state_) {
        xkb_keysym_t ksym = xkb_state_key_get_one_sym(kd->xkb_state_, key + 8);
        ooey::KeyEvent ev{static_cast<int>(ksym), ks};
        kd->input_manager->push_key_event(ev);

        if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
            if (ksym == XKB_KEY_BackSpace || ksym == XKB_KEY_Delete) {
                return;
            }

            char buf[64];
            int len = xkb_keysym_to_utf8(ksym, buf, sizeof(buf));
            if (len > 0) {
                for (int i = 0; i < len; ++i) {
                    unsigned char ch = static_cast<unsigned char>(buf[i]);
                    if (ch >= 32 || ch == '\n' || ch == '\t') {
                        kd->input_manager->push_text_event({static_cast<char32_t>(ch)});
                    }
                }
            }
        }
    } else {
        ooey::KeyEvent ev{static_cast<int>(key), ks};
        kd->input_manager->push_key_event(ev);
    }
}

static void keyboard_modifiers(void* data, wl_keyboard* /*wl_keyboard*/, uint32_t /*serial*/, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
    KeyboardData* kd = static_cast<KeyboardData*>(data);
    if (kd && kd->xkb_state_) {
        xkb_state_update_mask(kd->xkb_state_, mods_depressed, mods_latched, mods_locked, 0, 0, group);
    }
}
static void keyboard_repeat_info(void* /*data*/, wl_keyboard* /*wl_keyboard*/, int32_t /*rate*/, int32_t /*delay*/) {}

static const wl_keyboard_listener g_keyboard_listener = {
    keyboard_keymap,
    keyboard_enter,
    keyboard_leave,
    keyboard_key,
    keyboard_modifiers,
    keyboard_repeat_info
};

// xdg_surface configure: ack then create buffer if needed
static void xdg_surface_configure(void* data, xdg_surface* surface, uint32_t serial) {
    WindowBackend* backend = static_cast<WindowBackend*>(data);
    if (!backend) {
        return;
    }
    xdg_surface_ack_configure(surface, serial);
    backend->handle_xdg_surface_configure(serial);
}

static void xdg_toplevel_configure(void* data, xdg_toplevel* /*toplevel*/, int32_t width, int32_t height, wl_array* /*states*/) {
    WindowBackend* backend = static_cast<WindowBackend*>(data);
    if (!backend) {
        return;
    }
    backend->handle_xdg_toplevel_configure(width, height);
}

WindowBackend::WindowBackend() = default;

WindowBackend::~WindowBackend() {
    destroy();
}

bool WindowBackend::create(const Size& size, const char* title) {
    width_ = size.width;
    height_ = size.height;
    title_ = title ? title : "ooey wayland";

    display_ = wl_display_connect(nullptr);
    if (!display_) {
        std::cerr << "Failed to connect to Wayland display\n";
        return false;
    }

    WaylandState state{};
    registry_ = wl_display_get_registry(display_);
    wl_registry_add_listener(registry_, &g_registry_listener, &state);
    wl_display_roundtrip(display_);

    if (!state.compositor || !state.shm) {
        std::cerr << "Compositor or shm not available\n";
        return false;
    }

    compositor_ = state.compositor;
    shm_ = state.shm;

    if (state.wm_base) {
        xdg_wm_base_add_listener(state.wm_base, &g_xdg_wm_base_listener, this);
    }

    surface_ = wl_compositor_create_surface(compositor_);

    // If xdg wm base is available, create an xdg surface/toplevel so the compositor maps our surface
    xdg_surface* xs = nullptr;
    xdg_toplevel* xt = nullptr;
    if (state.wm_base) {
        xs = xdg_wm_base_get_xdg_surface(state.wm_base, surface_);
        if (xs) {
            xdg_surface_add_listener(xs, &g_xdg_surface_listener, this);
            xt = xdg_surface_get_toplevel(xs);
            if (xt) {
                xdg_toplevel_add_listener(xt, &g_xdg_toplevel_listener, this);
                xdg_toplevel_set_title(xt, title_.c_str());
            }
        }
        // Do not attach content before acknowledging configure; wait for configure callback
        waiting_for_configure_ = true;
        // Commit the surface (no buffer attached) to prompt the compositor to send a configure
        wl_surface_commit(surface_);
        wl_display_roundtrip(display_);
    }

    // Store xdg objects in members if created
    xdg_surface_ = xs;
    xdg_toplevel_ = xt;

    // If no xdg (older compositor?), create shm target immediately
    if (!xdg_surface_) {
        render_target_ = std::make_unique<RenderTarget>(display_, shm_, surface_, width_, height_);
    }

    // Setup seat for input if available
    if (state.seat) {
        seat_ = state.seat;
        // grab pointer and keyboard if present
        wl_seat_add_listener(seat_, nullptr, nullptr); // no-op: concrete listeners created when creating pointer/keyboard
        // Create pointer
        pointer_obj_ = wl_seat_get_pointer(seat_);
        auto pd = std::make_unique<PointerData>();
        pd->input_manager = input_manager_;
        pointer_data_ = std::move(pd);
        wl_pointer_add_listener(pointer_obj_, &g_pointer_listener, pointer_data_.get());

        // Create keyboard
        keyboard_obj_ = wl_seat_get_keyboard(seat_);
        auto kd = std::make_unique<KeyboardData>();
        kd->input_manager = input_manager_;
        kd->xkb_ctx_ = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        kd->keymap_ = nullptr;
        kd->xkb_state_ = nullptr;
        keyboard_data_ = std::move(kd);
        wl_keyboard_add_listener(keyboard_obj_, &g_keyboard_listener, keyboard_data_.get());
        // Process any pending enter/motion events so our listener gets initial pointer coords
        if (display_) {
            wl_display_roundtrip(display_);
        }
    }

    return true;
}

void WindowBackend::destroy() {
    if (render_target_) {
        render_target_.reset();
    }
    if (xdg_toplevel_) {
        xdg_toplevel_destroy(xdg_toplevel_);
        xdg_toplevel_ = nullptr;
    }
    if (xdg_surface_) {
        xdg_surface_destroy(xdg_surface_);
        xdg_surface_ = nullptr;
    }
    if (surface_) {
        wl_surface_destroy(surface_);
        surface_ = nullptr;
    }
    if (shm_) {
        wl_shm_destroy(shm_);
        shm_ = nullptr;
    }
    if (pointer_obj_) {
        wl_pointer_destroy(pointer_obj_);
        pointer_obj_ = nullptr;
    }
    pointer_data_.reset();
    if (keyboard_obj_) {
        wl_keyboard_destroy(keyboard_obj_);
        keyboard_obj_ = nullptr;
    }
    keyboard_data_.reset();
    if (compositor_) {
        wl_compositor_destroy(compositor_);
        compositor_ = nullptr;
    }
    if (registry_) {
        wl_registry_destroy(registry_);
        registry_ = nullptr;
    }
    if (display_) {
        wl_display_disconnect(display_);
        display_ = nullptr;
    }
}

bool WindowBackend::poll_events() {
    if (!display_) {
        return false;
    }
    // Dispatch pending events; return true to keep running
    wl_display_dispatch_pending(display_);
    wl_display_flush(display_);
    return true;
}

void WindowBackend::poll_input() {
    // Input events are pushed from listeners already
}

void WindowBackend::set_input_manager(InputManager* manager) {
    input_manager_ = manager;
    // Update any existing listener contexts so they receive the input manager
    if (pointer_data_) {
        pointer_data_->input_manager = input_manager_;
    }
    if (keyboard_data_) {
        keyboard_data_->input_manager = input_manager_;
    }
}

IRenderTarget* WindowBackend::get_render_target() {
    return render_target_.get();
}

void WindowBackend::handle_xdg_surface_configure(uint32_t serial) {
    // If we were waiting for configure, create our render target now
    if (waiting_for_configure_) {
        render_target_ = std::make_unique<RenderTarget>(display_, shm_, surface_, width_, height_);
        waiting_for_configure_ = false;
        // Present initial empty frame
        if (render_target_) {
            render_target_->clear(Color{0, 0, 0, 255});
            render_target_->present();
            if (display_) {
                wl_display_roundtrip(display_);
            }
        }
    }
}

void WindowBackend::handle_xdg_toplevel_configure(int32_t width, int32_t height) {
    if (width > 0 && height > 0) {
        pending_width_ = width;
        pending_height_ = height;
        this->width_ = width;
        this->height_ = height;
        // If render target exists, recreate it with new size
        if (render_target_) {
            render_target_.reset();
            render_target_ = std::make_unique<RenderTarget>(display_, shm_, surface_, this->width_, this->height_);
        }
    }
}

} // namespace ooey::wayland
