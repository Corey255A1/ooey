# OOEY Project Coding Standards

These standards ensure a consistent, maintainable, and readable C++ codebase. All contributions must adhere to these rules.

## 1. Naming Conventions
- **Classes and Structs:** `PascalCase` (e.g., `Application`, `RectPrimitive`, `Geometry`).
- **Interfaces:** `PascalCase` prefixed with `I` (e.g., `IWindowBackend`, `IRenderTarget`, `IDrawable`, `IInputProvider`).
- **Methods and Functions:** `snake_case` (e.g., `draw_geometry()`, `poll_events()`).
- **Variables and Parameters:** `snake_case` (e.g., `window_title`, `size`).
- **Private/Protected Class Members:** `snake_case_` with a trailing underscore (e.g., `window_backend_`, `running_`). Public struct members (like in Plain Old Data structures) do not need trailing underscores.
- **Namespaces:** `snake_case` (e.g., `ooey`).
- **Enums:** `PascalCase` for both the `enum class` name and its values (e.g., `PointerState::Released`).

## 2. Formatting
- **Indentation:** 4 spaces (no tabs).
- **Braces:** K&R / 1TBS style. The opening brace goes on the same line as the statement (class, function, if, while, etc.).
- **Headers:** Always use `#pragma once` for include guards. Include project files using quotes (`#include "ooey/types.hpp"`) and system/library files using angle brackets (`#include <vector>`).
- **Pointers/References:** Attach the `*` or `&` to the type, not the variable name (e.g., `IRenderTarget* target`, not `IRenderTarget *target`).

## 3. Language & Architecture
- **Language:** C++20 strictly. Avoid C++20 Modules for now; stick to traditional header/source inclusion to ensure broad compatibility with build tools.
- **Memory Management:** Prefer modern smart pointers (`std::unique_ptr`, `std::shared_ptr`). Avoid raw `new`/`delete`.
- **Explicit Move Semantics:** When a function takes ownership of an object (e.g., a `std::unique_ptr` or storing a `std::function`), explicitly take the parameter by rvalue reference (`&&`) instead of by value (e.g., `void set_backend(std::unique_ptr<IWindowBackend>&& backend)`). This makes the transfer of ownership hyper-explicit in the API surface.
- **Composition over Inheritance:** Use interfaces for abstracting platform details and `std::function` for callbacks, but prefer composition (like the Retained Mode Scene Graph) for building complex UI logic.
- **Method Length:** Keep functions and methods short, focused, and single-purpose. If a method grows large or has commented steps/phases (e.g., `// 1. Create ...`), split those steps out into well-named private or protected helper methods to make the code self-documenting and maintainable.