#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <boost/asio.hpp>
#include "ooey/ooey.hpp"
#include "gooey/application.hpp"
#include "ooey/platform.hpp"
#include "gooey/mvvmc/view.hpp"
#include "gooey/controls/label.hpp"
#include "gooey/controls/text_box.hpp"
#include "gooey/controls/list_control.hpp"
#include "gooey/renderer/primitives/rounded_rect_primitive.hpp"
#include "gooey/mvvmc/i_controller.hpp"

using boost::asio::ip::tcp;

// ---------------------------------------------------------
// 1. Network ViewModel
// ---------------------------------------------------------
class NetworkViewModel {
public:
    gooey::Property<int> selected_index{0};
    gooey::Property<std::string> text_content{""};
    std::function<void()> on_exit_requested;
};

// ---------------------------------------------------------
// 2. Custom Network Controller
// ---------------------------------------------------------
class NetworkController : public gooey::IController {
public:
    NetworkController(boost::asio::io_context& io_context, std::shared_ptr<NetworkViewModel> vm)
        : io_context_(io_context),
          acceptor_(io_context, tcp::endpoint(tcp::v4(), 12345)),
          socket_(io_context),
          vm_(vm) {
        
        boost::system::error_code ec;
        acceptor_.non_blocking(true, ec);
        std::cout << "NetworkController: Listening on port 12345...\n";
    }

    ~NetworkController() override {
        if (socket_.is_open()) {
            socket_.close();
        }
    }

    void process_events() override {
        // Accept incoming connection if not connected
        if (!socket_.is_open()) {
            boost::system::error_code ec;
            acceptor_.accept(socket_, ec);
            if (!ec) {
                std::cout << "NetworkController: Client connected!\n";
                socket_.non_blocking(true, ec);
            }
        }

        // Read commands from the socket
        if (socket_.is_open()) {
            char data[1024];
            boost::system::error_code ec;
            size_t bytes_read = socket_.read_some(boost::asio::buffer(data, sizeof(data)), ec);

            if (!ec && bytes_read > 0) {
                read_buffer_.append(data, bytes_read);
                process_buffer();
            } else if (ec && ec != boost::asio::error::would_block && ec != boost::asio::error::try_again) {
                // Connection closed or error occurred
                std::cout << "NetworkController: Client disconnected or socket error: " << ec.message() << "\n";
                socket_.close();
                read_buffer_.clear();
            }
        }
    }

private:
    void process_buffer() {
        size_t newline_pos;
        while ((newline_pos = read_buffer_.find('\n')) != std::string::npos) {
            std::string line = read_buffer_.substr(0, newline_pos);
            read_buffer_.erase(0, newline_pos + 1);

            // Strip trailing carriage return if present
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (!line.empty()) {
                handle_command(line);
            }
        }
    }

    void handle_command(const std::string& cmd) {
        std::cout << "NetworkController: Processing command: \"" << cmd << "\"\n";
        
        if (cmd.rfind("SELECT ", 0) == 0) {
            std::string val_str = cmd.substr(7);
            try {
                int index = std::stoi(val_str);
                vm_->selected_index.set(index);
            } catch (...) {
                std::cerr << "NetworkController: Failed to parse index in: " << cmd << "\n";
            }
        } else if (cmd.rfind("TEXT ", 0) == 0) {
            std::string text = cmd.substr(5);
            vm_->text_content.set(text);
        } else if (cmd == "EXIT") {
            if (vm_->on_exit_requested) {
                vm_->on_exit_requested();
            }
        } else {
            std::cerr << "NetworkController: Unknown command ignored: " << cmd << "\n";
        }
    }

    boost::asio::io_context& io_context_;
    tcp::acceptor acceptor_;
    tcp::socket socket_;
    std::shared_ptr<NetworkViewModel> vm_;
    std::string read_buffer_;
};

