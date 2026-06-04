# OOEY Localization: Runtime and Compiler-Level Dynamic Translation

This document details the architectural design and implementation of the reactive runtime localization and compile-time key extraction/verification in the OOEY framework.

---

## 1. Architectural Overview

To deliver localization (i18n) that is both extremely developer-friendly and safe from memory leaks, OOEY implements a **Reactive Localization Architecture**. The system operates across two boundaries:

1. **Runtime Framework (`Gooey`)**: Elements subscribe dynamically to a global locale property using weak bindings, updating themselves immediately on language swap without memory leaks or dangling pointers.
2. **Layout Compiler (`Tooey`)**: The DSL parses `@tr("key")` markers into dedicated AST nodes, translates them into the correct dynamic C++ setter calls, extracts them into static templates, and verifies their existence at build time.

```mermaid
graph TD
    %% Tool / Build Boundary
    subgraph Build Phase (Tooey Compiler)
        DSL[TodoView.ooey] -->|tooey_compiler| Lexer[Lexer: TokenType::LOCALIZATION]
        Lexer --> Parser[Parser: PropertyType::Localization]
        Parser --> Codegen[Codegen: C++ Output]
        Parser --> Extract[Key Extractor & Verifier]
        Extract -->|warns on missing| DictCheck[localization.hpp]
        Extract -->|dumps| JSON[locales/en_US.json]
    end

    %% Runtime Boundary
    subgraph Runtime Phase (Gooey Framework)
        Codegen -->|Generates| View[C++ View Component]
        View -->|set_localized_text| Control[UI Controls: Label/Button/CheckBox/TextBox]
        Control -->|RAII weak bind| LM[LocalizationManager active_locale]
        LM -->|triggers update| Control
    end
```

---

## 2. Runtime Localization Engine

