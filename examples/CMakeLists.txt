add_executable(hello_ooey hello_ooey.cpp)
target_link_libraries(hello_ooey PRIVATE ooey)

if(OOEY_BUILD_CONTROLS)
    target_link_libraries(hello_ooey PRIVATE ooey_controls)

    add_executable(hello_ooey_mvvmc hello_ooey_mvvmc.cpp)
    target_link_libraries(hello_ooey_mvvmc PRIVATE ooey ooey_controls)
endif()
