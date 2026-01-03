// SPDX-License-Identifier: MIT
// atomics.c
//
// Minimal cross-platform atomic primitives used across vitte.
//
// Policy:
//  - Prefer C11 <stdatomic.h> when available.
//  - Fall back to compiler intrinsics on MSVC and GCC/Clang.
//  - Keep the API tiny and deterministic.
//
// This file pairs with atomics.h.
// atomics.h is expected to define:
//   - steel_atomic_u32 / steel_atomic_u64 (opaque structs or typedefs)
//   - steel_atomic_bool
//   - memory order enum (or use int)
//   - function declarations implemented here.
//
// If your atomics.h differs, adjust one side accordingly.

#include "atomics.h"

#include <stdint.h>
#include <stdbool.h>

//------------------------------------------------------------------------------
// C11 stdatomic path
//------------------------------------------------------------------------------

#if !defined(STEEL_NO_STDATOMIC)

  #if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
    #if !defined(__STDC_NO_ATOMICS__)
      #define STEEL_HAS_C11_ATOMICS 1
    #endif
  #endif

#endif

#if defined(STEEL_HAS_C11_ATOMICS)

#include <stdatomic.h>

// The header should map these types to _Atomic(...) or wrappers.
// We implement the functions in terms of stdatomic operations.

uint32_t steel_atomic_u32_load(const steel_atomic_u32* a, steel_memory_order mo)
{
    return atomic_load_explicit((const _Atomic uint32_t*)&a->v, (memory_order)mo);
}

void steel_atomic_u32_store(steel_atomic_u32* a, uint32_t v, steel_memory_order mo)
{
    atomic_store_explicit((_Atomic uint32_t*)&a->v, v, (memory_order)mo);
}

uint32_t steel_atomic_u32_fetch_add(steel_atomic_u32* a, uint32_t add, steel_memory_order mo)
{
    return atomic_fetch_add_explicit((_Atomic uint32_t*)&a->v, add, (memory_order)mo);
}

uint32_t steel_atomic_u32_fetch_sub(steel_atomic_u32* a, uint32_t sub, steel_memory_order mo)
{
    return atomic_fetch_sub_explicit((_Atomic uint32_t*)&a->v, sub, (memory_order)mo);
}

bool steel_atomic_u32_compare_exchange(steel_atomic_u32* a, uint32_t* expected, uint32_t desired,
                                      steel_memory_order mo_success, steel_memory_order mo_failure)
{
    return atomic_compare_exchange_strong_explicit((_Atomic uint32_t*)&a->v, expected, desired,
                                                   (memory_order)mo_success, (memory_order)mo_failure);
}

uint64_t steel_atomic_u64_load(const steel_atomic_u64* a, steel_memory_order mo)
{
    return atomic_load_explicit((const _Atomic uint64_t*)&a->v, (memory_order)mo);
}

void steel_atomic_u64_store(steel_atomic_u64* a, uint64_t v, steel_memory_order mo)
{
    atomic_store_explicit((_Atomic uint64_t*)&a->v, v, (memory_order)mo);
}

uint64_t steel_atomic_u64_fetch_add(steel_atomic_u64* a, uint64_t add, steel_memory_order mo)
{
    return atomic_fetch_add_explicit((_Atomic uint64_t*)&a->v, add, (memory_order)mo);
}

uint64_t steel_atomic_u64_fetch_sub(steel_atomic_u64* a, uint64_t sub, steel_memory_order mo)
{
    return atomic_fetch_sub_explicit((_Atomic uint64_t*)&a->v, sub, (memory_order)mo);
}

bool steel_atomic_u64_compare_exchange(steel_atomic_u64* a, uint64_t* expected, uint64_t desired,
                                      steel_memory_order mo_success, steel_memory_order mo_failure)
{
    return atomic_compare_exchange_strong_explicit((_Atomic uint64_t*)&a->v, expected, desired,
                                                   (memory_order)mo_success, (memory_order)mo_failure);
}

bool steel_atomic_bool_load(const steel_atomic_bool* a, steel_memory_order mo)
{
    return atomic_load_explicit((const _Atomic bool*)&a->v, (memory_order)mo);
}

void steel_atomic_bool_store(steel_atomic_bool* a, bool v, steel_memory_order mo)
{
    atomic_store_explicit((_Atomic bool*)&a->v, v, (memory_order)mo);
}

bool steel_atomic_bool_compare_exchange(steel_atomic_bool* a, bool* expected, bool desired,
                                       steel_memory_order mo_success, steel_memory_order mo_failure)
{
    return atomic_compare_exchange_strong_explicit((_Atomic bool*)&a->v, expected, desired,
                                                   (memory_order)mo_success, (memory_order)mo_failure);
}

