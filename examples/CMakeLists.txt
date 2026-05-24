add_executable(hello_ooey hello_ooey.cpp)
target_link_libraries(hello_ooey PRIVATE ooey)

if(OOEY_BUILD_CONTROLS)
    target_link_libraries(hello_ooey PRIVATE ooey_controls)

    add_executable(hello_ooey_mvvmc hello_ooey_mvvmc.cpp)
    target_link_libraries(hello_ooey_mvvmc PRIVATE ooey ooey_controls)

    add_executable(hello_ooey_text hello_ooey_text.cpp)
    target_link_libraries(hello_ooey_text PRIVATE ooey ooey_controls)
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
