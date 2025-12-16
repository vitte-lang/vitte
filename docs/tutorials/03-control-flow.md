# Control Flow

Master conditionals and loops in Vitte.

## If/Else Statements

```vitte
let age = 18

if age >= 18 {
    print("You are an adult")
} else if age >= 13 {
    print("You are a teenager")
} else {
    print("You are a child")
}
```

## Loops

### While Loop

```vitte
let mut i = 0
while i < 5 {
    print(i)
    i = i + 1
}
```

### For Loop

```vitte
for i in 0..5 {
    print(i)  // Prints 0, 1, 2, 3, 4
}
```

### Breaking Out

```vitte
let mut counter = 0
while true {
    if counter == 5 {
        break
    }
    counter = counter + 1
}
```

### Continue

```vitte
for i in 0..10 {
    if i % 2 == 0 {
        continue  // Skip even numbers
    }
    print(i)
}
```

## Pattern Matching (Match)

```vitte
let value = 2

match value {
    1 => print("One"),
    2 => print("Two"),
    3 => print("Three"),
    _ => print("Other"),
}
```

## Expressions

In Vitte, blocks return values:

```vitte
let number = if condition { 5 } else { 6 }

let result = {
    let x = 3
    x + 1
}  // result is 4
```

## Next: Functions

Learn to write reusable code with [Functions](./04-functions.md).
