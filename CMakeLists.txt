cmake_minimum_required(VERSION 3.16)

project(openmp_cholesky_factorization LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(C2_BUILD_TYPE_VALUE "${CMAKE_BUILD_TYPE}")
if(C2_BUILD_TYPE_VALUE STREQUAL "")
    set(C2_BUILD_TYPE_VALUE "unspecified")
endif()

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/bench/build_info.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/build_info.h
    @ONLY
)

option(C2_ENABLE_WARNINGS "Enable common compiler warnings" ON)

if(C2_ENABLE_WARNINGS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-Wall -Wextra -Wpedantic)
    elseif(MSVC)
        add_compile_options(/W4)
    endif()
endif()

include(CTest)
# Enable OpenMP explicitly because `v3` is the first required shared-memory parallel version.
find_package(OpenMP REQUIRED)

add_library(cholesky_filesystem_compat INTERFACE)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
    target_link_libraries(cholesky_filesystem_compat INTERFACE stdc++fs)
endif()

add_library(cholesky_openmp_runtime INTERFACE)
# Keep the OpenMP runtime in one interface target so current and archived variants share the same parallel build setup.
target_link_libraries(cholesky_openmp_runtime INTERFACE OpenMP::OpenMP_CXX)

function(add_cholesky_variant variant_name variant_source_root)
    add_library(${variant_name} ${variant_source_root}/src/cholesky.cpp)
    target_include_directories(${variant_name} PUBLIC ${variant_source_root}/include)
    # Link every variant against the same OpenMP runtime so the current implementation and archived milestones build uniformly.
    target_link_libraries(${variant_name} PUBLIC cholesky_openmp_runtime)

    add_library(${variant_name}_test_support test/cholesky_test_suite.cpp)
    target_include_directories(${variant_name}_test_support PUBLIC
        ${variant_source_root}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/test
    )
    target_link_libraries(${variant_name}_test_support PUBLIC ${variant_name})

    add_library(${variant_name}_benchmark_support bench/cholesky_benchmark_support.cpp)
    target_include_directories(${variant_name}_benchmark_support PUBLIC
        ${variant_source_root}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/bench
        ${CMAKE_CURRENT_BINARY_DIR}/generated
    )
    target_link_libraries(${variant_name}_benchmark_support PUBLIC ${variant_name})

    if(variant_name STREQUAL "cholesky")
        add_executable(cholesky_example example/main.cpp)
        target_link_libraries(cholesky_example PRIVATE cholesky)
    endif()

    add_executable(${variant_name}_tests test/test_cholesky.cpp)
    target_link_libraries(${variant_name}_tests PRIVATE ${variant_name}_test_support)
    set_property(GLOBAL APPEND PROPERTY CHOLESKY_ALL_TEST_TARGETS ${variant_name}_tests)

    if(variant_name STREQUAL "cholesky")
        add_test(NAME cholesky_tests COMMAND cholesky_tests)
    else()
        add_test(NAME ${variant_name}_tests COMMAND ${variant_name}_tests)
    endif()

    add_executable(${variant_name}_benchmark bench/benchmark_cholesky.cpp)
    target_link_libraries(${variant_name}_benchmark PRIVATE ${variant_name}_benchmark_support)
    set_property(GLOBAL APPEND PROPERTY CHOLESKY_ALL_BENCHMARK_TARGETS ${variant_name}_benchmark)

    add_executable(${variant_name}_reference_logdet_driver test/reference_logdet_driver.cpp)
    target_include_directories(${variant_name}_reference_logdet_driver PRIVATE
        ${variant_source_root}/include
    )
    target_link_libraries(${variant_name}_reference_logdet_driver PRIVATE ${variant_name})
    set_property(GLOBAL APPEND PROPERTY CHOLESKY_ALL_BENCHMARK_TARGETS ${variant_name}_reference_logdet_driver)

    add_executable(${variant_name}_benchmark_suite bench/cholesky_benchmark_suite.cpp)
    target_include_directories(${variant_name}_benchmark_suite PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/generated
    )
    add_dependencies(${variant_name}_benchmark_suite ${variant_name}_benchmark)
    target_link_libraries(${variant_name}_benchmark_suite PRIVATE
        ${variant_name}_test_support
        ${variant_name}_benchmark_support
        cholesky_filesystem_compat
    )
    set_property(GLOBAL APPEND PROPERTY CHOLESKY_ALL_BENCHMARK_SUITE_TARGETS ${variant_name}_benchmark_suite)
