// vitte/src/asm/src/asm/common/asm_macros.S
//
// Common assembler macros for Vitte (GNU as syntax) — "max"
//
// Goals:
//   - Centralize portability knobs (ELF/Mach-O) and symbol decoration.
//   - Provide helpers for alignment, function prolog/epilog markers, visibility.
//   - Keep files consistent across aarch64/x86_64 sources.
//
// Supported assemblers:
//   - GNU as / clang integrated assembler (preferred)
//   - Notes: Some directives differ between Mach-O and ELF. We keep to a subset.
//
// Usage:
//   - Include this file (via .include or build system) from other .S files
//   - Example:
//       .include "asm_macros.S"
//       VITTE_FUNC_BEGIN(vitte_memcpy_neon)
//       ...
//       VITTE_FUNC_END(vitte_memcpy_neon)
//
// ---------------------------------------------------------------------------

    // -----------------------------------------------------------------------
    // Platform detection
    // -----------------------------------------------------------------------
    // The build system should define one of:
    //   - VITTE_ASM_ELF
    //   - VITTE_ASM_MACHO
    // If neither is defined, default to ELF.
    // -----------------------------------------------------------------------

#ifndef VITTE_ASM_ELF
#ifndef VITTE_ASM_MACHO
#define VITTE_ASM_ELF 1
#endif
#endif

    // -----------------------------------------------------------------------
    // Basic helpers
    // -----------------------------------------------------------------------

#ifndef VITTE_P2ALIGN
    // p2align N[,fill[,max]]
    // Align to 2^N bytes
#define VITTE_P2ALIGN(n)        .p2align n
#endif

#ifndef VITTE_ALIGN
    // align N => align to N bytes (gas: .balign)
#define VITTE_ALIGN(n)          .balign n
#endif

#ifndef VITTE_TEXT
#define VITTE_TEXT              .text
#endif

#ifndef VITTE_RODATA
#define VITTE_RODATA            .section .rodata
#endif

#ifndef VITTE_DATA
#define VITTE_DATA              .data
#endif

#ifndef VITTE_BSS
#define VITTE_BSS               .bss
#endif

    // -----------------------------------------------------------------------
    // Symbol visibility / linkage
    // -----------------------------------------------------------------------

#ifndef VITTE_GLOBAL
#define VITTE_GLOBAL(sym)       .global sym
#endif

#ifndef VITTE_HIDDEN
    // Not always supported on Mach-O; guarded below.
#if defined(VITTE_ASM_ELF)
#define VITTE_HIDDEN(sym)       .hidden sym
#else
#define VITTE_HIDDEN(sym)       /* hidden unsupported */
#endif
#endif

#ifndef VITTE_WEAK
#define VITTE_WEAK(sym)         .weak sym
#endif

#ifndef VITTE_TYPE_FUNC
    // ELF uses .type sym, %function
#if defined(VITTE_ASM_ELF)
#define VITTE_TYPE_FUNC(sym)    .type sym, %function
#else
#define VITTE_TYPE_FUNC(sym)    /* Mach-O: no .type */
#endif
#endif

#ifndef VITTE_SIZE_FUNC
    // ELF uses .size sym, .-sym
#if defined(VITTE_ASM_ELF)
#define VITTE_SIZE_FUNC(sym)    .size sym, .-sym
#else
#define VITTE_SIZE_FUNC(sym)    /* Mach-O: no .size */
#endif
#endif

    // -----------------------------------------------------------------------
    // Function begin/end markers
    // -----------------------------------------------------------------------

#ifndef VITTE_FUNC_BEGIN
#define VITTE_FUNC_BEGIN(sym)   \
    VITTE_TEXT;                 \
    VITTE_P2ALIGN(2);           \
    VITTE_GLOBAL(sym);          \
    VITTE_TYPE_FUNC(sym);       \
sym:
#endif

#ifndef VITTE_FUNC_END
#define VITTE_FUNC_END(sym)     \
    VITTE_SIZE_FUNC(sym)
#endif

    // -----------------------------------------------------------------------
    // Section helpers (optional)
    // -----------------------------------------------------------------------

#ifndef VITTE_SECTION_TEXT
#define VITTE_SECTION_TEXT()    VITTE_TEXT
#endif

#ifndef VITTE_SECTION_RODATA
#define VITTE_SECTION_RODATA()  VITTE_RODATA
#endif

#ifndef VITTE_SECTION_DATA
#define VITTE_SECTION_DATA()    VITTE_DATA
#endif

#ifndef VITTE_SECTION_BSS
#define VITTE_SECTION_BSS()     VITTE_BSS
#endif

    // -----------------------------------------------------------------------
    // Debug / CFI (optional)
    // -----------------------------------------------------------------------
    // If you want full unwind info, define VITTE_ASM_CFI=1 in build.
    // For leaf functions, we usually omit. Macros included for completeness.
    // -----------------------------------------------------------------------

#ifndef VITTE_ASM_CFI
#define VITTE_ASM_CFI 0
#endif

#if VITTE_ASM_CFI
#define VITTE_CFI_STARTPROC     .cfi_startproc
#define VITTE_CFI_ENDPROC       .cfi_endproc
#define VITTE_CFI_DEF_CFA(reg, off)  .cfi_def_cfa reg, off
#define VITTE_CFI_OFFSET(reg, off)  .cfi_offset reg, off
#define VITTE_CFI_SAME_VALUE(reg)   .cfi_same_value reg
#else
#define VITTE_CFI_STARTPROC     /* cfi off */
#define VITTE_CFI_ENDPROC       /* cfi off */
#define VITTE_CFI_DEF_CFA(reg, off) /* cfi off */
#define VITTE_CFI_OFFSET(reg, off)  /* cfi off */
#define VITTE_CFI_SAME_VALUE(reg)   /* cfi off */
#endif

    // -----------------------------------------------------------------------
    // AArch64 helpers
    // -----------------------------------------------------------------------

#ifndef VITTE_A64_DUP_BYTE_TO_VREG
    // dup vN.16b, wX
#define VITTE_A64_DUP_BYTE_TO_VREG(vn, wx) dup vn.16b, wx
#endif

#ifndef VITTE_A64_PREFETCH_LD
    // prfm pldl1keep, [addr, #imm]
#define VITTE_A64_PREFETCH_LD(addr, imm) prfm pldl1keep, [addr, #imm]
#endif

#ifndef VITTE_A64_PREFETCH_ST
    // prfm pstl1keep, [addr, #imm]
#define VITTE_A64_PREFETCH_ST(addr, imm) prfm pstl1keep, [addr, #imm]
#endif

    // -----------------------------------------------------------------------
    // x86_64 helpers (placeholders for future)
    // -----------------------------------------------------------------------
    // Kept for symmetry. If you add x86_64 asm, you can extend these.
    // -----------------------------------------------------------------------

#ifndef VITTE_X64_FUNC_BEGIN
#define VITTE_X64_FUNC_BEGIN(sym) VITTE_FUNC_BEGIN(sym)
#endif

#ifndef VITTE_X64_FUNC_END
#define VITTE_X64_FUNC_END(sym)   VITTE_FUNC_END(sym)
#endif

    // -----------------------------------------------------------------------
    // End of file
    // -----------------------------------------------------------------------
