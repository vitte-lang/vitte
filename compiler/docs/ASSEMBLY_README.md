# Assembly Language Support in Vitte Compiler

## Overview

The Vitte compiler provides comprehensive support for assembly language integration with C code generation. This documentation set covers everything needed to write, optimize, and test high-performance code using inline assembly.

## Documentation Structure

### 1. **[assembly_integration.md](assembly_integration.md)** - Core Architecture
**Target Audience**: All users  
**Content**:
- Compilation pipeline with assembly support
- How assembly is processed through each compiler phase
- Integration with C backend
- Cross-platform considerations
- Error handling and diagnostics

**Key Sections**:
- Compilation Pipeline (traditional vs. enhanced)
- Inline Assembly Processing (parse → IR → codegen)
- C Backend Integration (GCC vs MSVC)
- Constraint Validation
- Optimization Interaction
- Platform Detection and Portable Abstractions

**When to Read**: Start here to understand how assembly fits into the overall compiler architecture.

### 2. **[inline_assembly.md](inline_assembly.md)** - Syntax and Constraints
**Target Audience**: Developers writing assembly code  
**Content**:
- Complete inline assembly syntax reference
- Constraint system documentation
- Operand types and modifiers
- Architecture-specific instructions
- Best practices for clean, maintainable code

**Key Sections**:
- Inline Assembly Syntax (GCC-style)
- x86-64 Architecture Support (40+ instructions)
- ARM64 (aarch64) Support
- WebAssembly (limited support)
- Constraint Reference Table
- Operand Size Suffixes
- Clobber Lists

**When to Read**: Use as a reference while writing assembly code. Bookmark the constraint table.

### 3. **[assembly_patterns.md](assembly_patterns.md)** - Practical Patterns
**Target Audience**: Developers solving specific problems  
**Content**:
- 14 concrete assembly patterns with working examples
- Calling convention details (System V, Microsoft x64, ARM64)
- C function calls from assembly
- Global variable access
- Struct field manipulation
- Memory barriers and synchronization
- Atomic operations
- Variable arguments (varargs) handling
- Stack usage and frame management

**Key Patterns**:
- Pattern 1: C Function Calls
- Pattern 2: Global Variable Access
- Pattern 3: Struct Field Access
- Pattern 4: Memory Barriers
- Pattern 5: Atomic Operations
- Pattern 6: Varargs from Assembly
- Pattern 7: Stack Management
- Pattern 8-14: Advanced techniques

**When to Read**: Look up patterns matching your use case. Each pattern includes working code.

### 4. **[assembly_testing.md](assembly_testing.md)** - Testing and Verification
**Target Audience**: QA, performance engineers, developers testing code  
**Content**:
- Unit testing assembly functions
- CTest integration
- Property-based testing
- Benchmarking strategies
- Architecture-specific testing
- Integration tests
- Performance regression detection
- Test documentation

**Key Sections**:
- Unit Testing Framework
- C Implementation Comparison
- Property-Based Testing
- Edge Case Testing
- Benchmarking (simple timing, perf, VTune)
- Architecture-Specific Tests (x86-64, ARM64)
- Regression Test Suite
- Performance Verification Checklist

**When to Read**: Before shipping code with assembly. Provides templates for testing.

## Example Files

The `examples/` directory contains complete, working examples:

### **[inline_asm.vitte](../examples/inline_asm.vitte)** - Basic Inline Assembly
11 fundamental examples:
```
1. cpu_count() - CPUID instruction
2. bit_scan_forward() - BSF instruction
3. memory_barrier() - Memory fence (mfence)
4. atomic_increment() - Lock prefix with increment
5. simd_add_4() - SIMD vector addition
6. read_timestamp_counter() - RDTSC
7. read_barrier() - Load fence (lfence)
8. cpu_pause() - Spinlock pause instruction
9. get_return_address() - Frame introspection
...and more
```

**Use for**: Learning basic syntax, understanding instruction usage

### **[asm_performance.vitte](../examples/asm_performance.vitte)** - Performance Optimization
10 performance-critical examples:
```
1. mul_fast() - Fast multiplication
2. clz_64() - Count leading zeros
3. rol() - Rotate left operation
4. ror() - Rotate right operation
5. popcount() - Population count (POPCNT)
6. compare_exchange() - Atomic CAS operation
7. memcpy_fast() - Fast memory copy
8. memset_fast() - Fast memory fill
...and more
```

**Use for**: Performance-critical code, comparing asm vs C

