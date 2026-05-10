# Stage 2: Verification Self-Hosted Vitte Compiler

## Overview

Stage 2 is the **verification compiler** used to confirm the bootstrap process is reproducible. It is compiled from Vitte source using the Stage 1 compiler (vittec1) and should produce an **identical binary** to Stage 1.

## Seed-Compat Entry Contract

`stage2/src/main.vit` is intentionally kept as a **bootstrap-compatible shim** used by the bootstrap chain.
Keep this entry minimal and stable for bootstrap reproducibility; full toolchain logic remains in `toolchain/src/*`.

## Purpose

- **Reproducibility verification**: Check Stage 1 output is reproducible
- **Bug detection**: Identify non-deterministic compilation
- **Production validation**: Ensure bootstrap correctness

## Architecture

```
Stage 1 (Full - vittec1)
        ↓
[Compile stage2/src/main.vit using vittec1]
        ↓
Stage 2 (Verified - vittec2)
        ↓
Checksum Verification
        ↓
vittec1 == vittec2  ?  ✓ SUCCESS : ✗ FAILURE
```

## Verification Process

### What Gets Verified

1. **Binary Reproducibility**
   ```bash
   sha256sum build/vittec1 build/vittec2
   # Should produce identical checksums
   ```

2. **Feature Parity**
   - Both compilers have same feature set
   - Both handle same inputs correctly
   - Both generate compatible outputs

3. **Correctness**
   - No compiler bugs detected in Stage 1
   - No non-deterministic behavior
   - Compilation is reproducible

## Components

Stage 2 includes all Stage 1 features plus **verification functionality**:

### Frontend
- **Lexer with tracking**: Enhanced error reporting with line/column info
- **Parser with full AST**: Complete abstract syntax tree construction
- **Semantic Analyzer**: Complete type checking and validation

### Middle-end
- **IR Generator with optimizations**: SSA form, CFG, optimization annotations
- **Optimizer**: Full optimization passes

### Backend
- **Code Generator with optimization**: Register allocation, instruction selection
- **Advanced Linker**: Symbol resolution, relocation, library linking

### Verification
- **Binary comparator**: Compare vittec1 and vittec2
- **Feature checker**: Verify all features present
- **Reproducibility validator**: Ensure deterministic output

## Features

✓ All Stage 1 features
✓ Enhanced error reporting
✓ Full optimization passes
✓ Advanced linking
✓ Verification functionality
✓ Reproducibility checking
✓ Diagnostic reporting

## Source Structure

```
src/main.vit                Main verified compiler implementation
├── Enhanced compiler stages
├── Verification functions
├── Diagnostic support
├── compile_source()        Full compilation pipeline
├── compile()               CLI entry point
└── verify_bootstrap()      Verification functionality
```

## Usage

### Compile with Stage 2

```bash
# Automatic in bootstrap process
cd toolchain
make bootstrap

# Results in:
# build/vittec1 (Stage 1 compiler)
# build/vittec2 (Stage 2 - verified compiler)

# Verify they match
make verify
```

### Manual Compilation

From Stage 1 compiler:

```bash
./build/vittec1 -o build/vittec2 toolchain/stage2/src/main.vit
```

### Verification

```bash
# Check binary identity
cmp build/vittec1 build/vittec2
echo $?  # 0 = identical, 1 = different

# Check checksums
sha256sum build/vittec1 build/vittec2

# Full verification report
./build/vittec2 --verify
```

## Key Functions

### Compilation Pipeline

Same as Stage 1, but with enhanced implementations:

- `lexer_scan(source)` → `[tokens]` (with error tracking)
- `parser_parse(tokens)` → `[ast]` (complete AST)
- `semantic_check(ast)` → `bool` (full checking)
- `ir_generate(ast)` → `[ir]` (with optimization)
- `backend_codegen(ir)` → `[asm]` (optimized)
- `linker_link(objects, libs)` → `binary` (advanced)

### Verification Functions

- `verify_reproducibility()` → `bool`
- `can_self_compile()` → `true`
- `verify_bootstrap()` → `bool`
- `create_verification_report()` → `string`
- `check_features()` → `[features]`

