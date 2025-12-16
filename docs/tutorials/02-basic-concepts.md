# Basic Concepts

Learn about variables, types, and basic operations in Vitte.

## Variables

Variables store data values. Declare them with `let`:

```vitte
let name = "Alice"
let age = 30
let score = 95.5
```

## Type Annotations

You can explicitly specify types:

```vitte
let name: String = "Bob"
let count: i32 = 42
let ratio: f64 = 0.75
```

## Immutability

By default, variables are immutable. Use `mut` for mutable variables:

```vitte
let mut counter = 0
counter = counter + 1  // OK with mut
```

## Basic Types

- **Integers**: `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`
- **Floating-point**: `f32`, `f64`
- **Boolean**: `bool` (true or false)
- **String**: `String` (heap-allocated) or `&str` (string slice)
- **Characters**: `char`

## Operations

```vitte
let x = 10
let y = 3

let sum = x + y      // 13
let diff = x - y     // 7
let product = x * y  // 30
let quotient = x / y // 3
let remainder = x % y // 1
```

## Printing Values

```vitte
let name = "Vitte"
let version = "0.2.0"

print("Language: {}")       // Placeholder syntax (to be defined)
print("Name: " + name)
```

## Next: Control Flow

Proceed to [Control Flow](./03-control-flow.md) to learn if/else, loops, and pattern matching.
