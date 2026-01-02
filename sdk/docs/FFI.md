# C FFI Guide

Calling C code from Vitte and exposing Vitte to C.

## Calling C Functions

### Declaring Extern Functions

```vitte
// Simple C function
extern "C" fn printf(fmt: *const i8, ...) -> i32

extern "C" fn strlen(s: *const i8) -> usize

extern "C" fn malloc(size: usize) -> *mut i8

extern "C" fn free(ptr: *mut i8)
```

### Using C Functions

```vitte
use std::ffi::CStr

fn main() {
    // Print using C printf
    let msg = c"Hello from Vitte!\n"
    unsafe {
        printf(msg.as_ptr())
    }
}
```

## Type Mappings

### Vitte to C Type Mapping

| Vitte | C | Notes |
|-------|---|-------|
| `i8` | `int8_t` | Signed byte |
| `u8` | `uint8_t` | Unsigned byte |
| `i16` | `int16_t` | Signed short |
| `u16` | `uint16_t` | Unsigned short |
| `i32` | `int32_t` | Signed int |
| `u32` | `uint32_t` | Unsigned int |
| `i64` | `int64_t` | Signed long |
| `u64` | `uint64_t` | Unsigned long |
| `f32` | `float` | Single precision |
| `f64` | `double` | Double precision |
| `*const T` | `const T*` | Immutable pointer |
| `*mut T` | `T*` | Mutable pointer |
| `()` | `void` | Unit/void |

### String Handling

C strings vs Vitte strings:

```vitte
use std::ffi::{CStr, CString}

// C string literal
let c_str = c"hello"  // *const i8

// Convert Vitte String to C string
let vitte_str = "hello"
let c_str = CString::new(vitte_str)?
let ptr = c_str.as_ptr()

// Convert C string to Vitte String
let c_ptr = some_c_function()  // *const i8
let c_str = CStr::from_ptr(c_ptr)
let vitte_str = c_str.to_string_lossy().into_owned()
```

## Exposing Vitte to C

### Marking Functions for Export

```vitte
#[no_mangle]
pub extern "C" fn add(a: i32, b: i32) -> i32 {
    a + b
}

#[no_mangle]
pub extern "C" fn greet(name: *const i8) {
    let c_str = unsafe { CStr::from_ptr(name) }
    let name_str = c_str.to_string_lossy()
    print!("Hello, {}!", name_str)
}
```

### Generating C Headers

```bash
# Generate C header from Vitte library
vitte-bindgen my_lib.muf --output my_lib.h
```

Generated header:

```c
#ifndef MY_LIB_H
#define MY_LIB_H

#include <stdint.h>

int32_t add(int32_t a, int32_t b);
void greet(const char* name);

#endif  // MY_LIB_H
```

### Using from C

```c
#include "my_lib.h"
#include <stdio.h>

int main() {
    int result = add(5, 3);
    printf("Result: %d\n", result);  // Output: Result: 8

    greet("C Program");
    return 0;
}
```

## Struct Sharing

### Passing Structs to C

```vitte
#[repr(C)]
pub struct Point {
    x: f64,
    y: f64,
}

#[no_mangle]
pub extern "C" fn distance(p1: *const Point, p2: *const Point) -> f64 {
    unsafe {
        let dx = (*p2).x - (*p1).x
        let dy = (*p2).y - (*p1).y
        (dx * dx + dy * dy).sqrt()
    }
}
```

### C Calling Convention

```vitte
// Specify calling convention
#[no_mangle]
pub extern "C" fn callback(f: extern "C" fn(i32) -> i32) -> i32 {
    f(42)
}
```

## Callbacks

### Passing Callbacks to C

```vitte
extern "C" {
    fn set_callback(cb: extern "C" fn(i32) -> i32)
}

extern "C" fn my_callback(x: i32) -> i32 {
    x * 2
}

fn main() {
    unsafe {
        set_callback(my_callback)
    }
}
```

## Memory Safety

### Using Unsafe Correctly

```vitte
// Always check pointers from C
extern "C" fn process_string(s: *const i8) -> usize {
    if s.is_null() {
        return 0
    }

    unsafe {
        CStr::from_ptr(s).len()
    }
}

// Validate before use
fn safe_strlen(ptr: *const i8) -> usize {
    if ptr.is_null() {
        eprintln!("Warning: null pointer to strlen")
        return 0
    }

    unsafe {
        CStr::from_ptr(ptr).len()
    }
}
```

## Linking

### Automatic Linking

In `muffin.muf`:

```toml
[dependencies.mylib]
path = "../mylib"
link = "mylib"  # Link against libmylib
```

### Manual Linking

```bash
vittec main.vit -lmylib -L/path/to/lib -o main
```

## Build Integration

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.10)
project(my_project)

# Find Vitte
find_package(Vitte REQUIRED)

# Compile Vitte library
vitte_compile(
    NAME my_vitte_lib
    SOURCES src/lib.vit
    OUTPUT my_vitte_lib.a
)

# Link with C executable
add_executable(my_app src/main.c)
target_link_libraries(my_app my_vitte_lib)
```

## Debugging FFI

### Enabling Debug Info

```bash
vittec --ffi-debug --emit=c17 main.vit -o main
```

### Common Errors

**"Undefined reference to symbol"**
- Ensure C library is linked: `vittec main.vit -lname -o main`
- Check symbol exists in library: `nm -D /path/to/lib.so | grep symbol`

**"Segmentation fault in FFI call"**
- Verify pointer validity before use
- Check alignment requirements
- Ensure correct calling convention

**"Type mismatch in C FFI"**
- Verify C types match Vitte types
- Check struct layout with `#[repr(C)]`
- Use explicit casts when needed

## Examples

See [examples/ffi_c/](../../examples/ffi_c/) for complete working examples.

## Performance

FFI calls have minimal overhead:
- Function call overhead ≈ 1-2 nanoseconds
- No marshalling for simple types
- Struct passing by value is efficient with `#[repr(C)]`

## See Also

- [ABI.md](ABI.md) - Application Binary Interface
- [examples/ffi_c/](../../examples/ffi_c/) - FFI examples
- [CONTRIBUTING.md](../CONTRIBUTING.md) - Contributing