### Information

- `version_text()` → "vittec2 stage2-vitte 0.1.0"
- `banner_text()` → "vittec2 stage2 verification bootstrap"
- `get_stage_info()` → Stage information
- `get_capabilities()` → Compiler capabilities

## Verification Results

### Success Criteria

✓ **Binary Match**: vittec1 == vittec2
✓ **Checksums**: SHA256 identical
✓ **Features**: All features present
✓ **Self-compile**: Can compile itself
✓ **Reproducible**: Deterministic output

### Failure Diagnosis

If vittec2 ≠ vittec1:

```bash
# 1. Check file sizes
ls -l build/vittec1 build/vittec2

# 2. Compare binaries
cmp build/vittec1 build/vittec2

# 3. Check differences
diff <(hexdump -C build/vittec1) <(hexdump -C build/vittec2) | head -20

# 4. Review build logs
tail -100 build/logs/bootstrap.log
```

### Common Issues

**Different timestamps**: Strip timestamps before comparing
```bash
strip build/vittec1 build/vittec2
cmp build/vittec1 build/vittec2
```

**Different optimization**: Ensure consistent flags
```bash
make JOBS=1 rebuild-strict  # Single job, full optimization
```

**Non-deterministic behavior**: May indicate compiler bug
```bash
./build/vittec2 --dump-ir > ir1.txt
./build/vittec2 --dump-ir > ir2.txt
diff ir1.txt ir2.txt
```

## Integration

Stage 2 is used as:

1. **Production compiler**: When verification passes
2. **Distribution binary**: What gets shipped
3. **Final bootstrap result**: `VITTE_BOOTSTRAP` environment variable

```bash
export VITTE_BOOTSTRAP=$(pwd)/toolchain/build/vittec2
make all
```

## Performance

- **Binary size**: ~2.5MB (same as Stage 1)
- **Compilation time**: ~3-5 minutes
- **Verification overhead**: ~1 minute (checksumming, comparison)
- **Total Stage 2 time**: ~4-6 minutes

## Bootstrap Success Indicator

```bash
# After bootstrap completes:

# Check all stages exist
test -f build/vittec0 && echo "✓ Seed"
test -f build/vittec1 && echo "✓ Stage 1"
test -f build/vittec2 && echo "✓ Stage 2"

# Verify Stage 2 matches Stage 1
if cmp -s build/vittec1 build/vittec2; then
  echo "✓ Bootstrap SUCCESSFUL"
  exit 0
else
  echo "✗ Bootstrap FAILED - Binary mismatch"
  exit 1
fi
```

## Limitations

Stage 2 is:
- Based on Stage 1 implementation
- Inherits any Stage 1 bugs
- Limited to features in Stage 1
- Still a bootstrap compiler (may have limitations)

## Future Development

After verification passes:

1. **Production use** - Use vittec2 for all builds
2. **Feature expansion** - Add new features in next iteration
3. **Optimization** - Improve code generation
4. **Distribution** - Ship vittec2 as primary compiler

## References

- [Stage 1: First Self-Hosted](../stage1/README.md)
- [Bootstrap Design](../BOOTSTRAP_DESIGN.md)
- [Integration Guide](../INTEGRATION.md)
- [Examples](../EXAMPLES.md)

## Troubleshooting

**Stage 2 compilation fails**:
```bash
./bootstrap.sh -v normal        # Verbose mode
tail -50 build/logs/bootstrap.log
```

**Binary mismatch (vittec1 ≠ vittec2)**:
```bash
ls -la build/vittec{1,2}                    # Check sizes
sha256sum build/vittec{1,2}                # Check hashes
cmp build/vittec1 build/vittec2            # Compare binaries
```

**Verification script not found**:
```bash
make -C toolchain verify                    # Run verification
```

---

**Version**: 0.1.0  
**Status**: Verification compiler  
**Role**: Bootstrap stage 2 reproducibility check  
**Success Criteria**: vittec1 == vittec2
