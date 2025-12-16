# Functions

Write reusable, organized code with functions.

## Basic Function

```vitte
fn greet(name: String) {
    print("Hello, " + name + "!")
}

fn main() {
    greet("Alice")
}
```

## Return Values

```vitte
fn add(a: i32, b: i32) -> i32 {
    a + b
}

fn main() {
    let result = add(5, 3)  // result = 8
    print(result)
}
```

**Note**: The last expression is returned without `return`. You can also use `return` explicitly:

```vitte
fn check(x: i32) -> bool {
    if x > 0 {
        return true
    }
    false
}
```

## Multiple Parameters

```vitte
fn multiply(x: i32, y: i32, z: i32) -> i32 {
    x * y * z
}
```

## Optional Return (No Return Value)

```vitte
fn print_info(name: String) {
    print("Name: " + name)
    // Implicitly returns unit type ()
}
```

## Function Scope

Variables defined in functions are local:

```vitte
fn example() {
    let local_var = 42
}

// local_var is not accessible here
```

## Shadowing

Inner scopes can redefine names:

```vitte
let x = 5
{
    let x = 10
    print(x)  // 10
}
print(x)  // 5
```

## Next: Types and Structs

Organize related data with [Types and Structs](./05-types-and-structs.md).
