cmake_minimum_required(VERSION 3.14)
project(script_guest_cpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Required linker flags for libloong compatibility
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -Wl,-Ttext-segment=0x200000")

include(FetchContent)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 9.1.0
)
FetchContent_MakeAvailable(fmt)

# Guest executable
add_executable(guest_app
    src/main.cpp
	# This needs to be pre-generated in order to build
    ${CMAKE_CURRENT_SOURCE_DIR}/libloong_api.hpp
)

target_link_libraries(guest_app PRIVATE fmt::fmt)
target_include_directories(guest_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Install target
install(TARGETS guest_app DESTINATION bin)
