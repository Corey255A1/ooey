# OOEY Memory Debugging & Leak Resolution

This document provides a detailed walkthrough of the diagnostics, analysis, and step-by-step resolutions of critical memory management and runtime crash issues in the OOEY framework.

## 1. Heap Corruption and Exit Crash: `free(): chunks in smallbin corrupted`

### The Symptoms
When launching `hello_reflection` and closing the application using the "X" (close) button or programmatically exiting, the program terminated abruptly with:
```
free(): chunks in smallbin corrupted
Aborted
```
Under AddressSanitizer (ASan), this was flagged as a severe `heap-use-after-free` or double-free crash during the destruction sequence of the application.

### Diagnostic Analysis
Using ASan logs, we traced the crash to a cyclic dependency in the lifetime of UI controls and their corresponding ViewModels:
1. **Binding Subscriptions**: In `hello_reflection.cpp`, `MainViewModel` holds state properties (e.g., `volume_`). The View controls (like `ScrollBar` and `Label`) are bound to these properties.
2. **Cyclic Reference Cycle**:
   - `MainViewModel` owns the properties and subscriptions.
   - The view layout is defined by `MainView`, which strongly captures and keeps references to the `MainViewModel`.
   - The property binding lambda captures the `ScrollBar` or `Label` controls.
   - The callbacks of those controls (e.g., `on_value_changed`) strongly capture the `MainViewModel` (via `std::shared_ptr`).
3. **The Tear-Down Cascade**:
   - When the user closes the window, `Application` initiates destruction of the view hierarchy, destroying `MainView` and cleaning up elements.
   - Clearing the view hierarchy triggers property unsubscription.
   - During `Property::unsubscribe`, the subscription callback lambda is destroyed.
   - Because the lambda strongly captured the UI controls, and the controls strongly captured the ViewModel, destroying the callback lambda triggers the destruction of the controls, which in turn decrements the ViewModel reference count.
   - If the ViewModel's refcount hits zero *while* the unsubscription method of the property is still executing on the stack, the ViewModel (and the property itself) is deleted out from under the running `unsubscribe()` call.
   - This results in a classic `use-after-free` inside `Property::unsubscribe()`, leading to corrupted heap pointers and crashing with `free(): chunks in smallbin corrupted`.

### The Resolution
We broke the strong cyclic references by capturing `MainViewModel` as a `std::weak_ptr<MainViewModel>` within the UI control callback lambdas.
- **Before**:
  ```cpp
  volume_scroll->on_value_changed([view_model](double val) {
      view_model->volume.set(static_cast<float>(val));
  });
  ```
- **After**:
  ```cpp
  std::weak_ptr<MainViewModel> weak_vm = view_model;
  volume_scroll->on_value_changed([weak_vm](double val) {
      if (auto vm = weak_vm.lock()) {
          vm->volume.set(static_cast<float>(val));
      }
  });
  ```
This ensures that the ViewModel does not have its lifetime extended by UI control callbacks, preventing recursive/untimely deletion during the unsubscription cascade.

---

## 2. Dynamic Library Memory Leaks on Exit: Fontconfig & FreeType

### The Symptoms
Running tests under LeakSanitizer (LSan) triggered thousands of memory leaks, reporting:
```
SUMMARY: AddressSanitizer: 159536 byte(s) leaked in 5560 allocation(s).
```
The leaks were tracked down to the Fontconfig dynamically resolved symbol structures in the Linux font rendering subsystem (`LinuxFontBackend`).

### Diagnostic Analysis
We investigated how the dynamically loaded libraries (`libfontconfig.so.1` and `libfreetype.so.6`) and their allocations were managed:
1. **Dynamic Symbol Resolution**:
   - `LinuxFontBackend` resolves Fontconfig and FreeType APIs dynamically using `dlopen`/`dlsym` to keep the core engine binary free of hard linked dependencies.
2. **The Leak Source**:
   - On every font query inside `match_font`, the code was calling `FcInitLoadConfigAndFonts()`. This function parses XML configuration files and allocates a new `FcConfig` object every single time.
   - The returned `FcConfig*` was never destroyed, causing thousands of configurations to accumulate in memory during rendering passes and unit tests.
3. **The Symbols Resolution Loss (`<unknown module>`)**:
   - During cleanup, `LinuxFontBackend` was calling `dlclose()` on the dynamically loaded libraries.
   - When LeakSanitizer ran at process exit, the libraries had already been unloaded. Consequently, LSan could not resolve the addresses of the leaked allocations, reporting them as `<unknown module>`, which obfuscated debugging.

