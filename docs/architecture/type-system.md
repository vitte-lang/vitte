# Type System

Vitte's type system: design, inference, and checking.

## Core Types

### Primitive Types

- **Integers**: `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`, `isize`, `usize`
- **Floats**: `f32`, `f64`
- **Boolean**: `bool`
- **Character**: `char` (32-bit Unicode)
- **Unit**: `()` (no value)
- **Never**: `!` (diverging, never returns)

### Compound Types

#### Tuples
```vitte
let pair: (i32, String) = (42, "hello")
```

#### Arrays
```vitte
let arr: [i32; 5] = [1, 2, 3, 4, 5]
```

#### Structs
```vitte
struct Point {
    x: i32,
    y: i32,
}
```

#### Enums
```vitte
enum Option<T> {
    Some(T),
    None,
}
```

### Reference Types

- **Immutable Reference**: `&T` - borrowing, read-only
- **Mutable Reference**: `&mut T` - borrowing, mutable access
- **Raw Pointer**: `*const T`, `*mut T` - unsafe

### Function Types

```vitte
let f: fn(i32) -> i32 = |x| x * 2
```

## Type Inference

Vitte uses **Hindley-Milner style type inference**:

1. **Constraint Generation** - Collect type constraints from code
2. **Unification** - Solve constraints to infer types
3. **Propagation** - Propagate inferred types through expressions

### Examples

```vitte
// Type inferred from literal
let x = 42           // i32
let y = 3.14         // f64
let s = "hello"      // &str

// Type inferred from context
let nums: Vec<i32> = vec![1, 2, 3]

// Type inferred from function calls
let result = max(10, 20)  // i32 (from max's signature)
```

## Generics

Parametric polymorphism allows code reuse across types:

```vitte
fn first<T>(items: &[T]) -> Option<T> {
    if items.len() > 0 {
        Some(items[0])
    } else {
        None
    }
}
```

### Trait Bounds

Constrain generic types:

```vitte
fn print_it<T: Display>(value: T) {
    println!("{}", value)
}
```

## Traits

Define shared behavior:

```vitte
trait Drawable {
    fn draw(&self);
}

impl Drawable for Circle {
    fn draw(&self) {
        print("Drawing circle")
    }
}
```

### Trait Objects

Runtime polymorphism:

```vitte
let drawable: Box<dyn Drawable> = Box::new(Circle::new())
drawable.draw()
```

## Type Checking Rules

### Ownership

```vitte
let s1 = String::from("hello")
let s2 = s1                    // Move: s1 no longer valid
```

### Borrowing

```vitte
let s = String::from("hello")
let r1 = &s      // Multiple immutable borrows OK
let r2 = &s
let r3 = &mut s  // ERROR: can't borrow mutably while immutable borrows exist
```

### Type Compatibility

```vitte
let x: i32 = 5
let y: f64 = x as f64   // Explicit cast required
let z: i32 = 3.14       // ERROR: no implicit conversion
```

## Advanced Features

### Associated Types

```vitte
trait Iterator {
    type Item
    fn next(&mut self) -> Option<Self::Item>
}
```

### Lifetimes

Track how long references are valid:

```vitte
fn longest<'a>(s1: &'a str, s2: &'a str) -> &'a str {
    if s1.len() > s2.len() { s1 } else { s2 }
}
```

### Higher-Ranked Trait Bounds (HRTB)

```vitte
fn call_once<F>(f: F)
where
    F: for<'a> Fn(&'a str) -> &'a str,
{
    // ...
}
```

## Type Equivalence

### Structural Equivalence

Structs with same fields but different names are different types:

```vitte
struct A { x: i32 }
struct B { x: i32 }
// A and B are NOT equivalent
```

### Nominal vs Structural

- **Nominal**: Names matter (structs, classes)
- **Structural**: Structure matters (tuples)

## Future Enhancements

- [ ] Dependent types
- [ ] Refinement types
- [ ] Intersection types
- [ ] Union types

## See Also

- [Compiler Architecture](./compiler.md) - How types are checked
- [Language Reference](../language-spec/reference.md) - Type syntax
