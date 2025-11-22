cmake_minimum_required(VERSION 3.9.4)
project(loongarch_emulator CXX)

set(CMAKE_CXX_STANDARD 20)

option(NATIVE "Enable native optimizations" OFF)
option(LTO "Enable link-time optimization" OFF)

set(SOURCES
	src/main.cpp
)

# Link to libloong
add_subdirectory(../lib libloong)

add_executable(laemu ${SOURCES})
target_link_libraries(laemu loong)

if (NATIVE)
	target_compile_options(laemu PRIVATE -march=native)
endif()

if (LTO)
	include(CheckIPOSupported)
	check_ipo_supported(RESULT supported OUTPUT error)
	if (supported)
		set_property(TARGET laemu PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
	endif()
endif()

# Compiler optimizations
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(laemu PRIVATE -O3 -Wall -Wextra)
endif()
