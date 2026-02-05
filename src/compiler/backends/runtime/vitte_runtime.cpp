#include "vitte_runtime.hpp"

#include <cstdlib>
#include <cstdio>
#include <exception>

namespace vitte::runtime {

/* -------------------------------------------------
 * Allocation
 * ------------------------------------------------- */

void* alloc(std::size_t size) {
    void* p = std::malloc(size);
    if (!p) {
        panic("allocation failed");
    }
    return p;
}

void dealloc(void* ptr) {
    std::free(ptr);
}

/* -------------------------------------------------
 * Panic / abort
 * ------------------------------------------------- */

[[noreturn]]
void panic(const char* msg) {
    std::fprintf(stderr, "[vitte panic] %s\n", msg);
    std::fflush(stderr);
    std::abort();
}

/* -------------------------------------------------
 * Assertions (used by lowered MIR)
 * ------------------------------------------------- */

void assert_true(bool cond, const char* msg) {
    if (!cond) {
        panic(msg);
    }
}

/* -------------------------------------------------
 * Unreachable
 * ------------------------------------------------- */

[[noreturn]]
void unreachable() {
#if defined(__GNUC__) || defined(__clang__)
    __builtin_unreachable();
#else
    std::abort();
#endif
}

/* -------------------------------------------------
 * Exception boundary (optional)
 * ------------------------------------------------- */

void terminate() {
    panic("terminate called");
}

} // namespace vitte::runtime
