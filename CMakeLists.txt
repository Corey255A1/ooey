cmake_minimum_required(VERSION 3.20)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

project(ooey VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for tooling (clangd, etc)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define the ooey library target
add_library(ooey)

# Find dependencies
find_package(X11 REQUIRED)
find_package(OpenGL REQUIRED)

# Add source files
target_sources(ooey PRIVATE 
    src/ooey.cpp
    src/application.cpp
    src/primitives/rect_primitive.cpp
    src/primitives/line_primitive.cpp
    src/primitives/text_primitive.cpp
    src/view.cpp
    src/input.cpp
    src/controller.cpp
    src/platform/x11/x11_window_backend.cpp
)

# Setup include directories
target_include_directories(ooey PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(ooey PUBLIC 
    X11::X11 
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
