# Stage 0: Bootstrap Seed Compiler

## Overview

Stage 0 is the **minimal bootstrap compiler** - the entry point for the entire three-stage bootstrap process. It is compiled from C (using the system C compiler) and serves as the foundation to compile Stage 1, the first self-hosted Vitte compiler.

## Purpose

- **Trust anchor**: First compiler in the bootstrap chain
- **Entry point**: Compiles Stage 1 (stage1/src/main.vit)
- **Minimal but complete**: Has just enough features to bootstrap the full compiler
- **Reproducibility**: Source is canonical and pinned by SHA256

## Bootstrap Chain

```
┌─────────────────────┐
│  C Compiler (cc)    │  System compiler
└──────────┬──────────┘
           │
           ├─ Compile: seed/src/main.c (not Vitte - C source)
           │
           ▼
┌─────────────────────┐
│ Stage 0: vittec0    │  Bootstrap seed compiler
│ (Minimal compiler)  │
└──────────┬──────────┘
           │
           ├─ Compile: stage1/src/main.vit (using vittec0)
           │
           ▼
┌─────────────────────┐
│ Stage 1: vittec1    │  First self-hosted compiler
│ (Full compiler)     │
└──────────┬──────────┘
           │
           ├─ Compile: stage2/src/main.vit (using vittec1)
           │
           ▼
┌─────────────────────┐
│ Stage 2: vittec2    │  Verification compiler
│ (Verified compiler) │
└─────────────────────┘
```

## Key Characteristics

✓ **Minimal**: Only essential compiler features
✓ **Self-contained**: Standalone compiler, no dependencies on Stage 1
✓ **Deterministic**: Reproducible compilation output
✓ **Canonical**: Source of truth for bootstrap chain
✓ **Verifiable**: Can be audited for correctness

## Components

### Frontend (Input Processing)
- **Lexer** (`lexer_scan`): Tokenization with minimal overhead
- **Parser** (`parser_parse`): Basic AST construction
- **Semantic Analyzer** (`semantic_check`): Essential validation

### Middle-end (Transformation)
- **IR Generator** (`ir_generate`): Simple intermediate representation
- **Minimal optimizer**: Basic optimization support

### Backend (Output Generation)
- **Code Generator** (`backend_codegen`): Assembly generation
- **Linker** (`linker_link`): Object file linking

## Compilation Pipeline

```
Source Code
    ↓
[Lexer] → Tokens
    ↓
[Parser] → AST
    ↓
[Semantic] → Validated AST
    ↓
[IR Gen] → Intermediate Representation
    ↓
[Codegen] → Assembly/Object Code
    ↓
[Linker] → Executable Binary
    ↓
Stage 1 Compiler (vittec1)
```

## Features

✓ Token-based lexical analysis
✓ Syntax parsing to AST
✓ Basic semantic validation
✓ IR generation
✓ Assembly code generation
✓ Object file linking
✓ Error reporting

## Source Structure

```
seed/src/main.vit               Main seed compiler
├── Compiler stages (6 phases)
│   ├── lexer_scan()            Tokenization
│   ├── parser_parse()          AST construction
│   ├── semantic_check()        Validation
│   ├── ir_generate()           IR generation
│   ├── backend_codegen()       Code generation
│   └── linker_link()           Linking
├── compile_source()            Main compilation function
├── compile()                   CLI interface
└── Helper functions
    ├── version_text()
    ├── get_stage_info()
    ├── check_features()
    └── print_info()
```

## Usage

### Build Seed Compiler

The seed compiler must first be compiled from C:

```bash
# This is done automatically by bootstrap.sh
cd toolchain

# Manual compilation (if needed)
cc -o build/vittec0 seed/src/main.c

# Or compile from Vitte (requires existing compiler)
./bootstrap.sh check
```

### Use Seed Compiler

Once built, use it to compile Stage 1:

```bash
# Compile Stage 1 source
./build/vittec0 toolchain/stage1/src/main.vit -o build/vittec1

# Check version
./build/vittec0 --version

# Compile a Vitte file
./build/vittec0 hello.vit -o hello
```

### From Bootstrap Script

```bash
# Automatic bootstrap (builds all stages)
cd toolchain && ./bootstrap.sh

# Quick mode (fast build)
./bootstrap.sh quick

# Verbose output
./bootstrap.sh -v normal
```

