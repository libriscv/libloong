#!/bin/bash

export CC=x86_64-w64-mingw32-gcc
export CXX=x86_64-w64-mingw32-g++

mkdir -p .build_mingw
pushd .build_mingw
cmake .. -DCMAKE_BUILD_TYPE=Release -DLA_BINARY_TRANSLATION=OFF -DCMAKE_TOOLCHAIN_FILE=../mingw_toolchain.cmake
make -j$(nproc)
popd

