/*
 * toolchain/tests/compile_smoke_test.c
 *
 * Vitte toolchain – compile/link smoke test (C).
 *
 * This is a very small program used by:
 *  - toolchain/scripts/smoke.sh (optionally)
 *  - CI (compile step + link step)
 *
 * Requirements:
 *  - Must compile as C17 cleanly.
 *  - Must be freestanding from project headers (no dependency on Vitte headers).
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

static uint64_t tc_fnv1a64(const void* data, size_t len) {
    const uint8_t* p = (const uint8_t*)data;
    uint64_t h = 1469598103934665603ull;
    for (size_t i = 0; i < len; ++i) {
        h ^= (uint64_t)p[i];
        h *= 1099511628211ull;
    }
    return h;
}

static int tc_basic_math(void) {
    volatile int a = 7;
    volatile int b = 5;
    volatile int c = (a * 3) + (b * 11) - 9;
    return (int)c;
}

int main(void) {
    const char* msg = "vitte toolchain compile smoke test";
    uint64_t h = tc_fnv1a64(msg, strlen(msg));
    int x = tc_basic_math();

    /* Stable output for debugging in CI logs. */
    printf("ok: hash=%" PRIu64 " x=%d\n", (uint64_t)h, x);
    return 0;
}
