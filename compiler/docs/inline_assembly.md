# Assembly-C Integration Guide

## Overview

The Vitte compiler supports seamless integration of assembly language with C code for performance-critical sections. This guide covers all aspects of using assembly in complement to C.

## Table of Contents

1. [Inline Assembly](#inline-assembly)
2. [Architecture Support](#architecture-support)
3. [Constraint System](#constraint-system)
4. [Operand Types](#operand-types)
5. [Best Practices](#best-practices)
6. [Examples](#examples)
7. [Performance Considerations](#performance-considerations)

## Inline Assembly

### Syntax

Vitte uses GCC-style inline assembly syntax by default:

```vitte
asm [volatile] "assembly code"
    : output constraints
    : input constraints
    : clobbered registers
```

### Volatile Modifier

Mark assembly as volatile when it has side effects:

```vitte
asm volatile "mfence"  // Memory fence - don't optimize away
```

## Architecture Support

### x86-64 (Intel/AMD)

**Supported Instructions:**
- General purpose: mov, add, sub, mul, div, and, or, xor
- Bit operations: bsf, bsr, lzcnt, popcnt, rol, ror
- Memory: mov, lea, push, pop
- Atomic: lock prefix, cmpxchg, xchg
- SIMD: movaps, addps, mulps, etc.
- Barriers: mfence, lfence, sfence

**Registers:**
- General: rax, rbx, rcx, rdx, rsi, rdi, r8-r15
- Argument passing: rdi, rsi, rdx, rcx, r8, r9
- Return value: rax (or rdx:rax for 64-bit)
- Clobbered: rax, rcx, rdx, rsi, rdi, r8-r11

### ARM64 (AArch64)

**Supported Instructions:**
- General: mov, movz, movk, add, sub, mul, smull
- Bit operations: clz, cls, rbit, bitfield ops
- Memory: ldr, str, ldp, stp
- Atomic: ldaxr, stlxr, ldr with acquire, str with release
- SIMD: fadd, fmul, etc.
- Barriers: dmb, isb, dsb

**Registers:**
- General: x0-x30, sp, pc
- Argument passing: x0-x7
- Return value: x0 (or x0-x1 for 64-bit)
- Temporary: x8-x15
- Clobbered: x0-x18

### WebAssembly (Limited)

Inline assembly has limited support in WebAssembly:
- Only memory operations
- No register constraints
- Fallback to C implementation recommended

## Constraint System

### Output Constraints

| Code | Meaning | Notes |
|------|---------|-------|
| `r` | Register | General purpose register |
| `a` | rax/eax | Accumulator register |
| `b` | rbx/ebx | Base register |
| `c` | rcx/ecx | Counter register |
| `d` | rdx/edx | Data register |
| `D` | rdi | Destination index |
| `S` | rsi | Source index |
| `m` | Memory | Memory location |
| `i` | Immediate | Constant value |
| `x` | XMM | SIMD register |
| `f` | FPU | Floating point |

### Modifiers

| Modifier | Meaning |
|----------|---------|
| `=` | Write-only (output) |
| `+` | Read-write (input & output) |
| `&` | Early clobbered |

### Examples

```vitte
// Output to register
: "=r"(var)

// Output to memory
: "=m"(array[i])

// Input from register
: "r"(value)

// Input from memory
: "m"(struct_field)

// Read-write (modify in place)
: "+r"(counter)

// Specific register
: "=a"(result)  // rax on x86-64
```

## Operand Types

### Register Operands

Most efficient - operand stays in CPU register:

```vitte
asm "add %1, %0"
    : "=r"(sum)
    : "r"(a), "0"(b)  // "0" means "same as constraint 0"
```

### Memory Operands

Access memory directly:

```vitte
asm "mov %1, %0"
    : "=m"(dest)
    : "m"(src)
```

### Immediate Operands

Constant values:

```vitte
asm "add $10, %0"
    : "+r"(x)
    : "i"(10)
```

### Early Clobbering

Register modified before all inputs are used:

```vitte
asm "div %1"
    : "=a"(quotient), "=&d"(remainder)
    : "0"(dividend), "1"(0), "r"(divisor)
```

## Best Practices

### 1. Keep Assembly Blocks Small

```vitte
// GOOD - Focused operation
fn bit_scan_forward(x: u64) -> u32 {
    asm "bsf %1, %0"
        : "=r"(result)
        : "r"(x)
    return result
}

// BAD - Too much assembly
fn do_everything(x: u64) {
    asm "bsf %1, %0; add $5, %0; mov %0, (%1)"
        : "=r"(result), "=m"(dest)
        : "r"(x), "r"(ptr)
}
```

### 2. Mark Side Effects

```vitte
// Memory access
asm volatile "mov %0, (%1)"
    : "=r"(value)
    : "r"(ptr)
    : "memory"  // Tell compiler memory is modified

// I/O operations
asm volatile "outb %b0, %w1"
    : : "a"(value), "d"(port)
    : "memory"
```

### 3. Minimize Clobbers

```vitte
// GOOD - Only clobber what's needed
asm "add %1, %0"
    : "+r"(sum)
    : "r"(x)

// BAD - Clobbering unnecessary registers
asm "add %1, %0"
    : "+r"(sum)
    : "r"(x)
    : "rax", "rbx", "rcx", "rdx"
```

### 4. Use Proper Operand Sizes

```vitte
// x86-64 operand size suffixes
// b - byte (8-bit)
// w - word (16-bit)
// l - long (32-bit)
// q - quad (64-bit)

asm "movb %1, %0"     // 8-bit move
    : "=m"(dest)
    : "r"(src)

asm "movq %1, %0"     // 64-bit move
    : "=r"(result)
    : "m"(src)
```

### 5. Test on Target Architecture

```vitte
#if defined(__x86_64__)
    asm "bsf %1, %0"  // x86-64 specific
#elif defined(__aarch64__)
    asm "clz %0, %1"  // ARM64 specific
#else
    fallback_implementation()
#endif
```

## Examples

### Example 1: Fast Bit Operations

```vitte
fn popcount(x: u64) -> u32 {
    let count: u32
    
    asm "popcnt %1, %0"
        : "=r"(count)
        : "r"(x)
    
    return count
}
```

### Example 2: Atomic Operations

```vitte
fn atomic_increment(ptr: *mut i32) -> i32 {
    let result: i32
    
    asm volatile "lock; incl %0; mov %0, %1"
        : "+m"(*ptr), "=r"(result)
        : : "memory"
    
    return result
}
```

### Example 3: SIMD Operations

```vitte
fn add_vectors(a: [f32; 4], b: [f32; 4]) -> [f32; 4] {
    let result: [f32; 4]
    
    asm "addps %1, %0"
        : "=x"(result)
        : "x"(b), "0"(a)
    
    return result
}
```

### Example 4: Memory Operations

```vitte
fn fast_copy(dest: *mut void, src: *const void, size: usize) {
    asm volatile (
        "cld; rep movsq"
        : : "D"(dest), "S"(src), "c"(size >> 3)
        : "memory"
    )
}
```

## Performance Considerations

### When to Use Assembly

✅ **Use assembly when:**
- Hot path profiling shows this code is bottleneck
- Operation not available in C (bit scanning, atomic ops)
- Compiler generates suboptimal code
- Platform-specific optimizations needed

❌ **Don't use assembly when:**
- C code is fast enough
- Compiler can optimize effectively
- Portability is critical
- Code maintenance is difficult

### Performance Tips

1. **Profile First**
   ```bash
   perf record ./program
   perf report
   ```

2. **Minimize Register Pressure**
   - Use only necessary registers
   - Release resources early

3. **Pipeline Efficiency**
   - Avoid data dependencies
   - Use parallel operations
   - Consider instruction latency

4. **Cache Optimization**
   - Keep working set small
   - Prefetch when appropriate
   - Align data structures

5. **Benchmark Assembly vs C**
   ```vitte
   // Time both versions
   fn bench_c() { ... }
   fn bench_asm() { ... }
   ```

## Compiler Integration

### Code Generation

The compiler will:
1. Parse inline assembly syntax
2. Validate constraints and operands
3. Generate proper C output for GCC/MSVC
4. Emit warnings for potential issues
5. Optimize around assembly blocks

### Error Handling

```vitte
asm "invalid_instruction"  // Compiler error
// ^ Not in supported instruction set

asm "add %5, %0"           // Compiler error
// ^ Operand 5 doesn't exist
```

### Fallback Strategy

```vitte
fn optimized_op(x: i32) -> i32 {
    #ifdef HAVE_POPCNT
        asm "popcnt %1, %0"
            : "=r"(result)
            : "r"(x)
        return result
    #else
        return fallback_popcount(x)
    #endif
}
```

## Platform-Specific Examples

### x86-64 Memory Barrier

```vitte
fn memory_barrier() {
    asm volatile "mfence"
}
```

### ARM64 Memory Barrier

```vitte
fn memory_barrier() {
    asm volatile "dmb sy"
}
```

### Load Acquire (ARM64)

```vitte
fn load_acquire(ptr: *const i32) -> i32 {
    let value: i32
    
    asm "ldar %0, %1"
        : "=r"(value)
        : "m"(*ptr)
    
    return value
}
```

## Debugging

### Print Assembly Generated

```bash
# GCC
gcc -S -O2 program.c

# Clang
clang -S -O2 program.c
```

### Check Constraints

```vitte
asm "add %1, %0"
    : "=r"(sum)      // Output: register
    : "r"(a)         // Input: register
    : ;              // No clobbers
```

### Validate with Compiler

```bash
gcc -march=native -O2 -Wall program.c
```

## Summary

| Feature | Support | Platform |
|---------|---------|----------|
| Inline assembly | ✅ Full | x86-64, ARM64 |
| Atomic ops | ✅ Full | x86-64, ARM64 |
| SIMD | ✅ Full | x86-64, ARM64 |
| Barriers | ✅ Full | x86-64, ARM64 |
| C interop | ✅ Full | All |

---

See [examples/](../examples/) for more examples:
- `inline_asm.vitte` - Basic inline assembly
- `asm_performance.vitte` - Performance-critical code
- `asm_c_interop.vitte` - C-Assembly interoperability
