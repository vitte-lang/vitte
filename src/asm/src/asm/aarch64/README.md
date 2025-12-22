# AArch64 ASM fastpaths

This directory contains AArch64 implementations of a few low-level “fastpath” primitives.

## Entry points (C ABI)
All exported fastpath symbols are declared in:

- `src/asm/include/vitte/asm_fastpaths.h`

The runtime-facing stable wrappers live in:

- `src/asm/include/vitte/asm.h`
- `src/asm/src/vitte/asm_dispatch.c`

## Implementations in this folder
- `hash/vitte_fnv1a64.S`
- `memcpy/vitte_memcpy_baseline.S`, `memcpy/vitte_memcpy_neon.S`
- `memset/vitte_memset_baseline.S`, `memset/vitte_memset_neon.S`
- `utf8/vitte_utf8_validate_stub.S`
