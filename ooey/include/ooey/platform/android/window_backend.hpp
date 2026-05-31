#pragma once

#include "ooey/i_window_backend.hpp"
#include "ooey/types.hpp"
#include <memory>

struct android_app;
struct ANativeWindow;
struct AAssetManager;

namespace ooey::android {

extern AAssetManager* g_asset_manager;

class WindowBackend : public IWindowBackend {
public:
    WindowBackend(struct android_app* app);
    virtual ~WindowBackend() override;

    // Platform lifecycle hooks called from Android event loop
    void on_window_created(ANativeWindow* window);
    void on_window_destroyed();
    void on_window_resized();

    // Input handlers
    void handle_pointer_event(int id, float x, float y, PointerState state);
    void handle_key_event(int key_code, KeyState state);
    void handle_text_event(char32_t codepoint);

    // IWindowBackend interface implementation
    bool create(const Size& size, const char* title) override;
    void destroy() override;
    bool poll_events() override;
    void poll_input() override;
    IRenderTarget* get_render_target() override;
    void set_input_manager(InputManager* manager) override;
    void set_window_chrome(std::shared_ptr<WindowChrome> chrome) override;
    std::shared_ptr<WindowChrome> get_window_chrome() const override;
    void start_interactive_move() override;
    void start_interactive_resize(WindowResizeEdge edge) override;
    void request_close() override;
    Size get_size() const override;

private:
    void init_software_surface();
    void present_software_frame();

    struct android_app* app_{nullptr};
    ANativeWindow* native_window_{nullptr};
    InputManager* input_manager_{nullptr};

    int width_{0};
    int height_{0};
    bool running_{true};

    // Software rendering support (default fallback)
    std::unique_ptr<IRenderTarget> render_target_;
    std::vector<uint8_t> software_buffer_;
};

} // namespace ooey::android
