# Bootstrap Toolchain Examples

Practical examples for using the Vitte bootstrap toolchain.

## Basic Usage

### Example 1: Simple Bootstrap

```bash
cd toolchain
./bootstrap.sh
```

Output:
```
╔════════════════════════════════════════════════════════╗
║         Vitte Bootstrap Toolchain                      ║
║  Multi-stage compiler bootstrap from C to Vitte        ║
║  Copyright 2026 - Vitte Project                        ║
╚════════════════════════════════════════════════════════╝

System Information:
  OS: linux
  Architecture: x86_64
  CPUs: 8
  Source: /home/user/vitte
  Build: /home/user/vitte/build
  Install: /usr/local

[OK] All prerequisites met

[INFO] Preparing build directory...
[OK] Build directory prepared

[INFO] Starting bootstrap: normal mode
[INFO] Stage 0 (Seed): Compiling seed from C...
[INFO] Stage 1: First self-hosted compilation...
[INFO] Stage 2: Second self-hosted compilation...
[INFO] Verification: Comparing binary outputs...
[OK] Bootstrap completed successfully
```

### Example 2: Quick Development Build

Fast build for testing:

```bash
cd toolchain
./bootstrap.sh quick
```

Output:
```
[INFO] Starting bootstrap: quick mode
[INFO] Stage 0 (Seed): Compiling seed from C...
[OK] Seed compiler ready

[INFO] Stage 1: First self-hosted compilation...
[OK] Stage 1 compiler ready

[OK] Bootstrap completed successfully
```

Uses faster options:
- No optimization
- Skips some verification
- ~4-6 minutes total

### Example 3: Strict Production Build

Full build with all verifications:

```bash
cd toolchain
./bootstrap.sh strict
```

Output:
```
[INFO] Starting bootstrap: strict mode
[INFO] Enabling all verification checks...
[INFO] Enabling optimizations...

[INFO] Stage 0 (Seed): Compiling seed from C...
[OK] Seed verified

[INFO] Stage 1: First self-hosted compilation...
[OK] Stage 1 verified

[INFO] Stage 2: Second self-hosted compilation...
[OK] Stage 2 verified

[INFO] Running feature tests...
[OK] All features verified

[OK] Bootstrap completed successfully
```

Full verification:
- Level 3 optimization
- All verifications enabled
- PGO ready
- ~30-60 minutes total

## Command-Line Options

### Example 4: Parallel Compilation

Build with specific job count:

```bash
# Use 16 parallel jobs
./bootstrap.sh -j 16 normal

# Or with make
make -C toolchain JOBS=16 bootstrap

# Or via environment
JOBS=16 ./bootstrap.sh quick
```

Expected speedup:
- 2 jobs: ~1.8x
- 4 jobs: ~3.2x
- 8 jobs: ~5.5x
- 16 jobs: ~8x

### Example 5: Custom Installation Path

Install to custom location:

```bash
# Install prefix
./bootstrap.sh --prefix /opt/vitte normal

# Then use it
export PATH=/opt/vitte/bin:$PATH
vittec --version

# Or via environment
INSTALL_PREFIX=$HOME/.local ./bootstrap.sh normal
export PATH=$HOME/.local/bin:$PATH
```

### Example 6: Verbose Output

See all build details:

```bash
./bootstrap.sh -v normal
```

Shows:
- Each compilation command
- Compiler warnings
- Build statistics
- Timing information

## Using Bootstrapped Compiler

### Example 7: Build with Bootstrap Compiler

After bootstrap completes, use it for main build:

```bash
cd ..
export VITTE_BOOTSTRAP=$(pwd)/toolchain/build/vittec2

# Build everything with bootstrap compiler
make all

# Verify it worked
./bin/vitte --version
```

### Example 8: Makefile Integration

From project root:

```bash
# Build bootstrap and use it
make bootstrap-all

# Equivalent to:
cd toolchain && make bootstrap
cd ..
export VITTE_BOOTSTRAP=$(pwd)/toolchain/build/vittec2
make all
```

### Example 9: Compile User Program

Using bootstrapped compiler:

```bash
# Set up environment
export PATH="$(pwd)/toolchain/build:$PATH"

# Compile a simple program
vittec2 examples/hello.vit -o hello
./hello

# Or with full path
./toolchain/build/vittec2 examples/hello.vit -o hello_world
./hello_world
```

## Verification and Testing

### Example 10: Verify Bootstrap

Check that stages are consistent:

```bash
cd toolchain

# Quick status check
make status

# Output:
# Bootstrap Status:
#   ✓ Seed (vittec0)
#   ✓ Stage 1 (vittec1)
#   ✓ Stage 2 (vittec2)
#   ✗ Not installed

# Full verification
make verify

# Output:
# Checking binary consistency...
# Generating checksums...
# Verification complete
# Checksums:
# 6f7a4e2b... build/vittec0
# a3c5d1e9... build/vittec1
# a3c5d1e9... build/vittec2
```

### Example 11: Run Tests

After bootstrap:

```bash
# Run compiler tests
make test

# Run specific test suite
make test-compiler

# Run with bootstrapped compiler
VITTE=$(pwd)/toolchain/build/vittec2 make test
```

### Example 12: Dry Run

See what would happen without executing:

```bash
./bootstrap.sh dry-run

# Output:
# Bootstrap Plan:
#   Phase 1: Environment Validation
#     - check-platform
#     - check-tools
#     - check-space
#
#   Phase 2: Setup
#     - create-directories
#     - prepare-sources
#
#   ...
#   Estimated time: ~30 minutes
```

## Troubleshooting

### Example 13: Check Prerequisites

Verify your system is ready:

