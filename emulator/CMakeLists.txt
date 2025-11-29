cmake_minimum_required(VERSION 3.9.4)
project(loongarch_emulator CXX)
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
set(CMAKE_CXX_STANDARD 20)

option(NATIVE "Enable native optimizations" OFF)
option(LTO "Enable link-time optimization" OFF)

if (LTO)
	include(CheckIPOSupported)
	check_ipo_supported(RESULT supported OUTPUT error)
	if (supported)
		set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
	endif()
endif()

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
