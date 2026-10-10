include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(lox_tests
  test_language.cc
  test_functions.cc
  test_class.cc
  test_const.cc
  test_gc.cc
  )

target_include_directories(lox_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lox_tests PRIVATE lox_core GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(lox_tests)