# core.primitive

`vitte/stdlib/core/primitive` defines the fundamental Vitte type layout
metadata used by freestanding core code. The module does not depend on the
operating system and does not require dynamic allocation.

## Fixed Layout Types

| Type | Size | Align | Notes |
|------|------|-------|-------|
| `bool` | 1 | 1 | Boolean value |
| `byte` | 1 | 1 | Alias-level unsigned byte |
| `char` | 1 | 1 | Single byte character unit |
| `rune` | 4 | 4 | Unicode scalar value |
| `i8` | 1 | 1 | Signed integer |
| `i16` | 2 | 2 | Signed integer |
| `i32` | 4 | 4 | Signed integer |
| `i64` | 8 | 8 | Signed integer |
| `i128` | 16 | 16 | Signed integer |
| `u8` | 1 | 1 | Unsigned integer |
| `u16` | 2 | 2 | Unsigned integer |
| `u32` | 4 | 4 | Unsigned integer |
| `u64` | 8 | 8 | Unsigned integer |
| `u128` | 16 | 16 | Unsigned integer |
| `f32` | 4 | 4 | Required floating point |
| `f64` | 8 | 8 | Required floating point |
| `unit` | 0 | 1 | Single zero-sized value |

## Optional Floating Point

| Type | Size | Align | Availability |
|------|------|-------|--------------|
| `f16` | 2 | 2 | Only when the backend exposes `float16` |
| `f128` | 16 | 16 | Optional backend feature `float128` |

## Architecture-Dependent Types

| Type | 32-bit size/align | 64-bit size/align | Notes |
|------|-------------------|-------------------|-------|
| `int` | 4 / 4 | 8 / 8 | Pointer-sized signed integer |
| `uint` | 4 / 4 | 8 / 8 | Pointer-sized unsigned integer |
| `isize` | 4 / 4 | 8 / 8 | Signed pointer difference |
| `usize` | 4 / 4 | 8 / 8 | Object size and index type |
| pointer | 4 / 4 | 8 / 8 | Raw pointer |
| reference | 4 / 4 | 8 / 8 | Borrowed pointer |
| function pointer | 4 / 4 | 8 / 8 | Callable address |
| slice | 8 / 4 | 16 / 8 | Fat pointer: data pointer plus length |

## Compound Types

Tuples are laid out from their fields in declaration order with normal padding.
Fixed arrays have `element_size * count` bytes and the element alignment.
Slices are fat pointers. References and raw pointers have target pointer size.

`never` is uninhabited and has no runtime value layout.
