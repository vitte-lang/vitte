# Vitte Bootstrap Toolchain

Complete bootstrap toolchain for building the Vitte compiler from source. Implements a multi-stage bootstrap process from native seed to self-hosted Vitte.

## Overview

The bootstrap toolchain handles the complete compilation pipeline:

- **Stage 0 (Seed)**: Verified seed installed as `bin/vittec0`
- **Stage 1**: Bootstrap compiler built by stage0 and installed as `bin/vittec1`
- **Stage 2**: Full compiler entry built by stage1 and installed as `bin/vittec`

The active contract stops at stage2. A stage3 rebuild and byte-for-byte stage2/stage3 comparison remain separate self-hosting milestones.

## Architecture

```
Verified seed artifact
       ↓
[Stage 0: Seed] → bin/vittec0
       ↓
[Stage 1: Bootstrap] → bin/vittec1
       ↓
[Stage 2: Full compiler] → bin/vittec + bin/vitte
       ↓
Contract, version, artifact, alias, hash, parity and snapshot checks
```

## Components

The toolchain is implemented entirely in Vitte with the following modules:

### Core Modules (`toolchain/src/`)

1. **config.vit** - Configuration management
   - Build target definitions
   - Compilation options
   - Platform-specific settings

2. **platform.vit** - Platform detection and tool discovery
   - OS and architecture detection
   - Available tools detection
   - Environment capability checking

3. **compiler.vit** - Compiler interface
   - Compilation invocation
   - Command-line argument generation
   - Result handling

4. **build.vit** - Build orchestration
   - Multi-stage build management
   - Build cache handling
   - Artifact tracking

5. **orchestrator.vit** - High-level coordination
   - Bootstrap phase management
   - Dependency handling
   - Process orchestration

6. **main.vit** - Main entry point
   - Top-level bootstrap control
   - Mode selection (quick, strict, normal)
   - CLI interface

## Supported Targets

- **x86_64-linux** - Linux x86_64 (native toolchain)
- **aarch64-linux** - Linux ARM64 (AArch64)
- **x86_64-darwin** - macOS x86_64 (Native Compiler)
- **aarch64-darwin** - macOS ARM64 Apple Silicon (Native Compiler)
- **x86_64-windows** - Windows x86_64 (MinGW)

## Prerequisites

### Required Tools

- native bootstrap compiler (`native-cc` or `cc`)
- `ar` (archiver)
- `ranlib` (archive tool)
- Standard Unix tools (`mkdir`, `cp`, `rm`, etc.)

### Optional Tools

- `ccache` - Compiler cache for faster rebuilds
- `ninja` - Fast build system
- `git` - Version control

### System Requirements

- **Disk Space**: 2GB minimum
- **RAM**: 1GB minimum
- **CPU**: 2 cores recommended, 4+ cores ideal

## Usage

### Basic Bootstrap

```bash
cd toolchain
./bootstrap.sh
```

### Quick Development Build

```bash
./bootstrap.sh quick
```

Skips some verification steps for faster iteration.

### Strict Verification Build

```bash
./bootstrap.sh strict
```

Runs all verification and produces optimized binaries.

### Check Environment

```bash
./bootstrap.sh check
```

Verifies all prerequisites are available.

### Dry Run

```bash
./bootstrap.sh dry-run
```

Shows what would be done without executing.

## Options

```
-h, --help           Show help message
-v, --verbose        Verbose output
-j N, --jobs N       Use N parallel jobs
--prefix PREFIX      Installation prefix (default: /usr/local)
--build-dir DIR      Build directory (default: ./build)
--no-cache           Disable build cache
--keep-artifacts     Keep intermediate artifacts
```

### Example Commands

```bash
# Build with 8 parallel jobs
./bootstrap.sh -j 8 normal

# Install to custom location
./bootstrap.sh --prefix ~/vitte-install

# Verbose quick build
./bootstrap.sh -v quick

# Clean build with cache disabled
./bootstrap.sh --no-cache normal
```

## Environment Variables

```bash
BUILD_DIR=/custom/build ./bootstrap.sh
INSTALL_PREFIX=/opt/vitte ./bootstrap.sh
JOBS=16 ./bootstrap.sh
VERBOSE=1 ./bootstrap.sh
```

## Validation Gates

The root `make build` flow now runs a small set of bootstrap-specific gates in
addition to the stage bootstrap and snapshot checks:

- `compiler-real-native-gate` verifies `src/vitte/compiler/main.vit` builds as a
  native executable without leaving a `.bootstrap-bridge` sidecar behind.
- `compiler-test-suite-check-gate` verifies the stable compiler test suites still
  pass the regular `check` surface.
- `compiler-test-suite-bridge-gate` verifies the bootstrap compiler bridge stays
  limited to compiler test suite sources and that a bridged suite still executes
  successfully.
- `driver-native-json-surface-gate` verifies AST, HIR, MIR, and diagnostics
  JSON reports keep the expected native driver envelope.

These gates are narrower than `bootstrap-native-snapshots`: the snapshot suite
checks deterministic stage0/stage1/stage2 artifacts, while the new gates assert
integration behavior of the current driver and bootstrap path.

## Project Path CLI Fallback

The driver now accepts a project directory as the input path for `build`,
`check`, `run`, `dump-tokens`, `dump-ast`, `dump-hir`, and `dump-mir`.

