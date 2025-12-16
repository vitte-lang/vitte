# Code Examples

Working examples of Vitte programs demonstrating various features.

See subdirectories for complete runnable examples:

- **[fibonacci/](./fibonacci/)** - Recursive function example
- **[string_processing/](./string_processing/)** - String manipulation
- **[file_io/](./file_io/)** - File reading and writing
- **[modules/](./modules/)** - Module system and organization
- **[error_handling/](./error_handling/)** - Error handling patterns

## Quick Examples

### Hello World

```vitte
fn main() {
    println!("Hello, Vitte!")
}
```

### Variables and Types

```vitte
fn main() {
    let name = "Alice"
    let age = 30
    let height: f64 = 5.8

    let mut counter = 0
    counter = counter + 1

    println!("Name: {}", name)
    println!("Age: {}", age)
}
```

### Functions

```vitte
fn add(a: i32, b: i32) -> i32 {
    a + b
}

fn greet(name: &str) {
    println!("Hello, {}!", name)
}

fn main() {
    println!("5 + 3 = {}", add(5, 3))
    greet("World")
}
```

### Control Flow

```vitte
fn main() {
    // If/else
    let num = 7
    if num % 2 == 0 {
        println!("Even")
    } else {
        println!("Odd")
    }

    // For loop
    for i in 0..5 {
        println!("i = {}", i)
    }

    // Match
    match num {
        1 => println!("One"),
        2 => println!("Two"),
        _ => println!("Other"),
    }
}
```

### Structs

```vitte
struct Rectangle {
    width: f64,
    height: f64,
}

impl Rectangle {
    fn area(&self) -> f64 {
        self.width * self.height
    }

    fn perimeter(&self) -> f64 {
        2.0 * (self.width + self.height)
    }
}

fn main() {
    let rect = Rectangle {
        width: 5.0,
        height: 3.0,
    }

    println!("Area: {}", rect.area())
    println!("Perimeter: {}", rect.perimeter())
}
```

### Enums and Pattern Matching

```vitte
enum TrafficLight {
    Red,
    Yellow,
    Green,
}

impl TrafficLight {
    fn action(&self) -> &str {
        match self {
            TrafficLight::Red => "Stop",
            TrafficLight::Yellow => "Prepare",
            TrafficLight::Green => "Go",
        }
    }
}

fn main() {
    let light = TrafficLight::Red
    println!("{}", light.action())
}
```

### Error Handling

```vitte
fn parse_number(s: &str) -> Result<i32, String> {
    match s.parse() {
        Ok(num) => Ok(num),
        Err(_) => Err(format!("Cannot parse '{}'", s)),
    }
}

fn main() {
    match parse_number("42") {
        Ok(num) => println!("Parsed: {}", num),
        Err(err) => println!("Error: {}", err),
    }
}
```

## Running Examples

```bash
vittec examples/hello/main.vit -o hello
./hello
```

For detailed examples with explanations, see the subdirectories.
