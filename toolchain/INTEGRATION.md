# Bootstrap Toolchain Integration Guide

## Overview

This document explains how to integrate the Vitte bootstrap toolchain with the main build system and use it as the primary compiler for the project.

## Quick Start

### Step 1: Run Bootstrap

```bash
cd toolchain
./bootstrap.sh
```

This creates:
- `build/vittec0` - Seed compiler
- `build/vittec1` - First self-hosted compiler
- `build/vittec2` - Verified compiler

### Step 2: Verify Results

```bash
make verify
```

Checks that vittec1 and vittec2 are consistent.

### Step 3: Use in Main Build

```bash
cd ..
export VITTE_BOOTSTRAP=toolchain/build/vittec2
make all
```

## Integration Workflow

### Complete Build from Source

```bash
# Step 1: Bootstrap
cd toolchain
make bootstrap

# Step 2: Verify
make verify

# Step 3: Install (optional)
make install

# Step 4: Main build
cd ..
export VITTE_BOOTSTRAP=$(pwd)/toolchain/build/vittec2
make all
make test
```

### Makefile Integration

The main Makefile already supports this:

```makefile
# From project root Makefile

VITTE_BOOTSTRAP ?= $(BIN_DIR)/$(PROJECT)

# Bootstrap target
bootstrap: toolchain/build/vittec2
toolchain/build/vittec2:
	cd toolchain && $(MAKE) bootstrap

# Use bootstrap compiler
COMPILER = $(VITTE_BOOTSTRAP)
```

Usage:
```bash
make bootstrap          # Build bootstrap first
make all               # Uses VITTE_BOOTSTRAP
```

### Development Workflow

For quick iteration during development:

```bash
# Initial setup (once)
cd toolchain
make quick          # Fast bootstrap
cd ..

# During development
# Edit source files...

# Rebuild only what changed
make clean-cache
export VITTE_BOOTSTRAP=toolchain/build/vittec1
make rebuild
```

## Configuration

### Customize Bootstrap Behavior

**Via environment variables:**

```bash
# Use 16 parallel jobs
JOBS=16 ./bootstrap.sh

# Custom build directory
BUILD_DIR=/tmp/vitte-build ./bootstrap.sh

# Custom install location
INSTALL_PREFIX=$HOME/.vitte ./bootstrap.sh

# Verbose output
VERBOSE=1 ./bootstrap.sh
```

**Via Makefile variables:**

```bash
make -C toolchain JOBS=8 bootstrap
make -C toolchain INSTALL_PREFIX=/opt/vitte install
make -C toolchain BUILD_DIR=./mybuild bootstrap
```

**Via configuration file:**

Edit `toolchain/bootstrap-config.json`:

```json
{
  "compilation_defaults": {
    "optimization_level": 3,
    "debug_symbols": false
  },
  "advanced": {
    "parallel_jobs": 16,
    "use_ccache": true
  }
}
```

## Build Modes

### Mode: Normal (Standard)

```bash
./bootstrap.sh          # or
make bootstrap
```

- Platform validation
- Standard compilation
- Binary verification
- ~10-15 minutes

**Use for**: Standard development and testing

### Mode: Quick (Fast)

```bash
./bootstrap.sh quick    # or
make quick
```

- Minimal validation
- Fast compilation
- Skips verification
- ~4-6 minutes

**Use for**: Rapid iteration during development

### Mode: Strict (Production)

```bash
./bootstrap.sh strict   # or
make strict
```

- Full validation
- Optimized compilation
- Complete verification
- Profile-guided optimization
- ~30-60 minutes

**Use for**: Release builds, CI/CD verification

## Integration with CI/CD

### GitHub Actions Example

