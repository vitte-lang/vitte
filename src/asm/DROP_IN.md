# asm/ drop-in package for vitte-c

This zip is meant to be copied into your repository root.
It contains:
- `src/asm/` : GAS `.S` implementations + per-arch layout
- `include/vitte/asm.h` : stable API
- `include/vitte/cpu.h` : feature bits
- `src/vitte/asm_dispatch.c` : routing ref/fast via function pointers
- `src/vitte/cpu.c` : cpuid/getauxval feature detection (Linux/macOS)

## Minimal CMake wiring (root CMakeLists.txt)

```cmake
enable_language(ASM)

target_sources(vitte PRIVATE
  src/vitte/asm_dispatch.c
  src/vitte/cpu.c
)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
  target_sources(vitte PRIVATE
    src/asm/common/asm_macros.S
    src/asm/x86_64/memcpy/vitte_memcpy_baseline.S
    src/asm/x86_64/memcpy/vitte_memcpy_sse2.S
    src/asm/x86_64/memcpy/vitte_memcpy_avx2.S
    src/asm/x86_64/memset/vitte_memset_baseline.S
    src/asm/x86_64/memset/vitte_memset_sse2.S
    src/asm/x86_64/memset/vitte_memset_avx2.S
    src/asm/x86_64/hash/vitte_fnv1a64.S
    src/asm/x86_64/utf8/vitte_utf8_validate_stub.S
  )
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
  target_sources(vitte PRIVATE
    src/asm/common/asm_macros.S
    src/asm/aarch64/memcpy/vitte_memcpy_baseline.S
    src/asm/aarch64/memcpy/vitte_memcpy_neon.S
    src/asm/aarch64/memset/vitte_memset_baseline.S
    src/asm/aarch64/memset/vitte_memset_neon.S
    src/asm/aarch64/hash/vitte_fnv1a64.S
    src/asm/aarch64/utf8/vitte_utf8_validate_stub.S
  )
endif()
```

## Notes
- SIMD variants included here are **functional placeholders** (they route to baseline). Replace them progressively.
- `utf8_validate_fast` is a stub that delegates to a C reference validator you provide (recommended), or keep it unused.
