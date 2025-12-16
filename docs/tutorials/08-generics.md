# Generics

Write flexible, reusable code that works with any type.

## Generic Functions

```vitte
fn print_twice<T>(value: T) {
    print(value)
    print(value)
}

fn main() {
    print_twice(42)           // Works with i32
    print_twice("hello")      // Works with String
}
```

## Generic Structs

```vitte
struct Container<T> {
    value: T,
}

fn main() {
    let int_container = Container { value: 42 }
    let string_container = Container { value: "hello" }
}
```

## Generic Methods

```vitte
impl<T> Container<T> {
    fn get(&self) -> T {
        self.value
    }
}
```

## Trait Bounds

Constrain generic types:

```vitte
fn compare<T: Comparable>(a: T, b: T) -> bool {
    a.equals(b)
}
```

## Multiple Generics

```vitte
fn pair<T, U>(first: T, second: U) -> (T, U) {
    (first, second)
}
```

## Generic Enums

```vitte
enum Result<T, E> {
    Ok(T),
    Err(E),
}

enum Option<T> {
    Some(T),
    None,
}
```

## Lifetime Generics (Advanced)

```vitte
fn borrow<'a>(reference: &'a String) -> &'a String {
    reference
}
```

## Next: Memory Management

Learn about ownership and borrowing with [Memory Management](./09-memory-management.md).
