# TextBox Implementation History

Date: 2026-05-23

This document records the issues found while implementing text rendering and editing for `TextBox`, and the resolutions made during development and debugging.

## 1. Issue: Text rendering was not actually drawing glyphs

### Symptoms
- The `hello_ooey_text` example displayed placeholder shapes instead of readable text.
- `TextPrimitive` called `IRenderTarget::draw_text()`, but the platform backend was only drawing a rectangle or outline.

### Root cause
- `X11WindowBackend::OpenGLRenderTarget::draw_text()` was implemented as a debug placeholder that drew a wireframe rectangle around the expected text area.
- The backend did not invoke any real font rendering path, so `Label` and `TextBox` could update text state but nothing visible appeared.

### Resolution
- Implemented actual OpenGL text rendering for the X11 backend using X11 bitmap fonts and GL display lists.
- Loaded a fallback `fixed` font via `XLoadQueryFont()` and created GL lists with `glXUseXFont()`.
- Updated `draw_text()` to position the raster cursor and call display lists with the text string.
- Preserved the rectangle fallback only when font setup failed.

## 2. Issue: Backspace did not delete characters in `TextBox`

### Symptoms
- Typing worked, but pressing backspace had no effect.
- The `TextBox` never removed characters even when focused.

### Root cause
- The X11 backend was sending key events using raw X keycodes from `xev.xkey.keycode`.
- `TextBox::on_key_event()` checked for backspace as ASCII `8`, `127`, and a hardcoded `XK_BackSpace` constant, but the event code path did not match because `KeySym` was never passed.
- Additionally, the text event pipeline pushed every byte returned by `XLookupString()`, including control characters, so backspace could be handled inconsistently.

### Resolution
- Changed X11 key event dispatch to send the `KeySym` value in `KeyEvent` for both `KeyPress` and `KeyRelease`.
- Kept the old fallback values in `TextBox` but ensured the key event path now includes actual `XK_BackSpace` semantics.
- Filtered text events so only printable characters and valid newline/tab codepoints are forwarded, avoiding control byte insertion from the text input stream.

## 3. Issue: `TextBox` interaction and focus behavior

### Symptoms
- The `TextBox` had to correctly receive focus before keyboard events would work.
- If focus routing were wrong, backspace and keystrokes would not reach the text control.

### Resolution
- Verified the controller’s pointer routing and focus management already routed pressed clicks to `IInteractive` elements.
- Confirmed `TextBox::on_pointer_event()` sets `is_focused_` on press and updates background state.
- No architecture changes were needed beyond ensuring the correct `KeySym` and text-event filtering were delivered.

## Summary of changes
- Added real glyph rendering support to X11 `IRenderTarget` via `glXUseXFont()`.
- Corrected backspace handling by using `KeySym` instead of raw X keycodes.
- Filtered text input events to ignore non-printable/control characters.
- Kept fallback behavior for font setup failures and minimal X11 compatibility.

## 4. Issue: Wayland text rendering and backspace behavior

### Symptoms
- The Wayland text example showed visible but malformed characters, with a colon-like artifact next to each glyph.
- Backspace produced a blank or control-like character instead of removing the last character.

### Root cause
- The Wayland `ShmRenderTarget::draw_text()` implementation still used a placeholder rectangle logic for text drawing and returned an invalid default glyph mapping for unsupported characters.
- The Wayland keyboard listener forwarded raw UTF-8 bytes from `xkb_keysym_to_utf8()` without filtering control or non-printable codepoints.
- Backspace and delete keysyms were injected into the text event stream in addition to key events, causing the control key to be interpreted as a printable character.

### Resolution
- Added a built-in bitmap glyph set to the Wayland render target and implemented real glyph rendering for ASCII characters.
- Updated Wayland text measurement to compute width and height from glyph size instead of the previous generic heuristic.
- Changed Wayland key event dispatch to send `KeySym` values in `KeyEvent`, while filtering out `BackSpace` / `Delete` from the text event stream.
- Updated the Wayland text event path to only forward printable characters and newline/tab codepoints.
- Changed the default glyph fallback for unsupported characters to blank instead of a colon-like glyph.

## Summary of changes
- Added real glyph rendering support to X11 `IRenderTarget` via `glXUseXFont()`.
- Corrected backspace handling by using `KeySym` instead of raw X keycodes.
- Filtered text input events so only printable characters and valid newline/tab codepoints are forwarded.
- Added a minimal Wayland bitmap font renderer and proper Wayland key/text event filtering.
- Kept fallback behavior for font setup failures and minimal cross-platform compatibility.

## Notes
- X11 now renders actual glyphs through OpenGL bitmap fonts.
- Wayland now supports text rendering with a built-in bitmap font fallback and proper backspace/delete handling.
- Future improvements should include a shared text rendering abstraction and a richer font backend for both X11 and Wayland.