```bash
./bootstrap.sh check

# Output:
# System Information:
#   OS: linux
#   Architecture: x86_64
#   CPUs: 8
#   Memory: 16384 MB
#   C Compiler: gcc
#   Build Tool: make
#
# [OK] All prerequisites met
```

If prerequisites fail:

```bash
# Missing tools
[ERROR] Missing required tools: gcc, ar

# Fix:
sudo apt-get install build-essential    # Debian/Ubuntu
brew install gcc                         # macOS
```

### Example 14: Debug Build Failure

If bootstrap fails:

```bash
# Check logs
tail -100 build/logs/bootstrap.log

# Run with verbose output
./bootstrap.sh -v normal

# Check specific stage
make -C toolchain show-logs

# Investigate issues
gcc --version
ar --version
make --version
```

### Example 15: Clean and Rebuild

Start fresh:

```bash
# Soft clean (keep cache)
cd toolchain
make clean

# Full clean
make distclean

# Rebuild
./bootstrap.sh normal
```

## Advanced Usage

### Example 16: Cross-Compilation Target

Build for different platform:

```bash
# Build for Linux ARM64
./bootstrap.sh --target aarch64-linux normal

# Build for macOS Intel
./bootstrap.sh --target x86_64-darwin normal

# Build for macOS M1
./bootstrap.sh --target aarch64-darwin normal
```

### Example 17: Build Cache

Manage compilation cache:

```bash
# Use cache (default)
./bootstrap.sh normal

# Disable cache
./bootstrap.sh --no-cache normal

# Clear cache
make clean-cache

# Rebuild with fresh cache
./bootstrap.sh quick
```

### Example 18: Keep Artifacts

Preserve intermediate builds:

```bash
# Keep all build artifacts
./bootstrap.sh --keep-artifacts normal

# Check artifacts
ls -lh build/artifacts/

# Use kept artifacts later
cp build/artifacts/vittec1 my-backup/
```

## Continuous Integration

### Example 19: GitHub Actions

```yaml
name: Build

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Bootstrap
        run: cd toolchain && make JOBS=2 bootstrap
      
      - name: Verify
        run: cd toolchain && make verify
      
      - name: Build
        env:
          VITTE_BOOTSTRAP: ${{ github.workspace }}/toolchain/build/vittec2
        run: make all
      
      - name: Test
        run: make test
```

### Example 20: Local CI Simulation

Simulate CI locally:

```bash
#!/bin/bash

# Simulate GitHub Actions workflow
set -e

echo "Building..."
cd toolchain
make distclean
make JOBS=2 bootstrap

echo "Verifying..."
make verify

echo "Building project..."
cd ..
export VITTE_BOOTSTRAP=$(pwd)/toolchain/build/vittec2
make all
make test

echo "✓ All checks passed"
```

Save as `ci-local.sh` and run:

```bash
chmod +x ci-local.sh
./ci-local.sh
```

## Performance Testing

### Example 21: Benchmark Builds

Compare performance:

```bash
#!/bin/bash

echo "Quick build..."
time ./bootstrap.sh quick

echo "Normal build..."
time ./bootstrap.sh normal

echo "Strict build..."
time ./bootstrap.sh strict
```

### Example 22: Parallel Job Comparison

Compare job counts:

```bash
for jobs in 1 2 4 8 16; do
  echo "Building with $jobs jobs..."
  time JOBS=$jobs ./bootstrap.sh quick
done
```

## Integration Examples

### Example 23: Complete Project Build

Full build from source to binary:

```bash
# Clone and setup
git clone https://github.com/vitte/vitte.git
cd vitte

# Bootstrap compiler
cd toolchain
./bootstrap.sh

# Use compiler for main build
cd ..
export VITTE_BOOTSTRAP=$(pwd)/toolchain/build/vittec2
make all
make install

# Verify installation
vitte --version
```

### Example 24: Docker Build

Containerized bootstrap:

```dockerfile
FROM ubuntu:20.04

RUN apt-get update && apt-get install -y \
    build-essential \
    git

WORKDIR /vitte
COPY . .

RUN cd toolchain && \
    ./bootstrap.sh normal && \
    make verify

ENV VITTE_BOOTSTRAP=/vitte/toolchain/build/vittec2
RUN make all
RUN make install

ENTRYPOINT ["vitte"]
```

Build and run:

```bash
docker build -t vitte:latest .
docker run vitte:latest --version
```

## Useful Aliases

Add to `.bashrc` or `.zshrc`:

```bash
# Bootstrap shortcuts
alias vitte-quick="cd toolchain && ./bootstrap.sh quick"
alias vitte-build="cd toolchain && ./bootstrap.sh normal"
alias vitte-strict="cd toolchain && ./bootstrap.sh strict"
alias vitte-verify="cd toolchain && make verify"
alias vitte-clean="cd toolchain && make clean"

# Use bootstrapped compiler
function vitte-compile {
  export VITTE=$(pwd)/toolchain/build/vittec2
  $VITTE "$@"
}
```

Usage:

```bash
vitte-quick         # Quick bootstrap
vitte-compile hello.vit -o hello
vitte-verify        # Verify
```

---

**More Examples**

See the test suite for additional examples:
- [tests/bootstrap/](../../tests/bootstrap/)
- [tests/compiler/](../../tests/compiler/)

**Getting Help**

- Run `./bootstrap.sh --help` for command-line help
- Read [README.md](README.md) for overview
- Check [BOOTSTRAP_DESIGN.md](BOOTSTRAP_DESIGN.md) for architecture
- Review [INTEGRATION.md](INTEGRATION.md) for integration details

---

**Version**: 0.1.0  
**Last Updated**: May 10, 2026
