# Network-Controlled UI Architecture

Date: 2026-05-27

This document details the architectural updates and implementation of the TCP network-controlled user interface. This feature demonstrates how we can decouple the main loop event handling of `ooey` using a generic controller interface (`IController`), allowing network sockets (or other event sources) to drive visual components without standard physical pointers or keyboard input.

## 1. Decoupling Event Loop Routing with `IController`

To support custom input controllers (like automation test drivers, network servers, or script interpreters), we separated the core update-and-dispatch logic from the main application runner.

- **`IController` Interface**: We introduced `ooey::IController` with a single pure virtual method:
  ```cpp
  class IController {
  public:
      virtual ~IController() = default;
      virtual void process_events() = 0;
  };
  ```
- **Application Integration**: `ooey::Application` now holds its controller polymorphically via a `std::unique_ptr<IController>`. During initialization (`set_root_view`), it instantiates the default physical input event router (`ooey::Controller`). Developers can override this default behavior by calling `app.set_controller(std::move(custom_controller))` before invoking `app.run()`.
- **Main Loop Execution**: During each frame iteration of the run loop, the application calls `controller_->process_events()`. This enables the active controller to poll external sources (such as network buffers, files, or IPC streams) on the main thread alongside rendering.

## 2. Non-blocking Network Integration with Boost.Asio

To run a TCP server within the application without freezing the rendering thread, the networking logic must be completely non-blocking:
- The custom `NetworkController` constructs a `boost::asio::ip::tcp::acceptor` bound to port `12345`.
- Using `.non_blocking(true)`, the acceptor is configured to immediately return `boost::asio::error::would_block` if no client is currently trying to connect.
- In each call to `process_events()`, the controller:
  1. Checks if a client is connected. If not, it attempts to `accept` a connection in a non-blocking manner.
  2. If a client is connected, it attempts to read from the socket using a non-blocking `read_some()` call.
  3. If data is received, it accumulates it in a read buffer and parses it into discrete text commands delimited by newlines (`\n`).

## 3. Interaction Protocol & Data Binding

The network controller decodes incoming text lines using a lightweight command protocol:
- **`SELECT <index>`**: Updates the `NetworkViewModel::selected_index` property. The `ListControl` binds to this property to update the highlighted item and handle scroll limits.
- **`TEXT <content>`**: Updates the `NetworkViewModel::text_content` property. The `TextBox` control binds to this property to display the text.
- **`EXIT`**: Triggers the `on_exit_requested` callback, which commands `Application::quit()` to terminate the main run loop.

Because the custom controller overrides the default controller, it never queries the standard window backend input manager or routes physical events. As a result, the GUI is completely unresponsive to actual mouse and keyboard interactions, responding exclusively to the TCP stream.

## 4. Python Client Driver

A Python automation script (`examples/tcp_client.py`) is provided to drive the C++ application programmatically. The script performs the following sequence:
1. Connects to `127.0.0.1:12345` via a TCP stream.
2. Sends `TEXT Loading wizard assets...\n` to modify the text field.
3. Steps through selection indices `0` to `9` (with $800\text{ ms}$ pauses) to scroll the ListBox.
4. Sends `TEXT Network drive successful!\n`.
5. Steps through selection indices `9` down to `0` (with $500\text{ ms}$ pauses) to scroll the ListBox backward.
6. Sends the `EXIT\n` command to gracefully shutdown the server application.
