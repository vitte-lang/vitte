// vitte/src/asm/src/asm/x86_64/memset/vitte_memset_avx2.S
//
// memset AVX2 (x86_64 SysV) — "max"
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
//   - Unaligned dst supported (vmovdqu).
//
// Strategy:
//   - <= 32  : scalar/int stores
//   - >= 256 : 256B loop unrolled (8x32B stores) + prefetch
//   - Tail   : 128/64/32/16/8/4/1
//
// Notes:
//   - Requires AVX2 at runtime; dispatch must ensure capability.
//   - Uses vpbroadcastb to build the fill vector.
//   - Executes vzeroupper before return.
//
// Clobbers:
//   rax, rcx, r8..r11, ymm0..ymm3, flags

#include "common/asm_macros.S"

    VITTE_TEXT
    VITTE_P2ALIGN(4)
    VITTE_FUNC_BEGIN(vitte_memset_avx2)
    mov     %rdi, %rax            // save original dst

    test    %rdx, %rdx
    je      .Lret_zu

    // w/low byte of c
    mov     %esi, %r8d
    and     $0xFF, %r8d

    // Build 64-bit pattern in r9 (replicated byte)
    mov     %r8b, %r9b
    movzbq  %r9b, %r9
    mov     %r9, %r10
    shl     $8, %r10
    or      %r10, %r9
    mov     %r9, %r10
    shl     $16, %r10
    or      %r10, %r9
    mov     %r9, %r10
    shl     $32, %r10
    or      %r10, %r9            // r9 = pattern64

    // Build YMM0 = broadcast byte
    // Use XMM tmp with the byte then broadcast.
    movd    %r8d, %xmm0
    vpbroadcastb %xmm0, %ymm0

    // Small (<= 32)
    cmp     $32, %rdx
    ja      .Lgt_32

// ---------------------------------------------------------------------------
// Small path (<=32): 16/8/4/1
// ---------------------------------------------------------------------------
.Lle_32:
    cmp     $16, %rdx
    jb      .Lsm_lt16

    vmovdqu %xmm0, (%rdi)
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
    je      .Lret_zu
.Lsm_b:
    mov     %r8b, (%rdi)
    inc     %rdi
    dec     %rdx
    jne     .Lsm_b
    jmp     .Lret_zu

// ---------------------------------------------------------------------------
// Large path (>32)
// ---------------------------------------------------------------------------
.Lgt_32:
    // If huge, do 256B loop
    cmp     $512, %rdx
    jb      .Lmedium

// 256B blocks
.Lhuge:
    mov     %rdx, %rcx
    shr     $8, %rcx              // /256
    je      .Lhuge_tail

.Lhuge_loop:
    // Prefetch destination
    prefetcht0  512(%rdi)

    vmovdqu %ymm0,   0(%rdi)
    vmovdqu %ymm0,  32(%rdi)
    vmovdqu %ymm0,  64(%rdi)
    vmovdqu %ymm0,  96(%rdi)
    vmovdqu %ymm0, 128(%rdi)
    vmovdqu %ymm0, 160(%rdi)
    vmovdqu %ymm0, 192(%rdi)
    vmovdqu %ymm0, 224(%rdi)

    add     $256, %rdi
    dec     %rcx
    jne     .Lhuge_loop

.Lhuge_tail:
    and     $255, %rdx
    test    %rdx, %rdx
    je      .Lret_zu
    jmp     .Lmedium_from_huge

// Medium/tail
.Lmedium:
.Lmedium_from_huge:
    // 128
    cmp     $128, %rdx
    jb      .Llt_128
.Lloop_128:
    vmovdqu %ymm0,   0(%rdi)
    vmovdqu %ymm0,  32(%rdi)
    vmovdqu %ymm0,  64(%rdi)
    vmovdqu %ymm0,  96(%rdi)
    add     $128, %rdi
    sub     $128, %rdx
    cmp     $128, %rdx
    jae     .Lloop_128

.Llt_128:
    // 64
    cmp     $64, %rdx
    jb      .Llt_64
    vmovdqu %ymm0,  0(%rdi)
    vmovdqu %ymm0, 32(%rdi)
    add     $64, %rdi
    sub     $64, %rdx

.Llt_64:
    // 32
    cmp     $32, %rdx
    jb      .Llt_32
    vmovdqu %ymm0, (%rdi)
    add     $32, %rdi
    sub     $32, %rdx

.Llt_32:
    // <32: 16/8/4/1 scalar-ish
    test    %rdx, %rdx
    je      .Lret_zu

    // 16
    cmp     $16, %rdx
    jb      .Llt_16
    vmovdqu %xmm0, (%rdi)
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
    je      .Lret_zu
.Ltb:
    mov     %r8b, (%rdi)
    inc     %rdi
    dec     %rdx
    jne     .Ltb

.Lret_zu:
    vzeroupper
    ret

    VITTE_FUNC_END(vitte_memset_avx2)
