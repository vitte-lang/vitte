# Assembly-C Interoperability Patterns

## Overview

This document provides concrete patterns for using assembly code in conjunction with C code in the Vitte compiler. These patterns address common scenarios when you need the efficiency of assembly combined with C's flexibility.

## 1. Calling Convention Compatibility

### x86-64 System V ABI (Linux, BSD, macOS)

**Register Usage:**
- **Arguments**: rdi, rsi, rdx, rcx, r8, r9 (in order)
- **Return**: rax (integer), rdx:rax (128-bit), rflags
- **Preserved**: rbx, rsp, rbp, r12-r15
- **Clobbered**: rax, rcx, rdx, rsi, rdi, r8-r11

**Stack Alignment:**
- Requires 16-byte alignment before `call`
- Return address pushed automatically

### x86-64 Microsoft x64 ABI (Windows)

**Register Usage:**
- **Arguments**: rcx, rdx, r8, r9 (in order)
- **Return**: rax (integer), rdx:rax (128-bit)
- **Preserved**: rbx, rsp, rbp, rsi, rdi, r12-r15
- **Clobbered**: rax, rcx, rdx, r8-r11

**Stack Alignment:**
- 16-byte alignment before `call`
- Shadow space (32 bytes) required for arguments

### ARM64 (aarch64)

**Register Usage:**
- **Arguments**: x0-x7 (in order)
- **Return**: x0 (integer), x0-x1 (128-bit)
- **Preserved**: x19-x29, sp
- **Clobbered**: x0-x18, x30 (lr)

**Stack Alignment:**
- Requires 16-byte alignment at function entry

## 2. Inline Assembly - C Function Call

Call C functions from assembly:

```vitte
// C function
fn strlen(s: *const u8) -> usize

// Assembly calling C function
fn strlen_fast(s: *const u8) -> usize {
    let len: usize
    
    asm "call strlen"      // C calling convention preserved
        : "=a"(len)        // Return value in rax
        : "D"(s)           // First argument in rdi (System V)
        : "rcx", "rdx", "rsi", "r8", "r9", "r10", "r11"  // Clobbered
    
    return len
}
```

## 3. Global Variable Access from Assembly

Access module-level variables:

```vitte
// Global variable
var global_counter: i32 = 0

// Modify from assembly
fn increment_global() {
    asm volatile "lock; incl global_counter(%rip)"
        : : : "memory"
}

// Read from assembly
fn get_global() -> i32 {
    let value: i32
    
    asm "movl global_counter(%rip), %0"
        : "=r"(value)
    
    return value
}
```

## 4. Struct Field Access

Access and modify struct fields in assembly:

```vitte
struct Point {
    x: i32,  // offset 0
    y: i32,  // offset 4
}

fn point_distance_squared(p: *Point) -> i32 {
    let result: i32
    
    asm (
        "movl (%1), %%eax; imul %%eax, %%eax; "  // x*x -> rax
        "movl 4(%1), %%edx; imul %%edx, %%edx; " // y*y -> rdx
        "addl %%edx, %%eax"                       // sum in eax
        : "=a"(result)
        : "r"(p)
        : "rdx"
    )
    
    return result
}
```

## 5. Memory Barriers and Synchronization

### Full Memory Barrier

```vitte
fn full_barrier() {
    asm volatile "mfence"
}
```

### Load Barrier (Acquire)

```vitte
fn load_acquire(ptr: *const i32) -> i32 {
    let value: i32
    
    asm volatile "mov (%1), %0; lfence"
        : "=r"(value)
        : "r"(ptr)
        : "memory"
    
    return value
}
```

### Store Barrier (Release)

```vitte
fn store_release(ptr: *mut i32, value: i32) {
    asm volatile "mfence; mov %0, (%1)"
        : : "r"(value), "r"(ptr)
        : "memory"
}
```

## 6. Atomic Operations

### Atomic Compare-and-Swap

```vitte
fn atomic_cas(ptr: *mut i32, expected: i32, new_val: i32) -> bool {
    let old: i32
    let success: bool
    
    asm volatile (
        "lock; cmpxchg %2, %0; "
        "sete %3"
        : "+m"(*ptr), "=a"(old), "=r"(new_val), "=q"(success)
        : "1"(expected)
        : "memory"
    )
    
    return success
}
```

### Atomic Increment

```vitte
fn atomic_inc(ptr: *mut i32) -> i32 {
    let result: i32
    
    asm volatile "lock; incl %0; mov %0, %1"
        : "+m"(*ptr), "=r"(result)
        : : "memory"
    
    return result
}
```

## 7. Variable Arguments (varargs) from Assembly

Call variadic functions from assembly:

```vitte
// C function (variadic)
fn printf(fmt: *const u8, ...) -> i32

// Call from assembly
fn print_number(n: i32) -> i32 {
    let result: i32
    
    asm (
        "lea fmt(%rip), %rdi; "              // Format string
        "mov %1, %rsi; "                     // First argument
        "xor %eax, %eax; "                   // Clear rax for FP args
        "call printf; "
        : "=a"(result)
        : "r"(n)
        : "rcx", "rdx", "rsi", "r8", "r9", "r10", "r11"
    )
    
    return result
}

// Format string
const fmt: *const u8 = "Number: %d\n"
```

## 8. Stack Usage and Function Prologue/Epilogue

```vitte
fn use_stack(x: i32) -> i32 {
    let local1: i32
    let local2: i32
    
    asm (
        "push %rbp; mov %rsp, %rbp; "       // Prologue
        "sub $16, %rsp; "                    // Allocate locals
        
        "mov %1, -8(%rbp); "                 // Store x
        "mov -8(%rbp), %0; add $5, %0; "    // Load, modify, store
        
        "add $16, %rsp; pop %rbp; ret"       // Epilogue
        : "=a"(local1)
        : "r"(x)
    )
    
    return local1
}
```