## Key Functions

### Compilation Pipeline

```vitte
lexer_scan(source: string) → [tokens]
parser_parse(tokens: [string]) → [ast]
semantic_check(ast: [string]) → bool
ir_generate(ast: [string]) → [ir]
backend_codegen(ir: [string]) → [asm]
linker_link(objects: [string], libs: [string]) → string
```

### Entry Points

```vitte
compile_source(input: string, output: string, opts: CompileOptions) → bool
compile(argc: int, argv: [string]) → int
main_bootstrap() → int
```

### Information

```vitte
version_text() → "vittec0 stage0-vitte-seed 0.1.0"
get_stage_info() → string
check_features() → [string]
print_info() → string
```

## Verification

### Check Seed Compiler

```bash
# Check it was built
test -f build/vittec0 && echo "✓ Seed compiler exists"

# Check it runs
./build/vittec0 --version

# Check capabilities
./build/vittec0 --features
```

### Verify Compilation

```bash
# Try compiling a simple file
echo 'proc hello() -> int { give 0 }' > test.vit
./build/vittec0 test.vit -o test_compiled

# Check for errors
echo $?  # 0 = success
```

## Size and Performance

- **Binary size**: ~1.5 MB (minimal compiler)
- **Build time**: ~30-60 seconds (from C compiler)
- **Compilation speed**: ~10-20 files/second (minimal features)

## Design Decisions

### Minimal Feature Set

The seed compiler intentionally has limited features:
- **Why**: Minimize trusted code base for bootstrap
- **Benefit**: Easier to audit and verify correctness
- **Trade-off**: Slower compilation, fewer optimizations

### Single-Stage Compilation

Unlike a production compiler, the seed does minimal passes:
- Lexer, Parser, Semantic, IR, Codegen, Linker (6 phases)
- No advanced optimization
- No cross-compilation support (initially)

### Deterministic Output

All compilation is deterministic:
- Same input → Same binary output
- No timestamp/random data in output
- Reproducible across different systems

## Trust Model

The seed compiler is the **trust anchor** for the bootstrap:

```
System C Compiler
       ↓ (compile from C)
Stage 0 (vittec0) ← TRUST POINT
       ↓ (compile with vittec0)
Stage 1 (vittec1)
       ↓ (compile with vittec1)
Stage 2 (vittec2)
       ↓ (verify vittec1 == vittec2)
Production Compiler
```

## Limitations

Stage 0 is intentionally minimal:
- Limited optimization
- Basic error messages
- No advanced features
- Single-threaded compilation

**This is acceptable** because:
- Only used once (to build Stage 1)
- Stage 1 is full-featured
- Bootstrap is temporary, not permanent

## Integration

The seed compiler is used by:

1. **Bootstrap script** (`bootstrap.sh`): Orchestrates the build
2. **Make targets** (`make bootstrap`, `make quick`): Convenience interface
3. **Makefile automation**: Rebuilding when needed

## Next Steps

→ [Stage 1: First Self-Hosted](../stage1/README.md)  
→ [Stage 2: Verification](../stage2/README.md)  
→ [Bootstrap Design](../BOOTSTRAP_DESIGN.md)  
→ [Bootstrap Examples](../EXAMPLES.md)

## Troubleshooting

**Seed compiler won't build from C**:
```bash
# Check C compiler available
which cc || which gcc

# Manual build with verbose output
cc -v -o build/vittec0 seed/src/main.c
```

**Seed compiler fails to compile Stage 1**:
```bash
# Check seed exists and runs
./build/vittec0 --version

# Verbose compilation
./build/vittec0 -v stage1/src/main.vit -o build/vittec1
```

**Build directory doesn't exist**:
```bash
mkdir -p build
make bootstrap
```

## Summary

The **Stage 0 seed compiler** is the critical entry point for the bootstrap process. It's minimal by design, containing only the essential compiler features needed to compile a full-featured compiler (Stage 1). Once Stage 1 is built, the seed becomes less critical, but it remains the verifiable trust anchor for the entire bootstrap chain.

---

**Version**: 0.1.0  
**Status**: Bootstrap entry point  
**Role**: Stage 0 minimal compiler  
**Trust Level**: Highest (auditability critical)
