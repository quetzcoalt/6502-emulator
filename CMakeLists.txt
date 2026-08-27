cmake_minimum_required(VERSION 3.14)
project(my_project)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz
)

FetchContent_MakeAvailable(nlohmann_json)
FetchContent_MakeAvailable(googletest)

include(CTest)
enable_testing()

add_executable(6502 src/6502.cpp)
add_executable(my_tests tests/6502_test.cc)

target_link_libraries(my_tests
    PRIVATE
        GTest::gtest_main
        nlohmann_json::nlohmann_json
)

target_link_libraries(6502
    PRIVATE
        nlohmann_json::nlohmann_json
)

target_include_directories(my_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

file(COPY
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/entries
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/tests
)

include(GoogleTest)
gtest_discover_tests(my_tests)