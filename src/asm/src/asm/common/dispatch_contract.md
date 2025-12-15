# Dispatch contract (ABI / correctness)

Requirements for every asm entry point:

- ABI calling convention must be respected (SysV x86_64 / AAPCS64).
- Do not clobber callee-saved registers.
- Stack alignment = 16 bytes at call boundaries.
- memcpy/memset:
  - if n==0: return dst without reading src
  - must not read/write past n
- fnv1a64:
  - must match the reference C implementation exactly
- utf8_validate:
  - must never read past n
  - must return 1 if valid UTF-8, else 0
