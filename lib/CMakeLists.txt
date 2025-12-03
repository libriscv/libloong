#
# libloong loong emulator library
#

cmake_minimum_required(VERSION 3.5)
project(loong CXX)

option(LA_DEBUG "Enable debug output" OFF)
option(LA_BINARY_TRANSLATION "Enable binary translation" OFF)
option(LA_THREADED "Enable threaded support" ON)
set(LA_MASKED_MEMORY_BITS "0" CACHE STRING "Power-of-two memory arena size for masking (0 = disabled)")
option(LA_ENABLE_ARENA_BASE_REGISTER "Arena base register optimization (Linux only)" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Version information
set(loong_VERSION_MAJOR 0)
set(loong_VERSION_MINOR 1)

# Source files
set(SOURCES
	libloong/cpu.cpp
	libloong/elf_loader.cpp
	libloong/la64.cpp
	libloong/machine.cpp
	libloong/machine_accelerate.cpp
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
	libloong/linux/syscalls_threads.cpp
)

# Dispatch implementations
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
	AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0
	AND DEFINED LA_TAILCALL_DISPATCH
	AND LA_TAILCALL_DISPATCH)
	# Tail-call optimization dispatch (fastest)
	list(APPEND SOURCES
		libloong/tailcall_dispatch.cpp
		libloong/tailcall_inaccurate_dispatch.cpp
	)
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