The localization subsystem is driven by the [LocalizationManager](file:///home/corey/code/ooey/gooey/include/gooey/mvvmc/localization.hpp#L11-L77) class, which acts as a global reactive registry for translation dictionaries.

### Key Constructs
* **[LocalizedString](file:///home/corey/code/ooey/gooey/include/gooey/mvvmc/localization.hpp#L79-L85)**: A lightweight struct representing a localization key.
* **[tr](file:///home/corey/code/ooey/gooey/include/gooey/mvvmc/localization.hpp#L87-L89)**: A helper function `tr(key)` that constructs a `LocalizedString` inline.
* **`active_locale`**: A reactive `Property<std::string>` that controls the active translation language (e.g. `"en_US"`, `"es_ES"`).

### Safe, Leak-Free Subscriptions
When a UI control is bound to a localized key, it must react to changes in the active locale. However, subscribing a widget's member callback directly to a global singleton can create a cyclic strong reference or cause a `use-after-free` crash if the widget is destroyed.

To resolve this, OOEY elements register their subscriptions in their parent `sink_` using weak closures:
```cpp
void Label::set_localized_text(LocalizedString text) {
    localized_key_ = std::move(text);
    bind(LocalizationManager::get().active_locale, [this](const std::string&) {
        this->set_text(LocalizationManager::get().translate(localized_key_.key));
    });
}
```
* **Automatic Unsubscription**: When the control is destroyed, its local `sink_` is destructed, automatically cancelling the subscription on `active_locale`.
* **Zero-Boilerplate Factories**: Interactive controls expose static factories taking `LocalizedString` parameters directly, abstracting the binding setup from the developer:

| Control | Factory Helper | Setter Method |
| :--- | :--- | :--- |
| **[Label](file:///home/corey/code/ooey/gooey/include/gooey/controls/label.hpp)** | `Label::create(LocalizedString, ...)` | `set_localized_text(LocalizedString)` |
| **[Button](file:///home/corey/code/ooey/gooey/include/gooey/controls/button.hpp)** | `Button::create(LocalizedString)` | `set_localized_label_text(LocalizedString)` |
| **[CheckBox](file:///home/corey/code/ooey/gooey/include/gooey/controls/checkbox.hpp)** | `CheckBox::create(LocalizedString, bool)` | `set_localized_text(LocalizedString)` |
| **[TextBox](file:///home/corey/code/ooey/gooey/include/gooey/controls/text_box.hpp)** | `TextBox::create(LocalizedString)` | `set_localized_text(LocalizedString)` |

---

## 3. Compiler Integration

The `tooey_compiler` automates translation syntax verification and C++ code generation.

### 1. Tokenization of `@tr(...)`
The [Lexer](file:///home/corey/code/ooey/tooey/src/lexer.cpp#L268-L305) detects the `@tr(` marker, extracts the quoted or unquoted string key within, and produces a `TokenType::LOCALIZATION` token:
```cpp
if (match_str("@tr(")) {
    consume_str("@tr(");
    // ... handles single, double, or unquoted keys ...
    Token tok;
    tok.type = TokenType::LOCALIZATION;
    tok.text = key;
    tokens.push_back(tok);
}
```

### 2. AST Representation
The [Parser](file:///home/corey/code/ooey/tooey/src/parser.cpp#L144-L147) consumes the localization token, mapping it to the `PropertyType::Localization` type:
```cpp
} else if (val_tok.type == TokenType::LOCALIZATION) {
    p_val.type = PropertyType::Localization;
    p_val.rawData = val_tok.text;
}
```

### 3. Localization Code Generation
The [CodeGenerator](file:///home/corey/code/ooey/tooey/src/codegen.cpp#L394-L412) detects properties of type `PropertyType::Localization` and replaces standard text setters with their localized equivalents:
```cpp
if (val.type == PropertyType::Localization) {
    std::string loc_val = "gooey::tr(\"" + val.rawData + "\")";
    if (node->nodeType == "Button") {
        instantiations << "        " << var_name << "->set_localized_label_text(" << loc_val << ");\n";
    } else if (node->nodeType == "Label") {
        instantiations << "        " << var_name << "->set_localized_text(" << loc_val << ");\n";
    }
    // ... Handles CheckBox and TextBox ...
}
```

---

## 4. Build-Time Static Key Validation & Merging

To ensure missing translations are caught early in the development cycle, the compiler parses built-in keys and validates layout definitions at build time.

### Extraction Pipeline (Implemented in [compiler_main.cpp](file:///home/corey/code/ooey/tooey/src/compiler_main.cpp#L12-L135)):

1. **Dictionary Scanning**: The compiler reads [localization.hpp](file:///home/corey/code/ooey/gooey/include/gooey/mvvmc/localization.hpp) to parse all dictionary keys loaded into `LocalizationManager` (e.g. `welcome_message`, `todo_title`).
2. **Validation**: It matches keys parsed from the layout AST against the scanned set. If a key is missing, it logs a compile-time warning:
   ```bash
   Warning: Localization key 'missing_key' is missing from built-in translation dictionaries.
   ```
3. **Merging and Output**:
   The compiler merges newly discovered keys into a flat translation JSON template at `<output_dir>/locales/en_US.json`. If the JSON file already exists, it is parsed, new keys are appended with placeholder values, and it is written back cleanly:
   ```json
   {
       "add_task": "add_task",
       "delete": "delete",
       "todo_title": "todo_title",
       "toggle_done": "toggle_done"
   }
   ```

---

## 5. Verification Case Study: Task Manager (`TodoView`)

The declarative [TodoView.ooey](file:///home/corey/code/ooey/examples/todo/TodoView.ooey) layout demonstrates the end-to-end integration:

```tooey
VBox id=todoRootLayout:
    Label id=titleLabel text=@tr("todo_title")
    HBox id=inputRow:
        TextBox id=taskInput text=@binding.newTaskText
        Button id=btnAdd text=@tr("add_task") onClick=@signal.addTask
```

When compiled:
1. **Validation Warnings**: The compiler checks if `todo_title` and `add_task` are present in `localization.hpp`.
2. **Template Dump**: The compiler creates/updates the template file `/build/examples/todo_gen/locales/en_US.json`.
3. **C++ Codegen**: Emits localized widget code:
   ```cpp
   titleLabel = std::make_shared<gooey::controls::Label>();
   titleLabel->set_localized_text(gooey::tr("todo_title"));

   btnAdd = std::make_shared<gooey::controls::Button>();
   btnAdd->set_localized_label_text(gooey::tr("add_task"));
   ```
4. **Execution**: Swapping languages via `LocalizationManager::get().active_locale.set("es_ES")` updates all views instantly.

---

## 6. Dynamic Multi-Language Switching Example

An example program demonstrating full dynamic switching between English, Spanish, and German at runtime is implemented under the [localization](file:///home/corey/code/ooey/examples/localization) folder:

### 1. View Layout (`LocalizationView.ooey`)
Defined in [LocalizationView.ooey](file:///home/corey/code/ooey/examples/localization/LocalizationView.ooey):
```tooey
VBox id=mainLayout spacing=20 width="MatchParent" height="MatchParent":
    Label id=titleLabel text=@tr("welcome_title")
    Label id=helloLabel text=@tr("hello_world")
    
    HBox id=langSelectorRow spacing=10:
        Label id=selectLabel text=@tr("lang_selector")
        Button id=btnEn text=@tr("switch_en") onClick=@signal.switchToEnglish
        Button id=btnEs text=@tr("switch_es") onClick=@signal.switchToSpanish
        Button id=btnDe text=@tr("switch_de") onClick=@signal.switchToGerman
```

### 2. ViewModel Implementation
The [LocalizationViewModel](file:///home/corey/code/ooey/examples/localization/LocalizationViewModel.cpp) maps actions to the dynamic switcher APIs of the [LocalizationManager](file:///home/corey/code/ooey/gooey/include/gooey/mvvmc/localization.hpp):
```cpp
void LocalizationViewModel::switchToEnglish() {
    gooey::LocalizationManager::get().active_locale.set("en_US");
}
void LocalizationViewModel::switchToSpanish() {
    gooey::LocalizationManager::get().active_locale.set("es_ES");
}
void LocalizationViewModel::switchToGerman() {
    gooey::LocalizationManager::get().active_locale.set("de_DE");
}
```

### 3. Application Bootstrapping
In [main.cpp](file:///home/corey/code/ooey/examples/localization/main.cpp), translation maps for `en_US`, `es_ES`, and `de_DE` are registered dynamically into the framework before creating the views, demonstrating dynamic dictionary loading.

