#pragma once
#include <cstddef>
#include <cstdint>

namespace vitte::runtime {

/* -------------------------------------------------
 * Memory management
 * ------------------------------------------------- */

/* Allocate `size` bytes or panic on failure */
void* alloc(std::size_t size);

/* Free previously allocated memory */
void dealloc(void* ptr);

/* -------------------------------------------------
 * Panic / error handling
 * ------------------------------------------------- */

/* Abort execution with an error message */
[[noreturn]]
void panic(const char* msg);

/* Runtime assertion helper (used by lowered MIR) */
void assert_true(bool cond, const char* msg);

/* Simple printing helpers (temporary) */
void print_i32(int32_t value);

/* -------------------------------------------------
 * Control flow helpers
 * ------------------------------------------------- */

/* Mark unreachable code paths */
[[noreturn]]
void unreachable();

/* Termination hook (optional ABI boundary) */
void terminate();

} // namespace vitte::runtime
