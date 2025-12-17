// vitte/src/asm/src/asm/aarch64/memcpy/vitte_memcpy_neon.S
//
// memcpy NEON (AArch64) — version "max"
//
// API/ABI (AAPCS64):
//   x0 = void*       dst
//   x1 = const void* src
//   x2 = size_t      n
//   returns x0 = dst (original)
//
// Contract:
//   - memcpy semantics (NO overlap guarantee). For overlap use memmove.
//   - Unaligned pointers supported.
//   - n==0 supported.
//
// Perf strategy (pragmatic, portable AArch64):
//   - Very small: scalar (<= 32).
//   - Medium: 16/32-byte NEON chunks.
//   - Large: 128B unrolled loop using q0..q7.
//   - Optional prefetch on big copies (safe no-op if ignored).
//
// Notes:
//   - Keeps code simple: no alignment “fixups” required on AArch64.
//   - Uses only caller-saved vector regs (v0..v7).
//
// Clobbers:
//   x3..x12, v0..v7, flags

#include "common/asm_macros.S"

    VITTE_P2ALIGN(2)
    VITTE_FUNC_BEGIN(vitte_memcpy_neon)
    // Save original dst for return
    mov     x3, x0

    // if (n == 0) return
    cbz     x2, .Lret

    // if (n <= 32) use scalar+small-neon path
    cmp     x2, #32
    b.hi    .Lgt_32

// ---------------------------------------------------------------------------
// Small (<= 32): mix of NEON 16 + scalar tail
// ---------------------------------------------------------------------------
.Lle_32:
    cmp     x2, #16
    b.lo    .Lsmall_lt16

    // copy 16
    ldr     q0, [x1]
    str     q0, [x0]
    add     x1, x1, #16
    add     x0, x0, #16
    sub     x2, x2, #16

.Lsmall_lt16:
    // copy 8
    cmp     x2, #8
    b.lo    .Lsmall_lt8
    ldr     x4, [x1]
    str     x4, [x0]
    add     x1, x1, #8
    add     x0, x0, #8
    sub     x2, x2, #8

.Lsmall_lt8:
    // copy 4
    cmp     x2, #4
    b.lo    .Lsmall_tail
    ldr     w4, [x1]
    str     w4, [x0]
    add     x1, x1, #4
    add     x0, x0, #4
    sub     x2, x2, #4

.Lsmall_tail:
    // copy remaining 0..3 bytes
    cbz     x2, .Lret
.Lsmall_b:
    ldrb    w4, [x1], #1
    strb    w4, [x0], #1
    subs    x2, x2, #1
    b.ne    .Lsmall_b
    b       .Lret

// ---------------------------------------------------------------------------
// Medium/Large (> 32)
// ---------------------------------------------------------------------------
.Lgt_32:
    // If very large, do 128B unrolled loop
    cmp     x2, #256
    b.lo    .Lmedium

// ---------------------------------------------------------------------------
// Huge loop: 128B per iter (8x16B)
// ---------------------------------------------------------------------------
.Lhuge:
    // Prefetch (tunable). Harmless if ignored by uarch.
    // prfm PLDL1KEEP, [x1, #256]
    // prfm PSTL1KEEP, [x0, #256]
    prfm    pldl1keep, [x1, #256]
    prfm    pstl1keep, [x0, #256]

    cmp     x2, #128
    b.lo    .Lhuge_tail

.Lhuge_loop:
    // load 128B from src
    ldp     q0, q1, [x1, #0]
    ldp     q2, q3, [x1, #32]
    ldp     q4, q5, [x1, #64]
    ldp     q6, q7, [x1, #96]

    // store 128B to dst
    stp     q0, q1, [x0, #0]
    stp     q2, q3, [x0, #32]
    stp     q4, q5, [x0, #64]
    stp     q6, q7, [x0, #96]

    add     x1, x1, #128
    add     x0, x0, #128
    sub     x2, x2, #128

    // prefetch next window
    prfm    pldl1keep, [x1, #256]
    prfm    pstl1keep, [x0, #256]

    cmp     x2, #128
    b.hs    .Lhuge_loop

.Lhuge_tail:
    // fallthrough to medium handler with remaining (<128)
    b       .Lmedium_from_huge

// ---------------------------------------------------------------------------
// Medium: handle 33..255 (or tail from huge)
// ---------------------------------------------------------------------------
.Lmedium:
    // Optional: bring cache lines earlier for moderate sizes too
    prfm    pldl1keep, [x1, #128]
    prfm    pstl1keep, [x0, #128]

.Lmedium_from_huge:
    // Copy 64 if possible
    cmp     x2, #64
    b.lo    .Llt_64

.Lloop_64:
    ldp     q0, q1, [x1], #32
    ldp     q2, q3, [x1], #32
    stp     q0, q1, [x0], #32
    stp     q2, q3, [x0], #32

    sub     x2, x2, #64
    cmp     x2, #64
    b.hs    .Lloop_64

// Now < 64
.Llt_64:
    // Copy 32 if possible
    cmp     x2, #32
    b.lo    .Llt_32

    ldp     q0, q1, [x1], #32
    stp     q0, q1, [x0], #32
    sub     x2, x2, #32

// Now < 32
.Llt_32:
    // Copy 16 if possible
    cmp     x2, #16
    b.lo    .Llt_16

    ldr     q0, [x1], #16
    str     q0, [x0], #16
    sub     x2, x2, #16

// Now < 16
.Llt_16:
    // Copy 8 if possible
    cmp     x2, #8
    b.lo    .Llt_8

    ldr     x4, [x1], #8
    str     x4, [x0], #8
    sub     x2, x2, #8

// Now < 8
.Llt_8:
    // Copy 4 if possible
    cmp     x2, #4
    b.lo    .Ltail_bytes

    ldr     w4, [x1], #4
    str     w4, [x0], #4
    sub     x2, x2, #4

.Ltail_bytes:
    cbz     x2, .Lret
.Ltail_b:
    ldrb    w4, [x1], #1
    strb    w4, [x0], #1
    subs    x2, x2, #1
    b.ne    .Ltail_b

.Lret:
    mov     x0, x3
    ret

    VITTE_FUNC_END(vitte_memcpy_neon)
