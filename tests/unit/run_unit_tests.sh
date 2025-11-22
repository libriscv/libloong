#!/bin/bash
set -e

mkdir -p build_unit_tests
pushd build_unit_tests
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j4
ctest --output-on-failure $@
popd
