// vitte/src/asm/src/asm/x86_64/memcpy/vitte_memcpy_avx2.S
//
// memcpy AVX2 (x86_64 SysV) — version "max"
//
// ABI (SysV AMD64):
//   rdi = void*       dst
//   rsi = const void* src
//   rdx = size_t      n
//   rax = return dst (original)
//
// Contract:
//   - memcpy semantics: NO overlap guarantee (UB if overlap).
//   - Unaligned pointers supported (vmovdqu).
//   - n==0 supported.
//
// Strategy:
//   - <= 32  : scalar / SSE-ish via movsq/movsl/movsb style (here: integer moves)
//   - <= 128 : 16/32B chunks
//   - >= 256 : 256B loop unrolled (8x32B) + prefetch
//   - Tail   : 128/64/32/16/8/4/1
//
// Notes:
//   - Uses YMM registers; executes vzeroupper before return to avoid AVX->SSE penalty.
//   - Requires AVX2 at runtime; dispatch must ensure capability.
//   - Uses only caller-saved regs + ymm0..ymm7.
//
// Clobbers:
//   rax, rcx, r8..r11, ymm0..ymm7, flags

#include "common/asm_macros.S"

    VITTE_TEXT
    VITTE_P2ALIGN(4)
    VITTE_FUNC_BEGIN(vitte_memcpy_avx2)
    mov     %rdi, %rax            // save original dst for return

    test    %rdx, %rdx
    je      .Lret_zu

    // Small (<= 32): keep it simple and branchy but fast enough
    cmp     $32, %rdx
    ja      .Lgt_32

// ---------------------------------------------------------------------------
// Small path: up to 32 bytes using integer moves (safe unaligned)
// ---------------------------------------------------------------------------
.Lle_32:
    // if (n >= 16) copy 16
    cmp     $16, %rdx
    jb      .Lsm_lt16
    mov     (%rsi), %r8
    mov     8(%rsi), %r9
    mov     %r8, (%rdi)
    mov     %r9, 8(%rdi)
    add     $16, %rsi
    add     $16, %rdi
    sub     $16, %rdx

.Lsm_lt16:
    cmp     $8, %rdx
    jb      .Lsm_lt8
    mov     (%rsi), %r8
    mov     %r8, (%rdi)
    add     $8, %rsi
    add     $8, %rdi
    sub     $8, %rdx

.Lsm_lt8:
    cmp     $4, %rdx
    jb      .Lsm_tail
    mov     (%rsi), %r8d
    mov     %r8d, (%rdi)
    add     $4, %rsi
    add     $4, %rdi
    sub     $4, %rdx

.Lsm_tail:
    test    %rdx, %rdx
    je      .Lret_zu
.Lsm_b:
    movzbq  (%rsi), %r8
    mov     %r8b, (%rdi)
    inc     %rsi
    inc     %rdi
    dec     %rdx
    jne     .Lsm_b
    jmp     .Lret_zu

// ---------------------------------------------------------------------------
// Medium/Large
// ---------------------------------------------------------------------------
.Lgt_32:
    // If very large, do 256B loop (8x32)
    cmp     $512, %rdx
    jb      .Lmedium

// ---------------------------------------------------------------------------
// Huge loop: 256 bytes per iter (8 * 32B)
// ---------------------------------------------------------------------------
.Lhuge:
    // blocks256 = n / 256
    mov     %rdx, %rcx
    shr     $8, %rcx               // /256
    je      .Lhuge_tail

