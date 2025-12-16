# Compiler Documentation Index

Complete guide to all Vitte compiler documentation.

## Overview Documents

### [ASSEMBLY_README.md](ASSEMBLY_README.md) ⭐ START HERE
**15 min read** | Overview of entire assembly system  
Covers:
- Assembly feature overview
- Documentation structure and how to navigate
- Quick start guides for different user types
- Reference tables and common use cases
- Troubleshooting guide
- File organization

**Next**: [assembly_integration.md](assembly_integration.md)

---

## Assembly Language Documentation

### [assembly_integration.md](assembly_integration.md)
**25 min read** | Architecture and compiler integration  
Covers:
- How assembly is processed through compilation pipeline
- Inline assembly support and processing phases
- C backend integration (GCC vs MSVC)
- Constraint validation system
- Optimization interaction
- Cross-platform considerations
- Error handling and diagnostics
- Best practices summary

**Prerequisites**: Basic understanding of compilers  
**Audience**: Architects, compiler developers, advanced users

### [inline_assembly.md](inline_assembly.md)
**Reference document** | Complete syntax and constraint reference  
Covers:
- Complete inline assembly syntax
- GCC-style assembly template format
- Constraint system documentation
- x86-64 instruction support
- ARM64 instruction support
- WebAssembly support
- Operand types and modifiers
- Size suffixes and clobber lists
- Performance considerations
- When to use assembly vs C

**Audience**: Developers writing assembly code  
**Use as**: Reference manual - keep bookmarked

### [assembly_patterns.md](assembly_patterns.md)
**50 min read** | 14 practical assembly patterns  
Covers:
- Calling convention compatibility (System V, MSVC x64, ARM64)
- Pattern 1: Calling C functions from assembly
- Pattern 2: Global variable access
- Pattern 3: Struct field access
- Pattern 4: Memory barriers and synchronization
- Pattern 5: Atomic operations
- Pattern 6: Varargs (variadic functions)
- Pattern 7: Stack usage and frame management
- Pattern 8: Early clobbering
- Pattern 9-14: Advanced patterns
- Best practices for assembly-C code
- Common issues and solutions
- Verification checklist

**Audience**: Developers solving specific problems  
**Use for**: Finding code examples for your use case

### [assembly_testing.md](assembly_testing.md)
**40 min read** | Testing, verification, and benchmarking  
Covers:
- Unit testing assembly functions
- CTest integration
- Property-based testing strategies
- Edge case testing
- Benchmarking (simple timing, Linux perf, Windows VTune)
- Correctness verification (GDB, Valgrind)
- Architecture-specific testing
- Integration tests
- Coverage analysis
- Regression testing
- Performance regression detection
- Test documentation and reporting

**Audience**: QA engineers, performance engineers, testers  
**Use before**: Shipping code with assembly

### [assembly_quick_reference.md](assembly_quick_reference.md)
**Pocket reference** | One-page assembly cheat sheet  
Covers:
- Inline assembly syntax template
- Constraint quick reference table
- x86-64 and ARM64 register maps
- x86-64 and ARM64 instruction reference
- Operand size suffixes
- Argument passing conventions
- Common assembly patterns (5 examples)
- Constraint validation rules
- Clobber list rules
- Platform-specific code
- Testing checklist
- Performance tips
- Common mistakes
- Debugging tips

**Use**: Print and keep at desk  
**Quick lookup**: Register names, instruction syntax

---

## Build and Testing Documentation

### [BUILD_AND_TEST.md](BUILD_AND_TEST.md)
**20 min read** | Build system and test execution  
Covers:
- CMake configuration
- Makefile targets
- Building in debug and release modes
- Running unit tests
- Running integration tests
- Coverage analysis
- Performance profiling
- Continuous integration setup

**Prerequisites**: Basic CMake knowledge  
**Audience**: Build engineers, CI/CD maintainers

### [BUILD_INTEGRATION.md](BUILD_INTEGRATION.md)
**15 min read** | Integrating compiler into other projects  
Covers:
- Adding as subdirectory
- Using as external library
- CMake find_package integration
- Compiler API usage
- Linking and dependencies

**Audience**: Integration engineers

---

## Core Compiler Documentation

### [COMPILER_FILES.md](COMPILER_FILES.md)
**Reference** | Complete file inventory and purposes  
Covers:
- Header files (19 files documented)
- Implementation files (21 files documented)
- Test files
- Example files
- Configuration files
- Documentation files

**Use**: Understanding file organization, finding specific module

### [README_IMPLEMENTATION.md](README_IMPLEMENTATION.md)
**40 min read** | Implementation details of all modules  
Covers:
- Lexer implementation (complete)
- AST management
- Type system
- Symbol table
- Parser (framework)
- Semantic analysis (framework)
- HIR generation (framework)
- IR generation (framework)
- C backend (framework)
- Optimizer (framework)
- Code generation (framework)

**Audience**: Compiler developers extending functionality

