# Application Binary Interface (ABI)

Specification of the Vitte ABI - how data is represented and functions are called.

## Overview

The Vitte ABI defines:
- **Data types and layouts** - How values are stored in memory
- **Function calling convention** - How functions call each other
- **Function prologue/epilogue** - Stack management
- **Error handling** - How errors are returned
- **Exception handling** - How panics work
- **Thread-local storage** - How thread data works

## Data Layout

### Basic Types

All sizes are in bytes, alignment is what must be satisfied.

| Type | Size | Alignment | Notes |
|------|------|-----------|-------|
| `bool` | 1 | 1 | False=0, True=1 |
| `i8`, `u8` | 1 | 1 | |
| `i16`, `u16` | 2 | 2 | |
| `i32`, `u32`, `f32` | 4 | 4 | |
| `i64`, `u64`, `f64` | 8 | 8 | |
| `isize`, `usize` | 8* | 8* | * On 64-bit systems |
| `*T` (pointer) | 8* | 8* | * On 64-bit systems |

### Struct Layout

```vitte
#[repr(C)]
struct Point {
    x: i32,      // Offset 0, size 4
    y: i32,      // Offset 4, size 4
}
// Total size: 8 bytes, alignment: 4
```

With `#[repr(C)]`, structs follow C layout rules (field order preserved).

### Enum Layout

```vitte
enum Result<T, E> {
    Ok(T),       // Discriminant 0
    Err(E),      // Discriminant 1
}
```

Discriminant occupies space based on value range (1, 2, or 4 bytes typically).

### Array Layout

```vitte
let arr: [i32; 4]  // 4 consecutive i32 values = 16 bytes
```

Arrays are zero-indexed, contiguous in memory.

## Calling Convention

### Argument Passing

On x86_64-linux-gnu:

| Argument | Register | Fallback |
|----------|----------|----------|
| 1st (int-like) | `rdi` | Stack |
| 2nd (int-like) | `rsi` | Stack |
| 3rd (int-like) | `rdx` | Stack |
| 4th (int-like) | `rcx` | Stack |
| 5th (int-like) | `r8` | Stack |
| 6th (int-like) | `r9` | Stack |
| 1st (float) | `xmm0` | Stack |
| 2nd (float) | `xmm1` | Stack |
| ... | ... | ... |

On Windows x86_64 (MSVC):

| Argument | Register |
|----------|----------|
| 1st | `rcx` |
| 2nd | `rdx` |
| 3rd | `r8` |
| 4th | `r9` |

On ARM64:

| Argument | Register |
|----------|----------|
| 1st-8th (int) | `x0`-`x7` |
| 1st-8th (float) | `d0`-`d7` |

### Return Values

| Type | Location | Notes |
|------|----------|-------|
| Pointer | `rax` | |
| Integer ≤ 64-bit | `rax` | |
| Integer > 64-bit | Memory via implicit arg | |
| Float | `xmm0` | (x86_64) |
| Struct | Memory via implicit arg | Large structs |

### Stack Frame

```
Higher addresses
    ↑
    +----------+
    | arg n    |  (if more than 6 args)
    +----------+
    | ...      |
    +----------+
    | return addr |  (implicit)
    +----------+
    | saved rbp |  (if frame pointer used)
    +----------+
    | local vars|  (growing down)
    +----------+
    | ...      |
Lower addresses
```

## Function Prologue/Epilogue

### x86_64 Example

```asm
; Prologue
push rbp
mov rbp, rsp
sub rsp, <space_for_locals>

; Function body
; ...

; Epilogue
mov rsp, rbp
pop rbp
ret
```

## Error Handling

### Result Type

```vitte
enum Result<T, E> {
    Ok(T),
    Err(E),
}
```

Functions return `Result<T, E>`:
- Success: `Ok(value)` with discriminant 0
- Error: `Err(error)` with discriminant 1

### Panic Handling

Panics are unrecoverable errors:

```vitte
fn panicking_function() {
    panic!("Something went wrong")
}
```

At ABI level:
1. Panic is logged/reported
2. Runtime unwinds stack (if supported)
3. Program terminates with exit code 101

## Memory Management

### Stack Allocation

Local variables are allocated on the stack:

```vitte
fn example() {
    let x: i32 = 42      // Allocated on stack
    let s = String::new() // Header on stack, data on heap
}
```

### Heap Allocation

```vitte
let b = Box::new(42)     // Data allocated on heap
drop(b)                  // Deallocated on drop
```

Heap allocation uses system allocator (malloc/free).

## Calling Conventions Summary

| Platform | Convention | Notes |
|----------|-----------|-------|
| x86_64-linux-gnu | System V AMD64 ABI | Standard Unix |
| x86_64-windows | Microsoft x64 | Windows ABI |
| aarch64-linux | AAPCS64 | ARM 64-bit |
| armv7-linux | EABI | ARM 32-bit |
| wasm32 | WASM Linear Memory | Browser/WASI |

## Foreign Function Interface (FFI)

### `extern "C"` ABI

Functions declared with `extern "C"` use C ABI:

```vitte
extern "C" {
    fn strlen(s: *const i8) -> usize  // Uses C ABI
}

#[no_mangle]
pub extern "C" fn add(a: i32, b: i32) -> i32 {  // Uses C ABI
    a + b
}
```

## Compatibility

### ABI Stability

- The Vitte ABI is **stable** across versions within major releases
- Minor version changes maintain ABI compatibility
- Major version bumps may change ABI
- Semantic versioning: `MAJOR.MINOR.PATCH`

### Breaking Changes

Require major version bump:
- Changing struct/enum memory layout
- Changing calling conventions
- Adding new builtin types
- Removing features

## Size Optimization

### Representation Selectors

```vitte
#[repr(C)]            // C-compatible layout
struct S1 { x: u8, y: u16 }  // Size: 4 (with padding)

#[repr(transparent)]  // Same as wrapped type
struct Newtype(i32)  // Size: 4, alignment: 4

#[repr(packed)]       // No padding (careful with alignment!)
struct Packed { x: u8, y: u16 }  // Size: 3
```

## Debugging Support

### Debug Info

Vitte generates debug information (DWARF on Unix, PDB on Windows):

```bash
vittec main.vit -g -o main  # Include debug info
```

Debug info includes:
- Line number mappings
- Variable locations
- Type information
- Symbol names

## Testing ABI Compliance

```bash
# Compile with ABI checking
vittec main.vit --check-abi -o main

# Run FFI tests
vitte-test --ffi

# Validate struct layouts
vitte-validate-abi struct.h
```

## See Also

- [BUILDING.md](BUILDING.md) - Build instructions
- [FFI.md](FFI.md) - FFI details
- [TARGETS.md](TARGETS.md) - Target specifications