// ---------------------------------------------------------
// 3. Network View
// ---------------------------------------------------------
class NetworkView : public gooey::View {
public:
    explicit NetworkView(std::shared_ptr<NetworkViewModel> vm) : vm_(vm) {
        // Outer card frame
        auto frame = std::make_shared<gooey::RoundedRectPrimitive>(
            ooey::Rect{50, 50, 700, 480},
            16,
            ooey::Color{30, 30, 35},
            ooey::Color{60, 60, 70},
            2.0f
        );
        add_child(std::move(frame));

        // Title
        auto title = std::make_shared<gooey::Label>(
            "TCP Network-Driven UI",
            ooey::Font{"sans-serif", 24, ooey::FontWeight::Bold},
            ooey::Point{100, 100},
            ooey::Color{255, 255, 255}
        );
        add_child(std::move(title));

        // Info Label
        auto info_lbl = std::make_shared<gooey::Label>(
            "This window ignores physics pointer/keyboard events.\n"
            "Connect via python script on port 12345 to drive interactions.",
            ooey::Font{"sans-serif", 14},
            ooey::Point{100, 140},
            ooey::Color{160, 160, 160}
        );
        add_child(std::move(info_lbl));

        // List Control
        auto list = std::make_shared<gooey::ListControl>(
            ooey::Rect{100, 200, 280, 250},
            50,
            ooey::Font{"sans-serif", 16},
            ooey::Color{200, 200, 200},
            ooey::Color{45, 45, 50},
            ooey::Color{0, 120, 215},
            ooey::Color{255, 255, 255}
        );
        std::vector<std::string> items = {
            "Item 1: Debian",
            "Item 2: Wayland",
            "Item 3: Framebuffer",
            "Item 4: OpenGL",
            "Item 5: Boost Asio",
            "Item 6: Python Driver",
            "Item 7: Retained Mode",
            "Item 8: Scene Graph",
            "Item 9: Coordinator",
            "Item 10: Event Loop"
        };
        list->set_items(items);
        add_child(list);

        // Text input field
        auto input = std::make_shared<gooey::TextBox>(
            ooey::Rect{420, 200, 280, 40},
            ooey::Font{"sans-serif", 16},
            ooey::Color{240, 240, 240},
            ooey::Color{45, 45, 50}
        );
        add_child(input);

        // Selection details label
        auto details = std::make_shared<gooey::Label>(
            "Selected: Item 1: Debian",
            ooey::Font{"sans-serif", 16},
            ooey::Point{420, 270},
            ooey::Color{0, 200, 100}
        );
        add_child(details);

        // Data Bindings
        bind(vm_->selected_index, [list, details, items](int index) {
            if (index >= 0 && index < static_cast<int>(items.size())) {
                list->set_selected_index(index);
                details->set_text("Selected: " + items[index]);
            }
        });

        bind(vm_->text_content, [input](const std::string& text) {
            input->set_text(text);
        });
    }

private:
    std::shared_ptr<NetworkViewModel> vm_;
};

// ---------------------------------------------------------
// 4. Main Bootstrapper
// ---------------------------------------------------------
int main() {
    std::cout << "Starting OOEY Network Controlled UI Demo...\n";

    gooey::Application app;

    auto backend = ooey::create_default_window_backend();
    if (!backend || !backend->create({800, 600}, "OOEY Network Controlled UI")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }
    app.set_window_backend(std::move(backend));

    // Setup Model-View-ViewModel
    auto vm = std::make_shared<NetworkViewModel>();
    auto view = std::make_shared<NetworkView>(vm);

    vm->on_exit_requested = [&app]() {
        app.quit();
    };

    app.set_root_view(std::move(view));
    app.set_clear_color(ooey::Color{15, 15, 17});

    // Create boost asio context and custom network controller
    boost::asio::io_context io_context;
    auto net_controller = std::make_unique<NetworkController>(io_context, vm);

    // Override the application's controller with our custom network controller
    app.set_controller(std::move(net_controller));

    // Run the application
    app.run();

    return 0;
}
