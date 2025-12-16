# Vitte SDK Quick Reference

Fast lookup guide for common SDK tasks.

## Quick Links

- **[SDK.md](SDK.md)** - Full documentation
- **[BUILDING.md](BUILDING.md)** - Build system guide
- **[FFI.md](FFI.md)** - C FFI integration
- **[TARGETS.md](TARGETS.md)** - Platform support
- **[ABI.md](ABI.md)** - Binary interface
- **[PACKAGING.md](PACKAGING.md)** - Distribution

## Environment Setup

```bash
# Add SDK to PATH
export VITTE_SDK=/path/to/sdk
export PATH="$VITTE_SDK/bin:$PATH"

# Verify installation
vittec --version
vitte-new --version
vitte-build --version
```

## Create New Project

```bash
# CLI app
vitte-new my-app --template=cli

# Library
vitte-new my-lib --template=lib

# WebAssembly
vitte-new my-wasm --template=wasm

# Plugin
vitte-new my-plugin --template=plugin
```

## Build Commands

```bash
# Debug build
vitte-build

# Release (optimized)
vitte-build --release

# Clean
vitte-build --clean

# Rebuild
vitte-build --clean && vitte-build

# Watch mode (rebuild on changes)
vitte-build --watch

# Specific target
vitte-build --target aarch64-unknown-linux-gnu

# Specific toolchain
vitte-build --toolchain gcc

# With sanitizer
vitte-build --sanitizer address

# LTO (Link-Time Optimization)
vitte-build --release --lto

# Minimal size
vitte-build --release --optimize=minimal
```

## Testing

```bash
# Run tests
vitte-test

# Run with verbose output
vitte-test --verbose

# Run specific test
vitte-test my_test_name

# Generate coverage report
vitte-test --coverage
```

## Code Quality

```bash
# Format code
vitte-fmt

# Check formatting
vitte-fmt --check

# Lint code
vittec --lint main.vit

# Generate documentation
vitte-doc --output=target/doc
```

## Supported Targets

**Tier 1 (Production)**:
- x86_64-linux-gnu
- aarch64-apple-darwin
- x86_64-apple-darwin
- x86_64-pc-windows-gnu
- wasm32-unknown-unknown

**Tier 2 (Stable)**:
- aarch64-unknown-linux-gnu
- armv7-unknown-linux-gnueabihf
- i686-unknown-linux-gnu

**Tier 3 (Experimental)**:
- riscv64gc-unknown-linux-gnu
- thumbv7em-none-eabihf

## Common Headers

```c
// Core types and utilities
#include <vitte/core.h>

// Runtime interface
#include <vitte/runtime.h>

// Platform abstraction
#include <vitte/platform.h>

// Memory allocation
#include <vitte/alloc.h>
```

## C FFI Patterns

### Call C from Vitte

```vitte
extern "C" fn strlen(s: *const i8) -> usize

fn main() {
    let s = "hello\0";
    let len = strlen(s.as_ptr());
    println!("{}", len);  // 5
}
```

### Export Vitte Function to C

```vitte
#[no_mangle]
pub extern "C" fn add(a: i32, b: i32) -> i32 {
    a + b
}
```

### C calling exported function

```c
extern int add(int a, int b);

int main() {
    int result = add(3, 4);  // 7
    return 0;
}
```

## Configuration

Create `vitte.toml` in project root:

```toml
[package]
name = "my-app"
version = "0.1.0"

[build]
optimization = "release"
lto = true
strip = true

[target.x86_64-linux-gnu]
linker = "gcc"

[features]
default = ["std", "alloc"]

[dependencies]
vitte = "0.2.0"
```

## Directory Structure

```
my-app/
├── muffin.muf          # Project manifest
├── vitte.toml          # Vitte-specific config
├── src/
│   └── main.vit        # Entry point
├── lib/
│   └── lib.vit         # Library (if library)
├── tests/
│   └── test_*.vit      # Tests
├── examples/
│   └── example.vit     # Examples
└── target/
    └── my-app          # Output binary
```

## Troubleshooting

### Compiler not found
```bash
export VITTE_SDK=/correct/path
export PATH="$VITTE_SDK/bin:$PATH"
which vittec
```

### Build errors
```bash
# Clean and rebuild
vitte-build --clean
vitte-build --verbose

# Check target availability
vittec --list-targets
```

### Linking errors
```bash
# Check available libraries
pkg-config --cflags --libs vitte

# Verify sysroot
echo $VITTE_SDK/sysroot
ls $VITTE_SDK/sysroot/lib
```

### Cross-compilation issues
```bash
# Verify target exists
vittec --list-targets | grep target-name

# Use correct toolchain
vitte-build --target x86_64-linux-gnu --toolchain gcc
```

## Package Management

```bash
# Install binary release
curl -fsSL https://install.vitte.sh | sh

# Via package manager
brew install vitte           # macOS
sudo apt install vitte      # Debian/Ubuntu
choco install vitte         # Windows
```

## Development Tips

1. **Always use latest toolchain**: `vitte-build --upgrade-toolchain`
2. **Enable LTO for production**: `vitte-build --release --lto`
3. **Use address sanitizer during development**: `vitte-build --sanitizer address`
4. **Test on target platform before release**
5. **Use pkg-config for library discovery**: `pkg-config --cflags vitte`

## Performance Profiling

```bash
# Generate profiling data
vitte-build --profile

# Analyze with perf (Linux)
perf report

# Analyze with Instruments (macOS)
instruments -t "Time Profiler" ./my-app
```

## Version Info

SDK: 0.2.0
Language: Vitte 0.2.0+
Backend: C17
Targets: 14+ platforms

## Getting Help

- 📖 Full docs: [docs/SDK.md](SDK.md)
- 🐛 Issues: https://github.com/vitte-lang/vitte/issues
- 💬 Discussions: https://github.com/vitte-lang/vitte/discussions
- 📧 Email: support@vitte.dev
