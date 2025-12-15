// vitte/src/asm/src/asm/aarch64/memcpy/vitte_memcpy_baseline.S
//
// memcpy baseline (AArch64) — version "max"
//
// API/ABI (AAPCS64):
//   x0 = void*       dst
//   x1 = const void* src
//   x2 = size_t      n
//   returns x0 = dst (original)
//
// Contract:
//   - Same semantics as C memcpy: src/dst must not overlap (use memmove otherwise).
//   - Handles n==0.
//   - Works with unaligned pointers.
//   - Reasonable perf: small-size unrolled, large-size 64B loop (NEON).
//
// Notes:
//   - Uses NEON (q0..q3) for bulk copy; this is standard on AArch64.
//   - If you want a strictly integer-only version, tell me and I’ll generate it.
//
// Register plan:
//   x0  dst (advances), x3 saved_dst
//   x1  src (advances)
//   x2  n
//   x4  tmp
//   x5  tmp
//   x6  tmp
//   x7  tmp
//   v0..v3 for 64B blocks
//
// Clobbers: x3..x7, v0..v3, flags

    .text
    .p2align 2

    .global vitte_memcpy_baseline
    .type   vitte_memcpy_baseline, %function

vitte_memcpy_baseline:
    // Save original dst for return
    mov     x3, x0

    // if (n == 0) return dst
    cbz     x2, .Lret

    // Small copies: handle up to 32 bytes with scalar loads/stores
    // This avoids NEON overhead for tiny sizes.
    cmp     x2, #32
    b.hi    .Lge_33

// ---------------------------------------------------------------------------
// Tiny path (0..32)
// ---------------------------------------------------------------------------
.Lle_32:
    // Strategy:
    //   - copy 16 if available
    //   - copy 8 if available
    //   - copy 4 if available
    //   - copy remaining bytes (0..3)
    //
    // This is not fully branchless, but predictable and safe.

    // if (n >= 16)
    cmp     x2, #16
    b.lo    .Llt_16

    ldr     q0, [x1]
    str     q0, [x0]
    add     x1, x1, #16
    add     x0, x0, #16
    sub     x2, x2, #16

.Llt_16:
    // if (n >= 8)
    cmp     x2, #8
    b.lo    .Llt_8

    ldr     x4, [x1]
    str     x4, [x0]
    add     x1, x1, #8
    add     x0, x0, #8
    sub     x2, x2, #8

.Llt_8:
    // if (n >= 4)
    cmp     x2, #4
    b.lo    .Llt_4

    ldr     w4, [x1]
    str     w4, [x0]
    add     x1, x1, #4
    add     x0, x0, #4
    sub     x2, x2, #4

.Llt_4:
    // remaining 0..3 bytes
    cbz     x2, .Lret

.Lbyte_loop:
    ldrb    w4, [x1], #1
    strb    w4, [x0], #1
    subs    x2, x2, #1
    b.ne    .Lbyte_loop
    b       .Lret

// ---------------------------------------------------------------------------
// Medium/Large path (>= 33)
// ---------------------------------------------------------------------------
.Lge_33:
    // Copy 64B blocks while possible
    cmp     x2, #64
    b.lo    .Llt_64

.Lloop_64:
    // Load 4x16B, store 4x16B
    ldp     q0, q1, [x1], #32
    ldp     q2, q3, [x1], #32

    stp     q0, q1, [x0], #32
    stp     q2, q3, [x0], #32

    sub     x2, x2, #64
    cmp     x2, #64
    b.hs    .Lloop_64

// Now n < 64
.Llt_64:
    // Copy 32 if possible
    cmp     x2, #32
    b.lo    .Llt_32_tail

    ldp     q0, q1, [x1], #32
    stp     q0, q1, [x0], #32
    sub     x2, x2, #32

.Llt_32_tail:
    // Copy 16 if possible
    cmp     x2, #16
    b.lo    .Llt_16_tail

    ldr     q0, [x1], #16
    str     q0, [x0], #16
    sub     x2, x2, #16

.Llt_16_tail:
    // Copy 8 if possible
    cmp     x2, #8
    b.lo    .Llt_8_tail

    ldr     x4, [x1], #8
    str     x4, [x0], #8
    sub     x2, x2, #8

.Llt_8_tail:
    // Copy 4 if possible
    cmp     x2, #4
    b.lo    .Llt_4_tail

    ldr     w4, [x1], #4
    str     w4, [x0], #4
    sub     x2, x2, #4

.Llt_4_tail:
    // Remaining bytes
    cbz     x2, .Lret
.Lbyte_tail:
    ldrb    w4, [x1], #1
    strb    w4, [x0], #1
    subs    x2, x2, #1
    b.ne    .Lbyte_tail

.Lret:
    mov     x0, x3
    ret

    .size vitte_memcpy_baseline, .-vitte_memcpy_baseline
