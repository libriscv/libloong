#
# libloong tests
#

cmake_minimum_required(VERSION 3.5)
project(loongarch_tests CXX)

# Find loongarch-linux-gnu-gcc compiler
find_program(LOONGARCH_GCC loongarch64-linux-gnu-gcc-14 loongarch64-linux-gnu-gcc)
find_program(LOONGARCH_GXX loongarch64-linux-gnu-g++-14 loongarch64-linux-gnu-g++)

if(NOT LOONGARCH_GCC)
	message(WARNING "LoongArch compiler not found. Tests will not be compiled for LoongArch.")
	message(WARNING "Install loongarch64-linux-gnu-gcc-14 to compile LoongArch test programs.")
	return()
endif()

message(STATUS "Found LoongArch compiler: ${LOONGARCH_GCC}")

# Build test programs with LoongArch compiler
set(TEST_PROGRAMS
	cxx_test
	simple_add
	hello_world
	return_42_bare
	printf_test
	stream
)

# Output directory for LoongArch binaries
set(LA_TEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/loongarch_bins")
file(MAKE_DIRECTORY ${LA_TEST_DIR})

# Compile each test program
foreach(prog ${TEST_PROGRAMS})
	# Special handling for bare metal program
	if(prog STREQUAL "return_42_bare")
		add_custom_command(
			OUTPUT "${LA_TEST_DIR}/${prog}"
			COMMAND ${LOONGARCH_GCC} -static -nostdlib -nostartfiles -O0 -g
				-Wl,-Ttext-segment=0x200000
				"${CMAKE_CURRENT_SOURCE_DIR}/programs/${prog}.c"
				-o "${LA_TEST_DIR}/${prog}"
			DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/programs/${prog}.c"
			COMMENT "Building bare metal LoongArch test program: ${prog}"
		)
	elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/programs/${prog}.cpp")
		add_custom_command(
			OUTPUT "${LA_TEST_DIR}/${prog}"
			COMMAND ${LOONGARCH_GXX} -static -O0 -g
				-Wl,-Ttext-segment=0x200000
				"${CMAKE_CURRENT_SOURCE_DIR}/programs/${prog}.cpp"
				-o "${LA_TEST_DIR}/${prog}"
			DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/programs/${prog}.cpp"
			COMMENT "Building LoongArch test program: ${prog}"
		)
	else()
		add_custom_command(
			OUTPUT "${LA_TEST_DIR}/${prog}"
			COMMAND ${LOONGARCH_GCC} -static -O0 -g
				-Wl,-Ttext-segment=0x200000
				-mlasx
				"${CMAKE_CURRENT_SOURCE_DIR}/programs/${prog}.c"
				-o "${LA_TEST_DIR}/${prog}"
			DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/programs/${prog}.c"
			COMMENT "Building LoongArch test program: ${prog}"
		)
	endif()
	list(APPEND LA_TEST_BINARIES "${LA_TEST_DIR}/${prog}")
endforeach()

# Custom target for all LoongArch test binaries
add_custom_target(loongarch_test_programs ALL
	DEPENDS ${LA_TEST_BINARIES}
)

# Build C++ test runner
add_executable(test_runner
	test_runner.cpp
)

# Set C++ standard
set_property(TARGET test_runner PROPERTY CXX_STANDARD 20)

# Build debug test runner
add_executable(debug_test
	debug_test.cpp
)

target_link_libraries(test_runner PRIVATE
	loong
)
target_link_libraries(debug_test PRIVATE
	loong
)

set_property(TARGET debug_test PROPERTY CXX_STANDARD 20)

# Add test
enable_testing()
add_test(NAME basic_execution COMMAND test_runner "${LA_TEST_DIR}")

# Add unit tests subdirectory
add_subdirectory(unit)
