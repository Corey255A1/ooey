# Tutorial: Implementing a Custom Backend

One of the primary goals of OOEY is cross-platform portability. To help you understand how the engine works internally—or if you want to implement support for Windows, macOS, or Wayland—this guide breaks down how to build a custom backend.

## The Architecture
To get a window on the screen, you need to implement two interfaces:
1. `ooey::IRenderTarget`: How to draw things (lines, rectangles, etc.).
2. `ooey::IWindowBackend`: How to talk to the Operating System (create windows, poll events).

## Step 1: Implementing the Render Target
First, decide how you will draw. Are you using a graphics API like Vulkan or DirectX, or software rendering into memory?

Let's imagine you are making a `MyCustomRenderTarget`. You must implement:
- `clear(Color color)`
- `draw_rect(const Rect& rect, Color color)`
- `draw_line(const Point& start, const Point& end, Color color)`
- `present()`

```cpp
#include "ooey/i_render_target.hpp"

class MyCustomRenderTarget : public ooey::IRenderTarget {
public:
    void clear(ooey::Color color) override {
        // Translate ooey::Color to your graphics API
        // Tell your API to clear the screen
    }

    void draw_rect(const ooey::Rect& rect, ooey::Color color) override {
        // Push vertices to your graphics card based on the Rect
    }

    void draw_line(const ooey::Point& start, const ooey::Point& end, ooey::Color color) override {
        // Draw a line using your API
    }

    void present() override {
        // Tell the OS/Graphics Card to swap the buffers and show the frame
    }
};
```

## Step 2: Implementing the Window Backend
Next, you need to tell the OS to give you a window. 

You must implement:
- `create(const Size& size, const char* title)`
- `destroy()`
- `poll_events()`
- `get_render_target()`

```cpp
#include "ooey/i_window_backend.hpp"
#include <memory>

class MyCustomWindowBackend : public ooey::IWindowBackend {
public:
    bool create(const ooey::Size& size, const char* title) override {
        // 1. Call OS specific code to open a window (e.g. CreateWindowEx on Windows)
        // 2. Initialize your graphics context (e.g. Vulkan Instance)
        // 3. Instantiate your custom render target:
        render_target_ = std::make_unique<MyCustomRenderTarget>(/* context */);
        return true; // Return false if it failed
    }

    void destroy() override {
        // 1. Destroy graphics context
        // 2. Destroy OS window
    }

    bool poll_events() override {
        // Ask the OS if the user clicked 'X' to close, or pressed a key.
        // Return TRUE to keep the app running.
        // Return FALSE if the user requested the app to close.
        return true; 
    }

    ooey::IRenderTarget* get_render_target() override {
        return render_target_.get();
    }

private:
    std::unique_ptr<MyCustomRenderTarget> render_target_;
};
```

## Step 3: Plugging it into the Engine
Once your classes are written, using them is as simple as passing them to the `Application` class!

```cpp
int main() {
    ooey::Application app;
    
    // Inject your custom backend!
    auto backend = std::make_unique<MyCustomWindowBackend>();
    backend->create({800, 600}, "My Custom Window");

    app.set_window_backend(std::move(backend));

    app.set_render_callback([](ooey::IRenderTarget* target) {
        target->clear(ooey::Color{0, 0, 0});
        target->present();
    });

    app.run();
    return 0;
}
```

### Why this matters
Because your game logic/UI logic only ever talks to `IRenderTarget` and `Application`, your application code **never changes** regardless of what OS it is running on. You just inject a different backend at startup!
