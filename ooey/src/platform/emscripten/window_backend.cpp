#include "ooey/platform/emscripten/window_backend.hpp"
#include "ooey/renderer/gl_render_target.hpp"
#include <emscripten/html5.h>
#include <iostream>
#include <cstring>

namespace ooey::emscripten {

// Static event handlers to map to input manager
static EM_BOOL on_mouse_event(int event_type, const EmscriptenMouseEvent* mouse_event, void* user_data) {
    auto* input_mgr = static_cast<InputManager*>(user_data);
    if (!input_mgr) return EM_FALSE;

    PointerState state;
    if (event_type == EMSCRIPTEN_EVENT_MOUSEDOWN) {
        state = PointerState::Pressed;
    } else if (event_type == EMSCRIPTEN_EVENT_MOUSEUP) {
        state = PointerState::Released;
    } else if (event_type == EMSCRIPTEN_EVENT_MOUSEMOVE) {
        state = PointerState::Moved;
    } else {
        return EM_FALSE;
    }

    input_mgr->push_pointer_event({0, static_cast<int>(mouse_event->targetX), static_cast<int>(mouse_event->targetY), state});
    return EM_TRUE;
}

static EM_BOOL on_key_event(int event_type, const EmscriptenKeyboardEvent* key_event, void* user_data) {
    auto* input_mgr = static_cast<InputManager*>(user_data);
    if (!input_mgr) return EM_FALSE;

    KeyState state = (event_type == EMSCRIPTEN_EVENT_KEYDOWN) ? KeyState::Pressed : KeyState::Released;

    // Check for Backspace
    if (std::strcmp(key_event->key, "Backspace") == 0) {
        input_mgr->push_key_event({8, state});
        return EM_TRUE;
    } else if (std::strcmp(key_event->key, "Delete") == 0) {
        input_mgr->push_key_event({127, state});
        return EM_TRUE;
    }

    // For printable keys, trigger TextEvent on keydown
    if (event_type == EMSCRIPTEN_EVENT_KEYDOWN) {
        // If the key has length 1, it's a printable ASCII/UTF-8 character
        if (std::strlen(key_event->key) == 1) {
            char32_t codepoint = static_cast<char32_t>(key_event->key[0]);
            input_mgr->push_text_event({codepoint});
        }
    }

    return EM_TRUE;
}

WindowBackend::WindowBackend() = default;

WindowBackend::~WindowBackend() {
    destroy();
}

bool WindowBackend::create(const Size& size, const char* /*title*/) {
    // Create Emscripten WebGL Context
    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    attr.majorVersion = 1; // Support legacy fixed-function pipeline emulation
    attr.minorVersion = 0;
    
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context("#canvas", &attr);
    if (ctx <= 0) {
        std::cerr << "Emscripten WindowBackend: Failed to create WebGL context\n";
        return false;
    }
    
    emscripten_webgl_make_context_current(ctx);

    // Register mouse callbacks on canvas
    emscripten_set_mousedown_callback("#canvas", &input_manager_, EM_FALSE, [](int type, const EmscriptenMouseEvent* ev, void* data) {
        return on_mouse_event(type, ev, *static_cast<InputManager**>(data));
    });
    emscripten_set_mouseup_callback("#canvas", &input_manager_, EM_FALSE, [](int type, const EmscriptenMouseEvent* ev, void* data) {
        return on_mouse_event(type, ev, *static_cast<InputManager**>(data));
    });
    emscripten_set_mousemove_callback("#canvas", &input_manager_, EM_FALSE, [](int type, const EmscriptenMouseEvent* ev, void* data) {
        return on_mouse_event(type, ev, *static_cast<InputManager**>(data));
    });

    // Register keyboard callbacks globally (on window/document)
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, &input_manager_, EM_FALSE, [](int type, const EmscriptenKeyboardEvent* ev, void* data) {
        return on_key_event(type, ev, *static_cast<InputManager**>(data));
    });
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, &input_manager_, EM_FALSE, [](int type, const EmscriptenKeyboardEvent* ev, void* data) {
        return on_key_event(type, ev, *static_cast<InputManager**>(data));
    });

    render_target_ = std::make_unique<GlRenderTarget>(size.width, size.height);

    return true;
}

void WindowBackend::destroy() {
    // WebGL context is managed by browser
}

bool WindowBackend::poll_events() {
    return true;
}

void WindowBackend::poll_input() {
}

renderer::IRenderTarget* WindowBackend::get_render_target() {
    return render_target_.get();
}

} // namespace ooey::emscripten
