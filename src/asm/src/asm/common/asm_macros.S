#if defined(__APPLE__)
  #define SYM(x) _##x
#else
  #define SYM(x) x
#endif

#if defined(__ELF__)
  #define TYPE_FUNC(x) .type x, @function
  #define SIZE_FUNC(x) .size x, .-x
#else
  #define TYPE_FUNC(x)
  #define SIZE_FUNC(x)
#endif

#define GLOBL(x) .globl x
#define ALIGN16  .p2align 4
#define ALIGN32  .p2align 5

#define FUNC_BEGIN(name) \
  .text; \
  ALIGN16; \
  GLOBL(SYM(name)); \
  TYPE_FUNC(SYM(name)); \
  SYM(name):

#define FUNC_END(name) \
  SIZE_FUNC(SYM(name))
