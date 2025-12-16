# Assembly Quick Reference Card

## Inline Assembly Syntax

```vitte
asm [volatile] "template"
    : output_constraints
    : input_constraints
    : clobber_list
```

## Constraint Quick Reference

### Output Constraints
| Code | Register | Notes |
|------|----------|-------|
| `=r` | Any | Write-only register |
| `+r` | Any | Read-write register |
| `=m` | Memory | Write-only memory |
| `=a` | RAX/EAX | Accumulator |
| `=d` | RDX/EDX | Data register |
| `=x` | XMM | SIMD register |

### Input Constraints
| Code | Meaning | Usage |
|------|---------|-------|
| `r` | Register | Most efficient |
| `m` | Memory | Direct memory access |
| `i` | Immediate | Constant value |
| `0-9` | Same as output N | Reuse constraint |

### Modifiers
| Symbol | Meaning |
|--------|---------|
| `=` | Write-only (output) |
| `+` | Read-write (input & output) |
| `&` | Early clobbered |
| `,` | Separator |

## x86-64 Registers

### General Purpose
| Register | 64-bit | 32-bit | 16-bit | 8-bit |
|----------|--------|--------|--------|-------|
| Accumulator | RAX | EAX | AX | AL |
| Base | RBX | EBX | BX | BL |
| Counter | RCX | ECX | CX | CL |
| Data | RDX | EDX | DX | DL |
| Source Index | RSI | ESI | SI | SIL |
| Dest Index | RDI | EDI | DI | DIL |
| Base Pointer | RBP | EBP | BP | BPL |
| Stack Pointer | RSP | ESP | SP | SPL |
| r8-r15 | r8-r15 | r8d-r15d | r8w-r15w | r8b-r15b |

### Argument Passing (System V AMD64 ABI)
- 1st arg: RDI
- 2nd arg: RSI
- 3rd arg: RDX
- 4th arg: RCX
- 5th arg: R8
- 6th arg: R9

### Return Values
- Integer: RAX (or RDX:RAX for 128-bit)
- Floating: XMM0 (or XMM0-XMM1 for 128-bit)

### Clobbered by Caller
RCX, RDX, RSI, RDI, R8-R11, RAX

### Preserved by Callee
RBX, RSP, RBP, R12-R15

## x86-64 Instruction Reference

### Bit Operations
```asm
bsf  dst, src     # Bit scan forward → position of lowest set bit
bsr  dst, src     # Bit scan reverse → position of highest set bit
lzcnt dst, src    # Leading zero count
popcnt dst, src   # Population count (Nehalem+)
```

### Atomic Operations
```asm
lock incl mem     # Atomic increment
lock decl mem     # Atomic decrement
lock addl im, mem # Atomic add
cmpxchg new, mem  # Compare and exchange (lock prefix for atomic)
xchg reg, mem     # Exchange (implicitly atomic)
```

### SIMD (128-bit XMM)
```asm
movups xmm, mem   # Move unaligned single precision
movaps xmm, mem   # Move aligned single precision
addps xmm, xmm    # Add packed single precision
mulps xmm, xmm    # Multiply packed single precision
```

### Memory Operations
```asm
mov dst, src      # Move
lea dst, [addr]   # Load effective address
push reg          # Push to stack
pop reg           # Pop from stack
rep movsq         # Repeat move quad (memcpy)
```

## Operand Size Suffixes (x86-64)

| Suffix | Size | Example |
|--------|------|---------|
| (none) | Register size | `mov rax, rbx` |
| `b` | Byte (8-bit) | `movb %al, (%rdi)` |
| `w` | Word (16-bit) | `movw %ax, (%rdi)` |
| `l` | Long (32-bit) | `movl %eax, (%rdi)` |
| `q` | Quad (64-bit) | `movq %rax, (%rdi)` |

## ARM64 Registers

### General Purpose
| Register | 64-bit | 32-bit | Purpose |
|----------|--------|--------|---------|
| x0-x7 | x0-x7 | w0-w7 | Arguments/Returns |
| x8 | x8 | w8 | Indirect result |
| x9-x15 | x9-x15 | w9-w15 | Temporary |
| x16-x17 | x16-x17 | w16-w17 | Intra-procedure |
| x18 | x18 | w18 | Platform register |
| x19-x28 | x19-x28 | w19-w28 | Callee-saved |
| x29 | x29 | w29 | Frame pointer |
| x30 | x30 | w30 | Link register (return) |
| x31 | SP | WSP | Stack pointer |

