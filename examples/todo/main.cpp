#include <iostream>
#include <memory>
#include "ooey/ooey.hpp"
#include "gooey/application.hpp"
#include "ooey/platform.hpp"
#include "TodoViewModel.hpp"
#include "TodoView.hpp"

int main() {
    std::cout << "Starting OOEY Declarative TODO App...\n";

    gooey::Application app;

    auto backend = ooey::create_default_window_backend();
    if (!backend || !backend->create({800, 600}, "OOEY Declarative Task Manager")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }

    app.set_window_backend(std::move(backend));

    // Create ViewModel and the generated View
    auto viewModel = std::make_shared<TodoViewModel>();
    auto todoView = std::make_shared<TodoView>(viewModel);

    // Set clear color for a modern dark theme
    app.set_clear_color(ooey::Color{20, 20, 24});

    // Make the UI look more beautiful by adding padding and styling to the root container
    todoView->set_padding(25);

    app.set_root_view(todoView);

    app.run();

    return 0;
}
