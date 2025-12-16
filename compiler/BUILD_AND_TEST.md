# Complete Build and Test Guide

## Quick Start

### Build Compiler

```bash
cd compiler
mkdir -p build/debug
cd build/debug
cmake -DCMAKE_BUILD_TYPE=Debug ../..
make
```

### Run Unit Tests

```bash
cd compiler/build/debug
ctest
```

Or individually:

```bash
./test_lexer
./test_ast
./test_types
./test_symbol_table
```

### Test Examples

```bash
cd compiler
./test_examples.sh build
```

## Build Types

### Debug Build

```bash
mkdir -p build/debug
cd build/debug
cmake -DCMAKE_BUILD_TYPE=Debug ../..
make
```

Includes debug symbols (-g), no optimization (-O0), and assertions enabled.

### Release Build

```bash
mkdir -p build/release
cd build/release
cmake -DCMAKE_BUILD_TYPE=Release ../..
make
```

Optimized (-O2) for production use.

## Project Structure

```
compiler/
├── CMakeLists.txt              # Main build configuration
├── CMakeLists_full.txt         # Full compiler build
├── CMakeLists.txt (in build/)  # Build output config
├── BUILD_INTEGRATION.md        # Integration guide
│
├── include/
│   ├── compiler.h              # Master header
│   └── compiler/               # 18 header files
│       ├── ast.h
│       ├── parser.h
│       ├── lexer.h
│       ├── types.h
│       ├── hir.h
│       ├── ir.h
│       ├── sema.h
│       ├── symbol_table.h
│       ├── diagnostic.h
│       ├── backend_c.h
│       ├── backend.h
│       ├── frontend.h
│       ├── codegen.h
│       ├── lowering.h
│       ├── optimizer.h
│       ├── target.h
│       └── driver.h
│
├── src/
│   ├── main.c                  # Entry point
│   ├── vittec.c                # CLI interface
│   ├── version.c               # Version info
│   └── *.c                     # 18 implementation files
│
├── tests/
│   ├── unit/
│   │   ├── test_lexer.c        # Lexer tests (8 test cases)
│   │   ├── test_ast.c          # AST tests
│   │   ├── test_types.c        # Types tests
│   │   └── test_symbol_table.c # Symbol table tests
│   └── smoke/                  # Smoke tests
│
├── examples/
│   ├── hello.vitte
│   ├── fibonacci.vitte
│   ├── types.vitte
│   ├── arithmetic.vitte
│   ├── control.vitte
│   ├── structs.vitte
│   ├── functions.vitte
│   ├── README.md
│   └── (7 example programs)
│
└── build/
    ├── CMakeLists.txt
    ├── README.md
    └── .gitignore
```

## Testing Strategy

### Unit Tests

Test individual components:

- **Lexer** (8 test cases):
  - Empty input
  - Identifiers
  - Keywords
  - Integers (decimal, hex, binary, octal)
  - Floats
  - Strings
  - Punctuation
  - Operators
  - Line tracking

- **AST** (3 test cases):
  - Module creation
  - Adding declarations
  - Capacity growth

- **Types** (2 test cases):
  - Type table creation
  - Insert/lookup operations

- **Symbol Table** (2 test cases):
  - Table creation
  - Scope management

### Integration Tests

Test example programs through the full compilation pipeline.

### Smoke Tests

Basic sanity tests for the compiler.

## CMake Targets

### Executables

- `vittec` - Main compiler executable
- `test_lexer` - Lexer unit tests
- `test_ast` - AST unit tests
- `test_types` - Types unit tests
- `test_symbol_table` - Symbol table unit tests

### Libraries

- `vittec_compiler` - Static compiler library

## Compiler Flags

### MSVC

- `/W4` - Warning level 4
- `/WX` - Treat warnings as errors

### GCC/Clang

- `-Wall` - Enable all warnings
- `-Wextra` - Extra warnings
- `-Werror` - Treat warnings as errors
- `-g` - Debug symbols (Debug only)
- `-O0` - No optimization (Debug only)
- `-O2` - Optimize for speed (Release only)

## CLI Usage

```bash
vittec [OPTIONS] <input.vitte> [-o output]

Options:
  --tokens          Emit token stream
  --emit-c          Emit C code
  --json-diag       JSON diagnostics format
  -o <file>         Output file
  --help            Show this help
  --version         Show version
```

## Examples

### Tokenize a file

```bash
./build/debug/vittec --tokens examples/hello.vitte
```

### Generate C code

```bash
./build/debug/vittec --emit-c examples/hello.vitte -o /tmp/hello.c
```

### JSON diagnostics

```bash
./build/debug/vittec --json-diag examples/hello.vitte
```

## Troubleshooting

### CMake not found

Install CMake 3.16+:

```bash
# Ubuntu/Debian
sudo apt-get install cmake

# macOS
brew install cmake

# Windows
# Download from https://cmake.org/download/
```

### Compiler errors

Ensure C11 is supported:

```bash
gcc --version
# Should be 4.9+ for GCC, 3.3+ for Clang
```

### Test failures

Run with verbose output:

```bash
cd build/debug
ctest --verbose
```

## Next Steps

1. ✅ Lexer implementation complete
2. ✅ Unit tests for core modules
3. ✅ CMake integration
4. ✅ Example programs
5. TODO: Parser implementation
6. TODO: Semantic analysis
7. TODO: Code generation
8. TODO: Optimization passes

See [COMPILER_FILES.md](COMPILER_FILES.md) for details on each module.