### **[asm_c_interop.vitte](../examples/asm_c_interop.vitte)** - Assembly-C Boundaries
8 interoperability examples:
```
1. strlen_asm() - Call C function from assembly
2. get_errno() - Access global variables
3. fast_index_of() - String search with asm loops
4. point_distance_squared() - Struct field access
5. printf_fast() - Varargs function calls
...and more
```

**Use for**: C-Assembly boundary patterns, integration examples

### **[hybrid_vector_library.vitte](../examples/hybrid_vector_library.vitte)** - Complex Example
Complete vector library combining:
- Vector operations (create, get, set)
- Dot product with SIMD acceleration
- Vector scaling with broadcast
- Vector addition with parallel ops
- Matrix multiplication
- Cache-aware transposition
- Benchmarking infrastructure

**Use for**: Understanding architecture of complex hybrid code

## Quick Start

### For First-Time Users

1. **Read**: [assembly_integration.md](assembly_integration.md) (15 min)
   - Understand how assembly fits in the pipeline
   
2. **Study**: One example from `examples/inline_asm.vitte` (10 min)
   - See working code
   
3. **Reference**: [inline_assembly.md](inline_assembly.md) (30 min)
   - Look up constraint system details
   
4. **Practice**: Write your first assembly function
   - Try a simple bit operation
   - Test with C comparison

### For Performance Optimization

1. **Review**: [assembly_patterns.md](assembly_patterns.md) (30 min)
   - Find relevant pattern for your problem
   
2. **Study**: Matching example in `examples/asm_performance.vitte`
   - See similar implementation
   
3. **Implement**: Write assembly version
   - Keep C fallback
   - Measure performance
   
4. **Test**: Use [assembly_testing.md](assembly_testing.md)
   - Create benchmarks
   - Verify correctness

### For Integration Work

1. **Review**: [assembly_patterns.md](assembly_patterns.md) - Patterns 1-3, 6-7
   - C function calls
   - Global access
   - Stack management
   
2. **Study**: [asm_c_interop.vitte](../examples/asm_c_interop.vitte)
   - Real examples of boundaries
   
3. **Reference**: [assembly_integration.md](assembly_integration.md) - "Cross-Platform Considerations"
   - Handle platform differences
   
4. **Test**: Use [assembly_testing.md](assembly_testing.md) - "Integration Tests"

## Reference Tables

### Supported x86-64 Instructions

**Bit Operations**:
- `bsf` - Bit scan forward
- `bsr` - Bit scan reverse
- `lzcnt` - Leading zero count
- `popcnt` - Population count
- `rol`, `ror` - Rotate left/right
- `shl`, `shr`, `sal`, `sar` - Shift operations

**Atomic Operations**:
- `lock` prefix - Lock bus
- `cmpxchg` - Compare and exchange
- `xchg` - Exchange
- `add`, `sub` with `lock` - Atomic arithmetic

**SIMD Instructions**:
- `movaps`, `movups` - Move packed single
- `addps`, `subps`, `mulps` - Arithmetic
- `maxps`, `minps` - Min/max
- And 100+ more SSE/AVX instructions

**Memory Operations**:
- `mov`, `movz`, `movsx` - Move with/without extension
- `lea` - Load effective address
- `push`, `pop` - Stack operations

### Supported ARM64 Instructions

**Bit Operations**:
- `clz` - Count leading zeros
- `cls` - Count leading sign bits
- `rbit` - Reverse bits
- `ctz` - Count trailing zeros
- Bitfield extract/insert

**Atomic Operations**:
- `ldaxr`, `stlxr` - Load/store acquire/release
- `ldadd` - Atomic add

**SIMD Instructions**:
- `fadd`, `fsub`, `fmul` - Floating point
- `add`, `sub` - Vector add/sub
- And 200+ more NEON instructions

## Common Use Cases

