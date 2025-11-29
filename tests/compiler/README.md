# Unit Tests - Compiler

This directory contains unit tests for the Vitte compiler components.

## Structure

```
compiler/
├── lexer/           # Lexer tests
├── parser/          # Parser tests
├── sema/            # Semantic analysis tests
├── codegen/         # Code generation tests
└── optimizer/       # Optimizer tests
```

## Running Tests

```bash
make test-unit
```

## Test Categories

### Lexer Tests
- Token recognition
- Whitespace handling
- Comment parsing
- Error recovery

### Parser Tests
- Expression parsing
- Statement parsing
- Operator precedence
- Error messages

### Semantic Analysis Tests
- Symbol resolution
- Type checking
- Scope analysis
- Error detection

### Code Generation Tests
- IR generation
- Backend output
- Optimization
- Linking

### Optimizer Tests
- DCE (Dead Code Elimination)
- Constant folding
- Loop optimizations
- Inlining