void steel_atomic_thread_fence(steel_memory_order mo)
{
    atomic_thread_fence((memory_order)mo);
}

//------------------------------------------------------------------------------
// Fallback intrinsics path
//------------------------------------------------------------------------------

#else

#if defined(_MSC_VER)

#include <intrin.h>
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedCompareExchange)
#pragma intrinsic(_InterlockedExchange)

#if defined(_M_X64) || defined(_M_ARM64)
  #pragma intrinsic(_InterlockedExchangeAdd64)
  #pragma intrinsic(_InterlockedCompareExchange64)
  #pragma intrinsic(_InterlockedExchange64)
#endif

static void steel_compiler_barrier(void)
{
    _ReadWriteBarrier();
}

static void steel_full_fence(void)
{
#if defined(_M_X64) || defined(_M_IX86)
    _mm_mfence();
#elif defined(_M_ARM) || defined(_M_ARM64)
    __dmb(0xB); // full system barrier
#else
    _ReadWriteBarrier();
#endif
}

uint32_t steel_atomic_u32_load(const steel_atomic_u32* a, steel_memory_order mo)
{
    (void)mo;
    steel_compiler_barrier();
    return (uint32_t)a->v;
}

void steel_atomic_u32_store(steel_atomic_u32* a, uint32_t v, steel_memory_order mo)
{
    (void)mo;
    steel_compiler_barrier();
    a->v = v;
    steel_compiler_barrier();
}

uint32_t steel_atomic_u32_fetch_add(steel_atomic_u32* a, uint32_t add, steel_memory_order mo)
{
    (void)mo;
    return (uint32_t)_InterlockedExchangeAdd((volatile long*)&a->v, (long)add);
}

uint32_t steel_atomic_u32_fetch_sub(steel_atomic_u32* a, uint32_t sub, steel_memory_order mo)
{
    (void)mo;
    return (uint32_t)_InterlockedExchangeAdd((volatile long*)&a->v, -(long)sub);
}

bool steel_atomic_u32_compare_exchange(steel_atomic_u32* a, uint32_t* expected, uint32_t desired,
                                      steel_memory_order mo_success, steel_memory_order mo_failure)
{
    (void)mo_success; (void)mo_failure;
    long prev = _InterlockedCompareExchange((volatile long*)&a->v, (long)desired, (long)(*expected));
    if ((uint32_t)prev == *expected) return true;
    *expected = (uint32_t)prev;
    return false;
}

uint64_t steel_atomic_u64_load(const steel_atomic_u64* a, steel_memory_order mo)
{
    (void)mo;
    steel_compiler_barrier();
    return (uint64_t)a->v;
}

void steel_atomic_u64_store(steel_atomic_u64* a, uint64_t v, steel_memory_order mo)
{
    (void)mo;
    steel_compiler_barrier();
    a->v = v;
    steel_compiler_barrier();
}

uint64_t steel_atomic_u64_fetch_add(steel_atomic_u64* a, uint64_t add, steel_memory_order mo)
{
    (void)mo;
#if defined(_M_X64) || defined(_M_ARM64)
    return (uint64_t)_InterlockedExchangeAdd64((volatile long long*)&a->v, (long long)add);
#else
    // 32-bit MSVC: 64-bit interlocked ops require Windows APIs; keep minimal.
    // Implement via CAS loop.
    uint64_t exp;
    for (;;)
    {
        exp = steel_atomic_u64_load(a, mo);
        uint64_t desired = exp + add;
        uint64_t e2 = exp;
        if (steel_atomic_u64_compare_exchange(a, &e2, desired, mo, mo)) return exp;
    }
#endif
}

uint64_t steel_atomic_u64_fetch_sub(steel_atomic_u64* a, uint64_t sub, steel_memory_order mo)
{
    return steel_atomic_u64_fetch_add(a, (uint64_t)(0ull - sub), mo);
}

bool steel_atomic_u64_compare_exchange(steel_atomic_u64* a, uint64_t* expected, uint64_t desired,
                                      steel_memory_order mo_success, steel_memory_order mo_failure)
{
    (void)mo_success; (void)mo_failure;
#if defined(_M_X64) || defined(_M_ARM64)
    long long prev = _InterlockedCompareExchange64((volatile long long*)&a->v, (long long)desired, (long long)(*expected));
    if ((uint64_t)prev == *expected) return true;
    *expected = (uint64_t)prev;
    return false;
#else
    // 32-bit: not lock-free but works with CAS loop fallback.
    // Use a naive mutex-less CAS via 64-bit compare-exchange not available => fail.
    // Keep conservative: treat as not supported.
    (void)a; (void)desired;
    return false;
#endif
}

