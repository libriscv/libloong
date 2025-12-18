#!/bin/bash
set -e

echo "Building libloong Go bindings..."

# Use go generate to build native libraries
echo "Building native libraries with go generate..."
go generate

echo "Building Go package..."
go build -v .

echo "Building examples..."
mkdir -p ../build
go build -v -o ../build/go_hello_world ./examples/hello_world.go
go build -v -o ../build/go_vmcall ./examples/vmcall.go
go build -v -o ../build/go_memory_ops ./examples/memory_ops.go

echo ""
echo "✓ Build complete! Binaries are in ../build/"
echo ""
echo "Run examples:"
echo "  ../build/go_hello_world ../tests/programs/hello_world.elf"
echo "  ../build/go_vmcall ../tests/programs/cxx_test.elf <function_name>"
echo "  ../build/go_memory_ops ../tests/programs/hello_world.elf"
