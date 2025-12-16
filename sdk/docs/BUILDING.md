# Building with Vitte SDK

Guide for building Vitte projects using the SDK.

## Prerequisites

- Vitte SDK installed
- C compiler (GCC, Clang, or MSVC)
- Platform-specific build tools (Make, Ninja, or CMake)

## Project Structure

```
my-project/
├── muffin.muf           # Package manifest
├── src/
│   ├── main.vit         # Entry point
│   └── lib.vit          # Library code
├── tests/
│   └── tests.vit        # Test files
├── examples/
│   └── example.vit
├── build/               # Generated
└── target/              # Compilation output
```

## Building

### Using vittec Directly

```bash
# Simple compilation
vittec src/main.vit -o my-app

# With optimization
vittec src/main.vit -O3 -o my-app

# Emit intermediate representations
vittec src/main.vit --emit=c17,ir
```

### Using vitte-build

```bash
# Build according to muffin.muf
vitte-build

# Build with optimizations
vitte-build --release

# Clean build artifacts
vitte-build clean

# Watch for changes and rebuild
vitte-build watch
```

### Using CMake

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Compilation Targets

### Linux

```bash
vittec main.vit --target x86_64-linux-gnu -o main
./main
```

### macOS

```bash
vittec main.vit --target x86_64-apple-darwin -o main
./main
```

### Windows

```bash
vittec main.vit --target x86_64-windows-gnu -o main.exe
./main.exe
```

### WebAssembly

```bash
vittec main.vit --target wasm32-unknown-unknown -o main.wasm
# Run with wasmtime or web runtime
wasmtime main.wasm
```

### Embedded (ARM Cortex-M4)

```bash
vittec main.vit --target thumbv7em-none-eabihf -o main.elf
# Flash with appropriate tool
```

## Conditional Compilation

Use features to enable/disable code:

```vitte
#[cfg(feature = "debug")]
fn debug_log(msg: &str) {
    print(msg)
}

#[cfg(target_os = "windows")]
fn get_home() -> String {
    std::env::var("USERPROFILE").unwrap_or(".".to_string())
}

#[cfg(target_os = "unix")]
fn get_home() -> String {
    std::env::var("HOME").unwrap_or(".".to_string())
}
```

Enable features during build:

```bash
vittec main.vit --features=debug,logging
```

## Release Builds

Optimize for production:

```bash
# Release with maximum optimization
vitte-build --release

# Or directly with vittec
vittec main.vit -O3 --lto -o main
```

Profile settings in muffin.muf:

```toml
[profile.release]
opt-level = 3
debug = false
lto = true
codegen-units = 1
strip = true
```

## Linking and Dependencies

### Static Linking

```bash
vittec main.vit --static -o main
```

### Dynamic Linking

```bash
vittec main.vit --dynamic -o main
```

### With External Libraries

```bash
vittec main.vit -lm -lpthread -o main
```

## Testing

```bash
# Run all tests
vitte-test

# Run specific test
vitte-test my_module_test

# With output
vitte-test -- --nocapture

# Coverage
vitte-test --coverage
```

## Debugging

### Debug Builds

```bash
# Include debug symbols
vittec main.vit -g -o main

# With debugging optimizations
vittec main.vit -g -O1 -o main
```

### GDB

```bash
# Run with GDB
gdb ./main

# In GDB:
(gdb) break main
(gdb) run
(gdb) print x
(gdb) next
(gdb) quit
```

### Printing Debug Info

```vitte
import std::dbg

fn main() {
    let x = 42
    dbg!(x)  // Prints: x = 42
    dbg!(x + 1)
}
```

## Performance Optimization

### Compiler Flags

```bash
# Optimize for speed
vittec main.vit -O3 --lto -o main

# Optimize for size
vittec main.vit -Os -o main

# Optimize for debugging
vittec main.vit -O0 -g -o main
```

### Profiling

```bash
# Generate profile data
vittec main.vit --profile=cpu -o main
./main

# Analyze with perf
perf report
```

## Cross-Compilation

### Setting up Cross Toolchain

```bash
# Download and setup toolchain
vitte-toolchain install aarch64-linux-gnu

# Build for target
vittec main.vit --target aarch64-linux-gnu -o main
```

### Using Custom Sysroot

```bash
vittec main.vit --sysroot=/path/to/sysroot --target custom-target -o main
```

## Troubleshooting

### "Linker error: undefined reference"

**Cause**: Missing library or function

**Solution**:
```bash
# Link against library
vittec main.vit -lname_of_lib -o main

# Or specify library path
vittec main.vit -L/path/to/lib -lname_of_lib -o main
```

### "Symbol not found in dynamic lookup"

**Cause**: Missing shared library at runtime

**Solution**:
```bash
# Check dependencies
ldd ./main  # Linux
otool -L ./main  # macOS
dumpbin /dependents main.exe  # Windows

# Set library path
export LD_LIBRARY_PATH=/path/to/libs:$LD_LIBRARY_PATH
./main
```

### "Cannot find header file"

**Cause**: Missing include paths

**Solution**:
```bash
vittec main.vit -I/path/to/headers -o main
```

## Best Practices

1. **Use release builds for production** - Significantly faster and smaller
2. **Enable LTO** - Link-time optimization improves performance
3. **Strip debug symbols** - Reduces binary size
4. **Use conditional compilation** - Platform-specific optimizations
5. **Profile before optimizing** - Identify actual bottlenecks
6. **Test on target platform** - Catch platform-specific issues early
7. **Version your SDK** - Ensure reproducible builds

## See Also

- [ABI.md](ABI.md) - Application Binary Interface
- [TARGETS.md](TARGETS.md) - Supported targets
- [FFI.md](FFI.md) - C FFI integration
- [../CONTRIBUTING.md](../CONTRIBUTING.md) - Contributing guide
