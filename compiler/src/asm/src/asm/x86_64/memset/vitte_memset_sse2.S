// vitte/src/asm/src/asm/x86_64/memset/vitte_memset_sse2.S
//
// memset SSE2 (x86_64 SysV) — "max"
//
// ABI (SysV AMD64):
//   rdi = void*  dst
//   rsi = int    c   (low 8 bits used)
//   rdx = size_t n
//   rax = return dst (original)
//
// Behavior:
//   - Writes n bytes of (uint8_t)c.
//   - n==0 supported.
//   - Unaligned dst supported (movdqu).
//
// Strategy:
//   - <= 32  : 16 + 8 + 4 + bytes
//   - >= 64  : 64B loop using XMM0 fill
//   - Tail   : 32/16/8/4/1
//
// Notes:
//   - SSE2 baseline on x86_64.
//   - No AVX used.
//
// Clobbers:
//   rax, rcx, r8..r11, xmm0, flags

#include "common/asm_macros.S"

    VITTE_TEXT
    VITTE_P2ALIGN(4)
    VITTE_FUNC_BEGIN(vitte_memset_sse2)
    mov     %rdi, %rax            // save original dst

    test    %rdx, %rdx
    je      .Lret

    // byte value -> r8b
    mov     %esi, %r8d
    and     $0xFF, %r8d

    // pattern64 -> r9
    movzbq  %r8b, %r9
    mov     %r9, %r10
    shl     $8, %r10
    or      %r10, %r9
    mov     %r9, %r10
    shl     $16, %r10
    or      %r10, %r9
    mov     %r9, %r10
    shl     $32, %r10
    or      %r10, %r9

    // XMM0 = repeated byte using SSE2 integer ops
    movd    %r8d, %xmm0
    punpcklbw %xmm0, %xmm0
    punpcklwd %xmm0, %xmm0
    pshufd  $0, %xmm0, %xmm0

    // small <= 32
    cmp     $32, %rdx
    ja      .Lgt_32

// ---------------------------------------------------------------------------
// Small path (<= 32)
// ---------------------------------------------------------------------------
.Lle_32:
    cmp     $16, %rdx
    jb      .Lsm_lt16
    movdqu  %xmm0, (%rdi)
    add     $16, %rdi
    sub     $16, %rdx

.Lsm_lt16:
    cmp     $8, %rdx
    jb      .Lsm_lt8
    mov     %r9, (%rdi)
    add     $8, %rdi
    sub     $8, %rdx

.Lsm_lt8:
    cmp     $4, %rdx
    jb      .Lsm_tail
    mov     %r8d, (%rdi)
    add     $4, %rdi
    sub     $4, %rdx

.Lsm_tail:
    test    %rdx, %rdx
    je      .Lret
.Lsm_b:
    mov     %r8b, (%rdi)
    inc     %rdi
    dec     %rdx
    jne     .Lsm_b
    jmp     .Lret

// ---------------------------------------------------------------------------
// Large path (> 32): 64B loop
// ---------------------------------------------------------------------------
.Lgt_32:
    cmp     $64, %rdx
    jb      .Llt_64

.Lloop_64:
    movdqu  %xmm0,  0(%rdi)
    movdqu  %xmm0, 16(%rdi)
    movdqu  %xmm0, 32(%rdi)
    movdqu  %xmm0, 48(%rdi)

    add     $64, %rdi
    sub     $64, %rdx
    cmp     $64, %rdx
    jae     .Lloop_64

// Tail < 64
.Llt_64:
    // 32
    cmp     $32, %rdx
    jb      .Llt_32
    movdqu  %xmm0,  0(%rdi)
    movdqu  %xmm0, 16(%rdi)
    add     $32, %rdi
    sub     $32, %rdx

.Llt_32:
    // 16
    cmp     $16, %rdx
    jb      .Llt_16
    movdqu  %xmm0, (%rdi)
    add     $16, %rdi
    sub     $16, %rdx

.Llt_16:
    // 8
    cmp     $8, %rdx
    jb      .Llt_8
    mov     %r9, (%rdi)
    add     $8, %rdi
    sub     $8, %rdx

.Llt_8:
    // 4
    cmp     $4, %rdx
    jb      .Ltail_bytes
    mov     %r8d, (%rdi)
    add     $4, %rdi
    sub     $4, %rdx

.Ltail_bytes:
    test    %rdx, %rdx
    je      .Lret
.Ltb:
    mov     %r8b, (%rdi)
    inc     %rdi
    dec     %rdx
    jne     .Ltb

.Lret:
    ret

    VITTE_FUNC_END(vitte_memset_sse2)
