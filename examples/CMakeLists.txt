set(OOEY_BUILD_CONTROLS ON)
add_ooey_executable(hello_ooey hello_ooey.cpp)
target_link_libraries(hello_ooey PRIVATE ooey)

if(OOEY_BUILD_CONTROLS)
    target_link_libraries(hello_ooey PRIVATE ooey_controls)

    add_ooey_executable(hello_ooey_mvvmc hello_ooey_mvvmc.cpp)
    target_link_libraries(hello_ooey_mvvmc PRIVATE ooey ooey_controls)

    add_ooey_executable(hello_ooey_text hello_ooey_text.cpp)
    target_link_libraries(hello_ooey_text PRIVATE ooey ooey_controls)

    if(OOEY_BUILD_X11)
        add_ooey_executable(hello_ooey_text_x11 hello_ooey_text_x11.cpp)
        target_link_libraries(hello_ooey_text_x11 PRIVATE ooey ooey_controls)
    endif()

    add_ooey_executable(hello_ooey_modern hello_ooey_modern.cpp)
    target_link_libraries(hello_ooey_modern PRIVATE ooey ooey_controls)

    add_ooey_executable(hello_clock_sinusoid hello_clock_sinusoid.cpp)
    target_link_libraries(hello_clock_sinusoid PRIVATE ooey ooey_controls)

    add_ooey_executable(hello_list hello_list.cpp)
    target_link_libraries(hello_list PRIVATE ooey ooey_controls)

    add_ooey_executable(hello_layout hello_layout.cpp)
    target_link_libraries(hello_layout PRIVATE ooey ooey_controls)

    add_ooey_executable(hello_layout_mvvmc hello_layout_mvvmc.cpp)
    target_link_libraries(hello_layout_mvvmc PRIVATE ooey ooey_controls)

    add_ooey_executable(hello_wizard hello_wizard.cpp)
    target_link_libraries(hello_wizard PRIVATE ooey ooey_controls)

    add_ooey_executable(OoeyCode
        codeeditor/OoeyCode.cpp
        codeeditor/code_editor.cpp
    )
    target_link_libraries(OoeyCode PRIVATE ooey ooey_controls)
    target_include_directories(OoeyCode PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/codeeditor)

    add_ooey_executable(hello_image hello_image.cpp)
    target_link_libraries(hello_image PRIVATE ooey ooey_controls)

    add_ooey_executable(hello_paint hello_paint.cpp)
    target_link_libraries(hello_paint PRIVATE ooey ooey_controls)

    add_ooey_executable(performance_benchmark performance_benchmark.cpp)
    target_link_libraries(performance_benchmark PRIVATE ooey ooey_controls)

    add_ooey_executable(hello_fonts hello_fonts.cpp)
    target_link_libraries(hello_fonts PRIVATE ooey ooey_controls)

    add_ooey_executable(hello_sysinfo
        sysinfo/metrics.cpp
        sysinfo/styled_panel.cpp
        sysinfo/view_model.cpp
        sysinfo/view.cpp
        sysinfo/themes.cpp
        sysinfo/main.cpp
    )
    target_link_libraries(hello_sysinfo PRIVATE ooey ooey_controls)

    add_ooey_executable(hello_reflection hello_reflection.cpp)
    target_link_libraries(hello_reflection PRIVATE ooey ooey_controls)

    add_ooey_executable(hello_spreadsheet hello_spreadsheet.cpp)
    target_link_libraries(hello_spreadsheet PRIVATE ooey ooey_controls)


    if(NOT EMSCRIPTEN AND NOT ANDROID)
        find_package(Boost)
        find_package(Threads REQUIRED)
        if(Boost_FOUND)
            add_ooey_executable(hello_network hello_network.cpp)
            target_link_libraries(hello_network PRIVATE ooey ooey_controls Threads::Threads)
            target_include_directories(hello_network PRIVATE ${Boost_INCLUDE_DIRS})
        endif()
    endif()

    # Generate TodoView.hpp and TodoView.cpp from TodoView.ooey
    set(TODO_OOEY "${CMAKE_CURRENT_SOURCE_DIR}/todo/TodoView.ooey")
    set(TODO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/todo_gen")
    set(TODO_GEN_HDR "${TODO_GEN_DIR}/TodoView.hpp")
    set(TODO_GEN_SRC "${TODO_GEN_DIR}/TodoView.cpp")

    add_custom_command(
        OUTPUT "${TODO_GEN_HDR}" "${TODO_GEN_SRC}"
        COMMAND tooey_compiler "${TODO_OOEY}" "${TODO_GEN_DIR}" "TodoView" "TodoViewModel"
        DEPENDS "${TODO_OOEY}" tooey_compiler
        COMMENT "Compiling TodoView.ooey with tooey_compiler"
    )

    add_ooey_executable(hello_todo
        todo/main.cpp
        todo/TodoViewModel.cpp
        "${TODO_GEN_SRC}"
    )
    target_link_libraries(hello_todo PRIVATE ooey ooey_controls tooey)
    target_include_directories(hello_todo PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/todo"
        "${TODO_GEN_DIR}"
    )

    # Generate LocalizationView.hpp and LocalizationView.cpp from LocalizationView.ooey
    set(LOCALIZATION_OOEY "${CMAKE_CURRENT_SOURCE_DIR}/localization/LocalizationView.ooey")
    set(LOCALIZATION_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/localization_gen")
    set(LOCALIZATION_GEN_HDR "${LOCALIZATION_GEN_DIR}/LocalizationView.hpp")
    set(LOCALIZATION_GEN_SRC "${LOCALIZATION_GEN_DIR}/LocalizationView.cpp")

    add_custom_command(
        OUTPUT "${LOCALIZATION_GEN_HDR}" "${LOCALIZATION_GEN_SRC}"
        COMMAND tooey_compiler "${LOCALIZATION_OOEY}" "${LOCALIZATION_GEN_DIR}" "LocalizationView" "LocalizationViewModel"
        DEPENDS "${LOCALIZATION_OOEY}" tooey_compiler
        COMMENT "Compiling LocalizationView.ooey with tooey_compiler"
    )

    add_ooey_executable(hello_localization
        localization/main.cpp
        localization/LocalizationViewModel.cpp
        "${LOCALIZATION_GEN_SRC}"
    )
    target_link_libraries(hello_localization PRIVATE ooey ooey_controls tooey)
    target_include_directories(hello_localization PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/localization"
        "${LOCALIZATION_GEN_DIR}"
    )
endif()

if(OOEY_BUILD_WAYLAND)
    add_ooey_executable(hello_wayland hello_wayland.cpp)
    target_link_libraries(hello_wayland PRIVATE ooey)
    if(OOEY_BUILD_CONTROLS)
        target_link_libraries(hello_wayland PRIVATE ooey_controls)
    endif()

    if(OOEY_BUILD_CONTROLS)
        add_ooey_executable(hello_ooey_text_wayland hello_ooey_text_wayland.cpp)
        target_link_libraries(hello_ooey_text_wayland PRIVATE ooey ooey_controls)

        add_ooey_executable(hello_wayland_vulkan hello_wayland_vulkan.cpp)
        target_link_libraries(hello_wayland_vulkan PRIVATE ooey ooey_controls)
    endif()
endif()

if(EMSCRIPTEN)
    add_ooey_executable(hello_emscripten hello_emscripten.cpp)
    target_link_libraries(hello_emscripten PRIVATE ooey ooey_controls)
endif()
