// vitte/src/asm/src/asm/x86_64/memcpy/vitte_memcpy_baseline.S
//
// memcpy baseline (x86_64 SysV) — "max"
//
// ABI (SysV AMD64):
//   rdi = void*       dst
//   rsi = const void* src
//   rdx = size_t      n
//   rax = return dst (original)
//
// Contract:
//   - memcpy semantics (NO overlap guarantee).
//   - Unaligned pointers supported.
//   - n==0 supported.
//
// Strategy:
//   - Small (<= 32): integer moves.
//   - Medium: 64B loop using SSE XMM0..XMM3 (movdqu).
//   - Tail: 32/16/8/4/1.
//
// Notes:
//   - Uses XMM regs only (no AVX), no vzeroupper needed.
//   - Uses only caller-saved regs + xmm0..xmm3.
//
// Clobbers:
//   rax, rcx, r8..r11, xmm0..xmm3, flags

#include "common/asm_macros.S"

    VITTE_P2ALIGN(4)
    VITTE_FUNC_BEGIN(vitte_memcpy_baseline)
    mov     %rdi, %rax            // save original dst for return

    test    %rdx, %rdx
    je      .Lret

    // Small (<= 32)
    cmp     $32, %rdx
    ja      .Lgt_32

// ---------------------------------------------------------------------------
// Small path (<= 32): integer moves
// ---------------------------------------------------------------------------
.Lle_32:
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
    je      .Lret
.Lsm_b:
    movzbq  (%rsi), %r8
    mov     %r8b, (%rdi)
    inc     %rsi
    inc     %rdi
    dec     %rdx
    jne     .Lsm_b
    jmp     .Lret

// ---------------------------------------------------------------------------
// Medium/Large (> 32): 64B SSE loop
// ---------------------------------------------------------------------------
.Lgt_32:
    cmp     $64, %rdx
    jb      .Llt_64

.Lloop_64:
    movdqu  0(%rsi), %xmm0
    movdqu 16(%rsi), %xmm1
    movdqu 32(%rsi), %xmm2
    movdqu 48(%rsi), %xmm3

    movdqu  %xmm0,  0(%rdi)
    movdqu  %xmm1, 16(%rdi)
    movdqu  %xmm2, 32(%rdi)
    movdqu  %xmm3, 48(%rdi)

    add     $64, %rsi
    add     $64, %rdi
    sub     $64, %rdx
    cmp     $64, %rdx
    jae     .Lloop_64

// Tail < 64
.Llt_64:
    // Copy 32 if possible
    cmp     $32, %rdx
    jb      .Llt_32
    movdqu  0(%rsi), %xmm0
    movdqu 16(%rsi), %xmm1
    movdqu  %xmm0,  0(%rdi)
    movdqu  %xmm1, 16(%rdi)
    add     $32, %rsi
    add     $32, %rdi
    sub     $32, %rdx

.Llt_32:
    // Copy 16 if possible
    cmp     $16, %rdx
    jb      .Llt_16
    movdqu  0(%rsi), %xmm0
    movdqu  %xmm0,  0(%rdi)
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
    je      .Lret
.Ltb:
    movzbq  (%rsi), %r8
    mov     %r8b, (%rdi)
    inc     %rsi
    inc     %rdi
    dec     %rdx
    jne     .Ltb

.Lret:
    ret

    VITTE_FUNC_END(vitte_memcpy_baseline)
