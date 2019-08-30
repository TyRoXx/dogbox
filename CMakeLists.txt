cmake_minimum_required(VERSION 3.15)
project(dogbox LANGUAGES CXX)

set(DEPENDENCIES_INSTALL_DIR ${CMAKE_SOURCE_DIR}/../dogbox_dependencies/install)

option(DOGBOX_WITH_GCOV "compile with gcov support" OFF)
if(DOGBOX_WITH_GCOV)
    add_definitions(-coverage)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -coverage")
    add_custom_target(testcoverage
        COMMAND
            find ${CMAKE_CURRENT_BINARY_DIR} -type f -name '*.gcda' -delete &&
            $<TARGET_FILE:tests> &&
            ${DEPENDENCIES_INSTALL_DIR}/bin/lcov --capture --no-external --gcov-tool /usr/bin/gcov-8 --directory ${CMAKE_CURRENT_SOURCE_DIR} --directory ${CMAKE_CURRENT_BINARY_DIR} --output-file coverage.info &&
            ${DEPENDENCIES_INSTALL_DIR}/bin/genhtml coverage.info --output-directory coverage-html &&
            firefox coverage-html/index.html
        WORKING_DIRECTORY ${CMAKE_CURRENT_BUILD_DIR})
    add_dependencies(testcoverage tests)
endif()

set(CMAKE_PREFIX_PATH ${DEPENDENCIES_INSTALL_DIR})

find_package(SQLite3 REQUIRED)

set(BOOST_ROOT ${DEPENDENCIES_INSTALL_DIR})
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS system unit_test_framework filesystem REQUIRED)

set(benchmark_ROOT_DIR ${DEPENDENCIES_INSTALL_DIR})
find_package(benchmark REQUIRED)

find_library(FUSE_LIB fuse)

find_package(OpenSSL REQUIRED)

add_definitions(
    -Wall
    -Wextra
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wrestrict
    -Wnull-dereference
    -Wdouble-promotion
    -Wshadow
    -fstrict-aliasing
    -Wformat=2
    -Wuseless-cast
    -pedantic
    -Werror
    -Wconversion
    -Wunused
    -Wundef
    -Wvla
    -Wswitch-enum
    -Wuninitialized
    -Wstrict-overflow=5
    -std=c++17
    -D_FILE_OFFSET_BITS=64
    -DFUSE_USE_VERSION=26
)

include_directories(.)

add_subdirectory(blob_layer)
add_subdirectory(trees)
add_subdirectory(fuse_adaptor)
add_subdirectory(in_memory_fuse_frontend_shared)
add_subdirectory(benchmarks)
add_subdirectory(in_memory_fuse_frontend)
add_subdirectory(tests)