## 9. Early Clobber Pattern

Use `&` for registers modified before all inputs are consumed:

```vitte
fn divide(dividend: i32, divisor: i32) -> (i32, i32) {
    let quotient: i32
    let remainder: i32
    
    asm (
        "mov %2, %%eax; "
        "cltd; "               // Sign extend eax -> edx:eax
        "idiv %3"
        : "=&a"(quotient), "=d"(remainder)
        : "g"(dividend), "r"(divisor)
        : "flags"
    )
    
    return (quotient, remainder)
}
```

## 10. Volatile Operations

Use `volatile` when assembly has observable side effects:

```vitte
// I/O Port Access
fn out_port(port: u16, value: u8) {
    asm volatile "outb %0, %1"
        : : "a"(value), "Nd"(port)
        : "memory"  // Memory visible after I/O
}

fn in_port(port: u16) -> u8 {
    let value: u8
    
    asm volatile "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
        : "memory"
    
    return value
}
```

## 11. Conditional Assembly Compilation

Choose architecture-specific code at compile time:

```vitte
fn best_popcount(x: u64) -> u32 {
    let count: u32
    
    #if defined(__x86_64__)
        // Use POPCNT instruction if available
        asm "popcnt %1, %0"
            : "=r"(count)
            : "r"(x)
    #elif defined(__aarch64__)
        // Use NEON or FP popcount
        asm "fmov d0, %1; cnt v0.8b, v0.8b"
            : "=r"(count)
            : "r"(x)
    #else
        // Fallback C implementation
        count = c_popcount(x)
    #endif
    
    return count
}
```

## 12. Operand Constraint Patterns

### Input-Output (Read-Modify-Write)

```vitte
fn increment(x: i32) -> i32 {
    let result: i32 = x
    
    asm "inc %0"
        : "+r"(result)
    
    return result
}
```

### Early Clobbering Output

```vitte
fn complex_op(x: i32, y: i32) -> (i32, i32) {
    let a: i32
    let b: i32
    
    asm "movl %2, %0; addl %3, %0; movl %2, %1"
        : "=&r"(a), "=r"(b)      // '&' = early clobbered
        : "r"(x), "r"(y)
    
    return (a, b)
}
```

### Memory and Register Mix

```vitte
fn mixed_operands(arr: *const i32, x: i32) -> i32 {
    let result: i32
    
    asm "add (%1), %0"
        : "=r"(result)
        : "r"(arr), "0"(x)       // "0" reuses constraint 0
    
    return result
}
```

## 13. Clobber Lists

Inform compiler about used/modified registers:

```vitte
fn syscall(syscall_num: i32) -> i32 {
    let result: i32
    
    asm volatile "syscall"
        : "=a"(result)
        : "0"(syscall_num)
        : "rcx", "r11"           // Modified by syscall
        : "memory"               // Memory is modified
    
    return result
}
```

## 14. Performance Patterns

### Loop Unrolling in Assembly

```vitte
fn sum_fast(arr: *const i32, len: usize) -> i32 {
    let sum: i32 = 0
    
    asm volatile (
        "xor %%eax, %%eax; "                    // sum = 0
        "mov %1, %%rcx; "                       // loop counter
        "test %%rcx, %%rcx; jz 1f; "           // early exit
        "0: add (%1, %%rdx, 4), %%eax; "       // unroll x4
        "add 4(%1, %%rdx, 4), %%eax; "
        "add 8(%1, %%rdx, 4), %%eax; "
        "add 12(%1, %%rdx, 4), %%eax; "
        "add $4, %%rdx; "
        "cmp %%rdx, %%rcx; jg 0b; "
        "1:"
        : "=a"(sum)
        : "r"(arr), "d"(0), "c"(len)
        : "memory"
    )
    
    return sum
}
```

## Best Practices for Assembly-C Code

1. **Minimize Assembly Blocks**
   - Keep assembly focused on hot paths
   - Use C for everything else

2. **Document Register Usage**
   ```vitte
   // Using rax for result, rdi for input
   asm "bsf %1, %0"
       : "=a"(result)
       : "D"(input)
   ```

3. **Preserve ABI Compliance**
   - Follow calling convention
   - Preserve non-clobbered registers
   - Maintain stack alignment

4. **Use Macros for Portability**
   ```vitte
   #define MEMORY_BARRIER() asm volatile("mfence")
   #define CAS(ptr, exp, new) atomic_cas(ptr, exp, new)
   ```

5. **Test on Target Platform**
   - Different ABIs exist
   - Different instruction sets available
   - Always have C fallback

## Common Issues and Solutions

| Issue | Solution |
|-------|----------|
| Register spilling | Reduce constraint count, use memory |
| ABI mismatch | Check target ABI, use `__attribute__((ms_abi))` |
| Wrong operand size | Use proper suffix (b, w, l, q) |
| Memory ordering | Add explicit barriers, use volatile |
| Performance regression | Profile generated code, check asm output |

## Verification Checklist

- [ ] Assembly compiles without warnings
- [ ] Generated C output is readable
- [ ] All inputs/outputs specified
- [ ] Clobber list complete
- [ ] Tested on target platform
- [ ] C fallback available
- [ ] Performance improvement verified
- [ ] Documentation updated

---

See [inline_assembly.md](inline_assembly.md) for detailed constraint reference and [examples/](../examples/) for more examples.
