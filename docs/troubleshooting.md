# Troubleshooting Guide

Solutions for common problems when using Vitte.

## Installation Issues

### "vittec: command not found"

**Problem**: The compiler executable can't be found.

**Solutions**:
1. Verify installation completed successfully:
   ```bash
   ls -la ./target/debug/vittec
   ```
2. Add Vitte to PATH:
   ```bash
   export PATH="$HOME/vitte/target/debug:$PATH"
   ```
3. Use full path temporarily:
   ```bash
   ./target/debug/vittec hello.vit
   ```

### Build fails with "C compiler not found"

**Problem**: GCC, Clang, or MSVC is not available.

**Solutions**:

**Linux (Ubuntu/Debian)**:
```bash
sudo apt-get update
sudo apt-get install build-essential cmake
```

**macOS**:
```bash
# Option 1: Xcode Command Line Tools
xcode-select --install

# Option 2: Homebrew
brew install gcc cmake
```

**Windows**:
1. Download and install [MinGW-w64](https://www.mingw-w64.org/)
2. Or install [Visual Studio Community](https://visualstudio.microsoft.com/)
3. Add to PATH if needed

### "CMake not found"

**Problem**: CMake is required but not installed.

**Solutions**:
```bash
# Linux
sudo apt-get install cmake

# macOS
brew install cmake

# Windows (with Chocolatey)
choco install cmake

# Or download from https://cmake.org/download/
```

## Compilation Errors

### "Unexpected token" in parser

**Problem**: Vitte doesn't recognize your syntax.

**Solutions**:
1. Check [Language Reference](docs/language-spec/reference.md) for correct syntax
2. Review the error message carefully - it shows the unexpected token
3. Look at [Examples](examples/) for working code patterns
4. Check [Tutorials](docs/tutorials/) for your use case

**Common mistakes**:
```vitte
// ❌ WRONG: Missing -> in function signature
fn add(a: i32, b: i32) i32 {
    a + b
}

// ✓ CORRECT
fn add(a: i32, b: i32) -> i32 {
    a + b
}
```

### "Type mismatch" error

**Problem**: Variable types don't match expected types.

**Example error**:
```
error: type mismatch
expected: i32
found: f64
```

**Solutions**:
1. Check variable declarations match usage
2. Use type annotations when unclear:
   ```vitte
   let x: i32 = 5       // Explicit type
   let y = x + 3        // Type inferred
   let z: f64 = 5       // ERROR: 5 is i32, not f64
   let z: f64 = 5.0     // OK
   ```
3. Use `as` for explicit casting:
   ```vitte
   let x: i32 = 5
   let y: f64 = x as f64
   ```

### "Cannot borrow as mutable"

**Problem**: Trying to mutate something that's immutable.

**Example**:
```vitte
let x = 5
x = 10  // ERROR: x is immutable
```

**Solution**: Use `mut`:
```vitte
let mut x = 5
x = 10  // OK
```

### "Value moved" or "use after move"

**Problem**: Using a value after it's been moved.

**Example**:
```vitte
let s1 = String::from("hello")
let s2 = s1
print(s1)  // ERROR: s1 moved to s2
```

**Solutions**:
1. Use references to borrow instead of moving:
   ```vitte
   let s1 = String::from("hello")
   let s2 = &s1
   print(s1)  // OK
   ```
2. Clone the value:
   ```vitte
   let s1 = String::from("hello")
   let s2 = s1.clone()
   print(s1)  // OK
   ```

## Runtime Errors

### "Panicked at 'index out of bounds'"

**Problem**: Accessing an array/vector out of bounds.

**Example**:
```vitte
let arr = [1, 2, 3]
let x = arr[5]  // ERROR: index 5 doesn't exist
```

**Solutions**:
1. Check array bounds:
   ```vitte
   let arr = [1, 2, 3]
   if index < arr.len() {
       let x = arr[index]
   }
   ```
2. Use safe methods:
   ```vitte
   let arr = [1, 2, 3]
   match arr.get(5) {
       Some(val) => println!("{}", val),
       None => println!("Out of bounds"),
   }
   ```

### "Division by zero"

**Problem**: Dividing by zero causes a panic.

**Solution**: Check for zero:
```vitte
fn safe_divide(a: i32, b: i32) -> Result<i32, String> {
    if b == 0 {
        Err("Division by zero".to_string())
    } else {
        Ok(a / b)
    }
}
```

### "File not found"

**Problem**: Trying to open a non-existent file.

**Solution**: Handle the error:
```vitte
use std::fs::File

fn read_file(path: &str) -> Result<String, String> {
    match File::open(path) {
        Ok(file) => {
            // Read file
            Ok(contents)
        }
        Err(e) => {
            Err(format!("Cannot open {}: {}", path, e))
        }
    }
}
```

## Performance Issues

### Compilation takes too long

**Solutions**:
1. Use incremental compilation (if available)
2. Build in release mode:
   ```bash
   vittec main.vit -O3 -o main_optimized
   ```
3. Check for recursive/circular module dependencies

### Generated binary is large

**Causes & Solutions**:
1. **Debug symbols**: Strip them for release:
   ```bash
   strip main
   ```
2. **Not optimized**: Use `-O3` flag:
   ```bash
   vittec main.vit -O3 -o main
   ```
3. **Linking issues**: Check dependencies

## FAQ

### Can I use Vitte with C libraries?

Yes! See [FFI Example](examples/ffi_c/) for C interoperability.

### How do I debug my program?

**Options**:
1. Print debugging:
   ```vitte
   print("Debug: x = " + x)
   ```
2. Use GDB (if available):
   ```bash
   gdb ./my_program
   ```
3. Add assertions:
   ```vitte
   assert!(x > 0, "x must be positive")
   ```

### Why is my loop infinitely running?

Common causes:
```vitte
// ❌ WRONG: Infinite loop
loop {
    let x = 5
    // x never changes, condition never met
}

// ✓ CORRECT
let mut x = 0
while x < 10 {
    x = x + 1
}
```

### How do modules work?

See [Modules Tutorial](docs/tutorials/07-modules-and-packages.md) and [Example](examples/modules/).

### Where's the standard library documentation?

See [API Reference](docs/api/stdlib.md).

## Still Having Issues?

1. Check the [Language Reference](docs/language-spec/reference.md)
2. Review [Examples](examples/)
3. Search [GitHub Issues](https://github.com/vitte-lang/vitte/issues)
4. Ask on [Discord](https://discord.gg/vitte)
5. File a [bug report](https://github.com/vitte-lang/vitte/issues/new)

---

**Not finding what you need?** Help us improve by suggesting additions to this guide!
