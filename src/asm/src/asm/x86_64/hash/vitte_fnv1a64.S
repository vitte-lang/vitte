// vitte/src/asm/src/asm/x86_64/hash/vitte_fnv1a64.S
//
// FNV-1a 64-bit (x86_64) — version "max"
//
// ABI:
//   - SysV AMD64 (Linux/macOS/*BSD):
//       rdi = const uint8_t* data
//       rsi = size_t len
//       rax = return uint64_t hash
//
// Notes:
//   - Strict FNV-1a byte-serial semantics
//   - Handles len == 0
//   - Unrolled 8 bytes per iter to reduce branch overhead
//
// Registers:
//   rdi ptr (advances)
//   rsi len (counts down)
//   rax hash
//   rcx prime
//   rdx tmp
//   r8  tmp (block count)
//
// Clobbers: rax, rcx, rdx, r8, flags

#include "common/asm_macros.S"

    VITTE_P2ALIGN(4)
    VITTE_FUNC_BEGIN(vitte_fnv1a64)
    // hash = offset basis 0xcbf29ce484222325
    movabs  $0xcbf29ce484222325, %rax

    // if (len == 0) return
    test    %rsi, %rsi
    je      .Lret

    // prime = 0x00000100000001B3
    movabs  $0x00000100000001B3, %rcx

    // blocks = len / 8
    mov     %rsi, %r8
    shr     $3, %r8
    je      .Ltail

// ---------------------------------------------------------------------------
// Block loop: 8 bytes/iter (still byte-serial, but unrolled)
// ---------------------------------------------------------------------------
.Lblock:
    // b0
    movzbq  (%rdi), %rdx
    xor     %rdx, %rax
    imul    %rcx, %rax
    // b1
    movzbq  1(%rdi), %rdx
    xor     %rdx, %rax
    imul    %rcx, %rax
    // b2
    movzbq  2(%rdi), %rdx
    xor     %rdx, %rax
    imul    %rcx, %rax
    // b3
    movzbq  3(%rdi), %rdx
    xor     %rdx, %rax
    imul    %rcx, %rax
    // b4
    movzbq  4(%rdi), %rdx
    xor     %rdx, %rax
    imul    %rcx, %rax
    // b5
    movzbq  5(%rdi), %rdx
    xor     %rdx, %rax
    imul    %rcx, %rax
    // b6
    movzbq  6(%rdi), %rdx
    xor     %rdx, %rax
    imul    %rcx, %rax
    // b7
    movzbq  7(%rdi), %rdx
    xor     %rdx, %rax
    imul    %rcx, %rax

    add     $8, %rdi
    dec     %r8
    jne     .Lblock

// ---------------------------------------------------------------------------
// Tail 0..7 bytes
// ---------------------------------------------------------------------------
.Ltail:
    and     $7, %rsi
    je      .Lret

.Ltail_loop:
    movzbq  (%rdi), %rdx
    xor     %rdx, %rax
    imul    %rcx, %rax
    inc     %rdi
    dec     %rsi
    jne     .Ltail_loop

.Lret:
    ret

    VITTE_FUNC_END(vitte_fnv1a64)
