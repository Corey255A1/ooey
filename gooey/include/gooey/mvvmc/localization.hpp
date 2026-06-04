#pragma once

#include "gooey/mvvmc/property.hpp"
#include <string>
#include <map>
#include <vector>
#include <memory>

namespace gooey::mvvmc {

class LocalizationManager {
public:
    static LocalizationManager& get() {
        static LocalizationManager instance;
        return instance;
    }

    // Current active locale (reactive property)
    Property<std::string> active_locale{"en_US"};

    // Translate a key for the current locale
    std::string translate(const std::string& key) const {
        auto locale = active_locale.get();
        auto dict_it = dictionaries_.find(locale);
        if (dict_it != dictionaries_.end()) {
            auto key_it = dict_it->second.find(key);
            if (key_it != dict_it->second.end()) {
                return key_it->second;
            }
        }
        return key; // Fallback to key if not found
    }

    // Load translations for a specific locale
    void load_translations(const std::string& locale, const std::map<std::string, std::string>& translations) {
        dictionaries_[locale] = translations;
    }

private:
    LocalizationManager() {
        // Default built-in translations for testing
        std::map<std::string, std::string> en = {
            {"welcome_message", "Welcome to the MVVMC Wizard!"},
            {"start_wizard", "Start Wizard"},
            {"animal_selection", "Please select an animal from the list:"},
            {"selected_prefix", "Selected: "},
            {"selected_none", "Selected: None"},
            {"clock_title", "Page 3: Fancy Clock"},
            {"check_this_out", "Check this out"},
            {"continue", "Continue"},
            {"sinusoid_title", "Page 4: Scrolling Sinusoid!"},
            {"go_back", "Go Back"},
            {"finished_title", "Page 5: Finished"},
            {"end_text", "The End"},
            {"exit", "Exit"},
            {"todo_title", "OOEY TASK MANAGER"},
            {"add_task", "Add Task"},
            {"delete", "Delete"},
            {"toggle_done", "Mark Done / Undo"}
        };
        std::map<std::string, std::string> es = {
            {"welcome_message", "¡Bienvenido al Asistente MVVMC!"},
            {"start_wizard", "Iniciar Asistente"},
            {"animal_selection", "Por favor seleccione un animal de la lista:"},
            {"selected_prefix", "Seleccionado: "},
            {"selected_none", "Seleccionado: Ninguno"},
            {"clock_title", "Página 3: Reloj Elegante"},
            {"check_this_out", "Mira esto"},
            {"continue", "Continuar"},
            {"sinusoid_title", "Página 4: Sinusoide Desplazable"},
            {"go_back", "Volver"},
            {"finished_title", "Página 5: Terminado"},
            {"end_text", "El Fin"},
            {"exit", "Salir"},
            {"todo_title", "GESTOR DE TAREAS OOEY"},
            {"add_task", "Añadir Tarea"},
            {"delete", "Eliminar"},
            {"toggle_done", "Marcar Hecho / Deshacer"}
        };
        dictionaries_["en_US"] = en;
        dictionaries_["es_ES"] = es;
    }

    std::map<std::string, std::map<std::string, std::string>> dictionaries_;
};

struct LocalizedString {
    std::string key;
    std::vector<std::string> args;

    explicit LocalizedString(std::string k) : key(std::move(k)) {}
    LocalizedString(const char* k) : key(k) {}
};

inline LocalizedString tr(const std::string& key) {
    return LocalizedString(key);
}

} // namespace gooey::mvvmc

namespace gooey {
    using gooey::mvvmc::LocalizationManager;
    using gooey::mvvmc::LocalizedString;
    using gooey::mvvmc::tr;
}