Resolution order is:

1. `src/main.vit`
2. `src/vitte/compiler/main.vit`
3. `main.vit`

This means commands such as `./bin/vitte check .` or `./bin/vitte build . -o
target/app` resolve the project entry automatically instead of requiring the
fully qualified file path.

## Configuration

The bootstrap process is configured via `bootstrap-config.json`:

```json
{
  "bootstrap": {
    "stages": [
      {
        "stage": 0,
        "name": "seed",
        "compiler": "toolchain/seed/vittec0.seed",
        "sources": ["toolchain/seed/src/main.vit"],
        "artifact": "toolchain/seed/vittec0.seed",
        "output": "bin/vittec0"
      },
      ...
    ]
  },
  "targets": {
    "x86_64-linux": { ... },
    ...
  }
}
```

## Build Modes

### Normal (Default)

Standard bootstrap with basic verification:
- Platform check
- Tool availability check
- Bootstrap seed, stage1, stage2
- Binary verification

**Time**: ~15-30 minutes

### Quick

Fast development build:
- Skips some verification
- Minimal optimization
- Useful for testing

**Time**: ~5-10 minutes

### Strict

Full production build:
- All verifications enabled
- Maximum optimization
- Full reproducibility checks
- Profile-guided optimization ready

**Time**: ~30-60 minutes

## Verification

The bootstrap process verifies:

1. **Transition Integrity**: each stage is produced by the previous installed compiler
2. **Artifact Integrity**: each installed compiler matches its build artifact
3. **Identity**: source `VERSION_TEXT`, configured version and executable version agree
4. **Reproducibility**: two identical stage0-stage2 builds produce the same hashes

## Output

Build artifacts are placed in `target/bootstrap/` and installed in `bin/`:

```
target/bootstrap/
├── stage1/vittec1
├── stage2/vittec
└── cache/

bin/
├── vittec0
├── vittec1
├── vittec
└── vitte
```

## Troubleshooting

### Missing Tools

If tools are missing, the bootstrap will fail with:

```
[ERROR] Missing required tools: native-cc, ar
```

Solution: Install the appropriate development tools for your platform.

### Insufficient Disk Space

```
[ERROR] Insufficient disk space. Required: 2048MB, Available: 512MB
```

Solution: Free up disk space in the checkout containing `target/bootstrap`.

### Build Failures

Check the build logs:

```bash
cat target/reports/bootstrap/hard_gate_native.json
```

For more details, run with verbose mode:

```bash
./bootstrap.sh -v normal
```

## Stage Details

### Stage 0: Seed

The checked-in seed is verified against its manifest and installed atomically:

```bash
make seed-verify bootstrap-seed
```

### Stage 1: First Self-Hosted

First compiler compiled by the seed:

```bash
toolchain/scripts/bootstrap/stage1.sh
```

### Stage 2: Verification

The full compiler entry is compiled by stage1:

```bash
toolchain/scripts/bootstrap/stage2.sh
```

`python3 tools/check_bootstrap_stage_chain.py --artifacts` verifies every installed artifact and alias. Stage2 currently declares a transitional bridge policy; removing it is required before full self-hosting parity.

The stage2 build also writes `target/bootstrap/stage2/provenance.json` and `compiler-sources.sha256`. They bind the stage1 producer hash, the complete compiler source set, the exact build command and the resulting stage2 hash.

## Integration with Main Build

Once bootstrap completes, the compiler can be used:

```bash
export VITTE_BOOTSTRAP=./bin/vittec
make all
```

Or:

```bash
make VITTE_BOOTSTRAP=./bin/vittec all
```

## Performance

Typical build times on modern hardware:

| CPU | RAM | Mode | Time |
|-----|-----|------|------|
| i7-9700K (8c) | 16GB | normal | 12 min |
| i7-9700K (8c) | 16GB | quick | 4 min |
| i7-9700K (8c) | 16GB | strict | 45 min |
| Apple M1 (8c) | 16GB | normal | 8 min |
| Ryzen 5 (6c) | 8GB | normal | 15 min |

## Security Considerations

- Bootstrap binaries are compiled but not signed
- For production deployments, verify checksum lists:
  ```bash
  shasum -a 256 bin/vittec0 bin/vittec1 bin/vittec
  ```
- Regenerate bootstrap on clean systems for verification

## Development

To modify the bootstrap process:

1. Edit the appropriate module in `toolchain/src/`
2. Update configurations in `bootstrap-config.json`
3. Test with: `./bootstrap.sh dry-run`
4. Run full build: `./bootstrap.sh normal`

## Documentation

- [Bootstrap Architecture](docs/bootstrap-architecture.md)
- [Building the Compiler](docs/building.md)
- [Contributing Guide](docs/CONTRIBUTING.md)

## License

The Vitte bootstrap toolchain is part of the Vitte project and is licensed under the project's standard license.

## Support

For issues, questions, or contributions:

- GitHub Issues: [vitte/issues](https://github.com/vitte/vitte/issues)
- Documentation: [vitte.org](https://vitte.org)
- Community: [Vitte Community](https://vitte.org/community)

---

**Version**: 0.1.0  
**Last Updated**: May 10, 2026  
**Status**: Bootstrap Vitte
