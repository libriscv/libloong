#
# libloong loong emulator library
#

cmake_minimum_required(VERSION 3.5)
project(loong CXX)

option(LA_DEBUG "Enable debug output" OFF)
option(LA_BINARY_TRANSLATION "Enable binary translation" OFF)
option(LA_THREADED "Enable threaded support" ON)
set(LA_MASKED_MEMORY_BITS "0" CACHE STRING "Power-of-two memory arena size for masking (0 = disabled)")

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

# Generate settings header
configure_file(libloong_settings.h.in ${CMAKE_CURRENT_BINARY_DIR}/libloong_settings.h)
configure_file(libloong.pc.in ${CMAKE_CURRENT_BINARY_DIR}/libloong.pc)

# Create loong library
add_library(loong ${SOURCES})

# Binary translation support
if (LA_BINARY_TRANSLATION)
	target_sources(loong PRIVATE
		libloong/tr_api.cpp
		libloong/tr_emit.cpp
		libloong/tr_translate.cpp
		libloong/tr_compiler.cpp
	)
	target_compile_definitions(loong PUBLIC
		ENABLE_LIBTCC=1
	)

	# TinyCC integration for JIT compilation
	enable_language(C)
	include(FetchContent)
	FetchContent_Declare(tinycc
		GIT_REPOSITORY https://github.com/fwsGonzo/tinycc.git
		GIT_TAG        mob
	)
	FetchContent_MakeAvailable(tinycc)

	set(TINYCC_LOCATION    "${CMAKE_BINARY_DIR}/_deps/tinycc-src")
	set(CONFIG_H_LOCATION "${TINYCC_LOCATION}/config.h")
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CONFIG_H_LOCATION})
	set_source_files_properties(${CONFIG_H_LOCATION} PROPERTIES GENERATED TRUE)
	file(COPY "${TINYCC_LOCATION}/libtcc.h" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
	file(COPY "${TINYCC_LOCATION}/libtcc1.h" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
	file(COPY "${TINYCC_LOCATION}/lib-arm64.h" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

	target_sources(loong PRIVATE
		${tinycc_SOURCE_DIR}/libtcc.c
		${tinycc_SOURCE_DIR}/libtcc.h
		${CONFIG_H_LOCATION}
	)
	target_compile_definitions(loong PRIVATE
		CONFIG_TCC_PREDEFS=1
		TCC_VERSION="0.9.27"
		TCC_IS_NATIVE=1
		CONFIG_TCC_BACKTRACE=0
		CONFIG_TCC_BCHECK=0
	)
	# We support: Windows, Linux, macOS and Android
	if (ANDROID_TOOLCHAIN)
		target_compile_definitions(loong PRIVATE
			TARGETOS_ANDROID=1
			TCC_TARGET_ARM64=1
		)
	elseif (CMAKE_HOST_APPLE OR APPLE)
		target_compile_definitions(loong PRIVATE
			TARGETOS_Darwin=1
			TCC_TARGET_MACHO=1
			TCC_TARGET_ARM64=1
		)
	elseif (CMAKE_HOST_WIN32 OR MINGW_TOOLCHAIN)
		target_compile_definitions(loong PRIVATE
			TARGETOS_Windows=1
			TCC_TARGET_PE=1
			TCC_TARGET_X86_64=1
		)
	elseif (CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
		target_compile_definitions(loong PRIVATE
			TARGETOS_FreeBSD=1
			TCC_TARGET_X86_64=1
		)
	elseif (RISCV_TOOLCHAIN)
		target_compile_definitions(loong PRIVATE
			TARGETOS_Linux=1
			TCC_TARGET_RISCV64=1
		)
	elseif (CMAKE_HOST_UNIX)
		target_compile_definitions(loong PRIVATE
			TARGETOS_Linux=1
			TCC_TARGET_X86_64=1
		)
	endif()
	target_include_directories(loong PUBLIC
		"${CMAKE_CURRENT_BINARY_DIR}"
	)
endif()

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
