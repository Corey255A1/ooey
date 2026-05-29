#include "ooey/platform/wayland/window_backend.hpp"
#include "ooey/renderer/software_render_target.hpp"
#include "ooey/renderer/gl_render_target.hpp"
#include "ooey/renderer/window_chrome.hpp"
#ifdef OOEY_WAYLAND_HAS_EGL
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <GL/gl.h>
#endif
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
    WindowBackend* backend{};
    int last_x{0};
    int last_y{0};
    uint32_t last_button_serial{0};
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
static void pointer_enter(void* data, wl_pointer* /*wl_pointer*/, uint32_t /*serial*/, wl_surface* /*surface*/, wl_fixed_t surface_x, wl_fixed_t surface_y) {
    PointerData* pointer_data = static_cast<PointerData*>(data);
    if (!pointer_data || !pointer_data->input_manager || !pointer_data->backend) {
        return;
    }
    int x = wl_fixed_to_int(surface_x);
    int y = wl_fixed_to_int(surface_y);
    pointer_data->last_x = x;
    pointer_data->last_y = y;

    ooey::Pointer p{0, x, y, PointerState::Moved};

    if (pointer_data->backend->get_window_chrome()) {
        auto chrome = pointer_data->backend->get_window_chrome();
        Size window_size = pointer_data->backend->get_window_size();
        if (chrome->handle_pointer_event(p, window_size, pointer_data->backend)) {
            return;
        }
        p.x -= chrome->get_border_width();
        p.y -= (chrome->get_border_width() + chrome->get_title_bar_height());
    }

    pointer_data->input_manager->push_pointer_event(p);
}

static void pointer_leave(void* /*data*/, wl_pointer* /*wl_pointer*/, uint32_t /*serial*/, wl_surface* /*surface*/) {}

static void pointer_motion(void* data, wl_pointer* /*wl_pointer*/, uint32_t /*time*/, wl_fixed_t surface_x, wl_fixed_t surface_y) {
    PointerData* pointer_data = static_cast<PointerData*>(data);
    if (!pointer_data || !pointer_data->input_manager || !pointer_data->backend) {
        return;
    }
    int x = wl_fixed_to_int(surface_x);
    int y = wl_fixed_to_int(surface_y);
    pointer_data->last_x = x;
    pointer_data->last_y = y;

    ooey::Pointer p{0, x, y, PointerState::Moved};

    if (pointer_data->backend->get_window_chrome()) {
        auto chrome = pointer_data->backend->get_window_chrome();
        Size window_size = pointer_data->backend->get_window_size();
        if (chrome->handle_pointer_event(p, window_size, pointer_data->backend)) {
            return;
        }
        p.x -= chrome->get_border_width();
        p.y -= (chrome->get_border_width() + chrome->get_title_bar_height());
    }

    pointer_data->input_manager->push_pointer_event(p);
}

