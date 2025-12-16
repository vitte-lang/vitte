# SDK Templates Guide

Complete guide to using Vitte project templates.

## Available Templates

### 1. CLI (Command-Line Application)

Template for standalone command-line tools.

**Create**:
```bash
vitte-new my-cli --template=cli
cd my-cli
```

**Build & Run**:
```bash
vitte-build
./target/my-cli --help
```

**Perfect for**:
- System utilities
- Data processing tools
- Build tools
- Development utilities

**Example Files**:
- `src/main.vit` - Entry point with argument parsing
- `muffin.muf` - Project manifest

### 2. Library

Template for reusable code libraries.

**Create**:
```bash
vitte-new my-lib --template=lib
cd my-lib
```

**Build & Test**:
```bash
vitte-build
vitte-test
```

**Publish**:
```bash
vitte-publish my-lib 0.1.0
```

**Perfect for**:
- Algorithms & data structures
- File format parsers
- Encryption/hashing
- Utility functions
- Common abstractions

**Example Files**:
- `src/lib.vit` - Public API
- `tests/` - Test modules
- `examples/` - Usage examples

### 3. WebAssembly

Template for browser and WASM runtime applications.

**Create**:
```bash
vitte-new my-wasm --template=wasm
cd my-wasm
```

**Build**:
```bash
vitte-build --target wasm32-unknown-unknown
```

**Deploy**:
```bash
# Copy files to web host
cp target/wasm32-unknown-unknown/release/*.wasm dist/
cp index.html dist/
```

**Perfect for**:
- Browser applications
- WebAssembly libraries
- Computational tools
- Web-based IDEs/editors
- Game engines (web)

**Example Files**:
- `src/lib.vit` - WASM module
- `index.html` - Test/demo page
- `pkg/` - Generated bindings

**Browser Integration**:
```javascript
// Load and run WASM module
const wasm = await WebAssembly.instantiateStreaming(
  fetch('my_wasm.wasm'),
  {}
);
wasm.instance.exports.main();
```

### 4. Plugin

Template for dynamic loadable extensions.

**Create**:
```bash
vitte-new my-plugin --template=plugin
cd my-plugin
```

**Build**:
```bash
vitte-build --release
# Output: target/libmy_plugin.so/.dylib/.dll
```

**Install**:
```bash
mkdir -p ~/.vitte/plugins
cp target/release/libmy_plugin.* ~/.vitte/plugins/
```

**Perfect for**:
- Editor extensions
- Language plugins
- Compiler backends
- Application extensions
- Middleware modules

**Plugin Lifecycle**:
```vitte
// Initialize (called on load)
pub extern "C" fn plugin_init(ctx: *const PluginContext) -> PluginStatus { ... }

// Execute (called per operation)
pub extern "C" fn plugin_execute(name: *const i8, args: *const *const i8) -> i32 { ... }

// Cleanup (called on unload)
pub extern "C" fn plugin_deinit() { ... }
```

## Template Features

All templates include:

✅ **Proper Structure**: Organized source layout
✅ **Manifest**: Configured `muffin.muf`
✅ **Build Config**: `vitte.toml` settings
✅ **Examples**: Sample code (where applicable)
✅ **Tests**: Basic test structure
✅ **Documentation**: README with instructions
✅ **CI Ready**: GitHub Actions compatible

## Customization

### Rename Project

```bash
vitte-new old-name --template=cli
# Edit muffin.muf
# Change "old-name" to "new-name"
# Change package version if desired
```

### Add Dependencies

Edit `muffin.muf`:
```toml
[dependencies]
vitte = "0.2.0"
my-lib = "0.5.0"
another-lib = { version = "1.0", features = ["extra"] }
```

Then rebuild:
```bash
vitte-build
```

### Target Configuration

Edit `vitte.toml`:
```toml
[build]
optimization = "release"
lto = true
strip = true

[target.x86_64-linux-gnu]
linker = "gcc"

[target.aarch64-apple-darwin]
linker = "clang"
```

## Common Patterns

### CLI with Subcommands

```vitte
// src/main.vit
fn main() -> Result<(), Error> {
    let args = Args::parse()?;

    match args.command {
        Command::Build => build()?,
        Command::Test => test()?,
        Command::Deploy => deploy()?,
    }

    Ok(())
}
```

### Library with Documentation

```vitte
/// Calculate factorial
///
/// # Arguments
/// * `n` - Non-negative integer
///
/// # Returns
/// Factorial of n
pub fn factorial(n: usize) -> usize {
    match n {
        0 | 1 => 1,
        _ => n * factorial(n - 1)
    }
}
```

### WASM with JS Interop

```vitte
// src/lib.vit - Vitte
#[no_mangle]
pub extern "C" fn add(a: i32, b: i32) -> i32 {
    a + b
}
```

```javascript
// JavaScript
const { add } = wasm.instance.exports;
console.log(add(3, 4));  // 7
```

### Plugin with Metadata

```vitte
pub const PLUGIN_NAME: &str = "my-plugin"
pub const PLUGIN_VERSION: &str = "1.0.0"
pub const PLUGIN_API_VERSION: i32 = 1

#[no_mangle]
pub extern "C" fn plugin_info() -> *const i8 {
    b"my-plugin v1.0.0\0".as_ptr() as *const i8
}
```

## Cross-Compilation

Build for different target:

```bash
# From Linux, build for macOS
vitte-new my-app --template=cli
cd my-app
vitte-build --target x86_64-apple-darwin --cross

# From macOS, build for Linux
vitte-build --target x86_64-linux-gnu --cross
```

Requires:
- Target toolchain installed
- Sysroot for target platform
- Correct linker configuration

## Testing Templates

Each template includes test structure:

```bash
# Run all tests
vitte-test

# Run specific test
vitte-test my_test_name

# Generate coverage
vitte-test --coverage

# Profile tests
vitte-test --profile
```

## Publishing Templates

### Library Publishing

```bash
# Build library
vitte-build --release

# Create package
vitte-package my-lib-0.1.0.tar.gz

# Publish to registry
vitte-publish my-lib-0.1.0.tar.gz
```

### Binary Publishing

```bash
# Build binary
vitte-build --release

# Create installer (platform-specific)
# Windows: .msi, .exe
# macOS: .dmg
# Linux: .deb, .rpm

# Upload to release page
```

## Template Checklist

Before publishing:

- [ ] Tests passing: `vitte-test`
- [ ] Code formatted: `vitte-fmt`
- [ ] Linting clean: `vittec --lint`
- [ ] Documentation complete: `vitte-doc`
- [ ] Examples working
- [ ] Performance acceptable
- [ ] Cross-compilation verified
- [ ] Security reviewed

## Getting Started

1. Choose template matching your project type
2. Create project: `vitte-new NAME --template=TYPE`
3. Review `README.md` in template
4. Edit `muffin.muf` with your metadata
5. Implement functionality in `src/`
6. Write tests in `tests/`
7. Build: `vitte-build`
8. Test: `vitte-test`
9. Share!

## Template Locations

```
sdk/templates/
├── cli/                    # Command-line apps
├── lib/                    # Libraries
├── wasm/                   # WebAssembly
└── plugin/                 # Plugins
```

## See Also

- [SDK Documentation](SDK.md)
- [Building Guide](BUILDING.md)
- [FFI Guide](FFI.md)
- [Quick Reference](QUICK_REFERENCE.md)