### [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md)
**5 min read** | Current implementation status  
Covers:
- Completed components
- Partially completed components
- Stubified components
- Known limitations
- Test coverage

**Use**: Understanding what's ready and what isn't

### [IMPLEMENTATION_GUIDE.md](IMPLEMENTATION_GUIDE.md)
**Extensive guide** | Step-by-step implementation guide  
Covers:
- Architecture overview
- Each compiler phase in detail
- Implementation strategies
- Extension points
- Testing strategies

**Audience**: Contributors implementing new features

---

## Examples

### [../examples/inline_asm.vitte](../examples/inline_asm.vitte)
11 basic assembly examples:
1. CPUID instruction
2. Bit scan forward (BSF)
3. Memory barrier (mfence)
4. Atomic operations (lock prefix)
5. SIMD vector addition
6. Read timestamp counter (RDTSC)
7. Load barrier (lfence)
8. CPU pause instruction
9. Return address introspection
... and more

**Use**: Learning basic syntax, understanding instruction usage

### [../examples/asm_performance.vitte](../examples/asm_performance.vitte)
10 performance-critical examples:
1. Fast multiplication
2. Count leading zeros (CLZ)
3. Rotate left
4. Rotate right
5. Population count (POPCNT)
6. Atomic compare-and-swap
7. Fast memory copy
8. Fast memory fill
... and more

**Use**: Performance optimization, comparing asm vs C

### [../examples/asm_c_interop.vitte](../examples/asm_c_interop.vitte)
8 C-Assembly interoperability examples:
1. Call C functions from assembly
2. Global variable access
3. String search with assembly loops
4. Struct field manipulation
5. Varargs function calls
... and more

**Use**: Understanding C-Assembly boundaries, integration patterns

### [../examples/hybrid_vector_library.vitte](../examples/hybrid_vector_library.vitte)
Complete vector/matrix library:
- Vector creation and access
- Dot product with SIMD
- Vector scaling
- Vector addition
- Matrix multiplication
- Cache-aware transposition
- Benchmarking infrastructure

**Use**: Understanding complex hybrid code architecture

---

## Additional Documentation

### [QUICK_START.md](QUICK_START.md)
**10 min read** | Getting started immediately  
Covers:
- Installation
- First compilation
- Running tests
- Simple example

**Audience**: New users

### [README.md](README.md)
**Project overview** | What is Vitte?  
Covers:
- Language features
- Compilation targets
- Use cases
- Status

**Audience**: Everyone

### [COMPLETION_REPORT.md](COMPLETION_REPORT.md)
**Status summary** | What was completed in this work  
Covers:
- Files created
- Implementation status
- Testing coverage
- Documentation
- Examples

**Audience**: Project stakeholders

---

## Documentation Map by User Role

### For New Users
1. Read: [README.md](README.md)
2. Read: [QUICK_START.md](QUICK_START.md)
3. Explore: [examples/](../examples/)
4. Read: [ASSEMBLY_README.md](ASSEMBLY_README.md)

### For Assembly Developers
1. Start: [ASSEMBLY_README.md](ASSEMBLY_README.md)
2. Learn: [assembly_integration.md](assembly_integration.md)
3. Reference: [inline_assembly.md](inline_assembly.md)
4. Solve: [assembly_patterns.md](assembly_patterns.md)
5. Study: Example files
6. Test: [assembly_testing.md](assembly_testing.md)
7. Bookmark: [assembly_quick_reference.md](assembly_quick_reference.md)

### For Compiler Developers
1. Read: [README_IMPLEMENTATION.md](README_IMPLEMENTATION.md)
2. Reference: [COMPILER_FILES.md](COMPILER_FILES.md)
3. Study: [IMPLEMENTATION_GUIDE.md](IMPLEMENTATION_GUIDE.md)
4. Reference: [assembly_integration.md](assembly_integration.md) - "Inline Assembly Processing"
5. Check: [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md)

### For Build/Integration Engineers
1. Read: [BUILD_AND_TEST.md](BUILD_AND_TEST.md)
2. Reference: [BUILD_INTEGRATION.md](BUILD_INTEGRATION.md)
3. Reference: [COMPILER_FILES.md](COMPILER_FILES.md)

### For QA/Testers
1. Read: [BUILD_AND_TEST.md](BUILD_AND_TEST.md)
2. Study: [assembly_testing.md](assembly_testing.md)
3. Reference: [assembly_patterns.md](assembly_patterns.md) - Example code

### For Performance Engineers
1. Start: [ASSEMBLY_README.md](ASSEMBLY_README.md)
2. Study: [assembly_patterns.md](assembly_patterns.md)
3. Learn: [../examples/asm_performance.vitte](../examples/asm_performance.vitte)
4. Verify: [assembly_testing.md](assembly_testing.md) - "Benchmarking"
5. Reference: [assembly_quick_reference.md](assembly_quick_reference.md)

---

## Key Concepts Cross-Reference

