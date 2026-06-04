#pragma once

#include "gooey/mvvmc/property.hpp"
#include "gooey/mvvmc/subscription_sink.hpp"
#include <string>

class LocalizationViewModel : public gooey::mvvmc::ViewModel {
public:
    LocalizationViewModel();

    // Observable properties bound directly by the generated C++ view
    gooey::mvvmc::Property<std::string> counterText;

    // ViewModel action signals
    void switchToEnglish();
    void switchToSpanish();
    void switchToGerman();
    void incrementCounter();

private:
    int count_ = 0;
    gooey::mvvmc::SubscriptionSink sink_;

    void update_counter_text();
};
