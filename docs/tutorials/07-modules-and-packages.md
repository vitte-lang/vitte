# Modules and Packages

Organize large projects into manageable pieces.

## Module Basics

Create a module by defining a directory with a `mod.muf` file:

```
project/
├── muffin.muf
├── src/
│   ├── main.vit
│   ├── math/
│   │   ├── mod.muf
│   │   └── operations.vit
│   └── utils/
│       ├── mod.muf
│       └── helpers.vit
```

## Declaring Modules

In `src/math/mod.muf`:

```vitte
pub fn add(a: i32, b: i32) -> i32 {
    a + b
}

pub fn multiply(a: i32, b: i32) -> i32 {
    a * b
}
```

## Importing Modules

In `src/main.vit`:

```vitte
use math::{add, multiply}

fn main() {
    let result = add(5, 3)
    print(result)
}
```

## Nested Modules

```vitte
use math::advanced::matrix::multiply
```

## Visibility

Use `pub` to make items public:

```vitte
pub fn public_function() { }

fn private_function() { }  // Only accessible within module
```

## Re-exporting

```vitte
pub use math::{add, multiply}  // Re-export from another module
```

## Packages

Use `muffin.muf` to define your package:

```toml
[package]
name = "my_app"
version = "0.1.0"
edition = "2024"

[dependencies]
# Add dependencies here
```

## Next: Generics

Write flexible code with [Generics](./08-generics.md).
