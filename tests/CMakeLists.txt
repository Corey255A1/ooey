include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(OOEY_TEST_SRCS
    test_ooey.cpp
    test_core_architecture.cpp
    test_primitives.cpp
    test_layout.cpp
    test_image.cpp
    test_font.cpp
)

if(OOEY_BUILD_FRAMEBUFFER)
    list(APPEND OOEY_TEST_SRCS test_framebuffer.cpp)
endif()

add_executable(ooey_tests ${OOEY_TEST_SRCS})
target_link_libraries(ooey_tests PRIVATE ooey ooey_controls GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(ooey_tests)
