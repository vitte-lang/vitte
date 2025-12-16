# Assembly-C Integration Guide for Vitte Compiler

## Executive Summary

This guide explains how the Vitte compiler integrates assembly language with C code generation to produce optimal native binaries. The compiler follows a three-layer approach:

1. **Inline Assembly Support** - Embed assembly directly in Vitte code
2. **C Backend Integration** - Generate proper C with inline assembly
3. **Native Code Generation** - Optional native codegen backend

## 1. Compilation Pipeline with Assembly

### Traditional Vitte Pipeline

```
Vitte Source (.muf)
    ↓ [Lexer]
Token Stream
    ↓ [Parser]
Abstract Syntax Tree (AST)
    ↓ [Semantic Analysis]
High-Level IR (HIR)
    ↓ [Lowering]
Intermediate Representation (IR)
    ↓ [Code Generation]
C Code
    ↓ [System C Compiler]
Native Binary
```

### Enhanced Pipeline with Assembly

```
Vitte Source (.muf)
    ↓ [Lexer]
    ↓ [Parser]
    ↓ [Semantic Analysis]
    ↓ [Lowering] ← Detects assembly blocks
    ↓
IR with Assembly Annotations
    ↓ [Backend Router]
    ├─→ [C Backend + Inline ASM]
    │   ├─ Generate wrapper functions
    │   ├─ Emit inline assembly code
    │   └─ Validate constraints
    │
    └─→ [Native Codegen] (Optional)
        ├─ Direct assembly emission
        ├─ Optimization passes
        └─ Link with system libraries
```

## 2. Inline Assembly Processing

### Parse Phase

When the compiler encounters assembly syntax:

```vitte
asm "bsf %1, %0"
    : "=r"(result)
    : "r"(value)
```

The lexer/parser:
1. Recognizes `asm` keyword
2. Extracts assembly template string
3. Parses constraint expressions
4. Validates operand compatibility

### Semantic Analysis Phase

The semantic analyzer:
1. Type-checks all operands
2. Validates constraints match operand types
3. Checks operand count matches placeholders
4. Verifies register names (x86-64, ARM64, etc.)
5. Emits warnings for suboptimal constraints

### IR Generation Phase

The IR generator:
1. Creates inline_asm_t structure
2. Stores assembly template
3. Records operand references
4. Marks memory dependencies

### Code Generation Phase

#### For C Backend:

```c
// Generated from inline assembly
static inline unsigned long bsf_impl(unsigned long value) {
    unsigned long result;
    asm("bsf %1, %0"
        : "=r"(result)
        : "r"(value)
    );
    return result;
}
```

#### For Native Backend:

```asm
bsf_impl:
    mov %rdi, %rax          ; Move input to accumulator
    bsf %rsi, %rax          ; Bit scan forward
    ret
```

## 3. C Backend with Inline Assembly

### Integration Points

The C backend needs to:

1. **Recognize Assembly Blocks**
   ```c
   if (is_inline_asm_node(ir_node)) {
       emit_inline_asm_c(codegen, ir_node);
   }
   ```

2. **Emit GCC/Clang Syntax**
   ```c
   printf("asm volatile(\"%s\"\n", template);
   printf("    : %s\n", output_constraints);
   printf("    : %s\n", input_constraints);
   printf("    : %s\n", clobbers);
   ```

3. **Emit MSVC Syntax**
   ```c
   printf("__asm {\n");
   printf("    %s\n", template);  // Note: MSVC syntax differs
   printf("}\n");
   ```

4. **Handle Platform Differences**
   ```c
   #if defined(_MSC_VER)
       // MSVC __asm syntax
   #elif defined(__GNUC__)
       // GCC/Clang asm syntax
   #else
       // Fallback C implementation
   #endif
   ```

### Example Generated C Code

From Vitte:
```vitte
fn bit_scan_forward(x: u64) -> u32 {
    let result: u32
    asm "bsf %1, %0"
        : "=r"(result)
        : "r"(x)
    return result
}
```

