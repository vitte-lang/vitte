# Stage 3: Final Verification Self-Hosted Vitte Compiler

## Overview

Stage 3 is the **final verification compiler** used to confirm the bootstrap process matches native bootstrap compiler bootstrap sophistication. It is compiled from Vitte source using the Stage 2 compiler (vittec2) and should produce an **identical binary** to Stage 2.

## Seed-Compat Entry Contract

`stage3/src/main.vit` is intentionally kept as a **bootstrap-compatible shim** used by the bootstrap chain.
Keep this entry minimal and stable for bootstrap reproducibility; full toolchain logic remains in `toolchain/src/*`.

## Purpose

- **Final reproducibility verification**: Check Stage 2 output is reproducible
- **native-bootstrap equivalence**: Match the verification rigor of native bootstrap compiler bootstraps
- **Production validation**: Ensure bootstrap correctness with 3-stage verification

## Architecture

```
Stage 2 Compiler (vittec2)
           ↓
[Stage 3 Source] → vittec3 (binary)
           ↓
Verification: vittec2 == vittec3
           ↓
Production Compiler
```

## Verification

Stage 3 enables the same verification as native compilers:
- Stage 1: Host → Compiler A
- Stage 2: Compiler A → Compiler B
- Stage 3: Compiler B → Compiler C
- Verify: Compiler B == Compiler C

This matches the sophistication of native toolchain and other native bootstrap compiler bootstraps.