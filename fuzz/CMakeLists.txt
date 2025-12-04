cmake_minimum_required(VERSION 3.9)
project(loongarch_fuzzer C CXX)

# Fuzzer sanitizer mode
# Options: "address,undefined" or "memory,undefined"
set(FUZZER_MODE "address,undefined")

# Add libloong library
add_subdirectory(../lib lib)

# Configure libloong for fuzzing
target_compile_options(loong PUBLIC -g3 -O0)
target_compile_options(loong PUBLIC "-fsanitize=${FUZZER_MODE}" "-fsanitize=fuzzer")
target_compile_options(loong PUBLIC "-fprofile-instr-generate")
target_compile_options(loong PUBLIC "--coverage")
target_compile_definitions(loong PUBLIC FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION=1)

# LibFuzzer requires Clang
if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	message(FATAL_ERROR "libfuzzer is part of the Clang compiler suite. Please use clang++ to build the fuzzer.")
endif()

set(SOURCES
	fuzz.cpp
)

# Instruction Fuzzer
add_executable(vmfuzzer ${SOURCES})
target_link_libraries(vmfuzzer loong)
target_link_libraries(vmfuzzer "-fsanitize=${FUZZER_MODE}" "-fsanitize=fuzzer")
target_link_libraries(vmfuzzer "-fprofile-instr-generate")
target_link_libraries(vmfuzzer "--coverage" "-fuse-ld=lld")
target_compile_definitions(vmfuzzer PUBLIC FUZZER=1)

# ELF Fuzzer
add_executable(elffuzzer ${SOURCES})
target_link_libraries(elffuzzer loong)
target_link_libraries(elffuzzer "-fsanitize=${FUZZER_MODE}" "-fsanitize=fuzzer")
target_link_libraries(elffuzzer "-fprofile-instr-generate")
target_link_libraries(elffuzzer "--coverage" "-fuse-ld=lld")
target_compile_definitions(elffuzzer PUBLIC FUZZER=2)
