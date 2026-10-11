cmake_minimum_required(VERSION 3.15)
project(efkenlang)


set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Everything except main.cpp, shared by the executable and the tests.
add_library(lox_core
    src/chunk.cpp
    src/value.cpp
    src/debug.cpp
    src/vm.cpp
    src/compiler.cpp
    src/scanner.cpp
    src/obj.cpp
)

target_include_directories(lox_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)


add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE lox_core)

if (LOX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()