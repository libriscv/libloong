cmake_minimum_required(VERSION 3.18)
project(libloong_benchmark CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find libloong
set(LIBLOONG_DIR ${CMAKE_SOURCE_DIR}/..)
add_subdirectory(${LIBLOONG_DIR}/lib ${CMAKE_BINARY_DIR}/libloong)

# Guest program directory
set(GUEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/guest)
set(GUEST_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/guest_binaries)
file(MAKE_DIRECTORY ${GUEST_BINARY_DIR})

# Find LoongArch toolchain
find_program(LOONGARCH_CC loongarch64-linux-gnu-gcc-14
    PATHS /usr/bin /usr/local/bin
    REQUIRED
)
find_program(LOONGARCH_CXX loongarch64-linux-gnu-g++-14
    PATHS /usr/bin /usr/local/bin
    REQUIRED
)

# Build guest program
set(GUEST_SOURCE ${GUEST_DIR}/guest_main.cpp)
set(GUEST_BINARY ${GUEST_BINARY_DIR}/benchmark_guest.elf)

add_custom_command(
    OUTPUT ${GUEST_BINARY}
    COMMAND ${LOONGARCH_CXX}
        ${GUEST_SOURCE}
        -o ${GUEST_BINARY}
        -static
        -O2
        -march=loongarch64
        -Wl,-Ttext-segment=0x200000
    DEPENDS ${GUEST_SOURCE}
    COMMENT "Building LoongArch guest program"
    VERBATIM
)

add_custom_target(guest_binary ALL DEPENDS ${GUEST_BINARY})

# Host benchmark executable
add_executable(bench
    src/main.cpp
    src/benchmark.cpp
)

target_include_directories(bench PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${LIBLOONG_DIR}/lib
    ${CMAKE_BINARY_DIR}/libloong
)

target_link_libraries(bench PRIVATE
    loong
)

target_compile_definitions(bench PRIVATE
    GUEST_BINARY_PATH="${GUEST_BINARY}"
)

add_dependencies(bench guest_binary)

# Install target
install(TARGETS bench RUNTIME DESTINATION bin)