### Constraint System
- **Understand**: [assembly_integration.md](assembly_integration.md#5-constraint-validation)
- **Reference**: [inline_assembly.md](inline_assembly.md#constraint-system)
- **Quick lookup**: [assembly_quick_reference.md](assembly_quick_reference.md#constraint-quick-reference)

### Calling Conventions
- **System V ABI**: [assembly_patterns.md](assembly_patterns.md#1-calling-convention-compatibility)
- **MSVC x64**: [assembly_patterns.md](assembly_patterns.md#1-calling-convention-compatibility)
- **ARM64 AAPCS**: [assembly_patterns.md](assembly_patterns.md#1-calling-convention-compatibility)
- **Quick ref**: [assembly_quick_reference.md](assembly_quick_reference.md#x86-64-registers)

### Memory Operations
- **Barriers**: [assembly_patterns.md](assembly_patterns.md#5-memory-barriers-and-synchronization)
- **Examples**: [../examples/asm_c_interop.vitte](../examples/asm_c_interop.vitte)
- **Testing**: [assembly_testing.md](assembly_testing.md#2-validation-strategies)

### Atomic Operations
- **Pattern**: [assembly_patterns.md](assembly_patterns.md#6-atomic-operations)
- **Example**: [../examples/asm_performance.vitte](../examples/asm_performance.vitte)
- **Testing**: [assembly_testing.md](assembly_testing.md)

### SIMD Operations
- **Inline asm reference**: [inline_assembly.md](inline_assembly.md#operand-types)
- **Example**: [../examples/hybrid_vector_library.vitte](../examples/hybrid_vector_library.vitte)
- **Pattern reference**: [assembly_quick_reference.md](assembly_quick_reference.md#simd-operation)

### Testing
- **Unit tests**: [assembly_testing.md](assembly_testing.md#1-unit-testing-assembly-code)
- **Benchmarking**: [assembly_testing.md](assembly_testing.md#3-benchmarking)
- **Integration**: [assembly_testing.md](assembly_testing.md#6-integration-tests)
- **Build system**: [BUILD_AND_TEST.md](BUILD_AND_TEST.md)

---

## File Organization

```
compiler/
├── docs/
│   ├── ASSEMBLY_README.md              ⭐ Start here
│   ├── assembly_integration.md          Architecture
│   ├── inline_assembly.md               Reference
│   ├── assembly_patterns.md             14 Patterns
│   ├── assembly_testing.md              Testing
│   ├── assembly_quick_reference.md      Cheat sheet
│   ├── BUILD_AND_TEST.md                Build system
│   ├── BUILD_INTEGRATION.md             Integration
│   ├── COMPILER_FILES.md                File inventory
│   ├── README_IMPLEMENTATION.md         Implementation
│   ├── IMPLEMENTATION_STATUS.md         Status
│   ├── IMPLEMENTATION_GUIDE.md          How to extend
│   ├── QUICK_START.md                   First steps
│   ├── README.md                        Overview
│   ├── COMPLETION_REPORT.md             What's done
│   └── DOCUMENTATION_INDEX.md           This file
│
├── examples/
│   ├── inline_asm.vitte                 11 basic examples
│   ├── asm_performance.vitte            10 performance examples
│   ├── asm_c_interop.vitte              8 interop examples
│   ├── hybrid_vector_library.vitte      Complete library
│   └── ... (other language examples)
│
├── include/compiler/
│   ├── inline_asm.h                     Inline assembly API
│   ├── asm_optimizer.h                  Assembly optimizer
│   ├── native_codegen.h                 Native code generation
│   └── ... (other headers)
│
├── src/
│   ├── inline_asm.c                     Inline asm implementation
│   ├── asm_optimizer.c                  Optimizer implementation
│   ├── native_codegen.c                 Codegen implementation
│   └── ... (other implementations)
│
└── tests/
    ├── unit/
    │   ├── test_assembly_ops.c          Assembly tests
    │   └── ... (other tests)
    └── ... (other test categories)
```

---

## Quick Navigation

- **New to Vitte?** → [README.md](README.md) → [QUICK_START.md](QUICK_START.md)
- **New to assembly?** → [ASSEMBLY_README.md](ASSEMBLY_README.md)
- **Want to write assembly?** → [assembly_integration.md](assembly_integration.md) → [inline_assembly.md](inline_assembly.md) → [assembly_patterns.md](assembly_patterns.md)
- **Need an example?** → [examples/](../examples/)
- **Want to optimize code?** → [assembly_patterns.md](assembly_patterns.md) → [../examples/asm_performance.vitte](../examples/asm_performance.vitte)
- **Need to test code?** → [assembly_testing.md](assembly_testing.md)
- **Extending compiler?** → [README_IMPLEMENTATION.md](README_IMPLEMENTATION.md)
- **Building project?** → [BUILD_AND_TEST.md](BUILD_AND_TEST.md)
- **Cheat sheet?** → [assembly_quick_reference.md](assembly_quick_reference.md)

---

**Last Updated**: 2024-01-15  
**Maintainer**: Vitte Compiler Team  
**Coverage**: Core compiler, Assembly support, Build system, Testing
