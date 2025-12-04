cmake_minimum_required(VERSION 3.14)
project(libloong_script VERSION 1.0.0 LANGUAGES CXX)

# Only set these if not already set by parent project
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()
if(NOT CMAKE_CXX_STANDARD_REQUIRED)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

include(FetchContent)

# Fetch libfmt
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 9.1.0
)
FetchContent_MakeAvailable(fmt)

# Fetch libloong if not already available
if(NOT TARGET loong)
    # Check if we're in the libloong repository (local build)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/CMakeLists.txt")
        message(STATUS "Using local libloong from ${CMAKE_CURRENT_SOURCE_DIR}/../../lib")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib ${CMAKE_CURRENT_BINARY_DIR}/libloong)
    else()
        # Fetch from git (for external projects using FetchContent)
        FetchContent_Declare(
            libloong
            GIT_REPOSITORY https://github.com/fwsGonzo/libloong.git
            GIT_TAG main
        )
        FetchContent_MakeAvailable(libloong)
    endif()
endif()

# Script framework library
add_library(libloong_script STATIC
    script.cpp
    script.hpp
    host_bindings.hpp
    api_generator.hpp
)

# Create an alias for consistency with FetchContent usage
add_library(libloong::script ALIAS libloong_script)

target_include_directories(libloong_script PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(libloong_script PUBLIC
    loong
    fmt::fmt
)

# Only build examples if this is the main project
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    # Original example (code snippet based)
    add_executable(script_example
        example.cpp
    )
    target_link_libraries(script_example PRIVATE libloong_script)
endif()
