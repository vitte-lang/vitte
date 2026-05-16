# Vitte Bootstrap Architecture and Design

## Overview

The Vitte bootstrap toolchain implements a four-stage bootstrap process to create a self-hosting compiler. This document describes the architecture, design decisions, and implementation details.

## The Bootstrap Problem

Building a compiler in its own language requires solving a chicken-and-egg problem:

1. **Without a compiler**: How do we compile the compiler?
2. **With a compiler**: We have a circular dependency

The solution is a **multi-stage bootstrap**:

1. Write a minimal compiler in C (or another existing language)
2. Use that to compile the actual compiler in its own language
3. Use the newly compiled compiler to recompile itself (verification)

This ensures the final compiler can compile itself and all subsequent programs.

## Four-Stage Bootstrap Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                  BOOTSTRAP PROCESS                           │
└─────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────┐
│ Stage 0: SEED (Bootstrap from native seed)                         │
│ ────────────────────────────────────────────────────────│
│ Input:  native bootstrap compiler (native-cc, cc, system toolchain)              │
│ Source: toolchain/seed/src/main.vit                     │
│ Output: build/vittec0 (minimal compiler)                │
│                                                          │
│ Purpose: Create initial Vitte compiler                  │
│ Size:    ~500KB (minimal feature set)                   │
│ Time:    ~30 seconds                                    │
└──────────────────────────────────────────────────────────┘
         ↓
┌──────────────────────────────────────────────────────────┐
│ Stage 1: SELF-HOSTING (First Self-Compilation)          │
│ ────────────────────────────────────────────────────────│
│ Input:  vittec0 (seed compiler)                         │
│ Source: toolchain/stage1/src/main.vit                   │
│ Output: build/vittec1 (full-featured compiler)          │
│                                                          │
│ Purpose: Build full compiler using itself               │
│ Size:    ~2.5MB (complete feature set)                  │
│ Time:    ~5 minutes                                     │
└──────────────────────────────────────────────────────────┘
         ↓
┌──────────────────────────────────────────────────────────┐
│ Stage 2: VERIFICATION (Second Self-Compilation)         │
│ ────────────────────────────────────────────────────────│
│ Input:  vittec1 (full compiler)                         │
│ Source: toolchain/stage2/src/main.vit                   │
│ Output: build/vittec2 (verified compiler)               │
│                                                          │
│ Purpose: Build compiler for final verification          │
│ Expected: Will be compared with stage3                  │
│ Time:    ~5 minutes                                     │
└──────────────────────────────────────────────────────────┘
         ↓
┌──────────────────────────────────────────────────────────┐
│ Stage 3: FINAL VERIFICATION (Third Self-Compilation)    │
│ ────────────────────────────────────────────────────────│
│ Input:  vittec2 (verified compiler)                     │
│ Source: toolchain/stage3/src/main.vit                   │
│ Output: build/vittec3 (final verified compiler)         │
│                                                          │
│ Purpose: Final verification compilation                 │
│ Expected: vittec2 == vittec3 (byte-for-byte identical)  │
│ Time:    ~5 minutes                                     │
└──────────────────────────────────────────────────────────┘
         ↓
┌──────────────────────────────────────────────────────────┐
│ VERIFICATION: vittec2 checksums == vittec3 checksums   │
│ ────────────────────────────────────────────────────────│
│ If equal: Bootstrap successful, compiler is self-host   │
│ If unequal: Bootstrap failed, investigate differences   │
└──────────────────────────────────────────────────────────┘
         ↓
