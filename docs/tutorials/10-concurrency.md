# Concurrency

Write concurrent and parallel programs safely.

## Threads

Create lightweight threads:

```vitte
use std::thread

fn main() {
    let handle = thread::spawn(|| {
        print("Hello from thread!")
    })

    handle.join()  // Wait for thread to finish
}
```

## Channels (Message Passing)

Send data between threads:

```vitte
use std::channel

fn main() {
    let (sender, receiver) = channel::create()

    thread::spawn(move || {
        sender.send("Hello")
    })

    let message = receiver.recv()
    print(message)  // "Hello"
}
```

## Shared State (Mutex)

Safely share mutable state:

```vitte
use std::sync::Mutex

fn main() {
    let counter = Mutex::new(0)

    let mut handles = vec![]
    for _ in 0..10 {
        let c = counter.clone()
        handles.push(thread::spawn(move || {
            let mut num = c.lock()
            *num += 1
        }))
    }

    for h in handles {
        h.join()
    }

    print(counter.lock())  // 10
}
```

## Atomic Types

Low-level synchronization:

```vitte
use std::sync::atomic::{AtomicBool, Ordering}

let flag = AtomicBool::new(false)
flag.store(true, Ordering::SeqCst)
```

## Race Conditions

Vitte's type system prevents data races at compile time:

```vitte
// This will NOT compile:
let mut x = 5
let r1 = &mut x
let r2 = &mut x  // ERROR: cannot have two mutable references
```

## Best Practices

1. Use channels for simple message passing
2. Use Mutex for shared state
3. Use atomic types for simple flags
4. Avoid unnecessary locks
5. Watch for deadlocks with multiple locks

## Further Reading

- [Concurrency Module Docs](../api/stdlib.md#concurrency)
- Examples: See `examples/` directory
