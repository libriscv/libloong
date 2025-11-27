#
# libloong loong emulator library
#

cmake_minimum_required(VERSION 3.5)
project(loong CXX)

option(LA_32  "32-bit loong support" ON)
option(LA_64  "64-bit loong support" ON)
option(LA_DEBUG "Enable debug output" OFF)
option(LA_BINARY_TRANSLATION "Enable binary translation" OFF)
option(LA_THREADED "Enable threaded support" ON)
option(LA_MEMORY_TRAPS "Enable memory traps" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Version information
set(loong_VERSION_MAJOR 0)
set(loong_VERSION_MINOR 1)

# Source files
set(SOURCES
	libloong/common.cpp
	libloong/cpu.cpp
	libloong/elf_loader.cpp
	libloong/machine.cpp
	libloong/machine_bytecode_stats.cpp
	libloong/memory.cpp
	libloong/memory_rw.cpp
	libloong/decoder_cache.cpp
	libloong/debug.cpp
	libloong/serialize.cpp
	libloong/threaded_rewriter.cpp
	libloong/posix/signals.cpp
	libloong/posix/threads.cpp
	libloong/linux/syscalls.cpp
)

# Architecture-specific implementations
if (LA_32)
	list(APPEND SOURCES libloong/la32.cpp)
endif()
if (LA_64)
	list(APPEND SOURCES libloong/la64.cpp)
endif()

# Dispatch implementations
if (FALSE AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT MSVC)
	# Tail-call optimization dispatch (fastest)
	list(APPEND SOURCES libloong/tailcall_dispatch.cpp)
	message(STATUS "Using tail-call optimization dispatch")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	# Threaded dispatch for GCC/Clang (using computed goto)
	list(APPEND SOURCES
		libloong/threaded_dispatch.cpp
		libloong/threaded_inaccurate_dispatch.cpp
	)
	message(STATUS "Using threaded dispatch (computed goto)")
else()
	# Fallback handler-based dispatch for other compilers
	list(APPEND SOURCES libloong/cpu_dispatch.cpp)
	message(STATUS "Using handler-based dispatch")
endif()

# Binary translation support
if (LA_BINARY_TRANSLATION)
	list(APPEND SOURCES
		libloong/tr_api.cpp
		libloong/tr_emit.cpp
		libloong/tr_translate.cpp
	)
	if (NOT MSVC)
		list(APPEND SOURCES libloong/tr_compiler.cpp)
	endif()
endif()

# Generate settings header
configure_file(libloong_settings.h.in ${CMAKE_CURRENT_BINARY_DIR}/libloong_settings.h)
configure_file(libloong.pc.in ${CMAKE_CURRENT_BINARY_DIR}/libloong.pc)

# Create library
add_library(loong ${SOURCES})

target_include_directories(loong PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	$<INSTALL_INTERFACE:include>
)

# Compiler options
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(loong PRIVATE
		-Wall -Wextra -Wno-unused-parameter
		-fno-rtti
	)
	if (NOT LA_DEBUG)
		target_compile_options(loong PRIVATE -O3)
	endif()
endif()

# Platform-specific settings
if (UNIX AND NOT APPLE)
	target_link_libraries(loong PUBLIC pthread)
	if (LA_BINARY_TRANSLATION)
		target_link_libraries(loong PUBLIC dl)
	endif()
elseif (WIN32)
	target_link_libraries(loong PUBLIC wsock32 ws2_32)
endif()

# Installation
install(TARGETS loong
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
)

install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/libloong_settings.h
	DESTINATION include/${PROJECT_NAME}
)

install(FILES
	libloong/common.hpp
	libloong/cpu.hpp
	libloong/cpu_inline.hpp
	libloong/machine.hpp
	libloong/machine_inline.hpp
	libloong/memory.hpp
	libloong/memory_inline.hpp
	libloong/registers.hpp
	libloong/decoder_cache.hpp
	libloong/decoded_exec_segment.hpp
	libloong/elf.hpp
	libloong/types.hpp
	libloong/page.hpp
	libloong/instruction.hpp
	libloong/la_instr.hpp
	libloong/debug.hpp

	DESTINATION include/${PROJECT_NAME}
)

install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/libloong.pc
	DESTINATION lib/pkgconfig
)
