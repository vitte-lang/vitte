# Vitte SDK - Complete Documentation

The Vitte SDK provides everything needed to build, compile, and distribute applications written in Vitte.

## Overview

The SDK includes:
- **Compiler toolchain** - vittec and supporting tools
- **Sysroot** - Headers, libraries, and system definitions
- **Target configurations** - Multi-platform compilation specs
- **Templates** - Project and module scaffolding
- **Documentation** - API references and guides

## Quick Start

### Setting up the SDK

```bash
# Set SDK environment variable
export VITTE_SDK=/path/to/vitte/sdk

# Verify installation
vittec --version
vitte-config --version
```

### Creating a new project

```bash
# Using the template system
vitte-new my-app --template=cli
cd my-app

# Or manually
mkdir my-app
cd my-app
cat > muffin.muf << 'EOF'
[package]
name = "my-app"
version = "0.1.0"
edition = "2024"

[dependencies]
EOF

# Create source directory
mkdir src
cat > src/main.vit << 'EOF'
fn main() {
    print("Hello, Vitte!")
}
EOF

# Build
vittec muffin.muf --output my-app
./my-app
```

## Directory Structure

```
sdk/
├── bin/                      # Executables and tools
│   ├── vittec               # Main compiler
│   ├── vitte-config         # SDK configuration utility
│   ├── vitte-new            # Project scaffolder
│   ├── vitte-build          # Build system wrapper
│   ├── vitte-fmt            # Code formatter
│   ├── vitte-doc            # Documentation generator
│   ├── vitte-test           # Test runner
│   └── vitte-lsp            # Language Server Protocol
│
├── sysroot/                 # System root (ABI stable)
│   ├── include/vitte/       # Public headers
│   │   ├── core.h           # Core types and functions
│   │   ├── runtime.h        # Runtime ABI
│   │   ├── alloc.h          # Memory allocation
│   │   └── platform.h       # Platform abstractions
│   ├── lib/vitte/           # Libraries (static & shared)
│   ├── lib/pkgconfig/       # pkg-config files
│   └── share/vitte/
│       ├── targets/         # Target configurations (JSON)
│       ├── specs/           # Language specifications
│       └── templates/       # Project templates
│
├── toolchains/              # Compiler toolchains
│   ├── gcc/                 # GCC configuration
│   ├── clang/               # Clang configuration
│   └── msvc/                # MSVC configuration
│
├── config/                  # SDK configuration files
│   ├── defaults.toml        # Default settings
│   ├── platforms.toml       # Platform definitions
│   └── features.toml        # Feature flags
│
├── templates/               # Project templates
│   ├── cli/                 # Command-line application
│   ├── lib/                 # Library project
│   ├── wasm/                # WebAssembly project
│   └── plugin/              # Plugin/extension
│
├── docs/                    # Documentation
│   ├── BUILDING.md          # Build instructions
│   ├── PACKAGING.md         # Packaging guide
│   ├── TARGETS.md           # Target platform guide
│   ├── FFI.md               # C FFI guide
│   └── ABI.md               # ABI specification
│
├── scripts/                 # Utility scripts
├── mod.muf                  # SDK manifest
├── README.md                # This file
├── VERSION                  # Version number
└── LICENSE                  # License
```

## Key Features

### Multi-Platform Support

The SDK supports compilation to multiple platforms:
- Linux (x86_64, ARM, RISC-V)
- macOS (x86_64, ARM64)
- Windows (x86_64, x86)
- WebAssembly (wasm32-unknown-unknown)
- Embedded (ARM Cortex-M, RISC-V)

Configure with:
```bash
vittec --target x86_64-linux-gnu main.vit
vittec --target aarch64-apple-darwin main.vit
vittec --target wasm32-unknown-unknown main.vit
```

### C FFI Integration

Easily call C code and expose Vitte to C:

```vitte
// Call C function
extern "C" fn strlen(s: *const i8) -> usize

// Expose to C
#[no_mangle]
pub fn my_vitte_function(x: i32) -> i32 {
    x * 2
}
```

See [FFI.md](docs/FFI.md) for details.

### Package Configuration

Use `muffin.muf` to configure your project:

