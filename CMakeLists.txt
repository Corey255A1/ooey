cmake_minimum_required(VERSION 3.20)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

project(ooey VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(EMSCRIPTEN)
    set(OOEY_BUILD_X11 OFF CACHE BOOL "Build X11" FORCE)
    set(OOEY_BUILD_WAYLAND OFF CACHE BOOL "Build Wayland" FORCE)
    set(OOEY_BUILD_FRAMEBUFFER OFF CACHE BOOL "Build Framebuffer" FORCE)
endif()

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
        ooey/src/platform/wayland/window_backend.cpp
    )
    target_include_directories(ooey PUBLIC ${WAYLAND_INCLUDE_DIRS} ${XKBCOMMON_INCLUDE_DIRS} ${WAYLAND_PROTOCOLS_INCLUDE_DIRS})
    target_compile_options(ooey PUBLIC ${WAYLAND_CFLAGS_OTHER} ${XKBCOMMON_CFLAGS_OTHER} ${WAYLAND_PROTOCOLS_CFLAGS_OTHER})
    target_link_libraries(ooey PUBLIC ${WAYLAND_LIBRARIES} ${XKBCOMMON_LIBRARIES})

    pkg_check_modules(EGL egl)
    pkg_check_modules(WAYLAND_EGL wayland-egl)
    if(EGL_FOUND AND WAYLAND_EGL_FOUND)
        target_compile_definitions(ooey PUBLIC OOEY_WAYLAND_HAS_EGL)
        target_include_directories(ooey PUBLIC ${EGL_INCLUDE_DIRS} ${WAYLAND_EGL_INCLUDE_DIRS})
        target_compile_options(ooey PUBLIC ${EGL_CFLAGS_OTHER} ${WAYLAND_EGL_CFLAGS_OTHER})
        target_link_libraries(ooey PUBLIC ${EGL_LIBRARIES} ${WAYLAND_EGL_LIBRARIES})
    endif()
endif()

# Optional Framebuffer backend
option(OOEY_BUILD_FRAMEBUFFER "Build the Framebuffer backend (requires Linux framebuffer /dev/fb0)" ON)
if(OOEY_BUILD_FRAMEBUFFER)
    list(APPEND OOey_FRAMEBUFFER_SRCS
        ooey/src/platform/framebuffer/window_backend.cpp
    )
endif()

# Add source files
set(OOEY_SRCS
    ooey/src/ooey.cpp
    ooey/src/input.cpp
    ooey/src/platform.cpp
    ooey/src/renderer/bitmap_font.cpp
    ooey/src/renderer/gl_render_target.cpp
    ooey/src/renderer/software_render_target.cpp
)

if(OOEY_BUILD_X11)
    list(APPEND OOEY_SRCS
        ooey/src/platform/x11/window_backend.cpp
    )
endif()

if(EMSCRIPTEN)
    list(APPEND OOEY_SRCS
        ooey/src/platform/emscripten/window_backend.cpp
    )
endif()

list(APPEND OOEY_SRCS
    ${OOey_WAYLAND_SRCS}
    ${OOey_FRAMEBUFFER_SRCS}
)

target_sources(ooey PRIVATE ${OOEY_SRCS})

# Setup include directories
target_include_directories(ooey PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ooey/include>
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

if(EMSCRIPTEN)
    target_compile_options(ooey PUBLIC -sLEGACY_GL_EMULATION=1)
    target_link_options(ooey PUBLIC -sLEGACY_GL_EMULATION=1 -sALLOW_MEMORY_GROWTH=1)
else()
    target_link_libraries(ooey PUBLIC 
        OpenGL::GL
    )
endif()

# Define the gooey library target
add_library(gooey)
target_sources(gooey PRIVATE
    gooey/src/application.cpp
    gooey/src/controls/button.cpp
    gooey/src/controls/label.cpp
    gooey/src/controls/text_box.cpp
    gooey/src/controls/list_control.cpp
    gooey/src/mvvmc/controller.cpp
    gooey/src/mvvmc/navigation_coordinator.cpp
    gooey/src/mvvmc/view.cpp
    gooey/src/renderer/primitives/circle_primitive.cpp
    gooey/src/renderer/primitives/curve_primitive.cpp
    gooey/src/renderer/primitives/line_primitive.cpp
    gooey/src/renderer/primitives/polygon_primitive.cpp
    gooey/src/renderer/primitives/rect_primitive.cpp
    gooey/src/renderer/primitives/rounded_rect_primitive.cpp
    gooey/src/renderer/primitives/sinusoid_primitive.cpp
    gooey/src/renderer/primitives/text_primitive.cpp
)
target_include_directories(gooey PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gooey/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(gooey PUBLIC ooey)

# Create an alias target for backward compatibility
add_library(ooey_controls ALIAS gooey)

# Subdirectories
add_subdirectory(examples)

# Testing setup
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
