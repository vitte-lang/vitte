# Target Platforms

Supported compilation targets for Vitte.

## Target Naming

Targets follow the format: `{arch}-{vendor}-{os}-{env}`

Examples:
- `x86_64-linux-gnu` - Linux on x86_64
- `x86_64-apple-darwin` - macOS on x86_64
- `aarch64-apple-darwin` - macOS on ARM64
- `wasm32-unknown-unknown` - WebAssembly

## Supported Targets

### Desktop

| Target | OS | Architecture | Support |
|--------|-------|--------------|---------|
| `x86_64-linux-gnu` | Linux | x86_64 | ✅ Tier 1 |
| `x86_64-linux-musl` | Linux | x86_64 | ✅ Tier 1 |
| `x86_64-apple-darwin` | macOS | x86_64 | ✅ Tier 1 |
| `aarch64-apple-darwin` | macOS | ARM64 | ✅ Tier 1 |
| `x86_64-pc-windows-gnu` | Windows | x86_64 | ✅ Tier 1 |
| `x86_64-pc-windows-msvc` | Windows | x86_64 | ✅ Tier 2 |
| `i686-linux-gnu` | Linux | x86 | ✅ Tier 2 |
| `i686-pc-windows-gnu` | Windows | x86 | ✅ Tier 2 |
| `aarch64-linux-gnu` | Linux | ARM64 | ✅ Tier 2 |
| `armv7-linux-gnueabihf` | Linux | ARMv7 | ✅ Tier 2 |

### WebAssembly

| Target | Environment | Support |
|--------|-------------|---------|
| `wasm32-unknown-unknown` | Bare | ✅ Tier 1 |
| `wasm32-wasi` | WASI | ✅ Tier 2 |
| `wasm32-unknown-emscripten` | Emscripten | 🟡 Tier 3 |

### Embedded

| Target | CPU | Support |
|--------|-----|---------|
| `thumbv6m-none-eabi` | Cortex-M0 | ✅ Tier 2 |
| `thumbv7m-none-eabi` | Cortex-M3 | ✅ Tier 2 |
| `thumbv7em-none-eabi` | Cortex-M4 | ✅ Tier 2 |
| `thumbv7em-none-eabihf` | Cortex-M4F | ✅ Tier 2 |
| `thumbv8m.base-none-eabi` | Cortex-M33 | 🟡 Tier 3 |
| `riscv32i-unknown-none-elf` | RISC-V 32I | 🟡 Tier 3 |
| `riscv64imac-unknown-none-elf` | RISC-V 64 | 🟡 Tier 3 |

## Support Tiers

### Tier 1: Guaranteed to Work

- Extensively tested
- Available pre-built
- Receives security updates
- Stability guaranteed

### Tier 2: Generally Works

- Community supported
- May require setup
- Generally works but less tested
- Receives patches if maintainers available

### Tier 3: Experimental

- Limited testing
- May have issues
- Community support only
- No guarantees

## Specifying a Target

```bash
# List all available targets
vittec --list-targets

# Compile for specific target
vittec main.vit --target x86_64-linux-gnu -o main

# Check if target is available
vittec --target-info aarch64-linux-gnu
```

## Cross-Compilation

### Linux to macOS

```bash
# Install cross-compiler
brew install mingw-w64

# Compile
vittec main.vit --target x86_64-apple-darwin -o main
```

### Linux to Windows

```bash
# Install MinGW-w64
sudo apt-get install mingw-w64

# Compile
vittec main.vit --target x86_64-pc-windows-gnu -o main.exe
```

### To WebAssembly

```bash
# Compile for WASM
vittec main.vit --target wasm32-unknown-unknown -o main.wasm

# Run with runtime
wasmtime main.wasm
```

### To Embedded (ARM)

```bash
# For Cortex-M4
vittec firmware.vit --target thumbv7em-none-eabihf -o firmware.elf

# Flash with openocd
openocd -f board.cfg -c "program firmware.elf verify reset"
```

## Configuration Files

Each target has a configuration file in `sysroot/share/vitte/targets/`:

```json
{
  "name": "x86_64-linux-gnu",
  "architecture": "x86_64",
  "os": "linux",
  "env": "gnu",
  "linker": "gcc",
  "c_compiler": "gcc",
  "default_c_flags": ["-m64", "-march=x86-64"],
  "default_ld_flags": [],
  "llvm_target": "x86_64-unknown-linux-gnu",
  "sysroot": "sysroot",
  "libc": "glibc"
}
```

## Adding Custom Targets

Create a target configuration file:

```json
{
  "name": "my-custom-target",
  "architecture": "custom",
  "os": "custom-os",
  "linker": "custom-ld",
  "c_compiler": "custom-cc",
  "default_c_flags": ["--custom"],
  "sysroot": "custom-sysroot"
}
```

Place in `sysroot/share/vitte/targets/my-custom-target.json`.

## Platform-Specific Code

Use conditional compilation:

```vitte
#[cfg(target_os = "linux")]
fn get_config_dir() -> String {
    std::env::var("XDG_CONFIG_HOME")
        .unwrap_or_else(|_| format!("{}/.config", std::env::home()))
}

#[cfg(target_os = "windows")]
fn get_config_dir() -> String {
    std::env::var("APPDATA")
        .unwrap_or_else(|_| ".".to_string())
}

#[cfg(target_os = "macos")]
fn get_config_dir() -> String {
    std::env::var("XDG_CONFIG_HOME")
        .unwrap_or_else(|_| format!("{}/.config", std::env::home()))
}

#[cfg(target_arch = "wasm32")]
fn platform_specific() {
    // WASM-specific code
}
```

## ABI Compatibility

All targets support:
- Same ABI for function calls
- Same memory layout for structs (with `#[repr(C)]`)
- Compatible error handling (Result types)
- Compatible FFI with C

## Performance Characteristics

| Target | Optimization | Size Impact |
|--------|--------------|-------------|
| x86_64 | High | +5-10% |
| ARM64 | Medium | -10-15% |
| RISC-V | Medium | -5% |
| WebAssembly | High | +10-20% |
| Embedded (ARM) | Very High | -50% |

## Troubleshooting

### "Target not found"

```bash
# Ensure target is available
vittec --list-targets | grep your-target

# Install if available
vittec --install-target your-target
```

### Cross-compilation fails

```bash
# Check compiler is available
which gcc
gcc --version

# Verify target configuration
vittec --target-info your-target
```

### Binary won't run on target

- Verify target CPU supports required features
- Check libc compatibility
- Ensure required libraries installed on target

## See Also

- [BUILDING.md](BUILDING.md) - Build guide
- [ABI.md](ABI.md) - ABI specification
- [FFI.md](FFI.md) - C FFI
