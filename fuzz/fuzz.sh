#!/bin/bash

mkdir -p build
pushd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
popd

mkdir -p .fuzz_inputs
#./build/vmfuzzer -runs=1000000 .fuzz_inputs/
gdb --args ./build/elffuzzer .fuzz_inputs/