### The Resolution
We implemented a two-part fix:
1. **Persistent Configuration & Lifecycle Control**:
   - Instead of initializing a new `FcConfig` configuration object on every font query, we load it exactly once during backend initialization (`load_symbols`) using `FcInitLoadConfigAndFonts()`.
   - We store the pointer as a member (`fc_config`) in the implementation structure (`Impl`).
   - On shutdown, inside the `cleanup()` function, we release the configuration via `FcConfigDestroy(fc_config)` and invoke `FcFini()` to clean up the global Fontconfig state.
   - To make this work, we dynamically resolve `FcConfigDestroy` alongside the other Fontconfig functions.
2. **Unloading Dynamic Libraries Safely**:
   - Because our resource cleanup is now 100% complete and zero memory leaks remain, we safely call `dlclose()` on `freetype_lib` and `fontconfig_lib` upon exit. Unloading is clean and does not trigger any LSan leak reports or `<unknown module>` false positives.
3. **Global Engine Cache Eviction**:
   - Added explicit calls in `Application::~Application` to clear the `GlyphAtlasManager` glyph textures and clear the active `FontEngine` backend (`FontEngine::set_backend(nullptr)`), ensuring complete teardown of the engine.

### Impact and Performance
- **Zero Memory Leaks**: Resolving the unreleased configuration objects and correctly terminating Fontconfig completely cleared all LSan leak reports.
- **13x Test Execution Speedup**: Because XML configuration parsing and system font directory scanning are extremely slow operations, caching a single `FcConfig` rather than creating thousands of configurations reduced the total test suite runtime from **26 seconds** to **2.0 seconds** (100% tests passed successfully).

## 3. Discarded `ScopedSubscription` & Property Binding Lifetime

### The Symptoms
In the `hello_reflection` example, custom checkbox widgets (`CheckBox`) were successfully rendering their initial `checked` states and responding visually to click events, but the model properties bound to them (like `user.is_admin` and `settings.show_notifications`) were not being updated in the ViewModel when checkboxes were toggled.

### Diagnostic Analysis
We investigated how bindings were wired:
1. **Property Subscription Lifetime**:
   - `Property::subscribe(listener)` returns a `ScopedSubscription` object that manages the unsubscription behavior dynamically in its destructor (`~ScopedSubscription()`).
   - If the returned `ScopedSubscription` is discarded/not stored, the object is immediately destroyed at the end of the expression, causing it to automatically call `unsubscribe()` and deactivate the listener callback.
2. **The Root Cause**:
   - The checkbox update binding was written as:
     ```cpp
     is_admin_chk->checked.subscribe([weak_vm](bool val) {
         if (auto vm = weak_vm.lock()) {
             vm->user.is_admin.set(val);
         }
     });
     ```
   - Since the returned `ScopedSubscription` was not saved, it was destroyed immediately, rendering the binding dead.

### The Resolution
We replaced the raw `.subscribe(...)` calls with calls to `bind(...)`, which is a member of `GooeyElement` (and thus inherited by `MainView`):
```cpp
bind(is_admin_chk->checked, [weak_vm](bool val) {
    if (auto vm = weak_vm.lock()) {
        if (vm->user.is_admin.get() != val) {
            vm->user.is_admin.set(val);
        }
    }
});
```
Because `bind(...)` automatically registers the subscription in the parent view's `sink_` (a persistent `SubscriptionSink` matching the view's lifetime), the subscription is kept alive for the entire lifespan of the application window.

## 4. Reentrancy-Safe Property Subscriptions & Notifications

### The Symptoms
During teardown of complex view hierarchies, erasing a property listener from the map inside `Property::unsubscribe()` could trigger a cascade of destructions (e.g. freeing the control, releasing the ViewModel reference, and deleting the ViewModel/Property itself) *while* the `unsubscribe()` or `notify()` methods of that same `Property` were still executing. This reentrant destruction resulted in use-after-free/corrupted memory references.

### Diagnostic Analysis
1. **Unsubscription Reentrancy**:
   - Previously, `Property::unsubscribe(id)` directly called `listeners_.erase(id)`. Erasing the callback lambda immediately executed its destructor, which released all captured strong references. If this cascade resulted in the destruction of the owning ViewModel, the `Property` object was deleted under the feet of the active `unsubscribe` call.
