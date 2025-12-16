# Getting Started with Vitte

Learn how to write and run your first Vitte program.

## Installation

See [Installation Guide](../../INSTALL.md) for platform-specific instructions.

## Your First Program

Create a file named `hello.vit`:

```vitte
fn main() {
    print("Hello, Vitte!")
}
```

## Running Your Program

```bash
vittec hello.vit -o hello
./hello
```

Expected output:
```
Hello, Vitte!
```

## What Just Happened?

1. **`fn main()`** - Defines the entry point of your program
2. **`print()`** - A built-in function to display text
3. **`vittec hello.vit -o hello`** - Compiles your Vitte code to an executable
4. **`./hello`** - Runs the compiled executable

## Next Steps

- Read [Basic Concepts](./02-basic-concepts.md) to learn about variables and types
- Explore [Examples](../examples/README.md) for real-world patterns
- Check [Language Reference](../language-spec/reference.md) for detailed syntax
