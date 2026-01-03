# x86_64 ASM fastpaths

This directory contains x86_64 implementations of a few low-level “fastpath” primitives.

## Entry points (C ABI)
All exported fastpath symbols are declared in:

- `src/asm/include/vitte/asm_fastpaths.h`

The runtime-facing stable wrappers live in:

- `src/asm/include/vitte/asm.h`
- `src/asm/src/vitte/asm_dispatch.c`

## Notes
- The `*_sse2.S` and `*_avx2.S` variants are selected at runtime by `asm_dispatch.c` based on `vitte_cpu_detect_host()` feature flags.
