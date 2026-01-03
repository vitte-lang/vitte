# Bootstrap System

Complete multi-stage bootstrap system for Vitte compiler self-hosting and compilation.

## Overview

This bootstrap system enables the Vitte compiler to:
1. **Stage 0**: Host compilation with C compiler (vittec-host)
2. **Stage 1**: First self-hosted compilation
3. **Stage 2**: Verify compiler stability
4. **Stage 3**: Optimize and produce final release build

## Directory Structure

```
bootstrap/
├── stage0/              # Host compiler compilation
├── stage1/              # First self-hosted compilation  
├── stage2/              # Verification and optimization
├── stage3/              # Release build
├── config/              # Configuration files
├── tools/               # Bootstrap helper tools
├── cache/               # Build artifacts cache
├── logs/                # Bootstrap execution logs
├── bootstrap.sh         # Main bootstrap orchestrator
├── clean.sh             # Clean bootstrap artifacts
└── verify.sh            # Verification script
```

## Quick Start

```bash
./bootstrap.sh             # Run full bootstrap (all stages)
./bootstrap.sh --stage 0   # Run stage 0 only
./bootstrap.sh --stage 1   # Run stage 1 only
./bootstrap.sh --clean     # Clean bootstrap artifacts
./verify.sh                # Verify bootstrap integrity
```

## Stages

### Stage 0: Host Compilation
- Requires: C compiler (gcc/clang), CMake, Make
- Output: vittec-host (bootstrap compiler)
- Time: ~5-15 minutes

### Stage 1: Self-Hosted
- Requires: vittec-host from stage 0
- Compiles Vitte compiler using itself
- Output: vittec-stage1
- Time: ~10-20 minutes

### Stage 2: Verification
- Compiles vittec-stage1 using itself
- Compares output with stage 1
- Validates compiler stability
- Time: ~10-20 minutes

### Stage 3: Release
- Final optimization passes
- Strip debug symbols (optional)
- Package for distribution
- Time: ~5-10 minutes

## Configuration

Edit `config/bootstrap.conf` to customize:
- Compiler flags
- Optimization levels
- Target architecture
- Output directories
- Cache settings

## Monitoring

Check logs during execution:
```bash
tail -f logs/bootstrap.log
tail -f logs/stage0.log
tail -f logs/stage1.log
```

## Troubleshooting

- **Stage 0 fails**: Check C compiler (gcc/clang) installation
- **Stage 1/2 fails**: Check stage 0 compiler output
- **Memory issues**: Reduce parallel jobs or increase swap
- **Disk space**: Check `cache/` directory size

## Performance Tips

- Use parallel compilation: `-j$(nproc)`
- Enable ccache for faster rebuilds
- Use `-O2` optimization (balance speed/compile-time)
- Enable LTO for final release

## Security Notes

- All builds verified against checksums
- Bootstrap reproducible with fixed versions
- No external downloads (fully self-contained)
- Source audit at each stage
