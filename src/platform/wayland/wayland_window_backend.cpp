#include "ooey/platform/wayland/wayland_window_backend.hpp"
#include "ooey/i_render_target.hpp"
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

namespace ooey {

// Helper to create anonymous shared memory file (memfd or shm_open fallback)
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
    // Fallback to shm_open
    static int counter = 0;
    std::string name = "/ooey_shm_" + std::to_string(getpid()) + "_" + std::to_string(counter++);
    fd = shm_open(name.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd < 0) return -1;
    shm_unlink(name.c_str());
    // Round size up to page size to avoid 'data too big for buffer' edge cases
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

class ShmRenderTarget : public IRenderTarget {
public:
    ShmRenderTarget(struct wl_display* display, struct wl_shm* shm, struct wl_surface* surface, int width, int height)
        : shm_(shm), surface_(surface), width_(width), height_(height) {
        display_ = display;
        stride_ = width_ * 4;
        size_t size = stride_ * height_;
        int fd = create_shm_file(size);
        if (fd < 0) {
            std::cerr << "Failed to create shm file\n";
            data_ = nullptr;
            return;
        }
        data_ = static_cast<uint8_t*>(mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
        if (data_ == MAP_FAILED) {
            std::cerr << "mmap failed\n";
            data_ = nullptr;
            close(fd);
            return;
        }

        wl_shm_pool* pool = wl_shm_create_pool(shm_, fd, static_cast<int>(size));
        buffer_ = wl_shm_pool_create_buffer(pool, 0, width_, height_, stride_, WL_SHM_FORMAT_XRGB8888);
        wl_shm_pool_destroy(pool);
        close(fd);

        // Setup buffer release listener
        released_ = false;
        static const wl_buffer_listener buffer_listener = {[](void* data, wl_buffer* /*buffer*/) {
            ShmRenderTarget* self = static_cast<ShmRenderTarget*>(data);
            self->released_ = true;
        }};
        wl_buffer_add_listener(buffer_, &buffer_listener, this);
    }

    ~ShmRenderTarget() override {
        if (data_) {
            size_t size = stride_ * height_;
            munmap(data_, size);
            data_ = nullptr;
        }
        if (buffer_) {
            wl_buffer_destroy(buffer_);
            buffer_ = nullptr;
        }
    }

    void clear(Color color) override {
        if (!data_) return;
        uint8_t r = color.r;
        uint8_t g = color.g;
        uint8_t b = color.b;
        uint32_t pixel = (r << 16) | (g << 8) | (b);
        for (int y = 0; y < height_; ++y) {
            uint32_t* row = reinterpret_cast<uint32_t*>(data_ + y * stride_);
            for (int x = 0; x < width_; ++x) row[x] = pixel;
        }
    }

    void draw_geometry(const Geometry& geometry) override {
        if (!data_) return;
        // Simple rasterization: fill the geometry's bounding box with the first vertex color.
        if (geometry.vertices.empty()) return;
        int minx = INT_MAX, miny = INT_MAX, maxx = INT_MIN, maxy = INT_MIN;
        for (const auto& v : geometry.vertices) {
            int x = static_cast<int>(v.x);
            int y = static_cast<int>(v.y);
            if (x < minx) minx = x;
            if (y < miny) miny = y;
            if (x > maxx) maxx = x;
            if (y > maxy) maxy = y;
        }
        if (minx == INT_MAX) return;
        int w = maxx - minx;
        int h = maxy - miny;
        if (w <= 0) w = 1;
        if (h <= 0) h = 1;
        draw_filled_rect(minx, miny, w, h, geometry.vertices[0].color);
    }

    Size measure_text(const std::string& text, const Font& font) override {
        return Size{static_cast<int>(text.length() * (font.size * 0.6)), font.size};
    }

    void draw_text(const std::string& text, const Font& font, const Point& position, Color color) override {
        // Placeholder: draw a simple rectangle as text background
        Size s = measure_text(text, font);
        draw_filled_rect(position.x, position.y, s.width, s.height, color);
    }

    void present() override {
        if (!surface_ || !buffer_ || !display_) return;
        released_ = false;
        wl_surface_attach(surface_, buffer_, 0, 0);
        wl_surface_damage(surface_, 0, 0, width_, height_);
        wl_surface_commit(surface_);
        wl_display_flush(display_);

        // Wait for the compositor to release the buffer before returning
        while (!released_) {
            if (wl_display_dispatch(display_) < 0) break;
        }
    }

private:
    void draw_filled_rect(int x, int y, int w, int h, Color color) {
        if (!data_) return;
        uint32_t pixel = (color.r << 16) | (color.g << 8) | (color.b);
        for (int yy = y; yy < y + h; ++yy) {
            if (yy < 0 || yy >= height_) continue;
            uint32_t* row = reinterpret_cast<uint32_t*>(data_ + yy * stride_);
            for (int xx = x; xx < x + w; ++xx) {
                if (xx < 0 || xx >= width_) continue;
                row[xx] = pixel;
            }
        }
    }

    struct wl_shm* shm_{};
    struct wl_surface* surface_{};
    struct wl_buffer* buffer_{};
    struct wl_display* display_{};
    uint8_t* data_{};
    int width_{};
    int height_{};
    int stride_{};
    bool released_{false};
};

// Wayland backend implementation

struct wl_registry_listener registry_listener;

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
struct PointerData {
    InputManager* input_manager{};
    int last_x{0};
    int last_y{0};
};

static void pointer_enter(void* data, wl_pointer* /*wl_pointer*/, uint32_t /*serial*/, wl_surface* /*surface*/, wl_fixed_t /*sx*/, wl_fixed_t /*sy*/) {}
static void pointer_leave(void* /*data*/, wl_pointer* /*wl_pointer*/, uint32_t /*serial*/, wl_surface* /*surface*/) {}
static void pointer_motion(void* data, wl_pointer* /*wl_pointer*/, uint32_t /*time*/, wl_fixed_t sx, wl_fixed_t sy) {
    PointerData* pd = static_cast<PointerData*>(data);
    if (!pd->input_manager) return;
    int x = wl_fixed_to_int(sx);
    int y = wl_fixed_to_int(sy);
    pd->last_x = x;
    pd->last_y = y;
    ooey::Pointer p{0, x, y, PointerState::Moved};
    pd->input_manager->push_pointer_event(p);
}
static void pointer_button(void* data, wl_pointer* /*wl_pointer*/, uint32_t /*serial*/, uint32_t /*time*/, uint32_t button, uint32_t state) {
    PointerData* pd = static_cast<PointerData*>(data);
    if (!pd->input_manager) return;
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
struct KeyboardData {
    InputManager* input_manager{};
    struct xkb_context* xkb_ctx{};
    struct xkb_keymap* keymap{};
    struct xkb_state* xkb_state{};
};

static void keyboard_keymap(void* data, wl_keyboard* /*wl_keyboard*/, uint32_t format, int fd, uint32_t size) {
    KeyboardData* kd = static_cast<KeyboardData*>(data);
    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd);
        return;
    }
    char* map_str = static_cast<char*>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
    if (map_str == MAP_FAILED) {
        close(fd);
        return;
    }
    xkb_keymap_unref(kd->keymap);
    kd->keymap = xkb_keymap_new_from_string(kd->xkb_ctx, map_str, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(map_str, size);
    close(fd);
    xkb_state_unref(kd->xkb_state);
    kd->xkb_state = xkb_state_new(kd->keymap);
}

static void keyboard_enter(void* /*data*/, wl_keyboard* /*wl_keyboard*/, uint32_t /*serial*/, wl_surface* /*surface*/, struct wl_array* /*keys*/) {}
static void keyboard_leave(void* /*data*/, wl_keyboard* /*wl_keyboard*/, uint32_t /*serial*/, wl_surface* /*surface*/) {}
static void keyboard_key(void* data, wl_keyboard* /*wl_keyboard*/, uint32_t /*serial*/, uint32_t /*time*/, uint32_t key, uint32_t state) {
    KeyboardData* kd = static_cast<KeyboardData*>(data);
    if (!kd->input_manager) return;
    KeyState ks = (state == WL_KEYBOARD_KEY_STATE_PRESSED) ? KeyState::Pressed : KeyState::Released;
    ooey::KeyEvent ev{static_cast<int>(key), ks};
    kd->input_manager->push_key_event(ev);

    if (state == WL_KEYBOARD_KEY_STATE_PRESSED && kd->keymap && kd->xkb_state) {
        xkb_keysym_t ksym = xkb_state_key_get_one_sym(kd->xkb_state, key + 8);
        char buf[64];
        int len = xkb_keysym_to_utf8(ksym, buf, sizeof(buf));
        if (len > 0) {
            for (int i = 0; i < len; ++i) {
                kd->input_manager->push_text_event({static_cast<char32_t>(static_cast<unsigned char>(buf[i]))});
            }
        }
    }
}

static void keyboard_modifiers(void* /*data*/, wl_keyboard* /*wl_keyboard*/, uint32_t /*serial*/, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {}

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
    WaylandWindowBackend* backend = static_cast<WaylandWindowBackend*>(data);
    if (!backend) return;
    xdg_surface_ack_configure(surface, serial);
    backend->handle_xdg_surface_configure(serial);
}

static void xdg_toplevel_configure(void* data, xdg_toplevel* /*toplevel*/, int32_t width, int32_t height, wl_array* /*states*/) {
    WaylandWindowBackend* backend = static_cast<WaylandWindowBackend*>(data);
    if (!backend) return;
    backend->handle_xdg_toplevel_configure(width, height);
}

WaylandWindowBackend::WaylandWindowBackend() = default;

WaylandWindowBackend::~WaylandWindowBackend() {
    destroy();
}

bool WaylandWindowBackend::create(const Size& size, const char* title) {
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
        render_target_ = std::make_unique<ShmRenderTarget>(display_, shm_, surface_, width_, height_);
    }

