# Phase 1: Project Setup and Build System (Completed)

## Goal
Establish a solid foundation for the OOEY GUI engine using modern C++20 and a robust build system.

## Action Items

1. **Initialize CMake Project (Completed)**
   - Create a root `CMakeLists.txt`.
   - Enforce C++20 standard (`set(CMAKE_CXX_STANDARD 20)`).
   - Configure basic project settings (name, version, languages).

2. **Directory Structure (Completed)**
   - Create the standard foundational directories:
     - `include/ooey/`: Public header files.
     - `src/`: Implementation files.
     - `tests/`: Unit and integration tests.
     - `examples/`: Example applications using the OOEY engine.
     - `docs/`: Project documentation (where this file resides).

3. **Dependency Management (Completed)**
   - Plan how to handle external libraries (e.g., `find_package`, git submodules, or CMake FetchContent).
   - Keep it simple initially, preparing for future libraries like OpenGL, Wayland, and eventually `nlohmann/json`.

4. **Testing Framework Setup (Completed)**
   - Integrate a C++ testing framework (e.g., GoogleTest or Catch2) in the `tests/` directory using FetchContent.
   - Set up CTest for easy test execution.

5. **Basic "Hello World" Compile (Completed)**
   - Create a dummy library target and an example executable to verify the build system compiles and links correctly.
