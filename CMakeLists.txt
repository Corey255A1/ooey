cmake_minimum_required(VERSION 3.20)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

project(ooey VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for tooling (clangd, etc)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define the ooey library target
add_library(ooey)


# Optional X11 backend
option(OOEY_BUILD_X11 "Build the X11 backend (requires X11 and GLX)" ON)
if(OOEY_BUILD_X11)
    # Find dependencies
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)
endif()
# Optional Wayland backend
option(OOEY_BUILD_WAYLAND "Build the Wayland backend (requires wayland-client, xkbcommon, wayland-protocols)" OFF)
if(OOEY_BUILD_WAYLAND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WAYLAND REQUIRED wayland-client)
    # wayland-protocols provides xdg-shell client headers (xdg-shell-client-protocol.h)
    pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
    message(STATUS "Found wayland-protocols: ${WAYLAND_PROTOCOLS_VERSION}")

    # Ensure wayland-scanner is available to generate protocol headers (xdg-shell)
    find_program(WAYLAND_SCANNER NAMES wayland-scanner)
    if(NOT WAYLAND_SCANNER)
        message(FATAL_ERROR "wayland-scanner not found; please install wayland-scanner to generate protocol headers (wayland-protocols package)")
    endif()

    execute_process(
        COMMAND pkg-config --variable=pkgdatadir wayland-protocols
        OUTPUT_VARIABLE WAYLAND_PROTOCOLS_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    set(XDG_SHELL_XML "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml")

    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h"
        COMMAND ${WAYLAND_SCANNER} client-header "${XDG_SHELL_XML}" "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h"
        DEPENDS "${XDG_SHELL_XML}"
        COMMENT "Generating xdg-shell-client-protocol.h"
    )

    set(XDG_CLIENT_C "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.c")
    add_custom_command(
        OUTPUT "${XDG_CLIENT_C}"
        COMMAND ${WAYLAND_SCANNER} private-code "${XDG_SHELL_XML}" "${XDG_CLIENT_C}"
        DEPENDS "${XDG_SHELL_XML}"
        COMMENT "Generating xdg-shell-client-protocol.c"
    )

    add_custom_target(xdg_shell_protocol_gen DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h" "${XDG_CLIENT_C}")
    target_include_directories(ooey PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
    add_dependencies(ooey xdg_shell_protocol_gen)

    # Compile generated protocol C as an OBJECT library and link its objects into ooey
    add_library(xdg_shell_proto OBJECT "${XDG_CLIENT_C}")
    add_dependencies(xdg_shell_proto xdg_shell_protocol_gen)
    target_include_directories(xdg_shell_proto PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_sources(ooey PRIVATE $<TARGET_OBJECTS:xdg_shell_proto>)
    # Ensure the ooey target builds the xdg_shell_proto objects first
    add_dependencies(ooey xdg_shell_proto)
    pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)
    # Add the wayland source files
    list(APPEND OOey_WAYLAND_SRCS
        src/platform/wayland/window_backend.cpp
        src/platform/wayland/render_target.cpp
    )
    target_include_directories(ooey PUBLIC ${WAYLAND_INCLUDE_DIRS} ${XKBCOMMON_INCLUDE_DIRS} ${WAYLAND_PROTOCOLS_INCLUDE_DIRS})
    target_compile_options(ooey PUBLIC ${WAYLAND_CFLAGS_OTHER} ${XKBCOMMON_CFLAGS_OTHER} ${WAYLAND_PROTOCOLS_CFLAGS_OTHER})
    target_link_libraries(ooey PUBLIC ${WAYLAND_LIBRARIES} ${XKBCOMMON_LIBRARIES})
endif()

# Optional Framebuffer backend
option(OOEY_BUILD_FRAMEBUFFER "Build the Framebuffer backend (requires Linux framebuffer /dev/fb0)" ON)
if(OOEY_BUILD_FRAMEBUFFER)
    list(APPEND OOey_FRAMEBUFFER_SRCS
        src/platform/framebuffer/window_backend.cpp
        src/platform/framebuffer/render_target.cpp
    )
endif()

# Add source files
set(OOEY_SRCS
    src/ooey.cpp
    src/application.cpp
    src/renderer/primitives/rect_primitive.cpp
    src/renderer/primitives/line_primitive.cpp
    src/renderer/primitives/text_primitive.cpp
    src/renderer/primitives/circle_primitive.cpp
    src/renderer/primitives/rounded_rect_primitive.cpp
    src/renderer/primitives/polygon_primitive.cpp
    src/renderer/primitives/curve_primitive.cpp
    src/renderer/primitives/sinusoid_primitive.cpp
    src/mvvmc/view.cpp
    src/input.cpp
    src/mvvmc/controller.cpp
    src/renderer/bitmap_font.cpp
    src/platform/platform.cpp
    src/mvvmc/navigation_coordinator.cpp
)

if(OOEY_BUILD_X11)
    list(APPEND OOEY_SRCS
        src/platform/x11/window_backend.cpp
        src/platform/x11/render_target.cpp
    )
endif()

list(APPEND OOEY_SRCS
    ${OOey_WAYLAND_SRCS}
    ${OOey_FRAMEBUFFER_SRCS}
)

target_sources(ooey PRIVATE ${OOEY_SRCS})

# Setup include directories
target_include_directories(ooey PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(OOEY_BUILD_X11)
    target_compile_definitions(ooey PUBLIC OOEY_BUILD_X11)
    target_link_libraries(ooey PUBLIC X11::X11)
endif()
if(OOEY_BUILD_WAYLAND)
    target_compile_definitions(ooey PUBLIC OOEY_BUILD_WAYLAND)
endif()
if(OOEY_BUILD_FRAMEBUFFER)
    target_compile_definitions(ooey PUBLIC OOEY_BUILD_FRAMEBUFFER)
endif()

target_link_libraries(ooey PUBLIC 
    OpenGL::GL
)

# Option to build controls library
option(OOEY_BUILD_CONTROLS "Build the OOEY controls library" ON)

if(OOEY_BUILD_CONTROLS)
    add_library(ooey_controls)
    target_sources(ooey_controls PRIVATE 
        src/controls/button.cpp
        src/controls/label.cpp
        src/controls/text_box.cpp
        src/controls/list_control.cpp
    )
    target_link_libraries(ooey_controls PUBLIC ooey)
endif()

# Subdirectories
add_subdirectory(examples)

# Testing setup
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
