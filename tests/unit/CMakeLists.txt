#
# libloong unit tests
#
# Unit testing framework with codebuilder for on-the-fly compilation
# of C and C++ test programs
#

cmake_minimum_required(VERSION 3.10)

# Find Catch2
find_package(Catch2 3 QUIET)

if(NOT Catch2_FOUND)
	# Try to use the bundled Catch2
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../Catch2/CMakeLists.txt")
		message(STATUS "Using bundled Catch2 from tests/Catch2")
		add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../Catch2 ${CMAKE_CURRENT_BINARY_DIR}/Catch2)
	else()
		message(WARNING "Catch2 not found. Unit tests will not be built.")
		message(WARNING "Install Catch2 or run: git submodule update --init --recursive")
		return()
	endif()
endif()

# Find LoongArch compiler
find_program(LOONGARCH_GCC
	NAMES loongarch64-linux-gnu-gcc-14 loongarch64-linux-gnu-gcc loongarch64-unknown-linux-gnu-gcc
)

if(NOT LOONGARCH_GCC)
	message(WARNING "LoongArch compiler not found. Unit tests will not be built.")
	message(WARNING "Install loongarch64-linux-gnu-gcc-14 to enable unit tests.")
	return()
endif()

message(STATUS "Found LoongArch compiler: ${LOONGARCH_GCC}")

# Create unit test executable
add_executable(unit_tests
	unit_main.cpp
	test_basic.cpp
	test_cpp.cpp
	test_vmcall.cpp
	test_machine.cpp
	test_instructions.cpp
	test_native.cpp
)

# Set C++ standard
set_property(TARGET unit_tests PROPERTY CXX_STANDARD 20)
set_property(TARGET unit_tests PROPERTY CXX_STANDARD_REQUIRED ON)

# If the loong target does not exist, add the directory
if(NOT TARGET loong)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib libloong)
endif()

# Link against libloong and Catch2
target_link_libraries(unit_tests PRIVATE
	loong
	Catch2::Catch2WithMain
)

# Include test utilities
target_include_directories(unit_tests PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

# Enable CTest integration
include(CTest)
include(Catch)
catch_discover_tests(unit_tests)

# Add custom test target for convenience
add_custom_target(check
	COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
	DEPENDS unit_tests
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	COMMENT "Running unit tests"
)

# Print test info
message(STATUS "Unit tests configured successfully")
message(STATUS "  Run tests with: make test")
message(STATUS "  Or run directly: ./tests/unit/unit_tests")
message(STATUS "  Or with verbose output: make check")