endfunction()

add_cholesky_variant(cholesky ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB history_variant_dirs LIST_DIRECTORIES true
    ${CMAKE_CURRENT_SOURCE_DIR}/history/versions/cholesky_v*)

foreach(history_variant_dir ${history_variant_dirs})
    if(EXISTS ${history_variant_dir}/include/mphil_dis_cholesky.h AND EXISTS ${history_variant_dir}/src/cholesky.cpp)
        get_filename_component(history_variant_name ${history_variant_dir} NAME)
        add_cholesky_variant(${history_variant_name} ${history_variant_dir})
    endif()
endforeach()

set(v1_reference_root ${CMAKE_CURRENT_SOURCE_DIR}/history/versions/cholesky_v1_serial_opt)
if(EXISTS ${v1_reference_root}/include/mphil_dis_cholesky.h AND EXISTS ${v1_reference_root}/src/cholesky.cpp)
    add_library(cholesky_v1_reference test/cholesky_v1_reference.cpp)
    target_include_directories(cholesky_v1_reference PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test
        ${v1_reference_root}/include
    )

    add_executable(cholesky_against_v1_tests test/test_against_v1.cpp)
    target_include_directories(cholesky_against_v1_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/test
    )
    target_link_libraries(cholesky_against_v1_tests PRIVATE cholesky cholesky_v1_reference)
    add_test(NAME cholesky_against_v1_tests COMMAND cholesky_against_v1_tests)
    set_property(GLOBAL APPEND PROPERTY CHOLESKY_ALL_TEST_TARGETS cholesky_against_v1_tests)

    add_executable(cholesky_stability_tests test/test_stability.cpp)
    target_include_directories(cholesky_stability_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/test
    )
    target_link_libraries(cholesky_stability_tests PRIVATE cholesky)
    add_test(NAME cholesky_stability_tests COMMAND cholesky_stability_tests)
    set_property(GLOBAL APPEND PROPERTY CHOLESKY_ALL_TEST_TARGETS cholesky_stability_tests)
endif()

set(v2_reference_root ${CMAKE_CURRENT_SOURCE_DIR}/history/versions/cholesky_v2_serial_blocked)
if(EXISTS ${v2_reference_root}/include/mphil_dis_cholesky.h AND EXISTS ${v2_reference_root}/src/cholesky.cpp)
    add_library(cholesky_v2_reference test/cholesky_v2_reference.cpp)
    target_include_directories(cholesky_v2_reference PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test
        ${v2_reference_root}/include
    )

    add_executable(cholesky_openmp_consistency_tests test/test_openmp_consistency.cpp)
    target_include_directories(cholesky_openmp_consistency_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/test
    )
    # Link the OpenMP runtime here as well because this regression test sets explicit team sizes to check thread-count consistency.
    target_link_libraries(cholesky_openmp_consistency_tests PRIVATE
        cholesky
        cholesky_v2_reference
        cholesky_openmp_runtime
    )
    add_test(NAME cholesky_openmp_consistency_tests COMMAND cholesky_openmp_consistency_tests)
    set_property(GLOBAL APPEND PROPERTY CHOLESKY_ALL_TEST_TARGETS cholesky_openmp_consistency_tests)
endif()

get_property(all_test_targets GLOBAL PROPERTY CHOLESKY_ALL_TEST_TARGETS)
if(all_test_targets)
    add_custom_target(cholesky_all_tests DEPENDS ${all_test_targets})
endif()

get_property(all_benchmark_targets GLOBAL PROPERTY CHOLESKY_ALL_BENCHMARK_TARGETS)
if(all_benchmark_targets)
    add_custom_target(cholesky_all_benchmarks DEPENDS ${all_benchmark_targets})
endif()

get_property(all_benchmark_suite_targets GLOBAL PROPERTY CHOLESKY_ALL_BENCHMARK_SUITE_TARGETS)
if(all_benchmark_suite_targets)
    add_custom_target(cholesky_all_benchmark_suites DEPENDS ${all_benchmark_suite_targets})
endif()

if(all_test_targets AND all_benchmark_targets AND all_benchmark_suite_targets)
    add_custom_target(
        cholesky_all_variants
        DEPENDS
            ${all_test_targets}
            ${all_benchmark_targets}
            ${all_benchmark_suite_targets}
    )
endif()