### Argument Passing (ARM64 AAPCS)
- 1st arg: X0
- 2nd arg: X1
- 3rd arg: X2
- 4th arg: X3
- 5th arg: X4
- 6th arg: X5
- 7th arg: X6
- 8th arg: X7

## Common Assembly Patterns

### Bit Scan
```vitte
fn bit_position(x: u64) -> u32 {
    let pos: u32
    asm "bsf %1, %0" : "=r"(pos) : "r"(x)
    return pos
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

### Memory Barrier
```vitte
fn memory_barrier() {
    asm volatile "mfence"
}
```

### SIMD Operation
```vitte
fn add_vectors(a: *const f32, b: *const f32) -> f32 {
    let result: f32
    asm "movups (%1), %%xmm0; addps (%2), %%xmm0; "
        "movss %%xmm0, %0"
        : "=m"(result)
        : "r"(a), "r"(b)
        : "xmm0"
    return result
}
```

## Constraint Validation Rules

1. **Output constraints must start with `=` or `+`**
   ```vitte
   // ✓ Correct
   asm "bsf %1, %0" : "=r"(result) : "r"(x)
   
   // ✗ Error
   asm "bsf %1, %0" : "r"(result) : "r"(x)
   ```

2. **Operand count must match template**
   ```vitte
   // ✓ Correct (2 operands: %0, %1)
   asm "add %1, %0" : "=r"(a) : "r"(b)
   
   // ✗ Error (3 operands in template, 2 declared)
   asm "add %2, %1, %0" : "=r"(a) : "r"(b), "r"(c)
   ```

3. **Type must match constraint**
   ```vitte
   // ✓ Correct (i32 matches 'r')
   asm "inc %0" : "+r"(x)
   
   // ✗ Error (f32 doesn't match 'r')
   asm "inc %0" : "+r"(f)
   ```

## Clobber List Rules

List all registers/memory modified by assembly:

```vitte
// Modifies RAX and RDX
asm "div %1" : "=a"(quo), "=d"(rem) : "0"(x), "r"(y) : "flags"
                                                      ↑
                                              Don't forget!
```

### Common Clobbers
- `"memory"` - Memory is modified
- `"cc"` or `"flags"` - Condition flags
- `"rax"`, `"rcx"`, etc. - Specific registers

## Platform-Specific Code

### Conditional Compilation
```vitte
#if defined(__x86_64__)
    // x86-64 specific
#elif defined(__aarch64__)
    // ARM64 specific
#else
    // Fallback
#endif
```

### Attribute-Based
```c
// In generated C
__attribute__((target("sse4.2")))
void simd_function(void) { ... }
```

## Testing Checklist

Before shipping code with assembly:

- [ ] Works on target architecture
- [ ] C fallback available
- [ ] Tested with different compiler versions
- [ ] Performance verified with profiling
- [ ] Edge cases tested (0, max, patterns)
- [ ] Thread-safe if used in concurrent code
- [ ] Memory barriers where needed
- [ ] Documented with comments
- [ ] Code reviewed for correctness

## Performance Tips

1. **Minimize register pressure** - Use only necessary operands
2. **Avoid data dependencies** - Keep instructions independent
3. **Use appropriate sizes** - `movl` vs `movq`, not always `mov`
4. **Align memory** - 16-byte alignment for SIMD
5. **Profile first** - Measure before and after
6. **Keep it simple** - Assembly is hard to optimize

## Common Mistakes

| Mistake | Fix |
|---------|-----|
| Forgetting clobbers | List all modified registers/memory |
| Wrong operand order | Check template: `%0` is output, `%1` is input |
| Type mismatch | Ensure operand type matches constraint |
| Uninitialized operands | Initialize all input variables |
| Memory leaks in asm | Don't allocate memory in assembly |
| Race conditions | Use `lock` prefix or barriers |
| Platform-specific code without guards | Use `#if defined(...)` |

## Debugging Tips

```bash
# View generated assembly
gcc -S -O2 program.c

# Step through in debugger
gdb ./program
(gdb) disassemble function_name
(gdb) stepi

# Check with objdump
objdump -d ./program | grep -A20 function_name

# Profile
perf record ./program
perf report
```

## Resources

- **GCC Manual**: https://gcc.gnu.org/onlinedocs/gcc/
- **x86-64 ABI**: https://www.uclibc.org/docs/psABI-x86_64.pdf
- **ARM ABI**: https://developer.arm.com/documentation/ddi0487/
- **Intel ISA**: https://www.intel.com/content/www/us/en/developer/articles/
- **Compiler Explorer**: https://godbolt.org/

---

**Print this card and keep it at your desk while writing assembly code!**

For complete documentation, see [ASSEMBLY_README.md](ASSEMBLY_README.md)
