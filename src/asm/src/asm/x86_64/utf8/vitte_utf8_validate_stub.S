// vitte/src/asm/src/asm/x86_64/utf8/vitte_utf8_validate_stub.S
//
// UTF-8 validate (x86_64 SysV) — stub "max"
//
// Purpose:
//   - Provide a stable symbol in the x86_64 asm layer.
//   - Strict UTF-8 validator (scalar), intended to be replaceable by SIMD later.
//
// ABI (SysV AMD64):
//   rdi = const uint8_t* data
//   rsi = size_t         len
//   returns:
//     eax = int (1 = valid, 0 = invalid)
//
// Rules (strict UTF-8):
//   - Rejects overlong encodings
//   - Rejects surrogate halves (U+D800..U+DFFF)
//   - Enforces max codepoint U+10FFFF
//   - Accepts empty input
//
// Registers:
//   rdi ptr (advances)
//   rsi remaining
//   eax return (set at end)
//   edx b0
//   ecx b1
//   r8d b2
//   r9d b3
//   r10d tmp
//
// Clobbers: rax, rcx, rdx, r8..r10, flags

#include "common/asm_macros.S"

    VITTE_P2ALIGN(4)
    VITTE_FUNC_BEGIN(vitte_utf8_validate_stub)
    // if (len == 0) return 1
    test    %rsi, %rsi
    je      .Lok

.Lloop:
    // b0 = *p++
    movzbl  (%rdi), %edx
    inc     %rdi
    dec     %rsi

    // ASCII: b0 < 0x80
    test    $0x80, %dl
    jz      .Lcont_or_end

    // Reject 0x80..0xC1
    cmp     $0xC2, %dl
    jb      .Lbad

    // 2-byte: 0xC2..0xDF
    cmp     $0xE0, %dl
    jb      .Ltwo

    // 3-byte: 0xE0..0xEF
    cmp     $0xF0, %dl
    jb      .Lthree

    // 4-byte: 0xF0..0xF4
    cmp     $0xF5, %dl
    jae     .Lbad
    jmp     .Lfour

// ---------------------------------
// 2-byte sequence
// ---------------------------------
.Ltwo:
    test    %rsi, %rsi
    je      .Lbad
    movzbl  (%rdi), %ecx
    inc     %rdi
    dec     %rsi

    // (b1 & 0xC0) == 0x80
    mov     %ecx, %r10d
    and     $0xC0, %r10d
    cmp     $0x80, %r10d
    jne     .Lbad
    jmp     .Lcont_or_end

// ---------------------------------
// 3-byte sequence
//   - E0: b1 >= A0
//   - ED: b1 <= 9F
// ---------------------------------
.Lthree:
    cmp     $2, %rsi
    jb      .Lbad

    movzbl  (%rdi), %ecx
    movzbl  1(%rdi), %r8d
    add     $2, %rdi
    sub     $2, %rsi

    // cont checks
    mov     %ecx, %r10d
    and     $0xC0, %r10d
    cmp     $0x80, %r10d
    jne     .Lbad

    mov     %r8d, %r10d
    and     $0xC0, %r10d
    cmp     $0x80, %r10d
    jne     .Lbad

    // special constraints
    cmp     $0xE0, %dl
    jne     .Lchk_ed
    cmp     $0xA0, %cl
    jb      .Lbad
    jmp     .Lcont_or_end

.Lchk_ed:
    cmp     $0xED, %dl
    jne     .Lcont_or_end
    cmp     $0x9F, %cl
    ja      .Lbad
    jmp     .Lcont_or_end

// ---------------------------------
// 4-byte sequence
//   - F0: b1 >= 90
//   - F4: b1 <= 8F
// ---------------------------------
.Lfour:
    cmp     $3, %rsi
    jb      .Lbad

    movzbl  (%rdi), %ecx
    movzbl  1(%rdi), %r8d
    movzbl  2(%rdi), %r9d
    add     $3, %rdi
    sub     $3, %rsi

    // cont checks
    mov     %ecx, %r10d
    and     $0xC0, %r10d
    cmp     $0x80, %r10d
    jne     .Lbad

    mov     %r8d, %r10d
    and     $0xC0, %r10d
    cmp     $0x80, %r10d
    jne     .Lbad

    mov     %r9d, %r10d
    and     $0xC0, %r10d
    cmp     $0x80, %r10d
    jne     .Lbad

    // special constraints
    cmp     $0xF0, %dl
    jne     .Lchk_f4
    cmp     $0x90, %cl
    jb      .Lbad
    jmp     .Lcont_or_end

.Lchk_f4:
    cmp     $0xF4, %dl
    jne     .Lcont_or_end
    cmp     $0x8F, %cl
    ja      .Lbad
    jmp     .Lcont_or_end

// ---------------------------------
// Loop control
// ---------------------------------
.Lcont_or_end:
    test    %rsi, %rsi
    jne     .Lloop

.Lok:
    mov     $1, %eax
    ret

.Lbad:
    xor     %eax, %eax
    ret

    VITTE_FUNC_END(vitte_utf8_validate_stub)
