// vitte/src/asm/src/asm/aarch64/hash/vitte_fnv1a64.S
//
// FNV-1a 64-bit (AArch64) — version "max"
//
// API/ABI (AAPCS64):
//   x0 = const uint8_t* data
//   x1 = size_t len
//   returns x0 = uint64_t hash
//
// Behavior:
//   hash = FNV_OFFSET_BASIS_64 (0xcbf29ce484222325)
//   for i in 0..len:
//     hash ^= data[i]
//     hash *= FNV_PRIME_64 (0x00000100000001B3)
//
// Design:
//   - Fast path: process 8 bytes / iter using AArch64 "rev" trick to keep semantics
//     while reducing loop overhead.
//   - Tail: process remaining 0..7 bytes.
//   - No alignment requirement (uses unaligned LDR).
//   - Pure integer ops, no NEON.
//
// Correctness note:
//   FNV-1a is byte-serial. To batch 8 bytes, we compute the same sequence by
//   iterating bytes from the loaded 64-bit word in memory order.
//   On AArch64 (little-endian), LDR loads into register with low bits holding
//   lowest-address byte, so we can shift/and to extract bytes. For big-endian
//   targets (rare for aarch64), we'd need conditional handling. This file
//   provides a big-endian fallback (byte loop) guarded by __AARCH64EB__.
//
// ---------------------------------------------------------------------------
#include "common/asm_macros.S"

    VITTE_TEXT
    VITTE_P2ALIGN(2)
    VITTE_FUNC_BEGIN(vitte_fnv1a64)

// ---------------------------------------------------------------------------
// Multiplication strategy
// ---------------------------------------------------------------------------
// Default uses MUL by the 64-bit FNV prime.
// Optionally, define VITTE_FNV1A64_USE_SHIFT_ADD_MUL to use a shift/add
// sequence for (hash *= 0x00000100000001B3). This can be faster or slower
// depending on core MUL latency/throughput — measure.
//
// Clobbers: tmp when shift/add path is enabled.
//
.macro VITTE_FNV1A64_MUL_BY_PRIME dst, src, prime, tmp
#if defined(VITTE_FNV1A64_USE_SHIFT_ADD_MUL)
    // prime = 2^40 + 0x1B3, and 0x1B3 = 256+128+32+16+2+1
    add     \tmp, \src, \src, lsl #1      // *3
    add     \tmp, \tmp, \src, lsl #4      // +*16  => *19
    add     \tmp, \tmp, \src, lsl #5      // +*32  => *51
    add     \tmp, \tmp, \src, lsl #7      // +*128 => *179
    add     \tmp, \tmp, \src, lsl #8      // +*256 => *435 (=0x1B3)
    add     \dst, \tmp, \src, lsl #40     // +*2^40
#else
    mul     \dst, \src, \prime
#endif
.endm

// Constants
//   OFFSET = 0xcbf29ce484222325
//   PRIME  = 0x00000100000001B3
//
// Register plan:
//   x0  = ptr (advances)
//   x1  = len (counts down)
//   x2  = hash
//   x3  = prime
//   x4  = tmp word (8B chunk)
//   x5  = tmp byte (expanded)
//   x6  = tail count
//   x8  = saved ptr (optional / debug)
//   x9  = saved len (optional / debug)
//   x10 = scratch
//   x11 = scratch
//
// Clobbers: x2..x11, flags

    // Load prime into x3
    // prime = 0x00000100000001B3
    movz    x3, #0x01B3
    movk    x3, #0x0001, lsl #32

    // Load offset basis into x2
    // hash = 0xcbf29ce484222325
    movz    x2, #0x2325
    movk    x2, #0x8422, lsl #16
    movk    x2, #0x9ce4, lsl #32
    movk    x2, #0xcbf2, lsl #48

    // If len == 0 => return offset
    cbz     x1, .Lret

#if defined(__AARCH64EB__)
// ---------------------------------------------------------------------------
// Big-endian fallback
// ---------------------------------------------------------------------------
    // This implementation assumes little-endian in the 8-byte block loop.
    // For __AARCH64EB__, use a simple byte loop to preserve semantics.
    mov     x6, x1
.Lbe_loop:
    ldrb    w5, [x0], #1
    eor     x2, x2, x5
    VITTE_FNV1A64_MUL_BY_PRIME x2, x2, x3, x10
    subs    x6, x6, #1
    b.ne    .Lbe_loop
    b       .Lret
#else
    // Compute number of full 8-byte blocks: nblocks = len >> 3
    lsr     x6, x1, #3
    cbz     x6, .Ltail_setup

// ---------------------------------------------------------------------------
// Block loop: process 8 bytes at a time
// ---------------------------------------------------------------------------
    VITTE_P2ALIGN(4)
.Lblock_loop:
    // Prefetch ahead to help throughput on large buffers.
    // Tuning note: offset is a hint; feel free to measure/adjust per target.
    VITTE_A64_PREFETCH_LD(x0, #256)

    // Load 8 bytes (unaligned ok on AArch64)
    ldr     x4, [x0], #8

    // Process bytes in memory order:
    // byte0 = (x4 >> 0)  & 0xFF
    // byte1 = (x4 >> 8)  & 0xFF
    // ...
    // byte7 = (x4 >> 56) & 0xFF
    //
    // Unrolled for speed (8 steps).

    // b0..b7: use UBFX to extract bytes directly from the loaded word.
    // This cuts instructions vs (shift+mask) while preserving byte-serial semantics.

    // b0
    ubfx    x5, x4, #0, #8
    eor     x2, x2, x5
    VITTE_FNV1A64_MUL_BY_PRIME x2, x2, x3, x10

    // b1
    ubfx    x5, x4, #8, #8
    eor     x2, x2, x5
    VITTE_FNV1A64_MUL_BY_PRIME x2, x2, x3, x10

    // b2
    ubfx    x5, x4, #16, #8
    eor     x2, x2, x5
    VITTE_FNV1A64_MUL_BY_PRIME x2, x2, x3, x10

    // b3
    ubfx    x5, x4, #24, #8
    eor     x2, x2, x5
    VITTE_FNV1A64_MUL_BY_PRIME x2, x2, x3, x10

    // b4
    ubfx    x5, x4, #32, #8
    eor     x2, x2, x5
    VITTE_FNV1A64_MUL_BY_PRIME x2, x2, x3, x10

    // b5
    ubfx    x5, x4, #40, #8
    eor     x2, x2, x5
    VITTE_FNV1A64_MUL_BY_PRIME x2, x2, x3, x10

    // b6
    ubfx    x5, x4, #48, #8
    eor     x2, x2, x5
    VITTE_FNV1A64_MUL_BY_PRIME x2, x2, x3, x10

    // b7
    ubfx    x5, x4, #56, #8
    eor     x2, x2, x5
    VITTE_FNV1A64_MUL_BY_PRIME x2, x2, x3, x10

    subs    x6, x6, #1
    b.ne    .Lblock_loop

// ---------------------------------------------------------------------------
// Tail
// ---------------------------------------------------------------------------
.Ltail_setup:
    // tail = len & 7
    and     x6, x1, #7
    cbz     x6, .Lret

.Ltail_loop:
    ldrb    w5, [x0], #1
    // w5 zero-extends to x5
    eor     x2, x2, x5
    VITTE_FNV1A64_MUL_BY_PRIME x2, x2, x3, x10

    subs    x6, x6, #1
    b.ne    .Ltail_loop
#endif

.Lret:
    mov     x0, x2
    ret

    VITTE_FUNC_END(vitte_fnv1a64)
