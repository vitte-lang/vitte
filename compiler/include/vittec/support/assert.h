#ifndef VITTEC_INCLUDE_VITTEC_SUPPORT_ASSERT_H
    #define VITTEC_INCLUDE_VITTEC_SUPPORT_ASSERT_H



    #include <stdio.h>
#include <stdlib.h>

#define VITTEC_ASSERT(x) do { if (!(x)) {   fprintf(stderr, "assertion failed: %s (%s:%d)\n", #x, __FILE__, __LINE__);   abort(); }} while (0)

    #endif /* VITTEC_INCLUDE_VITTEC_SUPPORT_ASSERT_H */
