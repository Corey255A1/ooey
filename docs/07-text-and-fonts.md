# Text Rendering and Editing

Text is a fundamental requirement for any UI framework. OOEY provides a robust, cross-platform architecture for displaying and editing text.

## 1. Font Architecture

Fonts are defined by the `Font` struct, which encapsulates the styling requirements:
- **Family:** A string representing the typeface (e.g., "sans-serif", "monospace", "Arial").
- **Size:** The scale of the text in logical pixels.
- **Weight:** Normal, Bold, etc.
- **Style:** Normal, Italic.

To support both system fonts and minimal dependencies, text rendering is abstracted via `IFontBackend` or directly integrated into the `IRenderTarget` depending on the platform implementation. By default, `IRenderTarget` must be able to:
- Measure text dimensions (`measure_text`).
- Draw text (`draw_text`).

For platforms or minimal builds that do not hook into the OS font engine, a built-in text renderer (like `stb_truetype` or a basic bitmap font) serves as a fallback.

## 2. Displaying Text

### TextPrimitive
The lowest-level primitive for drawing text on the screen. It implements `IDrawable` and requires:
- `text`: A UTF-8 encoded string.
- `font`: The `Font` definition.
- `color`: The `Color` for the text fill.
- `position`: The origin point.

### Label Control
A high-level view component that wraps `TextPrimitive`. `Label` integrates with the layout system and MVVM-C reactive properties to allow seamless text updates via `ViewModel` bindings. It supports wrapping logic for multiline text.

## 3. Editing Text

To enable text editing, the system must process both character input (for typing) and control keys (for navigation, deletion).

### Input Expansion
- **Text Events:** A new event type, `TextEvent`, handles raw Unicode character input (e.g., UTF-8 strings or `char32_t`), distinct from structural `KeyEvent`s.
- **Control Keys:** `KeyEvent` remains responsible for navigation (Arrows, Home, End) and structural edits (Backspace, Delete, Enter).

### TextBox Control
The `TextBox` control provides editable single or multiline text surfaces.
- **Focus:** Integrates with the `Controller`'s focus management. Only the focused `TextBox` receives keyboard/text events.
- **Cursor State:** Maintains a cursor (caret) index and an optional selection range.
- **Rendering:** Combines a background bounding box, a `TextPrimitive` for the content, and an animated cursor `RectPrimitive` overlay.
- **Interactions:** Translates `PointerEvent` (clicks) into cursor positioning based on character hit-testing via `measure_text`.

## 4. MVVM-C Integration
The `TextBox` control utilizes reactive properties (`Property<std::string>`) to bind its contents to a `ViewModel`. When the user types, the `TextBox` updates the property, pushing the new string to the ViewModel, which can then perform validation or state updates.