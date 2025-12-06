cmake_minimum_required(VERSION 3.18)
project(gamedev_example CXX)

option(SANITIZERS "Enable sanitizers" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add LoongScript
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../script script)

# Fetch fmt library
include(FetchContent)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 10.1.1
)
FetchContent_MakeAvailable(fmt)

# Build the game engine executable
add_executable(asteroid_game
    main.cpp
)

target_link_libraries(asteroid_game PRIVATE
    libloong_script
    fmt::fmt
)

# Set output directory
set_target_properties(asteroid_game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Enable sanitizers if requested
if(SANITIZERS)
	target_compile_options(asteroid_game PRIVATE -fsanitize=address -fsanitize=undefined)
	target_link_options(asteroid_game PRIVATE -fsanitize=address -fsanitize=undefined)
endif()
