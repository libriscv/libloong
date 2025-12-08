#!/bin/bash
set -e

mkdir -p build_bintr_unit_tests
pushd build_bintr_unit_tests
cmake -DCMAKE_BUILD_TYPE=Debug -DLA_BINARY_TRANSLATION=ON ..
make -j4
ctest --output-on-failure $@
popd