```yaml
name: Bootstrap Build

on: [push, pull_request]

jobs:
  bootstrap:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest]
    steps:
      - uses: actions/checkout@v2
      
      - name: Run Bootstrap
        working-directory: toolchain
        run: make JOBS=2 bootstrap
      
      - name: Verify Artifacts
        working-directory: toolchain
        run: make verify
      
      - name: Main Build
        env:
          VITTE_BOOTSTRAP: ${{ github.workspace }}/toolchain/build/vittec2
        run: make all
      
      - name: Run Tests
        run: make test
```

### GitLab CI Example

```yaml
bootstrap:
  stage: bootstrap
  script:
    - cd toolchain
    - make JOBS=4 bootstrap
    - make verify
  artifacts:
    paths:
      - toolchain/build/vittec*
    expire_in: 1 day

build:
  stage: build
  dependencies:
    - bootstrap
  script:
    - export VITTE_BOOTSTRAP=$(pwd)/toolchain/build/vittec2
    - make all
    - make test
```

## Cross-Platform Builds

### Build for Different Targets

The toolchain supports cross-compilation targets:

```bash
# Linux x86_64
./bootstrap.sh --target x86_64-linux

# Linux ARM64
./bootstrap.sh --target aarch64-linux

# macOS x86_64
./bootstrap.sh --target x86_64-darwin

# macOS ARM64 (Apple Silicon)
./bootstrap.sh --target aarch64-darwin

# Windows (MinGW)
./bootstrap.sh --target x86_64-windows
```

Available targets in `bootstrap-config.json`:

```json
"targets": {
  "x86_64-linux": { ... },
  "aarch64-linux": { ... },
  "x86_64-darwin": { ... },
  "aarch64-darwin": { ... },
  "x86_64-windows": { ... }
}
```

## Cache Management

### Build Cache

The bootstrap process maintains a build cache:

```
build/.cache/
├── vittec0/
├── vittec1/
└── vittec2/
```

### Clear Cache

```bash
# Clear all caches
make clean-cache

# Rebuild without cache
./bootstrap.sh --no-cache normal
```

### Disable Caching

```bash
# Via environment
BUILD_CACHE_ENABLED=0 make bootstrap

# Via config file
{
  "advanced": {
    "build_cache_enabled": false
  }
}
```

## Artifact Management

### Build Artifacts Location

```
build/
├── vittec0              # Seed compiler
├── vittec1              # Stage 1 compiler
├── vittec2              # Stage 2 compiler (final)
├── vitte.a              # Compiled standard library
├── artifacts/
│   ├── checksums.txt
│   └── build-report.json
├── .cache/              # Build cache
└── logs/                # Build logs
```

### Keep Artifacts

```bash
# Preserve artifacts after successful build
./bootstrap.sh --keep-artifacts normal

# Or via Make
make -C toolchain -e KEEP_ARTIFACTS=1 bootstrap
```

### Clean Up

```bash
# Remove all build artifacts
make clean

# Remove only build cache
make clean-cache

# Remove only logs
make clean-logs

# Full clean (including binaries)
make distclean
```

## Troubleshooting Integration

### Missing Bootstrap Compiler

If `VITTE_BOOTSTRAP` is not set:

```bash
# Default location
export VITTE_BOOTSTRAP=$(pwd)/toolchain/build/vittec2

# Or set in Makefile
make VITTE_BOOTSTRAP=/path/to/compiler all
```

### Bootstrap Verification Failed

```bash
# Check bootstrap status
make -C toolchain status

# Run verification
make -C toolchain verify

# Check logs
tail -50 toolchain/build/logs/bootstrap.log
```

### Compiler Not Found

```bash
# Verify bootstrap completed
if [ ! -f toolchain/build/vittec2 ]; then
  echo "Bootstrap not completed, running now..."
  cd toolchain && make bootstrap
  cd ..
fi

export VITTE_BOOTSTRAP=$(pwd)/toolchain/build/vittec2
make all
```

## Performance Optimization

### Parallel Compilation

```bash
# Auto-detect CPU count
./bootstrap.sh

# Explicit job count
./bootstrap.sh -j 16 normal

# Via Make
make -C toolchain JOBS=16 bootstrap
```

