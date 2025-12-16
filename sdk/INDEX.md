# Vitte SDK File Index

Complete reference of all SDK files and their purposes.

## Documentation (docs/)

| File | Purpose | Audience |
|------|---------|----------|
| [SDK.md](docs/SDK.md) | Overview & features | Everyone |
| [BUILDING.md](docs/BUILDING.md) | Build system guide | Developers |
| [FFI.md](docs/FFI.md) | C foreign function interface | FFI users |
| [TARGETS.md](docs/TARGETS.md) | Platform support & targets | Cross-compile users |
| [ABI.md](docs/ABI.md) | Binary interface specification | Systems engineers |
| [PACKAGING.md](docs/PACKAGING.md) | Distribution guide | Package maintainers |
| [QUICK_REFERENCE.md](docs/QUICK_REFERENCE.md) | Fast command reference | All developers |
| [SPECIFICATION.md](docs/SPECIFICATION.md) | Formal SDK specification | Implementers |
| [TEMPLATES.md](docs/TEMPLATES.md) | Project template guide | New projects |

## Headers (sysroot/include/vitte/)

| File | Purpose | Size |
|------|---------|------|
| [core.h](sysroot/include/vitte/core.h) | Core types, initialization | ~100 lines |
| [runtime.h](sysroot/include/vitte/runtime.h) | Runtime support interface | ~80 lines |
| [platform.h](sysroot/include/vitte/platform.h) | OS abstraction layer | ~120 lines |
| [alloc.h](sysroot/include/vitte/alloc.h) | Memory allocation | ~60 lines |

## Configuration (config/)

| File | Purpose |
|------|---------|
| [defaults.toml](config/defaults.toml) | SDK default settings |
| [platforms.toml](config/platforms.toml) | 14+ platform definitions |
| [features.toml](config/features.toml) | Feature flags system |

## Target Configurations (sysroot/share/vitte/targets/)

| File | Target | Tier |
|------|--------|------|
| [x86_64-linux-gnu.json](sysroot/share/vitte/targets/x86_64-linux-gnu.json) | Linux x86_64 | 1 |
| [x86_64-apple-darwin.json](sysroot/share/vitte/targets/x86_64-apple-darwin.json) | macOS Intel | 1 |
| [aarch64-apple-darwin.json](sysroot/share/vitte/targets/aarch64-apple-darwin.json) | macOS ARM64 | 1 |
| [x86_64-pc-windows-gnu.json](sysroot/share/vitte/targets/x86_64-pc-windows-gnu.json) | Windows x86_64 | 1 |
| [wasm32-unknown-unknown.json](sysroot/share/vitte/targets/wasm32-unknown-unknown.json) | WebAssembly | 1 |
| [aarch64-unknown-linux-gnu.json](sysroot/share/vitte/targets/aarch64-unknown-linux-gnu.json) | Linux ARM64 | 2 |
| [armv7-unknown-linux-gnueabihf.json](sysroot/share/vitte/targets/armv7-unknown-linux-gnueabihf.json) | Linux ARM32 | 2 |
| [i686-unknown-linux-gnu.json](sysroot/share/vitte/targets/i686-unknown-linux-gnu.json) | Linux x86 | 2 |
| [riscv64gc-unknown-linux-gnu.json](sysroot/share/vitte/targets/riscv64gc-unknown-linux-gnu.json) | Linux RISC-V | 2 |
| [thumbv7em-none-eabihf.json](sysroot/share/vitte/targets/thumbv7em-none-eabihf.json) | ARM Cortex-M | 3 |

## Templates (templates/)

### CLI Template
```
templates/cli/
├── README.md
├── muffin.muf
└── src_main.vit
```
For command-line applications.

### Library Template
```
templates/lib/
├── README.md
├── muffin.muf
└── src_lib.vit
```
For reusable libraries.

### WebAssembly Template
```
templates/wasm/
├── README.md
├── muffin.muf
├── src_lib.vit
└── index.html
```
For browser/WASM applications.

### Plugin Template
```
templates/plugin/
├── README.md
├── muffin.muf
└── src_lib.vit
```
For dynamically loadable plugins.

## Toolchain Configs (toolchains/)

