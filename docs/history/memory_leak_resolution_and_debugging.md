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
2. **Preventing Symbol Resolution Loss**:
   - Commented out the `dlclose()` calls on `freetype_lib` and `fontconfig_lib`. Keeping these libraries loaded in the memory map until process termination allows ASan/LSan to successfully map code pointers to physical library functions.
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