static void pointer_button(void* data, wl_pointer* /*wl_pointer*/, uint32_t serial, uint32_t /*time*/, uint32_t button, uint32_t state) {
    PointerData* pointer_data = static_cast<PointerData*>(data);
    if (!pointer_data || !pointer_data->input_manager || !pointer_data->backend) {
        return;
    }
    pointer_data->last_button_serial = serial;

    PointerState pointer_state = (state == WL_POINTER_BUTTON_STATE_PRESSED) ? PointerState::Pressed : PointerState::Released;
    ooey::Pointer p{0, pointer_data->last_x, pointer_data->last_y, pointer_state};

    if (pointer_data->backend->get_window_chrome()) {
        auto chrome = pointer_data->backend->get_window_chrome();
        Size window_size = pointer_data->backend->get_window_size();
        if (chrome->handle_pointer_event(p, window_size, pointer_data->backend)) {
            return;
        }
        p.x -= chrome->get_border_width();
        p.y -= (chrome->get_border_width() + chrome->get_title_bar_height());
    }

    pointer_data->input_manager->push_pointer_event(p);
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
    KeyboardData* keyboard_data = static_cast<KeyboardData*>(data);
    if (!keyboard_data) {
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
    xkb_keymap_unref(keyboard_data->keymap_);
    keyboard_data->keymap_ = xkb_keymap_new_from_string(keyboard_data->xkb_ctx_, map_str, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(map_str, size);
    close(fd);
    xkb_state_unref(keyboard_data->xkb_state_);
    keyboard_data->xkb_state_ = xkb_state_new(keyboard_data->keymap_);
}

static void keyboard_enter(void* /*data*/, wl_keyboard* /*wl_keyboard*/, uint32_t /*serial*/, wl_surface* /*surface*/, struct wl_array* /*keys*/) {}
static void keyboard_leave(void* /*data*/, wl_keyboard* /*wl_keyboard*/, uint32_t /*serial*/, wl_surface* /*surface*/) {}

static void keyboard_key(void* data, wl_keyboard* /*wl_keyboard*/, uint32_t /*serial*/, uint32_t /*time*/, uint32_t key, uint32_t state) {
    KeyboardData* keyboard_data = static_cast<KeyboardData*>(data);
    if (!keyboard_data || !keyboard_data->input_manager) {
        return;
    }
    KeyState key_state = (state == WL_KEYBOARD_KEY_STATE_PRESSED) ? KeyState::Pressed : KeyState::Released;
    if (keyboard_data->keymap_ && keyboard_data->xkb_state_) {
        xkb_keysym_t key_sym = xkb_state_key_get_one_sym(keyboard_data->xkb_state_, key + 8);
        ooey::KeyEvent ev{static_cast<int>(key_sym), key_state};
        keyboard_data->input_manager->push_key_event(ev);

        if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
            if (key_sym == XKB_KEY_BackSpace || key_sym == XKB_KEY_Delete) {
                return;
            }

            char buf[64];
            int len = xkb_keysym_to_utf8(key_sym, buf, sizeof(buf));
            if (len > 0) {
                for (int i = 0; i < len; ++i) {
                    unsigned char ch = static_cast<unsigned char>(buf[i]);
                    if (ch >= 32 || ch == '\n' || ch == '\t') {
                        keyboard_data->input_manager->push_text_event({static_cast<char32_t>(ch)});
                    }
                }
            }
        }
    } else {
        ooey::KeyEvent ev{static_cast<int>(key), key_state};
        keyboard_data->input_manager->push_key_event(ev);
    }
}

static void keyboard_modifiers(void* data, wl_keyboard* /*wl_keyboard*/, uint32_t /*serial*/, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
    KeyboardData* keyboard_data = static_cast<KeyboardData*>(data);
    if (keyboard_data && keyboard_data->xkb_state_) {
        xkb_state_update_mask(keyboard_data->xkb_state_, mods_depressed, mods_latched, mods_locked, 0, 0, group);
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

WindowBackend::WindowBackend() {
    set_window_chrome(std::make_shared<WindowChrome>());
}

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

#ifdef OOEY_WAYLAND_HAS_EGL
    init_egl();
#endif

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
    xdg_surface* xdg_surface_ptr = nullptr;
    xdg_toplevel* xdg_toplevel_ptr = nullptr;
    if (state.wm_base) {
        xdg_surface_ptr = xdg_wm_base_get_xdg_surface(state.wm_base, surface_);
        if (xdg_surface_ptr) {
            xdg_surface_add_listener(xdg_surface_ptr, &g_xdg_surface_listener, this);
            xdg_toplevel_ptr = xdg_surface_get_toplevel(xdg_surface_ptr);
            if (xdg_toplevel_ptr) {
                xdg_toplevel_add_listener(xdg_toplevel_ptr, &g_xdg_toplevel_listener, this);
                xdg_toplevel_set_title(xdg_toplevel_ptr, title_.c_str());
            }
        }
        // Do not attach content before acknowledging configure; wait for configure callback
        waiting_for_configure_ = true;
        // Commit the surface (no buffer attached) to prompt the compositor to send a configure
        wl_surface_commit(surface_);
        wl_display_roundtrip(display_);
    }

    // Store xdg objects in members if created
    xdg_surface_ = xdg_surface_ptr;
    xdg_toplevel_ = xdg_toplevel_ptr;

    // If no xdg (older compositor?), create shm target immediately
    if (!xdg_surface_) {
        recreate_render_target(width_, height_);
    }

    // Setup seat for input if available
    if (state.seat) {
        seat_ = state.seat;
        // grab pointer and keyboard if present
        wl_seat_add_listener(seat_, nullptr, nullptr); // no-op: concrete listeners created when creating pointer/keyboard
        // Create pointer
        pointer_obj_ = wl_seat_get_pointer(seat_);
        auto pointer_data_ptr = std::make_unique<PointerData>();
        pointer_data_ptr->input_manager = input_manager_;
        pointer_data_ptr->backend = this;
        pointer_data_ = std::move(pointer_data_ptr);
        wl_pointer_add_listener(pointer_obj_, &g_pointer_listener, pointer_data_.get());

        // Create keyboard
        keyboard_obj_ = wl_seat_get_keyboard(seat_);
        auto keyboard_data_ptr = std::make_unique<KeyboardData>();
        keyboard_data_ptr->input_manager = input_manager_;
        keyboard_data_ptr->xkb_ctx_ = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        keyboard_data_ptr->keymap_ = nullptr;
        keyboard_data_ptr->xkb_state_ = nullptr;
        keyboard_data_ = std::move(keyboard_data_ptr);
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
#ifdef OOEY_WAYLAND_HAS_EGL
    cleanup_egl();
#endif
    if (mapped_data_) {
        munmap(mapped_data_, mapped_size_);
        mapped_data_ = nullptr;
    }
    if (wl_buffer_) {
        wl_buffer_destroy(wl_buffer_);
        wl_buffer_ = nullptr;
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
    if (!display_ || should_close_) {
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
    if (decorated_render_target_) {
        return decorated_render_target_.get();
    }
    return render_target_.get();
}

void WindowBackend::set_window_chrome(std::shared_ptr<WindowChrome> chrome) {
    window_chrome_ = chrome;
    if (window_chrome_ && render_target_) {
        decorated_render_target_ = std::make_unique<ooey::ChromeRenderTarget>(render_target_.get(), window_chrome_, Size{width_, height_});
    } else {
        decorated_render_target_.reset();
    }
}

void WindowBackend::start_interactive_move() {
    if (xdg_toplevel_ && seat_ && pointer_data_) {
        xdg_toplevel_move(xdg_toplevel_, seat_, pointer_data_->last_button_serial);
    }
}

void WindowBackend::start_interactive_resize(WindowResizeEdge edge) {
    if (xdg_toplevel_ && seat_ && pointer_data_) {
        uint32_t wl_edge = 0;
        switch (edge) {
            case WindowResizeEdge::Top:          wl_edge = 1; break;
            case WindowResizeEdge::Bottom:       wl_edge = 2; break;
            case WindowResizeEdge::Left:         wl_edge = 4; break;
            case WindowResizeEdge::TopLeft:      wl_edge = 5; break;
            case WindowResizeEdge::BottomLeft:   wl_edge = 6; break;
            case WindowResizeEdge::Right:        wl_edge = 8; break;
            case WindowResizeEdge::TopRight:     wl_edge = 9; break;
            case WindowResizeEdge::BottomRight:  wl_edge = 10; break;
            default: return;
        }
        xdg_toplevel_resize(xdg_toplevel_, seat_, pointer_data_->last_button_serial, wl_edge);
    }
}

static int create_shm_file(size_t size) {
#if defined(__linux__)
    int fd = -1;
#ifdef MFD_CLOEXEC
    fd = memfd_create("ooey_shm", MFD_CLOEXEC);
#endif
    if (fd >= 0) {
        if (ftruncate(fd, size) < 0) {
            close(fd);
            return -1;
        }
        return fd;
    }
#endif
    static int counter = 0;
    std::string name = "/ooey_shm_" + std::to_string(getpid()) + "_" + std::to_string(counter++);
    fd = shm_open(name.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd < 0) {
        return -1;
    }
    shm_unlink(name.c_str());
    long page = sysconf(_SC_PAGESIZE);
    if (page > 0) {
        size_t pages = (size + page - 1) / page;
        size = pages * page;
    }
    if (ftruncate(fd, size) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

void WindowBackend::recreate_render_target(int width, int height) {
#ifdef OOEY_WAYLAND_HAS_EGL
    if (use_egl_) {
        if (egl_window_) {
            wl_egl_window_resize(egl_window_, width, height, 0, 0);
            if (render_target_) {
                render_target_->resize(width, height);
            }
        } else {
            egl_window_ = wl_egl_window_create(surface_, width, height);
            if (!egl_window_) {
                std::cerr << "Wayland EGL error: Failed to create wl_egl_window. Falling back to software.\n";
                use_egl_ = false;
                recreate_render_target(width, height);
                return;
            }
            egl_surface_ = eglCreateWindowSurface(egl_display_, egl_config_, reinterpret_cast<EGLNativeWindowType>(egl_window_), nullptr);
            if (egl_surface_ == EGL_NO_SURFACE) {
                std::cerr << "Wayland EGL error: Failed to create EGL surface. Falling back to software.\n";
                wl_egl_window_destroy(egl_window_);
                egl_window_ = nullptr;
                use_egl_ = false;
                recreate_render_target(width, height);
                return;
            }
            if (!eglMakeCurrent(egl_display_, egl_surface_, egl_surface_, egl_context_)) {
                std::cerr << "Wayland EGL error: Failed to make EGL context current. Falling back to software.\n";
                eglDestroySurface(egl_display_, egl_surface_);
                egl_surface_ = nullptr;
                wl_egl_window_destroy(egl_window_);
                egl_window_ = nullptr;
                use_egl_ = false;
                recreate_render_target(width, height);
                return;
            }
            render_target_ = std::make_unique<GlRenderTarget>(width, height, [this]() {
                if (egl_display_ && egl_surface_) {
                    eglSwapBuffers(egl_display_, egl_surface_);
                }
            });
        }
        if (window_chrome_) {
            decorated_render_target_ = std::make_unique<ooey::ChromeRenderTarget>(render_target_.get(), window_chrome_, Size{width, height});
        } else {
            decorated_render_target_.reset();
        }
        return;
    }
#endif

    if (mapped_data_) {
        munmap(mapped_data_, mapped_size_);
        mapped_data_ = nullptr;
    }
    if (wl_buffer_) {
        wl_buffer_destroy(wl_buffer_);
        wl_buffer_ = nullptr;
    }

    int stride = width * 4;
    mapped_size_ = stride * height;
    int fd = create_shm_file(mapped_size_);
    if (fd < 0) {
        std::cerr << "Wayland error: Failed to create shm file\n";
        return;
    }

    mapped_data_ = static_cast<uint8_t*>(mmap(nullptr, mapped_size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    if (mapped_data_ == MAP_FAILED) {
        std::cerr << "Wayland error: mmap failed\n";
        mapped_data_ = nullptr;
        close(fd);
        return;
    }

    wl_shm_pool* pool = wl_shm_create_pool(shm_, fd, static_cast<int>(mapped_size_));
    wl_buffer_ = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);

    released_ = false;
    static const wl_buffer_listener buffer_listener = {
        [](void* data, wl_buffer* /*buffer*/) {
            WindowBackend* self = static_cast<WindowBackend*>(data);
            self->released_ = true;
        }
    };
    wl_buffer_add_listener(wl_buffer_, &buffer_listener, this);

    render_target_ = std::make_unique<SoftwareRenderTarget>(mapped_data_, width, height, stride, [this, width, height]() {
        if (!surface_ || !wl_buffer_ || !display_) {
            return;
        }
        released_ = false;
        wl_surface_attach(surface_, wl_buffer_, 0, 0);
        wl_surface_damage(surface_, 0, 0, width, height);
        wl_surface_commit(surface_);
        wl_display_flush(display_);

        // Wait for the compositor to release the buffer before returning
        while (!released_) {
            if (wl_display_dispatch(display_) < 0) {
                break;
            }
        }
    });
    if (window_chrome_) {
        decorated_render_target_ = std::make_unique<ooey::ChromeRenderTarget>(render_target_.get(), window_chrome_, Size{width, height});
    } else {
        decorated_render_target_.reset();
    }
}

void WindowBackend::handle_xdg_surface_configure(uint32_t serial) {
    if (waiting_for_configure_) {
        recreate_render_target(width_, height_);
        waiting_for_configure_ = false;
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
        if (render_target_) {
            recreate_render_target(this->width_, this->height_);
        }
    }
}

#ifdef OOEY_WAYLAND_HAS_EGL
bool WindowBackend::init_egl() {
    egl_display_ = eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(display_));
    if (egl_display_ == EGL_NO_DISPLAY) {
        std::cerr << "Wayland EGL: eglGetDisplay failed\n";
        return false;
    }

    EGLint major, minor;
    if (!eglInitialize(egl_display_, &major, &minor)) {
        std::cerr << "Wayland EGL: eglInitialize failed\n";
        egl_display_ = nullptr;
        return false;
    }

    std::cout << "Wayland EGL initialized: version " << major << "." << minor << "\n";

    if (!eglBindAPI(EGL_OPENGL_API)) {
        std::cerr << "Wayland EGL: eglBindAPI failed\n";
        eglTerminate(egl_display_);
        egl_display_ = nullptr;
        return false;
    }

    EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_NONE
    };

    EGLint num_configs;
    if (!eglChooseConfig(egl_display_, attribs, &egl_config_, 1, &num_configs) || num_configs < 1) {
        std::cerr << "Wayland EGL: eglChooseConfig failed\n";
        eglTerminate(egl_display_);
        egl_display_ = nullptr;
        return false;
    }

    EGLint context_attribs[] = {
        EGL_CONTEXT_MAJOR_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 0,
        EGL_NONE
    };

    egl_context_ = eglCreateContext(egl_display_, egl_config_, EGL_NO_CONTEXT, context_attribs);
    if (egl_context_ == EGL_NO_CONTEXT) {
        egl_context_ = eglCreateContext(egl_display_, egl_config_, EGL_NO_CONTEXT, nullptr);
    }

    if (egl_context_ == EGL_NO_CONTEXT) {
        std::cerr << "Wayland EGL: eglCreateContext failed\n";
        eglTerminate(egl_display_);
        egl_display_ = nullptr;
        return false;
    }

    use_egl_ = true;
    return true;
}

void WindowBackend::cleanup_egl() {
    if (egl_display_) {
        eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (egl_surface_) {
            eglDestroySurface(egl_display_, egl_surface_);
            egl_surface_ = nullptr;
        }
        if (egl_context_) {
            eglDestroyContext(egl_display_, egl_context_);
            egl_context_ = nullptr;
        }
        if (egl_window_) {
            wl_egl_window_destroy(egl_window_);
            egl_window_ = nullptr;
        }
        eglTerminate(egl_display_);
        egl_display_ = nullptr;
    }
    use_egl_ = false;
}
#endif

} // namespace ooey::wayland