| Use Case | Documentation | Example |
|----------|---|---|
| **Bit manipulation** | [inline_assembly.md](inline_assembly.md#operand-types) | [inline_asm.vitte](../examples/inline_asm.vitte) - bsf, popcount |
| **Atomic operations** | [assembly_patterns.md](assembly_patterns.md#11-operand-constraint-patterns) | [asm_performance.vitte](../examples/asm_performance.vitte) - CAS |
| **SIMD optimization** | [inline_assembly.md](inline_assembly.md#operand-types) | [hybrid_vector_library.vitte](../examples/hybrid_vector_library.vitte) |
| **System calls** | [assembly_patterns.md](assembly_patterns.md#13-clobber-lists) | sys_* examples |
| **C interop** | [assembly_patterns.md](assembly_patterns.md#2-inline-assembly---c-function-call) | [asm_c_interop.vitte](../examples/asm_c_interop.vitte) |
| **Performance critical** | [assembly_testing.md](assembly_testing.md#3-benchmarking) | [asm_performance.vitte](../examples/asm_performance.vitte) |

## Troubleshooting

### "Invalid constraint" Error

**Check**:
1. Constraint code is valid (r, m, i, =, +, &)
2. Constraint matches operand type
3. Operand count matches template

**Reference**: [inline_assembly.md](inline_assembly.md#constraint-system)

### "Operand count mismatch" Error

**Check**:
1. Each `%N` in template has corresponding operand
2. Output constraints come first
3. Input constraints follow outputs

**Reference**: [inline_assembly.md](inline_assembly.md#examples)

### Performance Regression

**Check**:
1. Compiler generated expected assembly
2. Benchmark includes warmup iterations
3. CPU performance scaling is stable
4. Clobber list isn't too aggressive

**Reference**: [assembly_testing.md](assembly_testing.md#9-performance-regression-detection)

### Portability Issues

**Check**:
1. Use `#if defined(__x86_64__)` guards
2. Provide C fallback
3. Test on all target platforms
4. Verify instruction availability

**Reference**: [assembly_integration.md](assembly_integration.md#9-cross-platform-considerations)

## Compiler Flags

```bash
# Enable assembly support
vittec --enable-inline-asm source.muf

# Generate assembly output
vittec --emit-asm source.muf

# Validate inline assembly
vittec --validate-asm source.muf

# Optimize assembly
vittec --optimize-asm=2 source.muf

# Target specific architecture
vittec --target=x86-64 source.muf
vittec --target=aarch64 source.muf

# Combined usage
vittec --enable-inline-asm --validate-asm --target=x86-64 program.muf -o program.c
```

## File Organization

```
compiler/
├── include/compiler/
│   ├── inline_asm.h           ← Header: Inline assembly API
│   ├── asm_optimizer.h        ← Header: Assembly optimizer
│   ├── native_codegen.h       ← Header: Native code generator
│   └── ...
├── src/
│   ├── inline_asm.c           ← Implementation: GCC/MSVC support
│   ├── asm_optimizer.c        ← Implementation: Optimization passes
│   ├── native_codegen.c       ← Implementation: Native code generation
│   └── ...
├── docs/
│   ├── ASSEMBLY_README.md     ← This file
│   ├── assembly_integration.md ← Architecture and integration
│   ├── inline_assembly.md     ← Syntax and constraints (REFERENCE)
│   ├── assembly_patterns.md   ← 14 practical patterns
│   └── assembly_testing.md    ← Testing and verification
└── examples/
    ├── inline_asm.vitte       ← 11 basic examples
    ├── asm_performance.vitte  ← 10 performance examples
    ├── asm_c_interop.vitte    ← 8 interop examples
    └── hybrid_vector_library.vitte ← Complete vector library
```

## Next Steps

### If You Want to...

- **Understand the architecture**: Read [assembly_integration.md](assembly_integration.md)
- **Write assembly code**: Read [inline_assembly.md](inline_assembly.md) + study examples
- **Solve a specific problem**: Check [assembly_patterns.md](assembly_patterns.md)
- **Test assembly code**: Read [assembly_testing.md](assembly_testing.md)
- **Optimize performance**: Study [asm_performance.vitte](../examples/asm_performance.vitte)
- **Integrate with C code**: Study [asm_c_interop.vitte](../examples/asm_c_interop.vitte)
- **Build a library**: Study [hybrid_vector_library.vitte](../examples/hybrid_vector_library.vitte)

## Support Resources

- **GCC Inline Assembly**: https://gcc.gnu.org/onlinedocs/gcc/Using-Inline-Assembly-with-C-Code.html
- **x86-64 ABI**: https://www.uclibc.org/docs/psABI-x86_64.pdf
- **ARM64 ABI**: https://github.com/ARM-software/abi-aa
- **Intel 64 ISA**: https://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-manual-combined-volumes.html
- **ARM v8 ISA**: https://developer.arm.com/documentation/ddi0487/latest

## Version Information

- **Assembly Support**: 1.0
- **Documentation**: 1.0
- **Last Updated**: 2024-01-15
- **Maintainer**: Vitte Compiler Team

---

**Start with**: [assembly_integration.md](assembly_integration.md) for architecture overview  
**Reference**: [inline_assembly.md](inline_assembly.md) for constraint details  
**Apply**: [assembly_patterns.md](assembly_patterns.md) for your specific use case  
**Verify**: [assembly_testing.md](assembly_testing.md) before shipping
