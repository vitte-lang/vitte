
/*
  aarch64 constant-time equality

  ABI:
    int steel_ct_eq(const void* a, const void* b, size_t n)

  Returns:
    1 if equal, 0 otherwise.

  Notes:
    - Constant-time w.r.t. buffer contents (no data-dependent branches).
    - Loop count depends on n (public).
*/

.text
.align 2

.global steel_ct_eq
.type steel_ct_eq, %function
steel_ct_eq:
  // x0 = a, x1 = b, x2 = n
  mov     x3, xzr            // acc = 0

  // Fast path: n == 0 => equal
  cbz     x2, 2f

  // qword loop: cnt8 = n / 8, tail = n % 8
  lsr     x5, x2, #3         // x5 = cnt8
  and     x6, x2, #7         // x6 = tail

0:
  cbz     x5, 1f
  ldr     x7, [x0], #8
  ldr     x8, [x1], #8
  eor     x7, x7, x8
  orr     x3, x3, x7
  subs    x5, x5, #1
  b.ne    0b

1:
  cbz     x6, 3f

4:
  ldrb    w7, [x0], #1
  ldrb    w8, [x1], #1
  eor     w7, w7, w8
  orr     w3, w3, w7
  subs    x6, x6, #1
  b.ne    4b

3:
  // return (acc == 0)
  cmp     x3, xzr
  cset    w0, eq
  ret

2:
  mov     w0, #1
  ret

.size steel_ct_eq, .-steel_ct_eq
