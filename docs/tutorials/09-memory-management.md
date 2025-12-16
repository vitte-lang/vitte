# Memory Management

Understand Vitte's approach to memory safety and performance.

## Ownership

Every value has one owner. When the owner is dropped, the value is freed:

```vitte
{
    let string = "hello"  // string is allocated
    // ...
}  // string is freed here
```

## Moves

Ownership transfers when you assign or pass a value:

```vitte
let s1 = "hello"
let s2 = s1  // Ownership moves to s2
// print(s1)  // ERROR: s1 no longer owns the value
```

## Borrowing

References allow temporary access without taking ownership:

```vitte
fn print_string(s: &String) {
    print(s)
}

let my_string = "hello"
print_string(&my_string)
print_string(&my_string)  // OK: my_string still owns
```

## Mutable Borrowing

Create mutable references for modification:

```vitte
fn modify(s: &mut String) {
    s.push_str("!")
}

let mut text = "hello"
modify(&mut text)
print(text)  // "hello!"
```

## Borrowing Rules

1. You can have **many immutable references** OR **one mutable reference**
2. References cannot outlive the value they reference

```vitte
let mut x = 5
let r1 = &x   // OK
let r2 = &x   // OK
let r3 = &mut x  // ERROR: can't have mutable ref with immutable refs

print(r1)
print(r2)
```

## Smart Pointers

### Box (Heap Allocation)

```vitte
let boxed = Box::new(42)
print(boxed)
```

### Reference Counting

```vitte
let shared = Rc::new(42)
let clone1 = Rc::clone(&shared)
let clone2 = Rc::clone(&shared)
```

## Next: Concurrency

Explore concurrent programming with [Concurrency](./10-concurrency.md).
