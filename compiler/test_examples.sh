#!/bin/bash

# Compile and test example files

BUILD_DIR="${1:-./build}"
COMPILER="${BUILD_DIR}/debug/vittec"

if [ ! -x "$COMPILER" ]; then
    echo "Error: vittec compiler not found at $COMPILER"
    echo "Please build the compiler first:"
    echo "  mkdir -p build/debug"
    echo "  cd build/debug"
    echo "  cmake -DCMAKE_BUILD_TYPE=Debug ../.."
    echo "  make"
    exit 1
fi

echo "=== Testing Vitte Compiler Examples ==="
echo

EXAMPLES=(
    examples/hello.vitte
    examples/fibonacci.vitte
    examples/types.vitte
    examples/arithmetic.vitte
    examples/control.vitte
    examples/structs.vitte
    examples/functions.vitte
)

for example in "${EXAMPLES[@]}"; do
    if [ -f "$example" ]; then
        echo "Testing: $example"
        "$COMPILER" --emit-c "$example" -o /tmp/out.c
        if [ $? -eq 0 ]; then
            echo "  ✓ OK"
        else
            echo "  ✗ FAILED"
        fi
    fi
done

echo
echo "=== Tests Complete ==="