### Compiler Cache (ccache)

If ccache is available:

```bash
# Auto-use if found
./bootstrap.sh

# Force enable
./bootstrap.sh --use-ccache

# Via config
{
  "advanced": {
    "use_ccache": true
  }
}
```

### Incremental Builds

```bash
# Keep intermediate artifacts
./bootstrap.sh --keep-artifacts normal

# Rebuild without clean
./bootstrap.sh normal  # Uses cache
```

## Debugging

### Enable Verbose Output

```bash
# Show all commands
./bootstrap.sh -v normal

# Show compiler warnings
export VERBOSE=1
make -C toolchain bootstrap
```

### Check Build Logs

```bash
# Last 100 lines
make -C toolchain show-logs

# Full log
cat toolchain/build/logs/bootstrap.log
```

### Dry Run

```bash
# See what would happen
./bootstrap.sh dry-run

# Show bootstrap plan
make -C toolchain dry-run
```

## Advanced Usage

### Custom Build Configuration

Create a custom config:

```bash
cp toolchain/bootstrap-config.json my-bootstrap.json

# Edit my-bootstrap.json...

# Use custom config
CONFIG_FILE=my-bootstrap.json ./bootstrap.sh normal
```

### Staged Bootstrap

Build individual stages:

```bash
cd toolchain

# Just seed
CC=native-cc vittec0_compile()
# build/vittec0 created

# Just stage 1
./build/vittec0 compile stage1/src/main.vit -o build/vittec1

# Just stage 2
./build/vittec1 compile stage2/src/main.vit -o build/vittec2
```

### Environment Validation Only

```bash
./bootstrap.sh check

# Output:
# System Information:
#   OS: linux
#   Architecture: x86_64
#   CPUs: 8
#   ...
# All prerequisites met
```

## Performance Benchmarks

Typical build times on different systems:

| System | CPU | RAM | Quick | Normal | Strict |
|--------|-----|-----|-------|--------|--------|
| GitHub Actions (Linux) | 2c | 7GB | 8m | 15m | 50m |
| Desktop (i7-9700K) | 8c | 16GB | 4m | 10m | 30m |
| Laptop (i5-1135G7) | 4c | 8GB | 10m | 20m | 60m |
| Mac Mini (M1) | 8c | 16GB | 3m | 8m | 25m |
| CI Server (32c) | 32c | 128GB | 1m | 3m | 8m |

## Best Practices

1. **Run bootstrap on clean environment** for CI/CD
   ```bash
   rm -rf build/
   ./bootstrap.sh normal
   ```

2. **Always verify before using**
   ```bash
   ./bootstrap.sh && make -C toolchain verify
   ```

3. **Keep artifacts for debugging**
   ```bash
   ./bootstrap.sh --keep-artifacts normal
   ```

4. **Use appropriate mode for context**
   - Development: `quick`
   - Testing: `normal`
   - Release: `strict`

5. **Cache bootstrap results**
   ```bash
   # Save bootstrapped compiler
   mkdir -p ~/.vitte-bootstrap
   cp build/vittec2 ~/.vitte-bootstrap/
   ```

## Troubleshooting Checklist

- [ ] Tools available? Run `./bootstrap.sh check`
- [ ] Disk space? Need 2GB+
- [ ] Bootstrap completed? Check `build/vittec2`
- [ ] Verification passed? Run `make verify`
- [ ] Environment set? `echo $VITTE_BOOTSTRAP`
- [ ] Cache issue? Try `make clean-cache`
- [ ] Check logs? `tail -100 build/logs/bootstrap.log`

## Getting Help

- **Documentation**: [README.md](README.md), [BOOTSTRAP_DESIGN.md](BOOTSTRAP_DESIGN.md)
- **Issues**: GitHub Issues (link to repo)
- **Discussion**: Community forums
- **Logs**: Check `build/logs/bootstrap.log`

---

**Version**: 0.1.0  
**Last Updated**: May 10, 2026
