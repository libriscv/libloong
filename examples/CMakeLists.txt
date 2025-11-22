cmake_minimum_required(VERSION 3.14)
project(libloong_examples LANGUAGES CXX)

# Add the libloong library
add_subdirectory(../lib libloong)

# Simple example
add_executable(simple_example simple.cpp)
target_link_libraries(simple_example loong)

# Custom syscall example
add_executable(custom_syscall_example custom_syscall.cpp)
target_link_libraries(custom_syscall_example loong)
set_target_properties(simple_example custom_syscall_example
	PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
)