    // Setup seat for input if available
    if (state.seat) {
        seat_ = state.seat;
        // grab pointer and keyboard if present
        wl_seat_add_listener(seat_, nullptr, nullptr); // no-op: concrete listeners created when creating pointer/keyboard
        // Create pointer
        wl_pointer* pointer = wl_seat_get_pointer(seat_);
        PointerData* pd = new PointerData();
        pd->input_manager = input_manager_;
        wl_pointer_add_listener(pointer, &g_pointer_listener, pd);

        // Create keyboard
        wl_keyboard* keyboard = wl_seat_get_keyboard(seat_);
        KeyboardData* kd = new KeyboardData();
        kd->input_manager = input_manager_;
        kd->xkb_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        kd->keymap = nullptr;
        kd->xkb_state = nullptr;
        wl_keyboard_add_listener(keyboard, &g_keyboard_listener, kd);
    }

    return true;
}

void WaylandWindowBackend::destroy() {
    if (render_target_) render_target_.reset();
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

bool WaylandWindowBackend::poll_events() {
    if (!display_) return false;
    // Dispatch pending events; return true to keep running
    wl_display_dispatch_pending(display_);
    wl_display_flush(display_);
    return true;
}

void WaylandWindowBackend::poll_input() {
    // Input events are pushed from listeners already
}

IRenderTarget* WaylandWindowBackend::get_render_target() {
    return render_target_.get();
}

void WaylandWindowBackend::handle_xdg_surface_configure(uint32_t serial) {
    // If we were waiting for configure, create our render target now
    if (waiting_for_configure_) {
        render_target_ = std::make_unique<ShmRenderTarget>(display_, shm_, surface_, width_, height_);
        waiting_for_configure_ = false;
        // Present initial empty frame
        if (render_target_) {
            render_target_->clear(Color{0,0,0,255});
            render_target_->present();
            if (display_) wl_display_roundtrip(display_);
        }
    }
}

void WaylandWindowBackend::handle_xdg_toplevel_configure(int32_t width, int32_t height) {
    if (width > 0 && height > 0) {
        pending_width_ = width;
        pending_height_ = height;
        this->width_ = width;
        this->height_ = height;
        // If render target exists, recreate it with new size
        if (render_target_) {
            render_target_.reset();
            render_target_ = std::make_unique<ShmRenderTarget>(display_, shm_, surface_, this->width_, this->height_);
        }
    }
}

} // namespace ooey
