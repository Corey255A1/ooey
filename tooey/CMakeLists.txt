add_library(tooey STATIC)

target_sources(tooey PRIVATE
    src/lexer.cpp
    src/parser.cpp
    src/codegen.cpp
)

target_include_directories(tooey PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_executable(tooey_compiler src/compiler_main.cpp)
target_link_libraries(tooey_compiler PRIVATE tooey)

add_executable(tooey_lint src/linter_main.cpp)
target_link_libraries(tooey_lint PRIVATE tooey)
