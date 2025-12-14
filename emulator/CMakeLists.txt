cmake_minimum_required(VERSION 3.9.4)
project(loongarch_emulator CXX)
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
set(CMAKE_CXX_STANDARD 20)

# Emulator build options
option(NATIVE "Enable native optimizations" OFF)
option(LTO "Enable link-time optimization" OFF)

# Embedded binary translation support
set(LA_EMBED_BINTR "" CACHE STRING "Path to embedded binary translation C file")

# Forward library options if not already set
# These can be set via command line: -DLA_MASKED_MEMORY_BITS=32
if (NOT DEFINED LA_DEBUG)
	option(LA_DEBUG "Enable debug output" OFF)
endif()
if (NOT DEFINED LA_BINARY_TRANSLATION)
	option(LA_BINARY_TRANSLATION "Enable binary translation" OFF)
endif()
if (NOT DEFINED LA_THREADED)
	option(LA_THREADED "Enable threaded support" ON)
endif()
if (NOT DEFINED LA_MASKED_MEMORY_BITS)
	set(LA_MASKED_MEMORY_BITS "0" CACHE STRING "Power-of-two memory arena size for masking (0 = disabled)")
endif()

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

# Add embedded binary translation if specified
if (LA_EMBED_BINTR)
	if (EXISTS "${LA_EMBED_BINTR}")
		message(STATUS "Embedding binary translation from: ${LA_EMBED_BINTR}")
		list(APPEND SOURCES "${LA_EMBED_BINTR}")
		# Force the embedded file to be compiled as C++ (it's written in C but compatible)
		set_source_files_properties("${LA_EMBED_BINTR}" PROPERTIES LANGUAGE CXX)
	else()
		message(WARNING "Embedded bintr file not found: ${LA_EMBED_BINTR}")
	endif()
endif()

# Link to libloong
add_subdirectory(../lib libloong)

add_executable(laemu ${SOURCES})
target_link_libraries(laemu loong)

if (NATIVE)
	target_compile_options(laemu PRIVATE -march=native)
endif()
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_link_libraries(laemu pthread -static -static-libgcc -static-libstdc++)
endif()
