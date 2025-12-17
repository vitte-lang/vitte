// vitte/src/asm/src/asm/aarch64/utf8/vitte_utf8_validate_stub.S
//
// UTF-8 validate (AArch64) — stub "max"
//
// Purpose:
//   - Provide a stable symbol for the asm layer.
//   - Baseline behavior: validate UTF-8 (best-effort) without SIMD.
//   - Designed as a stub: can be replaced later by a NEON-accelerated validator.
//
// API/ABI (AAPCS64):
//   x0 = const uint8_t* data
//   x1 = size_t         len
//   returns:
//     w0 = int (1 = valid, 0 = invalid)
//
// Optional out-arg variant (not used here):
//   - If you later want: (data, len, &error_offset) -> valid/invalid.
//
// Notes:
//   - This is a strict UTF-8 validator:
//       * rejects overlong encodings
//       * rejects surrogate halves (U+D800..U+DFFF)
//       * enforces max codepoint U+10FFFF
//   - If you want "permissive" mode (accept CESU-8, etc.), change rules.
//
// Register plan:
//   x0 ptr, x1 len
//   x2 i / remaining
//   x3 byte0
//   x4 byte1
//   x5 byte2
//   x6 byte3
//   x7 tmp
//
// Clobbers: x2..x7, flags
//
// Return convention:
//   w0 = 1 if ok else 0

#include "common/asm_macros.S"

    VITTE_P2ALIGN(2)
    VITTE_FUNC_BEGIN(vitte_utf8_validate_stub)
    // if (len == 0) return 1
    cbz     x1, .Lok

    mov     x2, x1          // remaining = len

.Lloop:
    // Load first byte
    ldrb    w3, [x0], #1
    subs    x2, x2, #1

    // ASCII fast path: b0 < 0x80
    tbz     w3, #7, .Lcont_or_end

    // Reject 0x80..0xC1 (continuation or overlong lead)
    // if (b0 < 0xC2) invalid
    cmp     w3, #0xC2
    b.lo    .Lbad

    // 2-byte sequence: 0xC2..0xDF
    cmp     w3, #0xE0
    b.lo    .Ltwo

    // 3-byte sequence: 0xE0..0xEF
    cmp     w3, #0xF0
    b.lo    .Lthree

    // 4-byte sequence: 0xF0..0xF4
    cmp     w3, #0xF5
    b.hs    .Lbad
    b       .Lfour

// ---------------------------------
// 2-byte: 110xxxxx 10xxxxxx
// ---------------------------------
.Ltwo:
    // need 1 continuation
    cbz     x2, .Lbad
    ldrb    w4, [x0], #1
    subs    x2, x2, #1

    // (b1 & 0xC0) == 0x80
    and     w7, w4, #0xC0
    cmp     w7, #0x80
    b.ne    .Lbad
    b       .Lcont_or_end

// ---------------------------------
// 3-byte: 1110xxxx 10xxxxxx 10xxxxxx
// Rules:
//   - b0==0xE0 => b1 >= 0xA0 (no overlong)
//   - b0==0xED => b1 <= 0x9F (no surrogates)
// ---------------------------------
.Lthree:
    // need 2 continuations
    cmp     x2, #2
    b.lo    .Lbad
    ldrb    w4, [x0], #1
    ldrb    w5, [x0], #1
    sub     x2, x2, #2

    // check b1 continuation
    and     w7, w4, #0xC0
    cmp     w7, #0x80
    b.ne    .Lbad
    // check b2 continuation
    and     w7, w5, #0xC0
    cmp     w7, #0x80
    b.ne    .Lbad

    // special constraints
    cmp     w3, #0xE0
    b.ne    .Lchk_ed
    // if b1 < 0xA0 => overlong
    cmp     w4, #0xA0
    b.lo    .Lbad
    b       .Lcont_or_end

.Lchk_ed:
    cmp     w3, #0xED
    b.ne    .Lcont_or_end
    // if b1 > 0x9F => surrogate range
    cmp     w4, #0x9F
    b.hi    .Lbad
    b       .Lcont_or_end

// ---------------------------------
// 4-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
// Rules:
//   - b0==0xF0 => b1 >= 0x90 (no overlong)
//   - b0==0xF4 => b1 <= 0x8F (<= U+10FFFF)
// ---------------------------------
.Lfour:
    // need 3 continuations
    cmp     x2, #3
    b.lo    .Lbad
    ldrb    w4, [x0], #1
    ldrb    w5, [x0], #1
    ldrb    w6, [x0], #1
    sub     x2, x2, #3

    // b1 continuation
    and     w7, w4, #0xC0
    cmp     w7, #0x80
    b.ne    .Lbad
    // b2 continuation
    and     w7, w5, #0xC0
    cmp     w7, #0x80
    b.ne    .Lbad
    // b3 continuation
    and     w7, w6, #0xC0
    cmp     w7, #0x80
    b.ne    .Lbad

    // special constraints: F0 / F4
    cmp     w3, #0xF0
    b.ne    .Lchk_f4
    // if b1 < 0x90 => overlong
    cmp     w4, #0x90
    b.lo    .Lbad
    b       .Lcont_or_end

.Lchk_f4:
    cmp     w3, #0xF4
    b.ne    .Lcont_or_end
    // if b1 > 0x8F => > U+10FFFF
    cmp     w4, #0x8F
    b.hi    .Lbad
    b       .Lcont_or_end

// ---------------------------------
// Loop control
// ---------------------------------
.Lcont_or_end:
    cbz     x2, .Lok
    b       .Lloop

.Lok:
    mov     w0, #1
    ret

.Lbad:
    mov     w0, #0
    ret

    VITTE_FUNC_END(vitte_utf8_validate_stub)
