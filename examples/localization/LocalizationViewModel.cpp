#include "LocalizationViewModel.hpp"
#include "gooey/mvvmc/localization.hpp"

LocalizationViewModel::LocalizationViewModel() {
    update_counter_text();

    // Dynamically update counterText property when active locale changes
    sink_.add(gooey::LocalizationManager::get().active_locale.subscribe([this](const std::string&) {
        this->update_counter_text();
    }));
}

void LocalizationViewModel::switchToEnglish() {
    gooey::LocalizationManager::get().active_locale.set("en_US");
}

void LocalizationViewModel::switchToSpanish() {
    gooey::LocalizationManager::get().active_locale.set("es_ES");
}

void LocalizationViewModel::switchToGerman() {
    gooey::LocalizationManager::get().active_locale.set("de_DE");
}

void LocalizationViewModel::incrementCounter() {
    count_++;
    update_counter_text();
}

void LocalizationViewModel::update_counter_text() {
    std::string prefix = gooey::LocalizationManager::get().translate("click_count_prefix");
    counterText.set(prefix + std::to_string(count_));
}
