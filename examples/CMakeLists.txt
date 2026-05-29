set(OOEY_BUILD_CONTROLS ON)
add_executable(hello_ooey hello_ooey.cpp)
target_link_libraries(hello_ooey PRIVATE ooey)

if(OOEY_BUILD_CONTROLS)
    target_link_libraries(hello_ooey PRIVATE ooey_controls)

    add_executable(hello_ooey_mvvmc hello_ooey_mvvmc.cpp)
    target_link_libraries(hello_ooey_mvvmc PRIVATE ooey ooey_controls)

    add_executable(hello_ooey_text hello_ooey_text.cpp)
    target_link_libraries(hello_ooey_text PRIVATE ooey ooey_controls)

    add_executable(hello_ooey_text_x11 hello_ooey_text_x11.cpp)
    target_link_libraries(hello_ooey_text_x11 PRIVATE ooey ooey_controls)

    add_executable(hello_ooey_modern hello_ooey_modern.cpp)
    target_link_libraries(hello_ooey_modern PRIVATE ooey ooey_controls)

    add_executable(hello_clock_sinusoid hello_clock_sinusoid.cpp)
    target_link_libraries(hello_clock_sinusoid PRIVATE ooey ooey_controls)

    add_executable(hello_list hello_list.cpp)
    target_link_libraries(hello_list PRIVATE ooey ooey_controls)

    add_executable(hello_wizard hello_wizard.cpp)
    target_link_libraries(hello_wizard PRIVATE ooey ooey_controls)

    if(NOT EMSCRIPTEN)
        find_package(Boost)
        find_package(Threads REQUIRED)
        if(Boost_FOUND)
            add_executable(hello_network hello_network.cpp)
            target_link_libraries(hello_network PRIVATE ooey ooey_controls Threads::Threads)
            target_include_directories(hello_network PRIVATE ${Boost_INCLUDE_DIRS})
        endif()
    endif()
endif()

if(OOEY_BUILD_WAYLAND)
    add_executable(hello_wayland hello_wayland.cpp)
    target_link_libraries(hello_wayland PRIVATE ooey)
    if(OOEY_BUILD_CONTROLS)
        target_link_libraries(hello_wayland PRIVATE ooey_controls)
    endif()

    if(OOEY_BUILD_CONTROLS)
        add_executable(hello_ooey_text_wayland hello_ooey_text_wayland.cpp)
        target_link_libraries(hello_ooey_text_wayland PRIVATE ooey ooey_controls)
    endif()
endif()

if(EMSCRIPTEN)
    add_executable(hello_emscripten hello_emscripten.cpp)
    target_link_libraries(hello_emscripten PRIVATE ooey ooey_controls)
    set_target_properties(hello_emscripten PROPERTIES SUFFIX ".html")
    target_link_options(hello_emscripten PRIVATE --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/shell.html)
endif()