.Lhuge_loop:
    // Prefetch future src lines (tuneable)
    prefetcht0  512(%rsi)
    prefetcht0  576(%rsi)

    vmovdqu  0(%rsi), %ymm0
    vmovdqu 32(%rsi), %ymm1
    vmovdqu 64(%rsi), %ymm2
    vmovdqu 96(%rsi), %ymm3
    vmovdqu 128(%rsi), %ymm4
    vmovdqu 160(%rsi), %ymm5
    vmovdqu 192(%rsi), %ymm6
    vmovdqu 224(%rsi), %ymm7

    vmovdqu  %ymm0,   0(%rdi)
    vmovdqu  %ymm1,  32(%rdi)
    vmovdqu  %ymm2,  64(%rdi)
    vmovdqu  %ymm3,  96(%rdi)
    vmovdqu  %ymm4, 128(%rdi)
    vmovdqu  %ymm5, 160(%rdi)
    vmovdqu  %ymm6, 192(%rdi)
    vmovdqu  %ymm7, 224(%rdi)

    add     $256, %rsi
    add     $256, %rdi
    dec     %rcx
    jne     .Lhuge_loop

.Lhuge_tail:
    // remaining n = n & 255
    and     $255, %rdx
    test    %rdx, %rdx
    je      .Lret_zu

    // fallthrough to medium/tail handling
    jmp     .Lmedium_from_huge

// ---------------------------------------------------------------------------
// Medium: handle remaining (<512) or tail from huge
// ---------------------------------------------------------------------------
.Lmedium:
.Lmedium_from_huge:
    // Copy 128 if possible (4x32B)
    cmp     $128, %rdx
    jb      .Llt_128
.Lloop_128:
    vmovdqu  0(%rsi), %ymm0
    vmovdqu 32(%rsi), %ymm1
    vmovdqu 64(%rsi), %ymm2
    vmovdqu 96(%rsi), %ymm3
    vmovdqu  %ymm0,  0(%rdi)
    vmovdqu  %ymm1, 32(%rdi)
    vmovdqu  %ymm2, 64(%rdi)
    vmovdqu  %ymm3, 96(%rdi)
    add     $128, %rsi
    add     $128, %rdi
    sub     $128, %rdx
    cmp     $128, %rdx
    jae     .Lloop_128

.Llt_128:
    // Copy 64 if possible (2x32B)
    cmp     $64, %rdx
    jb      .Llt_64
    vmovdqu  0(%rsi), %ymm0
    vmovdqu 32(%rsi), %ymm1
    vmovdqu  %ymm0,  0(%rdi)
    vmovdqu  %ymm1, 32(%rdi)
    add     $64, %rsi
    add     $64, %rdi
    sub     $64, %rdx

.Llt_64:
    // Copy 32 if possible
    cmp     $32, %rdx
    jb      .Llt_32
    vmovdqu  0(%rsi), %ymm0
    vmovdqu  %ymm0,  0(%rdi)
    add     $32, %rsi
    add     $32, %rdi
    sub     $32, %rdx

.Llt_32:
    // From here, use scalar for < 32
    test    %rdx, %rdx
    je      .Lret_zu

    // Copy 16
    cmp     $16, %rdx
    jb      .Llt_16
    mov     (%rsi), %r8
    mov     8(%rsi), %r9
    mov     %r8, (%rdi)
    mov     %r9, 8(%rdi)
    add     $16, %rsi
    add     $16, %rdi
    sub     $16, %rdx

.Llt_16:
    // Copy 8
    cmp     $8, %rdx
    jb      .Llt_8
    mov     (%rsi), %r8
    mov     %r8, (%rdi)
    add     $8, %rsi
    add     $8, %rdi
    sub     $8, %rdx

.Llt_8:
    // Copy 4
    cmp     $4, %rdx
    jb      .Ltail_bytes
    mov     (%rsi), %r8d
    mov     %r8d, (%rdi)
    add     $4, %rsi
    add     $4, %rdi
    sub     $4, %rdx

.Ltail_bytes:
    test    %rdx, %rdx
    je      .Lret_zu
.Ltb:
    movzbq  (%rsi), %r8
    mov     %r8b, (%rdi)
    inc     %rsi
    inc     %rdi
    dec     %rdx
    jne     .Ltb

// ---------------------------------------------------------------------------
// Return
// ---------------------------------------------------------------------------
.Lret_zu:
    vzeroupper
    ret

    VITTE_FUNC_END(vitte_memcpy_avx2)
