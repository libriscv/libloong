#!/bin/bash
set -e

mkdir -p .build
pushd .build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
popd

echo "Build complete. Executables are in the .build/ directory."
ls -la .build/
