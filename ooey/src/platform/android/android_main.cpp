#include <android/native_activity.h>
#include <android_native_app_glue.h>
#include <android/log.h>
#include <android/keycodes.h>
#include "ooey/platform/android/window_backend.hpp"
#include "gooey/application.hpp"
#include "ooey/input.hpp"
#include <memory>

#define LOG_TAG "OOEY_ANDROID"
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))

// Direct routing of Android lifecycle commands to our WindowBackend
static void android_handle_cmd(struct android_app* app, int32_t cmd) {
    auto* backend = static_cast<ooey::android::WindowBackend*>(app->userData);
    if (!backend) return;

    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            LOGI("APP_CMD_INIT_WINDOW received");
            if (app->window) {
                backend->on_window_created(app->window);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            LOGI("APP_CMD_TERM_WINDOW received");
            backend->on_window_destroyed();
            break;
        case APP_CMD_WINDOW_RESIZED:
            LOGI("APP_CMD_WINDOW_RESIZED received");
            backend->on_window_resized();
            break;
        case APP_CMD_DESTROY:
            LOGI("APP_CMD_DESTROY received");
            backend->destroy();
            break;
        default:
            break;
    }
}

// Convert Android motion events (Touches) and key events to OOEY input events
static int32_t android_handle_input(struct android_app* app, AInputEvent* event) {
    auto* backend = static_cast<ooey::android::WindowBackend*>(app->userData);
    if (!backend) return 0;

    int32_t type = AInputEvent_getType(event);
    if (type == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event);
        int32_t action_code = action & AMOTION_EVENT_ACTION_MASK;
        int32_t pointer_index = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

        int pointer_id = AMotionEvent_getPointerId(event, pointer_index);
        float x = AMotionEvent_getX(event, pointer_index);
        float y = AMotionEvent_getY(event, pointer_index);

        ooey::PointerState state = ooey::PointerState::Moved;
        if (action_code == AMOTION_EVENT_ACTION_DOWN || action_code == AMOTION_EVENT_ACTION_POINTER_DOWN) {
            state = ooey::PointerState::Pressed;
        } else if (action_code == AMOTION_EVENT_ACTION_UP || action_code == AMOTION_EVENT_ACTION_POINTER_UP) {
            state = ooey::PointerState::Released;
        }

        backend->handle_pointer_event(pointer_id, x, y, state);
        return 1;
    }

    if (type == AINPUT_EVENT_TYPE_KEY) {
        int32_t action = AKeyEvent_getAction(event);
        int32_t key_code = AKeyEvent_getKeyCode(event);
        int32_t meta_state = AKeyEvent_getMetaState(event);
        ooey::KeyState state = (action == AKEY_EVENT_ACTION_DOWN) ? ooey::KeyState::Pressed : ooey::KeyState::Released;
        
        // Translate Android keycode to standard values for OOEY
        int translated_key = key_code;
        if (key_code == AKEYCODE_DEL) {
            translated_key = 8; // Backspace
        } else if (key_code == AKEYCODE_ENTER) {
            translated_key = 13; // Enter
        }

        backend->handle_key_event(translated_key, state);

        // If it's a key down, generate text input events for printable characters
        if (action == AKEY_EVENT_ACTION_DOWN) {
            char32_t codepoint = 0;
            if (key_code >= AKEYCODE_A && key_code <= AKEYCODE_Z) { // A-Z
                bool shift = (meta_state & (AMETA_SHIFT_ON | AMETA_SHIFT_LEFT_ON | AMETA_SHIFT_RIGHT_ON)) != 0;
                codepoint = (shift ? 'A' : 'a') + (key_code - AKEYCODE_A);
            } else if (key_code >= AKEYCODE_0 && key_code <= AKEYCODE_9) { // 0-9
                bool shift = (meta_state & (AMETA_SHIFT_ON | AMETA_SHIFT_LEFT_ON | AMETA_SHIFT_RIGHT_ON)) != 0;
                if (!shift) {
                    codepoint = '0' + (key_code - AKEYCODE_0);
                } else {
                    const char shift_map[] = { ')', '!', '@', '#', '$', '%', '^', '&', '*', '(' };
                    codepoint = shift_map[key_code - AKEYCODE_0];
                }
            } else if (key_code == AKEYCODE_SPACE) {
                codepoint = ' ';
            } else if (key_code == AKEYCODE_COMMA) {
                codepoint = ',';
            } else if (key_code == AKEYCODE_PERIOD) {
                codepoint = '.';
            } else if (key_code == AKEYCODE_PLUS) {
                codepoint = '+';
            } else if (key_code == AKEYCODE_MINUS) {
                codepoint = '-';
            } else if (key_code == AKEYCODE_EQUALS) {
                codepoint = '=';
            }
            
            if (codepoint != 0) {
                backend->handle_text_event(codepoint);
            }
        }

        // Let system handle default actions like Back button
        if (key_code == AKEYCODE_BACK) {
            return 0;
        }
        return 1;
    }

    return 0;
}

// Entry point called by android_native_app_glue
void android_main(struct android_app* state) {
    LOGI("OOEY: android_main bootstrapper started");

    // Prevent NDK glue code from stripping symbols
    app_dummy();

    // Create the window backend and bind it to the Android app context
    auto backend = std::make_unique<ooey::android::WindowBackend>(state);
    state->onAppCmd = android_handle_cmd;
    state->onInputEvent = android_handle_input;

    // Standard Application instance
    gooey::Application app;
    app.set_window_backend(std::move(backend));

    // Optional clear color (sleek dark)
    app.set_clear_color(ooey::Color{28, 28, 33});

    // Run the main event loop
    // Notice: The custom application will load its root views and controllers.
    // In actual use, developers will subclass gooey::Application or inject a view tree.
    app.run();

    LOGI("OOEY: android_main loop terminated");
}
