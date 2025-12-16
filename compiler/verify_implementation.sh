#!/bin/bash

# Verification script for Vitte compiler implementation
# Checks all deliverables are in place

echo "=========================================="
echo "Vitte Compiler Implementation Verification"
echo "=========================================="
echo

# Check headers
echo "✓ Checking headers..."
HEADERS=(
    "include/compiler.h"
    "include/compiler/ast.h"
    "include/compiler/backend.h"
    "include/compiler/backend_c.h"
    "include/compiler/codegen.h"
    "include/compiler/compiler.h"
    "include/compiler/diagnostic.h"
    "include/compiler/driver.h"
    "include/compiler/frontend.h"
    "include/compiler/hir.h"
    "include/compiler/ir.h"
    "include/compiler/lexer.h"
    "include/compiler/lowering.h"
    "include/compiler/optimizer.h"
    "include/compiler/parser.h"
    "include/compiler/sema.h"
    "include/compiler/symbol_table.h"
    "include/compiler/target.h"
    "include/compiler/types.h"
)

for header in "${HEADERS[@]}"; do
    if [ -f "$header" ]; then
        echo "  ✓ $header"
    else
        echo "  ✗ MISSING: $header"
    fi
done

# Check source files
echo
echo "✓ Checking source files..."
SOURCES=(
    "src/compiler.c"
    "src/ast.c"
    "src/backend.c"
    "src/backend_c.c"
    "src/codegen.c"
    "src/diagnostic.c"
    "src/driver.c"
    "src/frontend.c"
    "src/hir.c"
    "src/ir.c"
    "src/lexer.c"
    "src/lowering.c"
    "src/optimizer.c"
    "src/parser.c"
    "src/sema.c"
    "src/symbol_table.c"
    "src/target.c"
    "src/types.c"
)

for source in "${SOURCES[@]}"; do
    if [ -f "$source" ]; then
        echo "  ✓ $source"
    else
        echo "  ✗ MISSING: $source"
    fi
done

# Check test files
echo
echo "✓ Checking test files..."
TESTS=(
    "tests/unit/test_lexer.c"
    "tests/unit/test_ast.c"
    "tests/unit/test_types.c"
    "tests/unit/test_symbol_table.c"
)

for test in "${TESTS[@]}"; do
    if [ -f "$test" ]; then
        echo "  ✓ $test"
    else
        echo "  ✗ MISSING: $test"
    fi
done

# Check example programs
echo
echo "✓ Checking example programs..."
EXAMPLES=(
    "examples/hello.vitte"
    "examples/fibonacci.vitte"
    "examples/types.vitte"
    "examples/arithmetic.vitte"
    "examples/control.vitte"
    "examples/structs.vitte"
    "examples/functions.vitte"
)

for example in "${EXAMPLES[@]}"; do
    if [ -f "$example" ]; then
        echo "  ✓ $example"
    else
        echo "  ✗ MISSING: $example"
    fi
done

# Check build configuration
echo
echo "✓ Checking build configuration..."
BUILD_FILES=(
    "CMakeLists.txt"
    "CMakeLists_full.txt"
    "Makefile"
    "build/CMakeLists.txt"
    "build/.gitignore"
    "build/README.md"
)

for file in "${BUILD_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "  ✓ $file"
    else
        echo "  ✗ MISSING: $file"
    fi
done

# Check documentation
echo
echo "✓ Checking documentation..."
DOCS=(
    "BUILD_AND_TEST.md"
    "BUILD_INTEGRATION.md"
    "COMPILER_FILES.md"
    "CREATION_RESUME.md"
    "DOCUMENTATION_INDEX.md"
    "IMPLEMENTATION_STATUS.md"
    "QUICK_START.md"
    "README_IMPLEMENTATION.md"
    "examples/README.md"
)

for doc in "${DOCS[@]}"; do
    if [ -f "$doc" ]; then
        echo "  ✓ $doc"
    else
        echo "  ✗ MISSING: $doc"
    fi
done

# Check scripts
echo
echo "✓ Checking scripts..."
SCRIPTS=(
    "test_examples.sh"
)

for script in "${SCRIPTS[@]}"; do
    if [ -f "$script" ]; then
        echo "  ✓ $script"
    else
        echo "  ✗ MISSING: $script"
    fi
done

# Summary
echo
echo "=========================================="
echo "Verification Complete!"
echo "=========================================="
echo
echo "Summary:"
echo "  Headers:        19 files"
echo "  Source files:   18 files"
echo "  Test files:     4 files"
echo "  Examples:       7 programs"
echo "  Build config:   6 files"
echo "  Documentation:  9 files"
echo "  Scripts:        1 file"
echo "  TOTAL:          ~64 files"
echo
echo "Next steps:"
echo "  1. make debug      - Build debug version"
echo "  2. make test       - Run unit tests"
echo "  3. make examples   - Test example programs"
echo
echo "For more info, see DOCUMENTATION_INDEX.md"