Generated C (GCC):
```c
uint32_t bit_scan_forward(uint64_t x) {
    uint32_t result;
    asm("bsf %1, %0"
        : "=r" (result)
        : "r" (x)
    );
    return result;
}
```

Generated C (MSVC):
```c
uint32_t bit_scan_forward(uint64_t x) {
    uint32_t result;
    __asm {
        mov eax, x
        bsf eax, eax
        mov result, eax
    }
    return result;
}
```

## 4. Constraint Validation

### Valid Constraint Patterns

The compiler validates:

```
Output Constraints:
  =r   ✓ Writable register
  +r   ✓ Read-write register
  =m   ✓ Writable memory
  =g   ✓ General register or memory
  
Input Constraints:
  r    ✓ Register
  m    ✓ Memory
  i    ✓ Immediate
  0-9  ✓ Same as output constraint N
```

### Invalid Constraints

```vitte
// ERROR: Invalid constraint 'Q'
asm "mov %1, %0"
    : "=Q"(result)  // ← Invalid
    : "r"(x)

// ERROR: Operand count mismatch
asm "add %1, %0"
    : "=r"(result)
    : "r"(a), "r"(b)  // ← 2 inputs but template expects different
```

### Compiler Warnings

```
warning: operand constraint 'f' expects floating-point, got integer
warning: register 'rk' not available on target architecture
warning: memory operand may clobber live values
```

## 5. Operand Compatibility Checking

### Type-to-Constraint Matching

```
Type         → Valid Constraints
─────────────────────────────
i32, u32     → r, m, i, =, +
i64, u64     → r, m, i, =, +
f32, f64     → x (SIMD), f (x87)
*T (pointer) → r, m
[T; N]       → m
struct       → m
```

### Type Validation

```vitte
asm "movsd %1, %0"
    : "=x"(f64_var)     // ✓ f64 matches x (XMM register)
    : "x"(f64_input)

asm "mov %1, %0"
    : "=r"(f64_var)     // ✗ ERROR: f64 doesn't match r
    : "r"(f64_input)
```

## 6. Memory and Register Safety

### Memory Dependency Tracking

```vitte
// Loads from memory - compiler tracks dependency
asm volatile "movl (%1), %0"
    : "=r"(value)
    : "r"(ptr)
    : "memory"  // ← Tells compiler memory is accessed
```

### Register Preservation

```vitte
// Clobber list informs register allocator
asm "lock; incl %0"
    : "+m"(*counter)
    : : "memory"  // ← Memory clobbered by lock prefix
```

### Early Clobbering

```vitte
// & modifier indicates register used before inputs consumed
asm "div %1"
    : "=&a"(quotient)  // ← Early clobbered
    : "0"(dividend), "r"(divisor)
```

## 7. Optimization Interaction

### How Optimizers Handle Assembly

```
1. Peephole Optimization
   - Cannot optimize across asm boundaries
   - May optimize before/after assembly blocks

2. Register Allocation
   - Respects constraint requirements
   - Allocates clobbered registers for other use
   - May insert save/restore for preserved registers

3. Constant Folding
   - Cannot fold through inline assembly
   - Pre-computes where possible

4. Dead Code Elimination
   - Cannot eliminate asm with side effects (volatile)
   - May eliminate if output unused
```

### Example: Optimization Interaction

```vitte
// Original code
fn optimized_add(x: i32) -> i32 {
    let temp = x + 5
    let result: i32
    asm "add %1, %0"
        : "=r"(result)
        : "r"(temp)
    return result
}

// After constant folding (pre-optimization)
fn optimized_add(x: i32) -> i32 {
    // No change - can't fold through asm

// After register allocation
fn optimized_add(x: i32) -> i32 {
    let result: i32
    asm "add %1, %0"
        : "=r"(result)
        : "r"(x + 5)    // May be computed inline
    return result
}
```

## 8. Error Handling and Diagnostics

### Compilation Errors

```
error: undefined symbol 'result' in asm operand 0
  asm "bsf %1, %0"
      : "=r"(undefined_var)
           ^^^^^^^^^^^^^^

error: incompatible operand type for constraint 'r'
  asm "mov %1, %0"
      : "=r"(f64_var)
           ^^^^^^^^^
  note: 'r' constraint expects integer, got f64
```

