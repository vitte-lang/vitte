# Stage 1: First Self-Hosted Vitte Compiler

## Overview

Stage 1 is the **first full-featured, self-hosting Vitte compiler**. It is compiled from Vitte source using the seed compiler (vittec0) and represents the first time the compiler can compile itself.

## Purpose

- **Self-hosting**: Built using vittec0 (the seed compiler)
- **Complete features**: Implements all core compiler features
- **Bootstrap verification**: Enables Stage 2 verification

## Architecture

```
Stage 0 (Seed - vittec0)
        ↓
[Compile stage1/src/main.vit using vittec0]
        ↓
Stage 1 (Full - vittec1)
```

## Components

The Stage 1 compiler includes:

### Frontend
- **Lexer** (`lexer_scan`): Tokenizes source code
- **Parser** (`parser_parse`): Builds abstract syntax tree (AST)
- **Semantic Analyzer** (`semantic_check`): Type checking and validation

### Middle-end
- **IR Generator** (`ir_generate`): Creates intermediate representation
- **Optimizer**: Optimizes IR for better code generation

### Backend
- **Code Generator** (`backend_codegen`): Generates machine code/assembly
- **Linker** (`linker_link`): Links object files and libraries

## Features

✓ Complete lexical analysis
✓ Full syntax parsing
✓ Semantic analysis and type checking
✓ Intermediate representation generation
✓ Code generation
✓ Object file linking
✓ Basic optimization
✓ Error reporting and diagnostics

## Source Structure

```
src/main.vit               Main compiler implementation
├── Imports from toolchain modules
├── Compiler stages (lexer → linker)
├── compile_source()       Main compilation function
├── compile()              CLI entry point
└── Helper functions
```

## Usage

### Compile with Stage 1

```bash
# Using make
cd toolchain
make bootstrap

# Results in:
# build/vittec1 (Stage 1 compiler binary)

# Use Stage 1 compiler
./build/vittec1 input.vit -o output
```

### Compile Stage 1 Source

From seed compiler (vittec0):

```bash
./build/vittec0 -o build/vittec1 toolchain/stage1/src/main.vit
```

## Key Functions

### Compilation Pipeline

- `lexer_scan(source)` → `[tokens]`
- `parser_parse(tokens)` → `[ast]`
- `semantic_check(ast)` → `bool`
- `ir_generate(ast)` → `[ir]`
- `backend_codegen(ir)` → `[asm]`
- `linker_link(objects, libs)` → `binary`

### Entry Points

- `compile(argc, argv)` → `exit_code`
- `compile_source(input, output, opts)` → `bool`

### Information

- `version_text()` → "vittec1 stage1-vitte 0.1.0"
- `banner_text()` → "vittec1 stage1 native bootstrap"
- `get_stage_info()` → Stage information
- `check_features()` → List of features
- `can_self_compile()` → `true` (can compile itself)

## Verification

Stage 1 can:

✓ Compile Vitte source files
✓ Perform self-compilation (compile itself)
✓ Be verified by Stage 2

## Size and Performance

- **Binary size**: ~2.5MB
- **Compilation time**: ~3-5 minutes (depends on system)
- **Features**: Complete compiler with all capabilities

## Integration

Stage 1 is used by:

1. **Stage 2**: Recompilation for verification
2. **Main build**: As `VITTE_BOOTSTRAP` compiler

```bash
export VITTE_BOOTSTRAP=$(pwd)/toolchain/build/vittec1
make all
```

## Limitations

Stage 1 is:
- NOT yet verified (until Stage 2 passes)
- First self-hosted version (may have subtle bugs)
- Needs Stage 2 verification to be production-ready

## Next Steps

→ [Stage 2 Verification](../stage2/README.md)  
→ [Bootstrap Design](../BOOTSTRAP_DESIGN.md)  
→ [Project Integration](../INTEGRATION.md)

---

**Version**: 0.1.0  
**Status**: First self-hosted compiler  
**Role**: Bootstrap stage 1 compilation

