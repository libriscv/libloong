#!/bin/bash

mkdir -p .build
pushd .build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release -- -j4
popd
