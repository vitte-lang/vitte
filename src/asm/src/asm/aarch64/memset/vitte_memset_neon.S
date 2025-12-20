// vitte/src/asm/src/asm/aarch64/memset/vitte_memset_neon.S
//
// memset NEON (AArch64) — version "max"
//
// API/ABI (AAPCS64):
//   x0 = void*   dst
//   x1 = int     c   (low 8 bits used)
//   x2 = size_t  n
//   returns x0 = dst (original)
//
// Behavior:
//   - Writes n bytes of (uint8_t)c to dst.
//   - Handles n == 0.
//   - Unaligned dst supported.
//
// Perf strategy:
//   - Very small (<= 32): scalar + 16B store.
//   - Medium: 32/64-byte chunks.
//   - Large: 128B unrolled loop using q0..q7, with prefetch.
//   - Tail: 64/32/16/8/4/1.
//
// Notes:
//   - NEON available on all AArch64.
//   - This is still "memset" semantics, no cache-bypass tricks.
//
// Clobbers: x3..x12, v0..v7, flags

#include "common/asm_macros.S"

    VITTE_TEXT
    VITTE_P2ALIGN(2)
    VITTE_FUNC_BEGIN(vitte_memset_neon)
    // Save original dst for return
    mov     x3, x0

    // if (n == 0) return
    cbz     x2, .Lret

    // w4 = byte value
    and     w4, w1, #0xFF

    // Build 64-bit replicated pattern in x5 from byte (w4)
    // x5 = b | (b<<8) | (b<<16) | (b<<32) with cascading ORR trick
    uxtb    w5, w4
    orr     x5, x5, x5, lsl #8
    orr     x5, x5, x5, lsl #16
    orr     x5, x5, x5, lsl #32

    // Prepare NEON fill registers
    dup     v0.16b, w4
    mov     v1.16b, v0.16b
    mov     v2.16b, v0.16b
    mov     v3.16b, v0.16b
    mov     v4.16b, v0.16b
    mov     v5.16b, v0.16b
    mov     v6.16b, v0.16b
    mov     v7.16b, v0.16b

    // Very small: <= 32
    cmp     x2, #32
    b.hi    .Lgt_32

// ---------------------------------------------------------------------------
// Small (<= 32): 16 + scalar tail
// ---------------------------------------------------------------------------
.Lle_32:
    cmp     x2, #16
    b.lo    .Lsmall_lt16

    str     q0, [x0], #16
    sub     x2, x2, #16

.Lsmall_lt16:
    cmp     x2, #8
    b.lo    .Lsmall_lt8

    str     x5, [x0], #8
    sub     x2, x2, #8

.Lsmall_lt8:
    cmp     x2, #4
    b.lo    .Lsmall_tail

    str     w4, [x0], #4
    sub     x2, x2, #4

.Lsmall_tail:
    cbz     x2, .Lret
.Lsmall_b:
    strb    w4, [x0], #1
    subs    x2, x2, #1
    b.ne    .Lsmall_b
    b       .Lret

// ---------------------------------------------------------------------------
// Medium/Large (> 32)
// ---------------------------------------------------------------------------
.Lgt_32:
    // If very large, do 128B loop
    cmp     x2, #256
    b.lo    .Lmedium

// ---------------------------------------------------------------------------
// Huge loop: 128B per iter (8x16B stores)
// ---------------------------------------------------------------------------
.Lhuge:
    // Prefetch destination (write intent). Safe hint.
    prfm    pstl1keep, [x0, #256]

    cmp     x2, #128
    b.lo    .Lhuge_tail

.Lhuge_loop:
    // Store 128B
    stp     q0, q1, [x0, #0]
    stp     q2, q3, [x0, #32]
    stp     q4, q5, [x0, #64]
    stp     q6, q7, [x0, #96]

    add     x0, x0, #128
    sub     x2, x2, #128

    // Prefetch next window
    prfm    pstl1keep, [x0, #256]

    cmp     x2, #128
    b.hs    .Lhuge_loop

.Lhuge_tail:
    // fallthrough to medium tail handler (<128)
    b       .Lmedium_from_huge

// ---------------------------------------------------------------------------
// Medium: handle 33..255 or tail from huge
// ---------------------------------------------------------------------------
.Lmedium:
    prfm    pstl1keep, [x0, #128]

.Lmedium_from_huge:
    // Store 64 if possible
    cmp     x2, #64
    b.lo    .Llt_64

.Lloop_64:
    stp     q0, q1, [x0], #32
    stp     q2, q3, [x0], #32

    sub     x2, x2, #64
    cmp     x2, #64
    b.hs    .Lloop_64

// Now < 64
.Llt_64:
    // Store 32 if possible
    cmp     x2, #32
    b.lo    .Llt_32

    stp     q0, q1, [x0], #32
    sub     x2, x2, #32

// Now < 32
.Llt_32:
    // Store 16 if possible
    cmp     x2, #16
    b.lo    .Llt_16

    str     q0, [x0], #16
    sub     x2, x2, #16

// Now < 16
.Llt_16:
    // Store 8 if possible
    cmp     x2, #8
    b.lo    .Llt_8

    str     x5, [x0], #8
    sub     x2, x2, #8

// Now < 8
.Llt_8:
    // Store 4 if possible
    cmp     x2, #4
    b.lo    .Ltail_bytes

    str     w4, [x0], #4
    sub     x2, x2, #4

.Ltail_bytes:
    cbz     x2, .Lret
.Ltail_b:
    strb    w4, [x0], #1
    subs    x2, x2, #1
    b.ne    .Ltail_b

.Lret:
    mov     x0, x3
    ret

    VITTE_FUNC_END(vitte_memset_neon)
