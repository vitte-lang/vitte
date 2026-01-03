# src/asm (max layout)

GAS `.S` (preprocessed) fastpaths.
- Keep **baseline** implementations correct.
- SIMD variants can be incremental; dispatch selects best available variant.

Public symbols are *not* exposed from here; use C dispatch layer:
- memcpy/memset/hash/utf8: selected via `src/vitte/asm_dispatch.c`

Conventions:
- All symbols are declared using `SYM()` from `common/asm_macros.S`
- Respect ABI: callee-saved regs, stack alignment.
