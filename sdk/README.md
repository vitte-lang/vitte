# Vitte SDK

The Vitte SDK provides everything needed to build, compile, test, and distribute applications written in Vitte.

## Quick Start

```bash
# Set environment
export VITTE_SDK=/path/to/vitte/sdk
export PATH="$VITTE_SDK/bin:$PATH"

# Create new project
vitte-new my-app --template=cli

# Build
cd my-app
vitte-build

# Run
./target/my-app
```

## Contents

### Toolchain (bin/)

- `vittec` - Main Vitte compiler
- `vitte-new` - Project scaffolder
- `vitte-build` - Build automation
- `vitte-fmt` - Code formatter
- `vitte-doc` - Documentation generator
- `vitte-test` - Test runner
- `vitte-lsp` - Language Server Protocol

### System Root (sysroot/)

**Headers** (ABI stable):
- `include/vitte/core.h` - Core types, initialization
- `include/vitte/runtime.h` - Runtime support
- `include/vitte/platform.h` - Platform abstraction (file I/O, threads, etc.)
- `include/vitte/alloc.h` - Custom memory allocators

**Libraries**:
- `lib/vitte/` - Runtime and core libraries
- `lib/pkgconfig/vitte.pc` - pkg-config metadata

**Configurations**:
- `share/vitte/targets/` - 10+ target configurations (JSON)
- `share/vitte/specs/` - Language specifications
- `share/vitte/templates/` - Project templates

### Configuration (config/)

- `defaults.toml` - SDK defaults
- `platforms.toml` - Platform definitions (14 supported targets)
- `features.toml` - Feature flags and profiles

### Templates (templates/)

- `cli/` - Command-line application
- `lib/` - Reusable library
- `wasm/` - WebAssembly module
- `plugin/` - Plugin/extension architecture

### Documentation (docs/)

- `SDK.md` - Complete SDK documentation
- `BUILDING.md` - Build instructions for all scenarios
- `FFI.md` - C FFI integration guide
- `TARGETS.md` - Supported platforms and cross-compilation
- `ABI.md` - Binary interface specification
- `PACKAGING.md` - Distribution and packaging guide
- `QUICK_REFERENCE.md` - Fast lookup reference
- `SPECIFICATION.md` - Formal SDK specification
- `TEMPLATES.md` - Project template guide

## Supported Platforms

**Tier 1 (Production-ready)**:
- ✅ Linux x86_64
- ✅ macOS x86_64 & ARM64
- ✅ Windows x86_64
- ✅ WebAssembly

**Tier 2 (Stable)**:
- Linux ARM64, ARM32
- Linux x86
- Various embedded ARM targets

**Tier 3 (Experimental)**:
- RISC-V, Cortex-M33

## Key Features

### Multi-Platform
Compile for 10+ platforms with a single source:
```bash
vittec main.vit --target x86_64-linux-gnu
vittec main.vit --target aarch64-apple-darwin
vittec main.vit --target wasm32-unknown-unknown
```

### C FFI
Call C from Vitte and vice versa:
```vitte
extern "C" fn strlen(s: *const i8) -> usize

#[no_mangle]
pub extern "C" fn add(a: i32, b: i32) -> i32 {
    a + b
}
```

### Project Templates
Scaffolds with best practices:
```bash
vitte-new cli-app --template=cli
vitte-new my-lib --template=lib
vitte-new wasm-module --template=wasm
```

### ABI Stable Headers
4 C headers defining stable ABI for:
- Core types & initialization
- Runtime support
- Platform abstractions
- Memory management

### Comprehensive Build System
```bash
vitte-build              # Debug build
vitte-build --release   # Optimized release
vitte-build --watch     # Rebuild on changes
vitte-build --clean     # Clean artifacts
```

## Environment Variables

```bash
VITTE_SDK              # SDK path (auto-detected)
VITTE_TARGET           # Override target platform
VITTE_SYSROOT          # Custom sysroot
VITTE_CC               # C compiler override
VITTE_CFLAGS           # Extra C flags
VITTE_LDFLAGS          # Extra linker flags
```

## File Structure

```
sdk/
├── bin/                    # Tools
├── sysroot/
│   ├── include/vitte/      # ABI headers (4 files)
│   ├── lib/vitte/          # Libraries
│   ├── lib/pkgconfig/      # pkg-config
│   └── share/vitte/
│       ├── targets/        # Target configs (5+ JSON)
│       ├── specs/          # Language specs
│       └── templates/      # Project templates
├── config/                 # Configuration (3 files)
├── templates/              # Templates (cli, lib, wasm, plugin)
├── docs/                   # Documentation (6 files)
└── toolchains/             # Compiler configs (gcc, clang, msvc)
```

## Quick Documentation

For detailed guides, see:
- [docs/SDK.md](docs/SDK.md) - Full SDK documentation
- [docs/BUILDING.md](docs/BUILDING.md) - Build guide
- [docs/FFI.md](docs/FFI.md) - C FFI guide
- [docs/TARGETS.md](docs/TARGETS.md) - Platforms & cross-compilation
- [docs/ABI.md](docs/ABI.md) - Binary interface specification
- [docs/PACKAGING.md](docs/PACKAGING.md) - Packaging & distribution
- [docs/QUICK_REFERENCE.md](docs/QUICK_REFERENCE.md) - Quick lookup
- [docs/SPECIFICATION.md](docs/SPECIFICATION.md) - Formal specification
- [docs/TEMPLATES.md](docs/TEMPLATES.md) - Project templates

## Version

SDK Version: 0.2.0
Compatible with: Vitte 0.2.0+

## License

Apache License 2.0 - See [LICENSE](LICENSE)
