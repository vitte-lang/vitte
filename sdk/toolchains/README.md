# Vitte Toolchain Configuration

This directory contains configuration for C toolchains (compilers, linkers, runtime libraries) used by Vitte.

## Supported Toolchains

### GCC (Linux, embedded)
- **Directory**: `gcc/`
- **Configuration**: `gcc/config.toml`
- **Supported Targets**:
  - x86_64-linux-gnu
  - aarch64-linux-gnu
  - armv7-linux-gnueabihf
  - riscv64-linux-gnu

### Clang/LLVM (Cross-platform)
- **Directory**: `clang/`
- **Configuration**: `clang/config.toml`
- **Supported Targets**:
  - All Linux targets
  - macOS (x86_64, ARM64)
  - WebAssembly (wasm32-unknown-unknown)

### Microsoft Visual C++ (Windows)
- **Directory**: `msvc/`
- **Configuration**: `msvc/config.toml`
- **Supported Targets**:
  - x86_64-pc-windows-msvc
  - i686-pc-windows-msvc

## Configuration Files

### `config.toml`
Global toolchain management configuration:
- Default toolchain selection per platform
- Toolchain detection and paths
- Compiler cache settings
- Cross-compilation setup
- Optimization profiles
- Sanitizer configurations

### `gcc/config.toml`
GCC-specific configuration:
- Compiler flags (base, debug, release, LTO)
- Linker configuration
- Runtime library settings
- Target-specific overrides
- Feature support (LTO, PGO, sanitizers)

### `clang/config.toml`
Clang/LLVM-specific configuration:
- Compiler flags
- LLD linker configuration
- Runtime library choices (libunwind, libc++)
- Multiple target support
- ThinLTO and advanced features

### `msvc/config.toml`
MSVC-specific configuration:
- MSVC compiler flags
- link.exe linker settings
- Runtime library selection
- Windows-specific features
- Incremental linking

## Environment Variables

```bash
# Override toolchain paths
export GCC_PATH=/custom/gcc/path
export CLANG_PATH=/custom/clang/path
export MSVC_PATH="C:\CustomMSVC"

# Cross-compilation
export SYSROOT=/path/to/sysroot
export VITTE_TARGET=aarch64-unknown-linux-gnu
export VITTE_CFLAGS="-march=native"
export VITTE_LDFLAGS="-Wl,-rpath=/custom/lib"
```

## Adding Custom Toolchain

1. Create new directory: `custom/`
2. Create `custom/config.toml` with configuration
3. Update global `config.toml` to reference new toolchain
4. Verify with: `vitte-build --toolchain=custom`

## Usage Examples

### Use GCC for Linux build
```bash
vitte-build --target x86_64-linux-gnu --toolchain gcc
```

### Use Clang for release build
```bash
vitte-build --release --toolchain clang
```

### Use MSVC for Windows
```bash
vitte-build --target x86_64-pc-windows-msvc --toolchain msvc
```

### Cross-compile for ARM64
```bash
vitte-build --target aarch64-linux-gnu --toolchain gcc --cross
```

## Optimization Profiles

The global `config.toml` defines optimization profiles:

- **debug**: `-g3 -Og` (debugging with optimizations)
- **release**: `-O3 -DNDEBUG` (fully optimized)
- **lto**: `-O3 -flto=full` (Link-Time Optimization)
- **minimal**: `-Os` (minimal size)

## Sanitizers

Built-in sanitizer configurations:
- **address**: AddressSanitizer (ASan)
- **thread**: ThreadSanitizer (TSan)
- **memory**: MemorySanitizer (MSan)
- **undefined**: UndefinedBehaviorSanitizer (UBSan)

Build with sanitizer:
```bash
vitte-build --sanitizer address
```

## See Also

- [SDK Documentation](../docs/SDK.md)
- [Building Guide](../docs/BUILDING.md)
- [Targets Guide](../docs/TARGETS.md)
