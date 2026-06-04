#include <iostream>
#include <memory>
#include <map>
#include <string>
#include "ooey/ooey.hpp"
#include "gooey/application.hpp"
#include "ooey/platform.hpp"
#include "gooey/mvvmc/localization.hpp"
#include "LocalizationViewModel.hpp"
#include "LocalizationView.hpp"

void load_example_dictionaries() {
    std::map<std::string, std::string> en = {
        {"welcome_title", "Welcome to OOEY Localization Demo!"},
        {"hello_world", "Hello World! This text dynamically updates."},
        {"lang_selector", "Select Language:"},
        {"switch_en", "English"},
        {"switch_es", "Spanish"},
        {"switch_de", "German"},
        {"current_lang_label", "Current Language: English"},
        {"click_count_prefix", "Click Count: "},
        {"click_me_btn", "Click Me!"}
    };

    std::map<std::string, std::string> es = {
        {"welcome_title", "¡Demostración de localización de OOEY!"},
        {"hello_world", "¡Hola Mundo! Este texto se actualiza dinámicamente."},
        {"lang_selector", "Seleccionar idioma:"},
        {"switch_en", "Inglés"},
        {"switch_es", "Español"},
        {"switch_de", "Alemán"},
        {"current_lang_label", "Idioma actual: Español"},
        {"click_count_prefix", "Número de clics: "},
        {"click_me_btn", "¡Haz clic en mí!"}
    };

    std::map<std::string, std::string> de = {
        {"welcome_title", "OOEY Lokalisierungs-Demo!"},
        {"hello_world", "Hallo Welt! Dieser Text aktualisiert sich dynamisch."},
        {"lang_selector", "Sprache wählen:"},
        {"switch_en", "Englisch"},
        {"switch_es", "Spanisch"},
        {"switch_de", "Deutsch"},
        {"current_lang_label", "Aktuelle Sprache: Deutsch"},
        {"click_count_prefix", "Klickzähler: "},
        {"click_me_btn", "Klick mich!"}
    };

    // Load translations dynamically into global LocalizationManager
    gooey::LocalizationManager::get().load_translations("en_US", en);
    gooey::LocalizationManager::get().load_translations("es_ES", es);
    gooey::LocalizationManager::get().load_translations("de_DE", de);
}

int main() {
    std::cout << "Starting OOEY Dynamic Localization Example...\n";

    // Load example dictionaries before initializing views
    load_example_dictionaries();

    // Default to English on startup
    gooey::LocalizationManager::get().active_locale.set("en_US");

    gooey::Application app;

    auto backend = ooey::create_default_window_backend();
    if (!backend || !backend->create({800, 600}, "OOEY Dynamic Localization Demo")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }

    app.set_window_backend(std::move(backend));

    // Create ViewModel and the generated View
    auto viewModel = std::make_shared<LocalizationViewModel>();
    auto localizationView = std::make_shared<LocalizationView>(viewModel);

    // Set clear color for a modern dark theme
    app.set_clear_color(ooey::Color{18, 18, 22});

    // Modern layout styles and padding
    localizationView->set_padding(30);

    app.set_root_view(localizationView);

    app.run();

    return 0;
}
