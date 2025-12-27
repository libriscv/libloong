#!/bin/bash
set -e

mkdir -p .build
cd .build
cmake .. -DCMAKE_BUILD_TYPE=Release -DLA_BINARY_TRANSLATION=ON
make -j$(nproc)

echo
echo "Build complete!"
echo "Run ./.build/bench to execute benchmarks"
echo "Run ./.build/bench --help for options"