bool steel_atomic_bool_load(const steel_atomic_bool* a, steel_memory_order mo)
{
    (void)mo;
    steel_compiler_barrier();
    return a->v ? true : false;
}

void steel_atomic_bool_store(steel_atomic_bool* a, bool v, steel_memory_order mo)
{
    (void)mo;
    steel_compiler_barrier();
    a->v = v ? 1 : 0;
    steel_compiler_barrier();
}

bool steel_atomic_bool_compare_exchange(steel_atomic_bool* a, bool* expected, bool desired,
                                       steel_memory_order mo_success, steel_memory_order mo_failure)
{
    (void)mo_success; (void)mo_failure;
    long prev = _InterlockedCompareExchange((volatile long*)&a->v, desired ? 1L : 0L, (*expected) ? 1L : 0L);
    if ((prev != 0) == *expected) return true;
    *expected = (prev != 0);
    return false;
}

void steel_atomic_thread_fence(steel_memory_order mo)
{
    (void)mo;
    steel_full_fence();
}

#elif defined(__GNUC__) || defined(__clang__)

static inline int steel_mo_to_builtin(steel_memory_order mo)
{
    // Map common C11 orders to __ATOMIC_* constants.
    switch ((int)mo)
    {
        case 0: return __ATOMIC_RELAXED;
        case 1: return __ATOMIC_CONSUME;
        case 2: return __ATOMIC_ACQUIRE;
        case 3: return __ATOMIC_RELEASE;
        case 4: return __ATOMIC_ACQ_REL;
        case 5: return __ATOMIC_SEQ_CST;
        default: return __ATOMIC_SEQ_CST;
    }
}

uint32_t steel_atomic_u32_load(const steel_atomic_u32* a, steel_memory_order mo)
{
    return __atomic_load_n((const uint32_t*)&a->v, steel_mo_to_builtin(mo));
}

void steel_atomic_u32_store(steel_atomic_u32* a, uint32_t v, steel_memory_order mo)
{
    __atomic_store_n((uint32_t*)&a->v, v, steel_mo_to_builtin(mo));
}

uint32_t steel_atomic_u32_fetch_add(steel_atomic_u32* a, uint32_t add, steel_memory_order mo)
{
    return __atomic_fetch_add((uint32_t*)&a->v, add, steel_mo_to_builtin(mo));
}

uint32_t steel_atomic_u32_fetch_sub(steel_atomic_u32* a, uint32_t sub, steel_memory_order mo)
{
    return __atomic_fetch_sub((uint32_t*)&a->v, sub, steel_mo_to_builtin(mo));
}

bool steel_atomic_u32_compare_exchange(steel_atomic_u32* a, uint32_t* expected, uint32_t desired,
                                      steel_memory_order mo_success, steel_memory_order mo_failure)
{
    return __atomic_compare_exchange_n((uint32_t*)&a->v, expected, desired, 0,
                                      steel_mo_to_builtin(mo_success), steel_mo_to_builtin(mo_failure));
}

uint64_t steel_atomic_u64_load(const steel_atomic_u64* a, steel_memory_order mo)
{
    return __atomic_load_n((const uint64_t*)&a->v, steel_mo_to_builtin(mo));
}

void steel_atomic_u64_store(steel_atomic_u64* a, uint64_t v, steel_memory_order mo)
{
    __atomic_store_n((uint64_t*)&a->v, v, steel_mo_to_builtin(mo));
}

uint64_t steel_atomic_u64_fetch_add(steel_atomic_u64* a, uint64_t add, steel_memory_order mo)
{
    return __atomic_fetch_add((uint64_t*)&a->v, add, steel_mo_to_builtin(mo));
}

uint64_t steel_atomic_u64_fetch_sub(steel_atomic_u64* a, uint64_t sub, steel_memory_order mo)
{
    return __atomic_fetch_sub((uint64_t*)&a->v, sub, steel_mo_to_builtin(mo));
}

bool steel_atomic_u64_compare_exchange(steel_atomic_u64* a, uint64_t* expected, uint64_t desired,
                                      steel_memory_order mo_success, steel_memory_order mo_failure)
{
    return __atomic_compare_exchange_n((uint64_t*)&a->v, expected, desired, 0,
                                      steel_mo_to_builtin(mo_success), steel_mo_to_builtin(mo_failure));
}

