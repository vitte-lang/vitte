# Language Reference

Complete specification of Vitte syntax and semantics.

## Lexical Structure

### Comments

```vitte
// Single-line comment

/* Multi-line comment
   can span
   multiple lines */
```

### Keywords

```
abstract   as        async      await      bool
break      case      catch      class      const
continue   def       do         else       enum
export     false     finally    float      fn
for        from      if         impl       import
in         interface is         let        loop
match      module    mut        null       override
private    protected pub        return     self
static     struct    super      this       throw
true       trait     try        type       use
var        void      while      yield
```

### Identifiers

Start with letter or underscore, followed by letters, digits, underscores:

```vitte
let my_var = 42
let _private = 10
let CamelCase = 5
let snake_case = 3
```

### Literals

```vitte
// Integer literals
42
0xFF        // Hexadecimal
0o755       // Octal
0b1010      // Binary

// Float literals
3.14
1e-5

// String literals
"hello"
"escape: \n \t \""

// Character literals
'a'
'\n'

// Boolean literals
true
false

// Unit literal
()
```

## Expressions and Statements

### Variable Declaration

```vitte
let x = 5              // Immutable
let mut y = 10         // Mutable
let z: i32 = 15        // With type annotation
```

### Assignment

```vitte
x = 10
mut_var = mut_var + 1
```

### Operators

#### Arithmetic
```vitte
a + b    // Addition
a - b    // Subtraction
a * b    // Multiplication
a / b    // Division
a % b    // Modulo
a ** b   // Exponentiation
```

#### Comparison
```vitte
a == b   // Equal
a != b   // Not equal
a < b    // Less than
a <= b   // Less or equal
a > b    // Greater than
a >= b   // Greater or equal
```

#### Logical
```vitte
a && b   // AND
a || b   // OR
!a       // NOT
```

#### Bitwise
```vitte
a & b    // AND
a | b    // OR
a ^ b    // XOR
!a       // NOT
a << b   // Left shift
a >> b   // Right shift
```

#### Assignment Operators
```vitte
a += b   // a = a + b
a -= b   // a = a - b
a *= b   // a = a * b
a /= b   // a = a / b
a %= b   // a = a % b
a &= b   // a = a & b
a |= b   // a = a | b
a ^= b   // a = a ^ b
a <<= b  // a = a << b
a >>= b  // a = a >> b
```

### Control Flow

#### If Expression
```vitte
if condition {
    // ...
} else if other_condition {
    // ...
} else {
    // ...
}

let value = if x > 0 { "positive" } else { "negative" }
```

#### While Loop
```vitte
while condition {
    // ...
}
```

#### For Loop
```vitte
for i in 0..10 {
    // i is 0, 1, ..., 9
}

for item in collection {
    // ...
}
```

#### Loop (Infinite)
```vitte
loop {
    if done {
        break
    }
}
```

#### Match Expression
```vitte
match value {
    1 => println!("one"),
    2 | 3 => println!("two or three"),
    4..=6 => println!("four to six"),
    _ => println!("other"),
}
```

## Functions

### Function Definition

```vitte
fn add(a: i32, b: i32) -> i32 {
    a + b
}

fn greet(name: &str) {
    println!("Hello, {}", name)
}
```

### Closures

```vitte
let add = |a: i32, b: i32| -> i32 { a + b }
let add = |a, b| a + b  // Types inferred

let nums = vec![1, 2, 3]
let doubled = nums.iter().map(|x| x * 2)
```

## Types

### Struct Definition

```vitte
struct Point {
    x: f64,
    y: f64,
}

struct Tuple(i32, String);

struct Unit;
```

### Enum Definition

```vitte
enum Color {
    Red,
    Green,
    Blue,
}

enum Result<T, E> {
    Ok(T),
    Err(E),
}
```

### Impl Block

```vitte
impl Point {
    fn new(x: f64, y: f64) -> Point {
        Point { x, y }
    }

    fn distance(&self) -> f64 {
        (self.x * self.x + self.y * self.y).sqrt()
    }
}
```

### Trait Definition

```vitte
trait Drawable {
    fn draw(&self);
    fn area(&self) -> f64;
}

impl Drawable for Point {
    fn draw(&self) {
        println!("({}, {})", self.x, self.y)
    }

    fn area(&self) -> f64 {
        0.0
    }
}
```

## Generics and Traits

```vitte
fn max<T: Comparable>(a: T, b: T) -> T {
    if a > b { a } else { b }
}

struct Wrapper<T> {
    value: T,
}

fn contains<T: PartialEq>(haystack: &[T], needle: T) -> bool {
    haystack.iter().any(|x| x == needle)
}
```

## Modules and Imports

```vitte
// Define module
mod math {
    pub fn add(a: i32, b: i32) -> i32 {
        a + b
    }
}

// Import
use math::add
use std::collections::HashMap
use std::io::{self, Read}
use std::io::*
```

## Error Handling

```vitte
fn divide(a: i32, b: i32) -> Result<i32, String> {
    if b == 0 {
        Err("Division by zero".to_string())
    } else {
        Ok(a / b)
    }
}

// Propagating errors
fn process() -> Result<String, String> {
    let result = divide(10, 2)?
    Ok(format!("Result: {}", result))
}
```

## Attributes

```vitte
#[derive(Debug, Clone)]
struct Person {
    name: String,
    age: u32,
}

#[test]
fn test_something() {
    assert_eq!(2 + 2, 4)
}

#[inline]
fn small_function() {
    // ...
}
```

## Unsafe Code

```vitte
unsafe {
    let ptr = &x as *const _;
    println!("{}", *ptr);
}

unsafe fn dangerous() {
    // ...
}
```

## Type Aliases

```vitte
type Result<T> = std::result::Result<T, String>
type Kilometers = f64
```

## See Also

- [Language Specification](./reference.md) - This document
- [Tutorials](../tutorials/) - Learn by example
- [API Reference](../api/) - Standard library docs
