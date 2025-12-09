#!/bin/bash

mkdir -p build_bintr
pushd build_bintr
cmake .. -DCMAKE_BUILD_TYPE=Debug -DLA_BINARY_TRANSLATION=ON
make -j$(nproc)
popd

mkdir -p .fuzz_inputs
./build_bintr/vmfuzzer -runs=1000000 .fuzz_inputs/
#./build_bintr/elffuzzer .fuzz_inputs/