bool steel_atomic_bool_load(const steel_atomic_bool* a, steel_memory_order mo)
{
    return __atomic_load_n((const uint8_t*)&a->v, steel_mo_to_builtin(mo)) != 0;
}

void steel_atomic_bool_store(steel_atomic_bool* a, bool v, steel_memory_order mo)
{
    uint8_t vv = v ? 1u : 0u;
    __atomic_store_n((uint8_t*)&a->v, vv, steel_mo_to_builtin(mo));
}

bool steel_atomic_bool_compare_exchange(steel_atomic_bool* a, bool* expected, bool desired,
                                       steel_memory_order mo_success, steel_memory_order mo_failure)
{
    uint8_t exp8 = (*expected) ? 1u : 0u;
    uint8_t des8 = desired ? 1u : 0u;
    bool ok = __atomic_compare_exchange_n((uint8_t*)&a->v, &exp8, des8, 0,
                                         steel_mo_to_builtin(mo_success), steel_mo_to_builtin(mo_failure));
    if (!ok) *expected = (exp8 != 0);
    return ok;
}

void steel_atomic_thread_fence(steel_memory_order mo)
{
    __atomic_thread_fence(steel_mo_to_builtin(mo));
}

#else

// Very conservative fallback: no real atomics.
// This is only for platforms where no atomics are available.

uint32_t steel_atomic_u32_load(const steel_atomic_u32* a, steel_memory_order mo)
{
    (void)mo;
    return a->v;
}

void steel_atomic_u32_store(steel_atomic_u32* a, uint32_t v, steel_memory_order mo)
{
    (void)mo;
    a->v = v;
}

uint32_t steel_atomic_u32_fetch_add(steel_atomic_u32* a, uint32_t add, steel_memory_order mo)
{
    (void)mo;
    uint32_t old = a->v;
    a->v = old + add;
    return old;
}

uint32_t steel_atomic_u32_fetch_sub(steel_atomic_u32* a, uint32_t sub, steel_memory_order mo)
{
    (void)mo;
    uint32_t old = a->v;
    a->v = old - sub;
    return old;
}

bool steel_atomic_u32_compare_exchange(steel_atomic_u32* a, uint32_t* expected, uint32_t desired,
                                      steel_memory_order mo_success, steel_memory_order mo_failure)
{
    (void)mo_success; (void)mo_failure;
    if (a->v == *expected) { a->v = desired; return true; }
    *expected = a->v;
    return false;
}

uint64_t steel_atomic_u64_load(const steel_atomic_u64* a, steel_memory_order mo)
{
    (void)mo;
    return a->v;
}

void steel_atomic_u64_store(steel_atomic_u64* a, uint64_t v, steel_memory_order mo)
{
    (void)mo;
    a->v = v;
}

uint64_t steel_atomic_u64_fetch_add(steel_atomic_u64* a, uint64_t add, steel_memory_order mo)
{
    (void)mo;
    uint64_t old = a->v;
    a->v = old + add;
    return old;
}

uint64_t steel_atomic_u64_fetch_sub(steel_atomic_u64* a, uint64_t sub, steel_memory_order mo)
{
    (void)mo;
    uint64_t old = a->v;
    a->v = old - sub;
    return old;
}

bool steel_atomic_u64_compare_exchange(steel_atomic_u64* a, uint64_t* expected, uint64_t desired,
                                      steel_memory_order mo_success, steel_memory_order mo_failure)
{
    (void)mo_success; (void)mo_failure;
    if (a->v == *expected) { a->v = desired; return true; }
    *expected = a->v;
    return false;
}

bool steel_atomic_bool_load(const steel_atomic_bool* a, steel_memory_order mo)
{
    (void)mo;
    return a->v != 0;
}

void steel_atomic_bool_store(steel_atomic_bool* a, bool v, steel_memory_order mo)
{
    (void)mo;
    a->v = v ? 1u : 0u;
}

bool steel_atomic_bool_compare_exchange(steel_atomic_bool* a, bool* expected, bool desired,
                                       steel_memory_order mo_success, steel_memory_order mo_failure)
{
    (void)mo_success; (void)mo_failure;
    bool cur = (a->v != 0);
    if (cur == *expected) { a->v = desired ? 1u : 0u; return true; }
    *expected = cur;
    return false;
}

void steel_atomic_thread_fence(steel_memory_order mo)
{
    (void)mo;
}

#endif // compiler fallback selection

#endif // !STEEL_HAS_C11_ATOMICS
