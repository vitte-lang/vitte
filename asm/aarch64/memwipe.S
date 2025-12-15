/*
  aarch64 memwipe

  ABI:
    void steel_memwipe(void* p, size_t n)

  Notes:
    - Overwrites memory with zeros.
    - No data-dependent branches; loop count depends on n (public).
    - Intended as a hard-to-optimize-away wipe primitive (assembly).
*/

.text
.align 2

.global steel_memwipe
.type steel_memwipe, %function
steel_memwipe:
  // x0 = p, x1 = n

  // If n == 0 => return
  cbz     x1, 3f

  // 16-byte chunks: cnt16 = n / 16, tail = n % 16
  lsr     x2, x1, #4         // x2 = cnt16
  and     x3, x1, #15        // x3 = tail

0:
  cbz     x2, 1f
  stp     xzr, xzr, [x0], #16
  subs    x2, x2, #1
  b.ne    0b

1:
  // 8-byte chunk if tail >= 8
  tst     x3, #8
  b.eq    2f
  str     xzr, [x0], #8

2:
  // Remaining bytes (0..7)
  and     x3, x3, #7
  cbz     x3, 3f

4:
  strb    wzr, [x0], #1
  subs    x3, x3, #1
  b.ne    4b

3:
  ret

.size steel_memwipe, .-steel_memwipe
