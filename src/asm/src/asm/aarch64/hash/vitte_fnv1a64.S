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
//   targets (rare for aarch64), we'd need conditional handling. We assume LE.
//
// ---------------------------------------------------------------------------

    .text
    .p2align  2

    .global   vitte_fnv1a64
    .type     vitte_fnv1a64, %function

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
//   x7  = tmp shift/mask
//   x8  = saved ptr (optional / debug)
//   x9  = saved len (optional / debug)
//   x10 = scratch
//   x11 = scratch
//
// Clobbers: x2..x11, flags

vitte_fnv1a64:
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

    // Compute number of full 8-byte blocks: nblocks = len >> 3
    lsr     x6, x1, #3
    cbz     x6, .Ltail_setup

// ---------------------------------------------------------------------------
// Block loop: process 8 bytes at a time
// ---------------------------------------------------------------------------
.Lblock_loop:
    // Load 8 bytes (unaligned ok on AArch64)
    ldr     x4, [x0], #8

    // Process bytes in memory order:
    // byte0 = (x4 >> 0)  & 0xFF
    // byte1 = (x4 >> 8)  & 0xFF
    // ...
    // byte7 = (x4 >> 56) & 0xFF
    //
    // Unrolled for speed (8 steps).

    // b0
    and     x5, x4, #0xFF
    eor     x2, x2, x5
    mul     x2, x2, x3

    // b1
    lsr     x7, x4, #8
    and     x5, x7, #0xFF
    eor     x2, x2, x5
    mul     x2, x2, x3

    // b2
    lsr     x7, x4, #16
    and     x5, x7, #0xFF
    eor     x2, x2, x5
    mul     x2, x2, x3

    // b3
    lsr     x7, x4, #24
    and     x5, x7, #0xFF
    eor     x2, x2, x5
    mul     x2, x2, x3

    // b4
    lsr     x7, x4, #32
    and     x5, x7, #0xFF
    eor     x2, x2, x5
    mul     x2, x2, x3

    // b5
    lsr     x7, x4, #40
    and     x5, x7, #0xFF
    eor     x2, x2, x5
    mul     x2, x2, x3

    // b6
    lsr     x7, x4, #48
    and     x5, x7, #0xFF
    eor     x2, x2, x5
    mul     x2, x2, x3

    // b7
    lsr     x7, x4, #56
    and     x5, x7, #0xFF
    eor     x2, x2, x5
    mul     x2, x2, x3

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
    mul     x2, x2, x3

    subs    x6, x6, #1
    b.ne    .Ltail_loop

.Lret:
    mov     x0, x2
    ret

    .size   vitte_fnv1a64, .-vitte_fnv1a64
