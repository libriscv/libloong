#!/bin/bash
set -e

echo "======================================"
echo "Comprehensive Rust Bindings Test"
echo "======================================"
echo ""

cd "$(dirname "$0")/.."

# Test 1: hello_world example
echo "Test 1: hello_world example"
echo "----------------------------"
cargo run --release --example hello_world -- test_programs/hello_world.elf 2>&1 | tee /tmp/test1.log | grep -q "Hello from LoongArch!"
if [ $? -eq 0 ]; then
    echo "✓ PASS: hello_world produced expected output"
else
    echo "✗ FAIL: hello_world did not produce expected output"
    exit 1
fi
echo ""

# Test 2: return_42 with hello_world
echo "Test 2: return_42 execution"
echo "---------------------------"
cargo run --release --example hello_world -- test_programs/return_42.elf 2>&1 | tee /tmp/test2.log | grep -q "Execution completed successfully"
if [ $? -eq 0 ]; then
    echo "✓ PASS: return_42 executed successfully"
else
    echo "✗ FAIL: return_42 execution failed"
    exit 1
fi
echo ""

# Test 3: Instruction counting
echo "Test 3: Instruction counting"
echo "----------------------------"
cargo run --release --example hello_world -- test_programs/return_42.elf 2>&1 | grep "Instructions executed:" | awk '{print $3}' > /tmp/test3.log
INSTR_COUNT=$(cat /tmp/test3.log)
if [ "$INSTR_COUNT" -gt 0 ] 2>/dev/null; then
    echo "✓ PASS: Counted $INSTR_COUNT instructions"
else
    echo "✗ FAIL: Instruction counting failed"
    exit 1
fi
echo ""

# Summary
echo "======================================"
echo "All tests PASSED!"
echo "======================================"
