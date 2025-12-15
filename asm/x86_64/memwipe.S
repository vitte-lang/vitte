
/*
  x86_64 memwipe

  ABI (SysV):
    void steel_memwipe(void* p, size_t n)
      rdi=p, rsi=n

  Notes:
    - Overwrites memory with zeros.
    - Loop count depends on n (public).
    - Intended to be hard to optimize away (assembly).
    - SysV x86_64. Windows x64 calling convention differs.
*/

.text
.globl steel_memwipe
.type steel_memwipe, @function
steel_memwipe:
  // rdi = p, rsi = n
  test    %rsi, %rsi
  je      .L_done

  // cnt16 = n / 16, tail = n % 16
  mov     %rsi, %rcx
  shr     $4, %rcx            // rcx = cnt16
  mov     %rsi, %rdx
  and     $15, %rdx           // rdx = tail

.L_loop16:
  test    %rcx, %rcx
  je      .L_tail8

  // store 16B of zeros
  movq    $0, 0(%rdi)
  movq    $0, 8(%rdi)
  add     $16, %rdi

  dec     %rcx
  jmp     .L_loop16

.L_tail8:
  test    $8, %rdx
  je      .L_tailbytes
  movq    $0, 0(%rdi)
  add     $8, %rdi

.L_tailbytes:
  and     $7, %rdx
  test    %rdx, %rdx
  je      .L_done

.L_loop1:
  movb    $0, 0(%rdi)
  inc     %rdi
  dec     %rdx
  jne     .L_loop1

.L_done:
  ret

.size steel_memwipe, .-steel_memwipe