```toml
[package]
name = "my-app"
version = "0.1.0"
edition = "2024"
authors = ["Your Name <email@example.com>"]
license = "Apache-2.0"
repository = "https://github.com/user/repo"

[dependencies]
std = "0.2"

[features]
default = ["std"]
minimal = []

[profile.release]
opt-level = 3
lto = true
```

## Environment Variables

```bash
VITTE_SDK              # Path to SDK (auto-detected or set manually)
VITTE_TARGET           # Target platform (x86_64-linux-gnu, etc.)
VITTE_SYSROOT          # Custom sysroot path
VITTE_CC               # C compiler override
VITTE_CFLAGS           # Additional C compiler flags
VITTE_LDFLAGS          # Additional linker flags
VITTE_RUST_BACKTRACE   # Rust backtrace for debugging
```

## Tools

### vittec - Main Compiler

```bash
# Compile to default target
vittec main.vit

# Compile with optimization
vittec main.vit -O3

# Cross-compile
vittec main.vit --target aarch64-linux-gnu

# Output specific format
vittec main.vit --emit=c17    # Emit C17 code
vittec main.vit --emit=ir     # Emit IR
```

### vitte-new - Project Scaffolder

```bash
# Create CLI application
vitte-new my-cli --template=cli

# Create library
vitte-new my-lib --template=lib

# Create WebAssembly project
vitte-new my-wasm --template=wasm
```

### vitte-fmt - Code Formatter

```bash
# Format all files in project
vitte-fmt

# Format specific file
vitte-fmt src/main.vit

# Check without modifying
vitte-fmt --check
```

### vitte-doc - Documentation Generator

```bash
# Generate docs from comments
vitte-doc --output=docs/

# Serve docs locally
vitte-doc --serve
```

### vitte-test - Test Runner

```bash
# Run all tests
vitte-test

# Run specific test
vitte-test fibonacci_test

# Show test output
vitte-test -- --nocapture
```

## ABI and Runtime

The Vitte ABI defines:
- Function calling conventions
- Data layout and alignment
- Error handling with Result types
- Memory management semantics

See [ABI.md](docs/ABI.md) for detailed specifications.

### Runtime Support

The SDK includes minimal runtime support:
- Panic handling
- Memory management (allocation, deallocation)
- Platform abstraction (PAL)
- Error handling primitives

## Building the SDK

To rebuild the SDK from source:

```bash
cd vitte
./scripts/bootstrap_stage0.sh    # Stage 0: Host compiler
./scripts/self_host_stage1.sh    # Stage 1-2: Self-hosting
make install-sdk
```

## Cross-Compilation

Set up toolchains for cross-compilation:

```bash
# Linux to Windows
vittec main.vit --target x86_64-windows-gnu

# Linux to macOS (requires cross-compiler setup)
vittec main.vit --target x86_64-apple-darwin

# To ARM embedded
vittec main.vit --target armv7-unknown-linux-gnueabihf
```

## Integration with Build Systems

### CMake

```cmake
find_package(Vitte REQUIRED)
vitte_compile_target(my_app SOURCES src/main.vit)
target_link_libraries(my_app vitte_runtime)
```

### Make

```makefile
VITTEC ?= vittec
TARGET ?= x86_64-linux-gnu

%.o: %.vit
	$(VITTEC) --target $(TARGET) -c $< -o $@
```

## Contributing to the SDK

To contribute:

1. Clone the repository
2. Make changes in the appropriate directory
3. Test with `make test-sdk`
4. Submit a pull request

See [CONTRIBUTING.md](../CONTRIBUTING.md) for guidelines.

## Getting Help

- **Tutorials**: See [docs/tutorials/](../docs/tutorials/)
- **Language Reference**: [docs/language-spec/reference.md](../docs/language-spec/reference.md)
- **API Reference**: [docs/api/stdlib.md](../docs/api/stdlib.md)
- **Examples**: [examples/](../examples/)
- **Issues**: [GitHub Issues](https://github.com/vitte-lang/vitte/issues)
- **Community**: [Discord](https://discord.gg/vitte)

## License

The SDK is licensed under the Apache License 2.0. See [LICENSE](LICENSE) for details.
