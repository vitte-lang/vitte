# Vitte Compiler Examples

This directory contains example Vitte programs to test the compiler.

## Examples

### hello.vitte
Simple "Hello, World!" program using the `say` keyword.

### fibonacci.vitte
Recursive Fibonacci function with integer parameters and return type.

### types.vitte
Demonstration of different type declarations (i32, f64, string, bool) and constants.

### arithmetic.vitte
Basic arithmetic operations: addition, subtraction, multiplication, division, modulo.

### control.vitte
Control flow structures: if/elif/else, while loops, and for loops.

### structs.vitte
Struct definition with fields and struct initialization with literal values.

### functions.vitte
Function definitions with parameters and return types, function calls.

## Building

Build the compiler:

```bash
mkdir -p build/debug
cd build/debug
cmake -DCMAKE_BUILD_TYPE=Debug ../..
make
```

## Testing

Run all examples:

```bash
./test_examples.sh build
```

Or test individual files:

```bash
./build/debug/vittec --emit-c examples/hello.vitte -o /tmp/hello.c
./build/debug/vittec --tokens examples/hello.vitte
./build/debug/vittec --json-diag examples/hello.vitte
```

## CLI Options

- `--tokens` - Emit token stream
- `--emit-c` - Emit C code
- `--json-diag` - JSON diagnostics output
- `-o <file>` - Output file
- `--help` - Show help
- `--version` - Show version
