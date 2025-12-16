# Vitte SDK Specification

Version: 0.2.0
Date: 2024
Status: Stable

## Overview

The Vitte SDK is the complete development toolkit for building applications with the Vitte programming language. It provides:

- C17 backend compilation
- ABI-stable C interfaces (4 headers)
- Multi-platform support (14+ targets)
- Project templates (4 types)
- Toolchain integration (3 compilers)
- Comprehensive documentation
- Build automation

## Components

### 1. Compiler (vittec)

**Purpose**: Translate Vitte source to C17

**Input**: `.vit` source files
**Output**: `.c` files → compiled binaries

**Features**:
- Type inference
- Memory safety
- Pattern matching
- Generics
- Error handling (Result types)
- FFI support

### 2. Build System (vitte-build)

**Purpose**: Automate compilation and linking

**Capabilities**:
- Multi-target compilation
- Incremental builds
- LTO support
- Sanitizer integration
- Watch mode
- Cross-compilation

### 3. ABI Headers

**core.h**: Fundamental types and initialization
- Version information
- Type definitions (unit, option, result, str, slice)
- Error handling (panic, Result)
- Memory management (initialization)

**runtime.h**: Runtime support
- Entry point callbacks
- String utilities
- Debug support
- Backtrace
- Benchmarking
- Type information

**platform.h**: OS abstraction
- File I/O (open, read, write, close)
- Directory operations
- Process management
- Time functions
- Thread support
- Dynamic loading
- Synchronization primitives

**alloc.h**: Memory management
- Allocator interface
- Arena allocator
- Memory tracking
- Statistics

### 4. Target Configuration

**Specification**: JSON format

**Required Fields**:
```json
{
  "name": "target-triple",
  "architecture": "x86_64",
  "os": "linux",
  "pointer_width": 64,
  "llvm_target": "x86_64-unknown-linux-gnu",
  "linker": "gcc",
  "tier": 1
}
```

**Tier Definitions**:
- **Tier 1**: Production-ready, regularly tested
- **Tier 2**: Stable, less frequent testing
- **Tier 3**: Experimental, minimal support

### 5. Templates

**CLI**: Command-line application
- Entry point: `fn main() -> Result<(), Error>`
- Argument parsing examples
- Error handling patterns

**Library**: Reusable code module
- Public module exports
- Documentation examples
- Test patterns

**WebAssembly**: Browser/WASM runtime
- Web API bindings
- Binary size optimization
- JavaScript interop examples

**Plugin**: Extensible architecture
- Dynamic loading symbols
- Plugin lifecycle (init, execute, deinit)
- Shared library format

### 6. Toolchain Configuration

**GCC**: Linux, ARM, embedded targets
**Clang**: Multi-platform, LTO/ThinLTO
**MSVC**: Windows native development

**Features**:
- Automatic detection
- Per-target configuration
- Optimization profiles
- Sanitizer support

## ABI Stability Guarantee

The 4 C headers provide a stable ABI with the following guarantees:

1. **Forward Compatibility**: Code compiled against SDK version X will work with runtime version X+1
2. **Data Layout**: Structure layouts are fixed (no padding additions)
3. **Function Signatures**: Function signatures are immutable
4. **Versioning**: Breaking changes increment major version number

## Supported Platforms

**Total Targets**: 14+

### Linux

- **x86_64-linux-gnu** (Tier 1)
- **aarch64-unknown-linux-gnu** (Tier 2)
- **armv7-unknown-linux-gnueabihf** (Tier 2)
- **i686-unknown-linux-gnu** (Tier 2)
- **riscv64gc-unknown-linux-gnu** (Tier 2)

### macOS

- **x86_64-apple-darwin** (Tier 1)
- **aarch64-apple-darwin** (Tier 1)

### Windows

- **x86_64-pc-windows-gnu** (Tier 1)
- **i686-pc-windows-msvc** (Tier 2)

### WebAssembly

- **wasm32-unknown-unknown** (Tier 1)

### Embedded

- **thumbv7em-none-eabihf** (Cortex-M4/M7, Tier 3)

## Feature Flags

```toml
[features]
std = []          # Standard library
alloc = []        # Dynamic allocation
ffi = []          # C FFI support
threads = []      # Threading
network = []      # Networking
filesystem = []   # File I/O
process = []      # Process management
collections = []  # Data structures
simd = []         # SIMD support
debug = []        # Debug symbols
panic_abort = []  # Panic behavior
lto = []          # Link-time optimization
```

## Performance Tiers

### Optimization Profiles

| Profile | Flags | Use Case |
|---------|-------|----------|
| debug | `-Og -g3` | Development, debugging |
| release | `-O3` | Production binaries |
| lto | `-O3 -flto=full` | Maximum performance |
| minimal | `-Os` | Embedded, size-critical |

### Benchmarks (Reference)

Compilation:
- Simple program: < 1s
- Medium project: 5-10s
- Large project: 30-60s

Runtime:
- Comparable to C17 compiled code
- Minimal overhead from safety checks

## Configuration Files

### `defaults.toml`

Global SDK defaults (compiler, diagnostics, runtime)

### `platforms.toml`

Platform definitions for 14+ targets with capabilities

### `features.toml`

Feature flag system and profiles

### `config.toml` (per project)

Project-specific build configuration

## Build Process

1. **Compilation**: `.vit` → C17
2. **Preprocessing**: C macro expansion
3. **C Compilation**: C17 → Assembly
4. **Linking**: Assembly + libraries → Binary
5. **Optimization**: LTO, stripping (optional)
6. **Output**: Executable or library

## Distribution

### Supported Formats

- **Linux**: ELF executable, `.so` shared object
- **macOS**: Mach-O executable, `.dylib`
- **Windows**: PE executable, `.dll`
- **WebAssembly**: `.wasm` module
- **Embedded**: `.elf`, `.hex`

### Packaging

- Source distribution: `.tar.gz`, `.zip`
- Binary distribution: Platform-specific installers
- Container: Docker images
- Package managers: apt, brew, choco

## Version Compatibility

- **SDK 0.2.0**: Compatible with Vitte 0.2.0+
- **Break policy**: Major version changes indicate breaking changes
- **Stability**: Minor versions are backward compatible

## Error Handling

### Compilation Errors

- **Syntax**: Compile-time detection
- **Type**: Type checker validation
- **Borrow**: Memory safety verification

### Runtime Errors

- **Panics**: Catchable via Result types
- **Segfaults**: Memory protection (when available)
- **Stack overflow**: Detection and clean termination

## Security Considerations

1. **Memory Safety**: Enforced at compile time
2. **Type Safety**: No undefined behavior
3. **FFI Safety**: Unsafe blocks require explicit annotation
4. **Input Validation**: Bounds checking enabled by default
5. **Stack Protection**: Enabled in default builds

## Conformance

The SDK conforms to:
- C17 standard for generated code
- POSIX for system interfaces
- DWARF for debug information
- Mach-O/ELF/PE for binary formats

## Future Extensions

Planned features:
- SIMD support improvements
- GPU compute integration
- Incremental compilation
- Distributed compilation
- IDE plugins
- LSP improvements