| File | Purpose |
|------|---------|
| [config.toml](toolchains/config.toml) | Global toolchain management |
| [gcc/config.toml](toolchains/gcc/config.toml) | GCC compiler configuration |
| [clang/config.toml](toolchains/clang/config.toml) | Clang/LLVM configuration |
| [msvc/config.toml](toolchains/msvc/config.toml) | MSVC compiler configuration |
| [README.md](toolchains/README.md) | Toolchain guide |

## Library Files (sysroot/lib/)

```
sysroot/lib/
├── vitte/                          # Runtime & core libs
├── pkgconfig/
│   └── vitte.pc                    # pkg-config metadata
└── [architecture-specific libs]
```

## Root Level

| File | Purpose |
|------|---------|
| [README.md](README.md) | SDK overview & quick start |
| [mod.muf](mod.muf) | SDK manifest & metadata |

## Complete File Statistics

- **Documentation**: 9 files
- **Headers**: 4 ABI-stable C headers
- **Configuration**: 7 TOML files
- **Target Configs**: 10 JSON files
- **Templates**: 4 template packages (13 files)
- **Toolchains**: 4 toolchain configs
- **Root Files**: 2 main files

**Total**: 50+ SDK files

## Organization by Purpose

### For New Users
1. Start: [README.md](README.md)
2. Quick Start: [QUICK_REFERENCE.md](docs/QUICK_REFERENCE.md)
3. Templates: [TEMPLATES.md](docs/TEMPLATES.md)

### For Developers
1. Build Guide: [BUILDING.md](docs/BUILDING.md)
2. C FFI: [FFI.md](docs/FFI.md)
3. Platform Support: [TARGETS.md](docs/TARGETS.md)

### For System Engineers
1. ABI Spec: [ABI.md](docs/ABI.md)
2. Full Spec: [SPECIFICATION.md](docs/SPECIFICATION.md)
3. Headers: [sysroot/include/vitte/](sysroot/include/vitte/)

### For DevOps/Distribution
1. Packaging: [PACKAGING.md](docs/PACKAGING.md)
2. Toolchain Config: [toolchains/README.md](toolchains/README.md)
3. Target Configs: [targets/](sysroot/share/vitte/targets/)

## File Access Patterns

### Adding New Target
1. Create JSON in `sysroot/share/vitte/targets/TRIPLE.json`
2. Update `config/platforms.toml`
3. Document in `docs/TARGETS.md`
4. Test with `vitte-build --target TRIPLE`

### Adding New Toolchain
1. Create directory `toolchains/NAME/`
2. Create `toolchains/NAME/config.toml`
3. Update `toolchains/config.toml`
4. Document in `toolchains/README.md`
5. Test with `vitte-build --toolchain NAME`

### Adding New Template
1. Create directory `templates/TYPE/`
2. Create template files
3. Update `docs/TEMPLATES.md`
4. Test with `vitte-new NAME --template=TYPE`

## Build Artifacts

When building with the SDK:

```
project/
└── target/
    ├── debug/
    │   └── app              # Debug binary
    └── release/
        └── app              # Release binary
```

Output formats:
- **Linux**: ELF executable
- **macOS**: Mach-O executable
- **Windows**: PE executable
- **WASM**: .wasm module

## Documentation Cross-References

Each document references related files:

- **SDK.md** → All other docs
- **BUILDING.md** → TARGETS.md, QUICK_REFERENCE.md
- **FFI.md** → ABI.md, core.h/platform.h
- **TARGETS.md** → Target JSON configs
- **ABI.md** → Headers in sysroot/include/
- **PACKAGING.md** → TARGETS.md, BUILDING.md
- **QUICK_REFERENCE.md** → All docs
- **SPECIFICATION.md** → ABI.md, TARGETS.md
- **TEMPLATES.md** → Template directories

## Version Information

- SDK Version: 0.2.0
- Language Version: Vitte 0.2.0+
- Specification Version: 0.2.0
- Header Version: 1.0 (stable ABI)

## License

All SDK files: Apache License 2.0
Third-party components: See [LICENSE-THIRD-PARTY.md](../LICENSE-THIRD-PARTY.md)

---

**Last Updated**: 2024
**Maintained By**: Vitte Project
**Status**: Production Ready