2. **Notification Reentrancy**:
   - Similarly, if `Property::notify()` iterated directly over `listeners_`, any listener callback that modified or unregistered subscriptions would mutate `listeners_` while it was being traversed, causing a crash or undefined behavior.

### The Resolution
We upgraded `Property`'s implementation in `gooey/include/gooey/mvvmc/property.hpp` to be completely reentrancy-safe:
1. **Deferred Lambda Destruction in Unsubscribe**:
   - In `unsubscribe()`, we find the listener in the map, *move* it into a local variable, and *then* erase the map entry. The map is left in a clean state, and the listener lambda is only destroyed when the local variable goes out of scope at the end of the function. If this destruction triggers deleting the `Property` object itself, it happens safely after all accesses to `this` inside `unsubscribe()` have completed:
     ```cpp
     void unsubscribe(uint32_t id) {
         auto it = listeners_.find(id);
         if (it != listeners_.end()) {
             auto listener = std::move(it->second);
             listeners_.erase(it);
         } // listener is safely destroyed here
     }
     ```
2. **Snapshot-Based Notifications**:
   - In `notify()`, we copy all active listener std::functions into a temporary `std::vector` snapshot before invoking them. This guarantees that modifying the subscriptions during notification callbacks will not mutate the map being traversed:
     ```cpp
     void notify() {
         std::vector<Listener> active_listeners;
         active_listeners.reserve(listeners_.size());
         for (const auto& kv : listeners_) {
             active_listeners.push_back(kv.second);
         }
         for (const auto& listener : active_listeners) {
             listener(value_);
         }
     }
     ```

---

## 5. Wayland Platform Resource Leaks & Code Generator Alignment

### The Symptoms
Profiling the compiled declarative `hello_todo` executable under LeakSanitizer (LSan) at exit revealed a direct leak:

```
=================================================================
==59467==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 192 byte(s) in 2 object(s) allocated from:
    #0 0x7f87400f4610 in calloc ../../../../src/libsanitizer/asan/asan_malloc_linux.cpp:77
    #1 0x7f874078b633  (/lib/x86_64-linux-gnu/libwayland-client.so.0+0x6633)

SUMMARY: AddressSanitizer: 192 byte(s) leaked in 2 allocation(s).
```

Additionally, during layout compilation of declarative `.ooey` files, recursive code generation for nested containers was missing the class name scope of the parent ViewModel, triggering compile-time warning mismatches.

### Diagnostic Analysis
1. **Un-destroyed Registry Bindings**:
   - In `WindowBackend::setup_registry()` inside `ooey/src/platform/wayland/window_backend.cpp`, the Wayland connection binds to the global desktop seat (`wl_seat`) and the XDG window manager base (`xdg_wm_base`) interfaces dynamically.
   - While `wl_compositor`, `wl_shm`, and standard buffer objects were properly cleaned up, references to the bound `wl_seat` and `xdg_wm_base` were abandoned without their matching destroy calls.
   - Since these structures are allocated internally by `libwayland-client` on the heap, abandoning them caused a leak of exactly two objects (192 bytes total) at exit.
2. **Missing Codegen Arguments**:
   - In the layout generator recursive node builder, recursive container bindings did not forward the ViewModel class type. Under strict sanitizer builds, this led to compiler discrepancies when generating weak ViewModel locks.

### The Resolution
1. **Wayland Client Cleanup**:
   - Extended the `WindowBackend` class header (`ooey/include/ooey/platform/wayland/window_backend.hpp`) to store the registry-bound `xdg_wm_base* wm_base_` pointer.
   - Updated the `WindowBackend::setup_registry` implementation to initialize `wm_base_ = state.wm_base`.
   - Modified `WindowBackend::destroy()` to cleanly invoke their respective destruction APIs, clearing the dynamic allocations from the Wayland client library:
     ```cpp
     if (wm_base_) {
         xdg_wm_base_destroy(wm_base_);
         wm_base_ = nullptr;
     }
     if (seat_) {
         wl_seat_destroy(seat_);
         seat_ = nullptr;
     }
     ```
2. **Code Generator VM Alignment**:
   - Fixed the recursive layout nodes compiler logic in `tooey/src/codegen.cpp` to forward the target ViewModel class string correctly, guaranteeing that every generated listener callback successfully and safely captures and locks `std::weak_ptr<ViewModel>` on runtime events.
