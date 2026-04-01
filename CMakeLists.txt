cmake_minimum_required(VERSION 3.16)

project(c2_coursework_cholesky LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(C2_ENABLE_WARNINGS "Enable common compiler warnings" ON)

if(C2_ENABLE_WARNINGS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-Wall -Wextra -Wpedantic)
    elseif(MSVC)
        add_compile_options(/W4)
    endif()
endif()

include(CTest)

add_library(cholesky_filesystem_compat INTERFACE)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
    target_link_libraries(cholesky_filesystem_compat INTERFACE stdc++fs)
endif()

add_library(cholesky src/cholesky.cpp)
target_include_directories(cholesky PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(cholesky_test_support test/cholesky_test_suite.cpp)
target_include_directories(cholesky_test_support PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/test
)
target_link_libraries(cholesky_test_support PUBLIC cholesky)

add_library(cholesky_benchmark_support bench/cholesky_benchmark_support.cpp)
target_include_directories(cholesky_benchmark_support PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/bench
)
target_link_libraries(cholesky_benchmark_support PUBLIC cholesky)

add_executable(cholesky_example example/main.cpp)
target_link_libraries(cholesky_example PRIVATE cholesky)

add_executable(cholesky_tests test/test_cholesky.cpp)
target_link_libraries(cholesky_tests PRIVATE cholesky_test_support)
add_test(NAME cholesky_tests COMMAND cholesky_tests)

add_executable(cholesky_benchmark bench/benchmark_cholesky.cpp)
target_link_libraries(cholesky_benchmark PRIVATE cholesky_benchmark_support)

add_executable(cholesky_benchmark_suite bench/cholesky_benchmark_suite.cpp)
target_link_libraries(cholesky_benchmark_suite PRIVATE
    cholesky_test_support
    cholesky_benchmark_support
    cholesky_filesystem_compat
)
