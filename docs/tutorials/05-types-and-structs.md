# Types and Structs

Create custom types to model your data.

## Structs

Structs group related data together:

```vitte
struct Person {
    name: String,
    age: i32,
    email: String,
}

fn main() {
    let person = Person {
        name: "Alice",
        age: 30,
        email: "alice@example.com",
    }

    print(person.name)  // "Alice"
    print(person.age)   // 30
}
```

## Methods

Attach functions to structs:

```vitte
impl Person {
    fn describe(&self) -> String {
        self.name + " is " + self.age + " years old"
    }
}

fn main() {
    let person = Person { /* ... */ }
    print(person.describe())
}
```

## Enums

Represent values that can be one of several variants:

```vitte
enum Color {
    Red,
    Green,
    Blue,
}

fn main() {
    let color = Color::Red
}
```

## Enums with Data

```vitte
enum Result<T> {
    Ok(T),
    Err(String),
}

fn divide(a: i32, b: i32) -> Result<i32> {
    if b == 0 {
        Result::Err("Division by zero")
    } else {
        Result::Ok(a / b)
    }
}
```

## Pattern Matching with Structs

```vitte
let person = Person { /* ... */ }

match person {
    Person { name: "Alice", age: a } => print("Found Alice, age " + a),
    Person { name: n, age: _ } => print("Found " + n),
}
```

## Next: Error Handling

Learn idiomatic error handling with [Error Handling](./06-error-handling.md).
