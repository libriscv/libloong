cmake_minimum_required(VERSION 3.14)
project(libloong_installed_example VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the installed libloong package
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBLOONG REQUIRED libloong)

# Create the example executable
add_executable(installed_example main.cpp)

# Link against the installed libloong library
target_include_directories(installed_example PRIVATE ${LIBLOONG_INCLUDE_DIRS})
target_link_libraries(installed_example ${LIBLOONG_LIBRARIES})
target_link_directories(installed_example PRIVATE ${LIBLOONG_LIBRARY_DIRS})

# Add pthread if on Linux
if (UNIX AND NOT APPLE)
	target_link_libraries(installed_example pthread)
endif()
