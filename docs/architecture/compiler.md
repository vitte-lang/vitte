# Compiler Architecture

Understanding the Vitte compiler pipeline and design.

## Overview

The Vitte compiler (vittec) transforms Vitte source code into portable C17, which can be compiled to native binaries on any platform with a C compiler.

```
Vitte Source Code
        ↓
    [Lexer]
        ↓
    [Parser]
        ↓
    [Type Checker]
        ↓
    [HIR (High-level IR)]
        ↓
    [IR Lowering]
        ↓
    [MIR/LIR (Mid-level/Low-level IR)]
        ↓
    [Optimization Passes]
        ↓
    [C17 Code Generation]
        ↓
C17 Source Code → [System C Compiler] → Native Binary
```

## Compilation Phases

### 1. Lexical Analysis (Lexer)

Converts source text into tokens:

```
Input:  let x = 42
Output: [KEYWORD(let), IDENTIFIER(x), OPERATOR(=), NUMBER(42)]
```

**Location**: `compiler/src/front/lexer.c`

### 2. Parsing

Builds an Abstract Syntax Tree (AST) from tokens:

```
let x = 42 + y
        ↓
    Assignment
    ├── name: x
    └── value: BinaryOp
        ├── op: +
        ├── left: 42
        └── right: Variable(y)
```

**Location**: `compiler/src/front/parser.c`

### 3. Type Checking

Validates types and performs type inference:

- Verifies type consistency
- Resolves generic types
- Checks method calls and field access
- Reports type errors with diagnostics

**Location**: `compiler/src/diag/type_checker.c`

### 4. High-level IR (HIR)

Intermediate representation closer to source semantics:

- Function bodies
- Ownership annotations
- Pattern matching structures

### 5. IR Lowering

Transforms HIR into lower-level IR for optimization:

- Explicitizes control flow
- Simplifies memory operations
- Prepares for backend

### 6. Optimization Passes

Applies transformations to improve performance:

- Dead code elimination
- Common subexpression elimination
- Inlining
- Constant folding

**Location**: `compiler/src/back/optimizer.c`

### 7. Code Generation

Produces C17 code:

- Allocates memory for variables
- Generates function calls
- Handles error propagation with Result types
- Emits cleanup code for resource management

**Location**: `compiler/src/back/codegen.c`

## Key Design Decisions

### Why C17?

1. **Portability** - Compiles on any platform with a modern C compiler
2. **Performance** - C compilers have decades of optimization
3. **Simplicity** - No need for a custom backend
4. **Interoperability** - Easy C FFI integration

### Memory Management

- **Ownership tracking** at compile time
- **Stack allocation** by default
- **Heap allocation** via `Box`, `Rc`, etc.
- **RAII pattern** - resources freed automatically

### Error Handling

- **Result<T, E>** type for recoverable errors
- **Type-checked error propagation** with `?` operator
- **Panic** for unrecoverable errors
- **Compile-time guarantees** - no null pointer dereferences

## Implementation Structure

```
compiler/
├── src/
│   ├── main.c          # Entry point
│   ├── vittec.c        # Main compiler logic
│   ├── version.c       # Version info
│   ├── front/          # Lexer and parser
│   ├── diag/           # Diagnostics and type checking
│   ├── back/           # Code generation and optimization
│   └── support/        # Utilities, data structures
├── tests/              # Compiler tests
├── include/            # Header files
└── docs/               # Documentation
```

## Testing

- **Smoke tests** - Basic compilation tests
- **Unit tests** - Individual component tests
- **Integration tests** - Full compilation pipeline
- **Fuzzing** - Random input testing for robustness

## Contributing

When modifying the compiler:

1. Understand which phase(s) your change affects
2. Add tests in `compiler/tests/`
3. Follow the code organization
4. Update relevant documentation

For detailed contribution guidelines, see [CONTRIBUTING.md](../../CONTRIBUTING.md).