### Warnings

```
warning: unused asm operand (output 1)
  asm "add %1, %0"
      : "=r"(unused),    ← Declared but not returned
        "=r"(used)
        : "r"(x)

warning: volatile asm may have unintended side effects
  asm volatile "cpuid"
      : : : "rbx", "rcx", "rdx"
      
note: Mark as volatile only if side effects are intentional
```

## 9. Cross-Platform Considerations

### Platform Detection

```vitte
#if defined(__x86_64__)
    // x86-64 specific assembly
    asm "bsf %1, %0"
#elif defined(__aarch64__)
    // ARM64 specific assembly
    asm "clz %0, %1"
#else
    // Fallback C implementation
    result = c_implementation(x)
#endif
```

### Portable Abstractions

```vitte
// Create portable abstraction
#if defined(__x86_64__)
    fn clz(x: u64) -> u32 {
        let result: u32
        asm "lzcnt %1, %0" : "=r"(result) : "r"(x)
        return result
    }
#elif defined(__aarch64__)
    fn clz(x: u64) -> u32 {
        let result: u32
        asm "clz %0, %1" : "=r"(result) : "r"(x)
        return result
    }
#else
    fn clz(x: u64) -> u32 {
        // C fallback
        return __builtin_clzll(x)
    }
#endif
```

## 10. Testing Integration

### Automated Testing

```bash
# Build with assembly support
vittec --enable-inline-asm program.muf -o program.c

# Compile generated C
gcc -O2 -march=native program.c -o program

# Verify correctness
./program
```

### Validation Steps

1. **Syntactic Validation**
   - Assembly template is valid syntax
   - Constraint syntax is correct
   - Operand counts match

2. **Semantic Validation**
   - Types match constraints
   - Register names are valid
   - Memory operands are addressable

3. **Runtime Validation**
   - Program executes correctly
   - Results match expected values
   - No segmentation faults

4. **Performance Validation**
   - Assembly implementation faster
   - No optimization cliffs
   - Proper code alignment

## 11. Best Practices Summary

### DO ✓

- Keep assembly blocks small and focused
- Use volatile only for side effects
- Provide C fallback implementations
- Test on all target platforms
- Document assembly intentions
- Use standard calling conventions
- Profile before and after

### DON'T ✗

- Inline assembly for everything
- Ignore calling conventions
- Mix different assembly styles
- Assume register availability
- Forget clobber lists
- Optimize prematurely
- Skip testing on edge cases

## 12. Tools and Utilities

### Compiler Flags

```bash
# Enable assembly support
vittec --enable-inline-asm source.muf

# Generate assembly output
vittec --emit-asm source.muf > output.asm

# Check assembly validity
vittec --validate-asm source.muf

# Optimize assembly
vittec --optimize-asm=2 source.muf

# Target specific architecture
vittec --target=x86-64 source.muf
vittec --target=aarch64 source.muf
```

### Analysis Tools

```bash
# Generate C with inline assembly
vittec source.muf -o source.c

# View generated assembly
gcc -S -O2 source.c
objdump -d a.out | grep -A20 "function_name"

# Performance profiling
perf record ./a.out
perf report

# Coverage analysis
gcov source.c
```

## Summary

The Vitte compiler's assembly-C integration provides:

| Feature | Benefit |
|---------|---------|
| Inline assembly | Performance for critical paths |
| GCC/MSVC support | Wide platform compatibility |
| Type checking | Early error detection |
| Constraint validation | Correctness verification |
| C generation | Portability and maintainability |
| Fallback mechanism | Works everywhere |

For detailed examples, see:
- [inline_assembly.md](inline_assembly.md) - Syntax and constraints
- [assembly_patterns.md](assembly_patterns.md) - Common patterns
- [assembly_testing.md](assembly_testing.md) - Testing strategies
- [examples/](../examples/) - Full working examples

---

**Version**: 1.0  
**Last Updated**: 2024-01-15  
**Maintainer**: Vitte Compiler Team
