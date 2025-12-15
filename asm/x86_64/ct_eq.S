/* TODO: x86_64 constant-time eq */

/*
  x86_64 constant-time equality

  ABI (SysV):
    int steel_ct_eq(const void* a, const void* b, size_t n)
      rdi=a, rsi=b, rdx=n

  Returns:
    1 if equal, 0 otherwise.

  Notes:
    - Constant-time w.r.t. buffer contents (no data-dependent branches).
    - Loop count depends on n (public).
    - Uses word loads (8B) then tail bytes.
    - This is SysV x86_64. Windows x64 calling convention differs (rcx, rdx, r8).
*/

.text
.globl steel_ct_eq
.type steel_ct_eq, @function
steel_ct_eq:
  // rdi=a, rsi=b, rdx=n
  xor     %rax, %rax          // acc = 0

  test    %rdx, %rdx
  je      .L_equal

  // cnt8 = n / 8, tail = n % 8
  mov     %rdx, %rcx
  shr     $3, %rcx            // rcx = cnt8
  mov     %rdx, %r8
  and     $7, %r8             // r8 = tail

.L_qword_loop:
  test    %rcx, %rcx
  je      .L_tail

  mov     (%rdi), %r9
  mov     (%rsi), %r10
  xor     %r10, %r9
  or      %r9, %rax

  add     $8, %rdi
  add     $8, %rsi
  dec     %rcx
  jmp     .L_qword_loop

.L_tail:
  test    %r8, %r8
  je      .L_finish

.L_byte_loop:
  movzbq  (%rdi), %r9
  movzbq  (%rsi), %r10
  xor     %r10, %r9
  or      %r9, %rax

  inc     %rdi
  inc     %rsi
  dec     %r8
  jne     .L_byte_loop

.L_finish:
  // return (acc == 0) ? 1 : 0
  test    %rax, %rax
  sete    %al
  movzbq  %al, %rax
  ret

.L_equal:
  mov     $1, %eax
  ret

.size steel_ct_eq, .-steel_ct_eq