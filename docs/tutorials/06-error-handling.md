# Error Handling

Write robust programs that handle errors gracefully.

## The Result Type

`Result<T, E>` represents either success (`Ok`) or failure (`Err`):

```vitte
fn parse_number(s: String) -> Result<i32, String> {
    // Try to parse...
    if is_valid_number(s) {
        Result::Ok(convert_to_i32(s))
    } else {
        Result::Err("Not a valid number")
    }
}
```

## Handling Results

### Match

```vitte
let result = parse_number("42")

match result {
    Result::Ok(number) => print("Got: " + number),
    Result::Err(error) => print("Error: " + error),
}
```

### Unwrap (Panics on Error)

```vitte
let number = parse_number("42").unwrap()
print(number)
```

### Unwrap with Default

```vitte
let number = parse_number("invalid").unwrap_or(0)
print(number)  // Prints 0
```

## The Option Type

`Option<T>` represents either a value (`Some`) or nothing (`None`):

```vitte
let maybe_name: Option<String> = Some("Alice")

match maybe_name {
    Option::Some(name) => print("Name: " + name),
    Option::None => print("No name"),
}
```

## ? Operator (Try)

Propagate errors up the call stack:

```vitte
fn process_file(path: String) -> Result<String, String> {
    let content = read_file(path)?  // Returns Err if read_file fails
    let processed = transform(content)?
    Result::Ok(processed)
}
```

## Panics (Last Resort)

Use only for unrecoverable errors:

```vitte
let value = some_value.expect("value should exist")
// or
panic("Something went wrong!")
```

## Next: Modules

Organize code with [Modules and Packages](./07-modules-and-packages.md).