┌──────────────────────────────────────────────────────────┐
│ INSTALLATION: Deploy vittec3 as production compiler     │
└──────────────────────────────────────────────────────────┘
```

## Toolchain Modules

### 1. Configuration Module (`config.vit`)

Manages all configuration aspects:

- **BuildTarget**: Platform-specific settings
  - OS, architecture, compiler triple
  - native bootstrap compiler flags and linker flags
  - Required libraries

- **CompileOptions**: Compilation parameters
  - Optimization level (0-3)
  - Debug symbols
  - Language standard version
  - IR emission

- **ToolchainConfig**: Aggregated configuration
  - Source/build directories
  - Selected target platform
  - Compilation options

Key functions:
- `default_build_target()` - Native platform target
- `target_x86_64_linux()` - Linux x86_64 target
- `default_compile_options()` - Standard compilation
- `optimized_toolchain_config()` - Production settings

### 2. Platform Detection Module (`platform.vit`)

Detects system capabilities and limitations:

- **PlatformInfo**: OS and CPU information
  - OS name (linux, macos, windows)
  - Architecture (x86_64, aarch64, etc.)
  - Endianness, pointer size
  - Support for POSIX features

- **ToolAvailability**: Tool detection
  - Compiler availability (native-cc, alternate native compiler, cc)
  - Linker tools (ar, ranlib)
  - Build utilities (make, ninja, ccache)

- **EnvironmentInfo**: Complete environment snapshot
  - Platform capabilities
  - Available tools and versions
  - System resources (CPU count, memory)

Key functions:
- `detect_platform()` - Get OS/architecture info
- `detect_tools()` - Check available build tools
- `recommended_job_count()` - Calculate parallel jobs
- `is_platform_supported()` - Verify platform

### 3. Compiler Module (`compiler.vit`)

Provides unified compiler invocation interface:

- **CompilationSource**: Source file metadata
  - File path, type (library/executable)
  - Module name and version
  - Dependencies

- **CompilationTarget**: Output specification
  - Output path and type
  - Object/binary format
  - Architecture triple

- **CompilationResult**: Compilation outcome
  - Success/failure status
  - Exit code
  - Compilation time
  - Error/warning counts

Key functions:
- `build_compiler_args()` - Generate command-line args
- `create_invocation()` - Build compiler invocation
- `invoke_compiler()` - Execute compilation
- `build_compiler_command()` - Generate command string

### 4. Build Module (`build.vit`)

Manages multi-stage bootstrap process:

- **BuildStage**: Individual stage definition
  - Stage number, name, description
  - Input/output compiler
  - Self-hosting flag
  - Verification requirements

- **BuildState**: Cumulative build state
  - Current stage
  - Accumulated artifacts
  - Build cache
  - Timing information

- **BuildReport**: Completion report
  - Success status
  - Completion statistics
  - Artifact list
  - Timing information

Key functions:
- `init_seed_stage()` - Create seed stage
- `init_stage1()` - Create stage1
- `init_stage2()` - Create stage2
- `create_build_report()` - Generate report
- `verify_bootstrap_consistency()` - Binary verification

### 5. Orchestrator Module (`orchestrator.vit`)

High-level bootstrap coordination:

- **BootstrapPhase**: Logical build phase
  - Phase number and name
  - Collection of actions
  - Parallel/sequential execution

- **BootstrapAction**: Atomic operation
  - Action type (validate, setup, compile, verify)
  - Dependencies
  - Timeout and retry settings

- **BootstrapPlan**: Complete execution plan
  - All phases in order
  - Total action count
  - Estimated time

Key functions:
- `create_validation_phase()` - Environment checks
- `create_setup_phase()` - Directory preparation
- `create_seed_phase()` - Seed compilation
- `create_stage1_phase()` - Stage1 compilation
- `create_stage2_phase()` - Stage2 compilation
- `create_verification_phase()` - Consistency checks
- `execute_bootstrap_plan()` - Main execution

### 6. Main Module (`main.vit`)

Top-level interface and entry point:

Key functions:
- `run_bootstrap()` - Standard mode
- `run_quick_bootstrap()` - Development mode
- `run_strict_bootstrap()` - Production mode
- `check_prerequisites()` - Environment validation
- `print_banner()` - Welcome message

## Bootstrap Phases

### Phase 1: Validation (Prerequisites Check)

```
check-platform      ← Verify supported OS/architecture
check-tools         ← Verify required compilers/tools
check-space         ← Verify disk space (2GB minimum)
```

**Outcome**: Build environment is valid or bootstrap aborts

### Phase 2: Setup

```
create-directories  ← Create build, cache, artifact directories
prepare-sources     ← Copy/validate source files
```

**Outcome**: Build environment ready for compilation

### Phase 3: Seed Bootstrap

```
compile-seed        ← cc → vittec0
```

**Dependencies**: Preparation complete, native bootstrap compiler available
**Output**: build/vittec0 (~500KB)

### Phase 4: Stage 1

```
compile-stage1      ← vittec0 → vittec1
```

**Dependencies**: Seed compiled successfully
**Output**: build/vittec1 (~2.5MB)

### Phase 5: Stage 2

```
compile-stage2      ← vittec1 → vittec2
```

**Dependencies**: Stage1 compiled successfully
**Output**: build/vittec2 (~2.5MB)

### Phase 6: Verification

```
verify-consistency  ← Check vittec1 == vittec2
verify-features     ← Run compiler feature tests
```

**Dependencies**: Stage2 completed
**Success Criteria**: Binaries identical, feature tests pass

### Phase 7: Installation

```
install-compiler    ← Copy to install prefix
install-libraries   ← Install stdlib
install-docs        ← Install documentation
```

**Dependencies**: Verification passed
**Outcome**: Compiler ready for use

## Key Design Decisions

### 1. Three Stages vs Two Stages

- **Stage 0 (Seed)**: Minimal compiler in C
  - Breaks dependency on external toolchain
  - Minimal feature set reduces complexity
  - Fast to compile

- **Stage 1 (Self-hosted)**: Full compiler in Vitte
  - Builds on seed compiler
  - Implements all features
  - Proves Vitte can host itself

- **Stage 2 (Verification)**: Re-compilation check
  - Ensures reproducible builds
  - Catches non-deterministic compilation bugs
  - Validates optimization correctness

### 2. Modular Vitte Implementation

All components in Vitte (not Shell/Python):
- **Pros**: Single language, consistent style, testable
- **Cons**: Requires Vitte to build Vitte (solved by bootstrap)

### 3. Configuration-Driven Process

Externalized configuration (bootstrap-config.json):
- Easy to modify build parameters
- Platform-specific overrides
- CI/CD integration-friendly

### 4. Parallel Execution Where Possible

Some phases can run in parallel:
- Environment checks (all 3 can run simultaneously)
- But compilation stages must be sequential (dependencies)

## Performance Characteristics

### Typical Build Times

| Stage | Time (1 thread) | Time (8 threads) | Output Size |
|-------|-----------------|------------------|-------------|
| Seed | 30-60s | 30-60s | 500KB |
| Stage 1 | 3-5 min | 1-2 min | 2.5MB |
| Stage 2 | 3-5 min | 1-2 min | 2.5MB |
| Total | 6-10 min | 2-4 min | 5.5MB |

### Build Modes

- **Quick**: Minimal verification, ~4-6 minutes
- **Normal**: Standard verification, ~8-12 minutes
- **Strict**: Full verification + optimization, ~25-45 minutes

## Reproducibility

Bootstrap is designed for reproducible builds:

1. **Deterministic compilation**: Same source → Same binary
2. **Stage verification**: vittec1 == vittec2 guarantees reproducibility
3. **Configuration management**: All options in JSON config
4. **Environment tracking**: Platform/tool versions recorded

### Why Reproducibility Matters

- Enables verification by independent parties
- Detects compiler bugs (non-determinism)
- Proves no injection attacks (binary diffs visible)
- Enables binary distribution verification

## Integration with Main Build

Once bootstrap completes:

```bash
export VITTE_BOOTSTRAP=build/vittec2
make all
```

The bootstrapped compiler becomes the input for building:
- Full standard library
- Tools and utilities
- Documentation
- Test suite

## Future Enhancements

### Planned Improvements

1. **Cross-compilation support**
   - Build for different platforms
   - Arm/x86_64/RISC-V targets

2. **Incremental bootstrapping**
   - Cache intermediate results
   - Rebuild only changed parts

3. **Profile-guided optimization**
   - PGO instrumentation
   - Optimized stage2 build

4. **Minimal bootstrap**
   - Reduce seed compiler size
   - Bootstrap from fewer dependencies

5. **Parallel multi-stage builds**
   - Build different targets simultaneously
   - Cross-compilation from same source

## Debugging Bootstrap Issues

### Common Problems

1. **Missing tools**
   ```bash
   ./bootstrap.sh check  # Identify missing tools
   ```

2. **Binary mismatch (vittec1 != vittec2)**
   ```bash
   diff <(hexdump -C build/vittec1) <(hexdump -C build/vittec2)
   ```

3. **Compilation failures**
   ```bash
   tail -100 build/logs/bootstrap.log
   ```

### Enable Verbose Logging

```bash
./bootstrap.sh -v normal    # Verbose output
VERBOSE=1 make bootstrap   # Verbose make
```

## References

- [Bootstrapping](https://en.wikipedia.org/wiki/Bootstrapping_(compilers))
- [native toolchain Bootstrap Process](https://native-cc.gnu.org/install/build.html)
- [Reproducible Builds](https://reproducible-builds.org/)

---

**Version**: 0.1.0  
**Last Updated**: May 10, 2026
