# Text Rendering and Editing

Text is a fundamental requirement for any UI framework. OOEY provides a robust, cross-platform architecture for displaying and editing text.

## 1. Font Architecture

Fonts are defined by the `Font` struct, which encapsulates the styling requirements:
- **Family:** A string representing the typeface (e.g., "sans-serif", "monospace", "Arial").
- **Size:** The scale of the text in logical pixels.
- **Weight:** Normal, Bold, etc.
- **Style:** Normal, Italic.

To support both system fonts and minimal dependencies, text rendering is integrated into the `IRenderTarget` implementation. By default, `IRenderTarget` must support:
- Measuring text dimensions (`measure_text`).
- Drawing text (`draw_text`).

### Unified Rendering Fallback
- **System Fonts:** Supported platforms use OS-specific font matching and rendering APIs (FreeType/Fontconfig on Linux, DirectWrite on Windows) to dynamically load and rasterize system TrueType/OpenType (.ttf/.otf) fonts.
- **Embedded Fallback:** If system fonts are missing, fail to load, or the OS APIs are unavailable, the engine falls back to `ooey::BitmapFont`, an embedded ASCII character set. The rendering logic maps characters to glyph pixel coordinates, yielding consistent, cross-platform layouts.

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

## 5. RichText and Code Editing

To support code editing, the framework provides the `CodeEditor` control alongside an extensible syntax highlighting engine.

### Extensible Syntax Highlighting

The syntax highlighting architecture is built on the `ISyntaxHighlighter` interface, allowing custom lexical parsing rules to be defined and updated reactively:

```cpp
class ISyntaxHighlighter {
public:
    virtual ~ISyntaxHighlighter() = default;
    virtual std::vector<HighlightedToken> highlight(const std::string& line, int start_state, int& out_end_state) = 0;
};
```

* **State-Based Parsing:** To ensure high performance with large files, the highlight loop accepts a `start_state` and returns an `out_end_state`. This allows the highlighter to parse multi-line elements (like C++ `/* ... */` block comments) efficiently.
* **Token Classification:** Highlighting maps source code segments to a set of distinct categories defined by `TokenType`:
  - `Normal` (default text)
  - `Keyword` (control flow, compiler keywords)
  - `Type` (data types)
  - `Comment` (single-line or multi-line comments)
  - `String` (string and char literals)
  - `Number` (integers, floats, hex values)
  - `Preprocessor` (macro defines and imports)
  - `Operator` (punctuation, math, logic symbols)

### CodeEditor Control

The `CodeEditor` control is a highly responsive, feature-rich editing component:
* **Interactive Line Numbers:** Displays line numbering in a dedicated left column that adjusts width dynamically based on the document size.
* **Composition-Based Scrollbar:** Embeds a vertical `ScrollBar` child widget to allow intuitive scrolling through large files.
* **Caret Navigation:** Supports standard coding shortcuts, including Left/Right/Up/Down arrows, Home/End (line margins), and PageUp/PageDown (page scrolling).
* **Code Editing Operations:** Implements standard text insertion/deletion (Backspace/Delete at any offset) and auto-indents newlines to match the leading whitespace of the preceding line.
* **Custom Styling:** Exposes a `token_colors` map allowing themes or users to customize colors for keywords, comments, preprocessors, types, background, divider lines, and carets.