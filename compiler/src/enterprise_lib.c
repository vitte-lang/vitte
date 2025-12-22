// Enterprise Library Implementation
// High-performance utilities for complex business applications

#include "compiler/enterprise_lib.h"
#include <stdatomic.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sched.h>

#if defined(__APPLE__)
#include <mach/mach_time.h>
#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
#define ENTERPRISE_HAS_X86_INTRINSICS 1
#else
#define ENTERPRISE_HAS_X86_INTRINSICS 0
#endif

#if ENTERPRISE_HAS_X86_INTRINSICS && !defined(ENTERPRISE_PORTABLE)
#define ENTERPRISE_USE_X86_INTRINSICS 1
#else
#define ENTERPRISE_USE_X86_INTRINSICS 0
#endif

static inline uint64_t enterprise_read_clock_monotonic_ns(void) {
#if defined(__APPLE__)
    static mach_timebase_info_data_t timebase = {0};
    if (timebase.denom == 0) {
        mach_timebase_info(&timebase);
    }
    uint64_t ticks = mach_absolute_time();
    return (ticks * timebase.numer) / timebase.denom;
#elif defined(CLOCK_MONOTONIC_RAW)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
#endif
}

// ============================================================================
// Ring Buffer Implementation
// ============================================================================

enterprise_ring_buffer_t* enterprise_ring_buffer_create(
    size_t capacity,
    size_t element_size
) {
    enterprise_ring_buffer_t *rb = malloc(sizeof(enterprise_ring_buffer_t));
    if (!rb) return NULL;

    rb->data = malloc(capacity * element_size);
    if (!rb->data) {
        free(rb);
        return NULL;
    }

    rb->write_pos = 0;
    rb->read_pos = 0;
    rb->capacity = capacity;
    rb->element_size = element_size;

    return rb;
}

void enterprise_ring_buffer_destroy(enterprise_ring_buffer_t *rb) {
    if (!rb) return;
    free(rb->data);
    free(rb);
}

bool enterprise_ring_buffer_push(
    enterprise_ring_buffer_t *rb,
    const void *element
) {
    while (1) {
        uint64_t write_pos = __atomic_load_n(&rb->write_pos, __ATOMIC_RELAXED);
        uint64_t read_pos = __atomic_load_n(&rb->read_pos, __ATOMIC_ACQUIRE);
        uint64_t next_pos = (write_pos + 1) % rb->capacity;

        if (next_pos == read_pos) {
            return false;
        }

        memcpy(
            (uint8_t *)rb->data + (write_pos * rb->element_size),
            element,
            rb->element_size
        );

        if (__atomic_compare_exchange_n(
                &rb->write_pos,
                &write_pos,
                next_pos,
                false,
                __ATOMIC_RELEASE,
                __ATOMIC_RELAXED)) {
            return true;
        }
        sched_yield();
    }
}

bool enterprise_ring_buffer_pop(
    enterprise_ring_buffer_t *rb,
    void *element
) {
    while (1) {
        uint64_t read_pos = __atomic_load_n(&rb->read_pos, __ATOMIC_RELAXED);
        uint64_t write_pos = __atomic_load_n(&rb->write_pos, __ATOMIC_ACQUIRE);

        if (read_pos == write_pos) {
            return false;
        }

        memcpy(
            element,
            (uint8_t *)rb->data + (read_pos * rb->element_size),
            rb->element_size
        );

        uint64_t next_pos = (read_pos + 1) % rb->capacity;
        if (__atomic_compare_exchange_n(
                &rb->read_pos,
                &read_pos,
                next_pos,
                false,
                __ATOMIC_RELEASE,
                __ATOMIC_RELAXED)) {
            return true;
        }
        sched_yield();
    }
}

bool enterprise_ring_buffer_is_full(const enterprise_ring_buffer_t *rb) {
    uint64_t write_pos = rb->write_pos;
    uint64_t read_pos = rb->read_pos;
    return ((write_pos + 1) % rb->capacity) == read_pos;
}

bool enterprise_ring_buffer_is_empty(const enterprise_ring_buffer_t *rb) {
    return rb->write_pos == rb->read_pos;
}

size_t enterprise_ring_buffer_available(const enterprise_ring_buffer_t *rb) {
    uint64_t write_pos = rb->write_pos;
    uint64_t read_pos = rb->read_pos;
    if (write_pos >= read_pos) {
        return write_pos - read_pos;
    }
    return rb->capacity - (read_pos - write_pos);
}

// ============================================================================
// Spinlock Implementation
// ============================================================================

void enterprise_spinlock_init(enterprise_spinlock_t *lock) {
    __atomic_store_n(&lock->state, 0, __ATOMIC_RELEASE);
    lock->owner_thread = 0;
}

void enterprise_spinlock_acquire(enterprise_spinlock_t *lock) {
    uint32_t backoff = 1;

    while (__atomic_exchange_n(&lock->state, 1, __ATOMIC_ACQUIRE) == 1) {
        for (uint32_t i = 0; i < backoff; i++) {
            sched_yield();
        }
        backoff = (backoff < 4096) ? backoff * 2 : 4096;
    }
}

void enterprise_spinlock_release(enterprise_spinlock_t *lock) {
    __atomic_store_n(&lock->state, 0, __ATOMIC_RELEASE);
}

bool enterprise_spinlock_trylock(enterprise_spinlock_t *lock) {
    return __atomic_exchange_n(&lock->state, 1, __ATOMIC_ACQUIRE) == 0;
}

void enterprise_spinlock_acquire_with_timeout(
    enterprise_spinlock_t *lock,
    uint64_t timeout_cycles
) {
    uint32_t backoff = 1;
    uint64_t start = enterprise_rdtsc();

    while (1) {
        if (__atomic_exchange_n(&lock->state, 1, __ATOMIC_ACQUIRE) == 0) {
            return;
        }

        if (timeout_cycles > 0) {
            uint64_t now = enterprise_rdtsc();
            if (now - start >= timeout_cycles) {
                return;
            }
        }

        for (uint32_t i = 0; i < backoff; i++) {
            sched_yield();
        }
        backoff = (backoff < 4096) ? backoff * 2 : 4096;
    }
}

// ============================================================================
// Read/Write Lock Implementation
// ============================================================================

void enterprise_rwlock_init(enterprise_rwlock_t *lock) {
    __atomic_store_n(&lock->readers, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&lock->writers, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&lock->write_waiters, 0, __ATOMIC_RELEASE);
}

void enterprise_rwlock_read_lock(enterprise_rwlock_t *lock) {
    while (1) {
        while (__atomic_load_n(&lock->writers, __ATOMIC_ACQUIRE) != 0) {
            sched_yield();
        }

        __atomic_fetch_add(&lock->readers, 1, __ATOMIC_ACQ_REL);
        if (__atomic_load_n(&lock->writers, __ATOMIC_ACQUIRE) == 0) {
            return;
        }

        __atomic_fetch_sub(&lock->readers, 1, __ATOMIC_ACQ_REL);
    }
}

void enterprise_rwlock_read_unlock(enterprise_rwlock_t *lock) {
    __atomic_fetch_sub(&lock->readers, 1, __ATOMIC_RELEASE);
}

void enterprise_rwlock_write_lock(enterprise_rwlock_t *lock) {
    __atomic_fetch_add(&lock->write_waiters, 1, __ATOMIC_ACQ_REL);

    while (1) {
        uint64_t expected = 0;
        if (__atomic_compare_exchange_n(
                &lock->writers,
                &expected,
                1,
                false,
                __ATOMIC_ACQ_REL,
                __ATOMIC_ACQUIRE)) {
            while (__atomic_load_n(&lock->readers, __ATOMIC_ACQUIRE) != 0) {
                sched_yield();
            }
            __atomic_fetch_sub(&lock->write_waiters, 1, __ATOMIC_ACQ_REL);
            return;
        }
        sched_yield();
    }
}

void enterprise_rwlock_write_unlock(enterprise_rwlock_t *lock) {
    __atomic_store_n(&lock->writers, 0, __ATOMIC_RELEASE);
}

bool enterprise_rwlock_write_trylock(enterprise_rwlock_t *lock) {
    uint64_t expected = 0;
    if (!__atomic_compare_exchange_n(
            &lock->writers,
            &expected,
            1,
            false,
            __ATOMIC_ACQ_REL,
            __ATOMIC_ACQUIRE)) {
        return false;
    }

    if (__atomic_load_n(&lock->readers, __ATOMIC_ACQUIRE) == 0) {
        return true;
    }

    __atomic_store_n(&lock->writers, 0, __ATOMIC_RELEASE);
    return false;
}

// ============================================================================
// Atomic Operations
// ============================================================================

void enterprise_atomic_init(enterprise_atomic_t *atom, uint64_t value) {
    __atomic_store_n(&atom->value, value, __ATOMIC_RELEASE);
}

uint64_t enterprise_atomic_load(const enterprise_atomic_t *atom) {
    return __atomic_load_n(&atom->value, __ATOMIC_ACQUIRE);
}

void enterprise_atomic_store(enterprise_atomic_t *atom, uint64_t value) {
    __atomic_store_n(&atom->value, value, __ATOMIC_RELEASE);
}

uint64_t enterprise_atomic_add_fetch(enterprise_atomic_t *atom, uint64_t delta) {
    return __atomic_add_fetch(&atom->value, delta, __ATOMIC_ACQ_REL);
}

uint64_t enterprise_atomic_sub_fetch(enterprise_atomic_t *atom, uint64_t delta) {
    return __atomic_sub_fetch(&atom->value, delta, __ATOMIC_ACQ_REL);
}

bool enterprise_atomic_compare_exchange(
    enterprise_atomic_t *atom,
    uint64_t expected,
    uint64_t new_value
) {
    return __atomic_compare_exchange_n(
        &atom->value,
        &expected,
        new_value,
        false,
        __ATOMIC_ACQ_REL,
        __ATOMIC_ACQUIRE
    );
}

// ============================================================================
// Memory Management
// ============================================================================

static inline void enterprise_memory_pool_lock(enterprise_memory_pool_t *pool) {
    while (__atomic_exchange_n(&pool->lock, 1, __ATOMIC_ACQUIRE) != 0) {
        sched_yield();
    }
}

static inline void enterprise_memory_pool_unlock(enterprise_memory_pool_t *pool) {
    __atomic_store_n(&pool->lock, 0, __ATOMIC_RELEASE);
}

enterprise_memory_pool_t* enterprise_memory_pool_create(
    size_t total_size,
    size_t block_size
) {
    if (block_size == 0 || total_size == 0) {
        return NULL;
    }

    if (block_size < sizeof(void *)) {
        block_size = sizeof(void *);
    }

    enterprise_memory_pool_t *pool = malloc(sizeof(enterprise_memory_pool_t));
    if (!pool) {
        return NULL;
    }

    pool->memory = malloc(total_size);
    if (!pool->memory) {
        free(pool);
        return NULL;
    }

    pool->total_size = total_size;
    pool->allocated = 0;
    pool->block_size = block_size;
    pool->lock = 0;
    pool->free_list = NULL;
    pool->free_blocks = 0;

    return pool;
}

void enterprise_memory_pool_destroy(enterprise_memory_pool_t *pool) {
    if (!pool) return;
    free(pool->memory);
    free(pool);
}

void* enterprise_memory_pool_alloc(enterprise_memory_pool_t *pool) {
    if (!pool) return NULL;

    enterprise_memory_pool_lock(pool);

    if (pool->free_list) {
        void *ptr = pool->free_list;
        pool->free_list = *(void **)pool->free_list;
        pool->free_blocks--;
        enterprise_memory_pool_unlock(pool);
        return ptr;
    }

    if (pool->allocated + pool->block_size > pool->total_size) {
        enterprise_memory_pool_unlock(pool);
        return NULL;
    }

    void *ptr = pool->memory + pool->allocated;
    pool->allocated += pool->block_size;

    enterprise_memory_pool_unlock(pool);
    return ptr;
}

void enterprise_memory_pool_free(enterprise_memory_pool_t *pool, void *ptr) {
    if (!pool || !ptr) return;

    // Ensure pointer belongs to pool and is block-aligned
    uintptr_t base = (uintptr_t)pool->memory;
    uintptr_t p = (uintptr_t)ptr;
    if (p < base || p >= base + pool->total_size) {
        return;
    }
    if (((p - base) % pool->block_size) != 0) {
        return;
    }

    enterprise_memory_pool_lock(pool);
    *(void **)ptr = pool->free_list;
    pool->free_list = ptr;
    pool->free_blocks++;
    enterprise_memory_pool_unlock(pool);
}

size_t enterprise_memory_pool_available(const enterprise_memory_pool_t *pool) {
    if (!pool || pool->block_size == 0) return 0;

    size_t remaining = (pool->total_size > pool->allocated)
        ? pool->total_size - pool->allocated
        : 0;

    return (remaining / pool->block_size) + pool->free_blocks;
}

void* enterprise_malloc_numa(size_t size, uint32_t node) {
    (void)node;
    return malloc(size);
}

void enterprise_free_numa(void *ptr) {
    free(ptr);
}

// ============================================================================
// Performance Monitoring
// ============================================================================

uint64_t enterprise_rdtsc(void) {
    uint64_t result;
#if ENTERPRISE_USE_X86_INTRINSICS
    uint32_t lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    result = ((uint64_t)hi << 32) | lo;
#else
    result = enterprise_read_clock_monotonic_ns();
#endif
    return result;
}

uint64_t enterprise_rdpmc(uint32_t counter_id) {
    uint64_t result = 0;
#if ENTERPRISE_USE_X86_INTRINSICS
    uint32_t lo, hi;
    __asm__ __volatile__("rdpmc" : "=a"(lo), "=d"(hi) : "c"(counter_id));
    result = ((uint64_t)hi << 32) | lo;
#else
    (void)counter_id;
#endif
    return result;
}

uint64_t enterprise_measure_operation_cycles(
    void (*operation)(void),
    uint32_t iterations
) {
    uint64_t start = enterprise_rdtsc();

    for (uint32_t i = 0; i < iterations; i++) {
        operation();
    }

    uint64_t end = enterprise_rdtsc();
    if (iterations == 0) {
        return 0;
    }
    return (end - start) / iterations;
}

enterprise_perf_monitor_t* enterprise_perf_monitor_create(
    uint32_t counter_id,
    size_t max_samples
) {
    enterprise_perf_monitor_t *monitor = malloc(sizeof(enterprise_perf_monitor_t));
    if (!monitor) return NULL;

    enterprise_perf_sample_t *samples = calloc(max_samples, sizeof(enterprise_perf_sample_t));
    if (!samples) {
        free(monitor);
        return NULL;
    }

    monitor->enabled = 0;
    monitor->counter_id = counter_id;
    monitor->samples = (uint64_t *)samples;
    monitor->sample_count = max_samples;
    monitor->current_sample = 0;
    return monitor;
}

void enterprise_perf_monitor_destroy(enterprise_perf_monitor_t *monitor) {
    if (!monitor) return;
    free((enterprise_perf_sample_t *)monitor->samples);
    free(monitor);
}

void enterprise_perf_monitor_start(enterprise_perf_monitor_t *monitor) {
    if (!monitor) return;
    __atomic_store_n(&monitor->enabled, 1, __ATOMIC_RELEASE);
}

void enterprise_perf_monitor_stop(enterprise_perf_monitor_t *monitor) {
    if (!monitor) return;
    __atomic_store_n(&monitor->enabled, 0, __ATOMIC_RELEASE);
}

void enterprise_perf_monitor_sample(enterprise_perf_monitor_t *monitor) {
    if (!monitor) return;
    if (__atomic_load_n(&monitor->enabled, __ATOMIC_ACQUIRE) == 0) return;

    size_t idx = __atomic_load_n((size_t *)&monitor->current_sample, __ATOMIC_RELAXED);
    if (idx >= monitor->sample_count) return;

    enterprise_perf_sample_t *samples = (enterprise_perf_sample_t *)monitor->samples;
    samples[idx].timestamp = enterprise_clock_monotonic();
    samples[idx].cycles = enterprise_rdtsc();
    samples[idx].instructions = 0;
    samples[idx].cache_misses = 0;
    samples[idx].branch_misses = 0;

    __atomic_fetch_add((size_t *)&monitor->current_sample, 1, __ATOMIC_ACQ_REL);
}

enterprise_perf_sample_t enterprise_perf_monitor_get_sample(
    const enterprise_perf_monitor_t *monitor,
    size_t index
) {
    enterprise_perf_sample_t empty = {0};
    if (!monitor || index >= monitor->sample_count) {
        return empty;
    }

    const enterprise_perf_sample_t *samples =
        (const enterprise_perf_sample_t *)monitor->samples;
    return samples[index];
}

// ============================================================================
// Cache Operations
// ============================================================================

void enterprise_cache_prefetch_t0(const void *ptr) {
#if ENTERPRISE_USE_X86_INTRINSICS
    __asm__ __volatile__("prefetcht0 (%0)" : : "r"(ptr));
#else
    __builtin_prefetch(ptr, 0, 3);
#endif
}

void enterprise_cache_prefetch_t1(const void *ptr) {
#if ENTERPRISE_USE_X86_INTRINSICS
    __asm__ __volatile__("prefetcht1 (%0)" : : "r"(ptr));
#else
    __builtin_prefetch(ptr, 0, 2);
#endif
}

void enterprise_cache_prefetch_t2(const void *ptr) {
#if ENTERPRISE_USE_X86_INTRINSICS
    __asm__ __volatile__("prefetcht2 (%0)" : : "r"(ptr));
#else
    __builtin_prefetch(ptr, 0, 1);
#endif
}

void enterprise_cache_clflush(void *ptr) {
#if ENTERPRISE_USE_X86_INTRINSICS
    __asm__ __volatile__("clflush (%0)" : : "r"(ptr) : "memory");
#else
    (void)ptr;
#endif
}

void enterprise_cache_mfence(void) {
#if ENTERPRISE_USE_X86_INTRINSICS
    __asm__ __volatile__("mfence" : : : "memory");
#else
    __sync_synchronize();
#endif
}

void enterprise_cache_lfence(void) {
#if ENTERPRISE_USE_X86_INTRINSICS
    __asm__ __volatile__("lfence" : : : "memory");
#else
    __sync_synchronize();
#endif
}

void enterprise_cache_sfence(void) {
#if ENTERPRISE_USE_X86_INTRINSICS
    __asm__ __volatile__("sfence" : : : "memory");
#else
    __sync_synchronize();
#endif
}

// ============================================================================
// SIMD Operations (portable fallbacks)
// ============================================================================

enterprise_simd_float4_t enterprise_simd_float4_add(
    const enterprise_simd_float4_t *a,
    const enterprise_simd_float4_t *b
) {
    enterprise_simd_float4_t result;
    for (size_t i = 0; i < 4; i++) {
        result.elements[i] = a->elements[i] + b->elements[i];
    }
    return result;
}

enterprise_simd_float4_t enterprise_simd_float4_multiply(
    const enterprise_simd_float4_t *a,
    const enterprise_simd_float4_t *b
) {
    enterprise_simd_float4_t result;
    for (size_t i = 0; i < 4; i++) {
        result.elements[i] = a->elements[i] * b->elements[i];
    }
    return result;
}

enterprise_simd_float4_t enterprise_simd_float4_dot(
    const enterprise_simd_float4_t *a,
    const enterprise_simd_float4_t *b
) {
    enterprise_simd_float4_t result = {{0, 0, 0, 0}};
    float dot = 0.0f;
    for (size_t i = 0; i < 4; i++) {
        dot += a->elements[i] * b->elements[i];
    }
    result.elements[0] = dot;
    return result;
}

float enterprise_simd_float4_reduce_sum(const enterprise_simd_float4_t *v) {
    float sum = 0.0f;
    for (size_t i = 0; i < 4; i++) {
        sum += v->elements[i];
    }
    return sum;
}

enterprise_simd_int32_4_t enterprise_simd_int32_4_add(
    const enterprise_simd_int32_4_t *a,
    const enterprise_simd_int32_4_t *b
) {
    enterprise_simd_int32_4_t result;
    for (size_t i = 0; i < 4; i++) {
        result.elements[i] = a->elements[i] + b->elements[i];
    }
    return result;
}

enterprise_simd_int32_4_t enterprise_simd_int32_4_multiply(
    const enterprise_simd_int32_4_t *a,
    const enterprise_simd_int32_4_t *b
) {
    enterprise_simd_int32_4_t result;
    for (size_t i = 0; i < 4; i++) {
        result.elements[i] = a->elements[i] * b->elements[i];
    }
    return result;
}

// ============================================================================
// Bit Operations
// ============================================================================

uint32_t enterprise_bit_scan_forward(uint64_t value) {
    if (value == 0) return 0;
    return (uint32_t)__builtin_ctzll(value);
}

uint32_t enterprise_bit_scan_reverse(uint64_t value) {
    if (value == 0) return 0;
    return 63u - (uint32_t)__builtin_clzll(value);
}

uint32_t enterprise_bit_count(uint64_t value) {
    return (uint32_t)__builtin_popcountll(value);
}

uint32_t enterprise_bit_count_leading_zeros(uint64_t value) {
    if (value == 0) return 64;
    return (uint32_t)__builtin_clzll(value);
}

uint32_t enterprise_bit_count_trailing_zeros(uint64_t value) {
    if (value == 0) return 64;
    return (uint32_t)__builtin_ctzll(value);
}

uint64_t enterprise_bit_rotate_left(uint64_t value, uint32_t count) {
    return (value << (count % 64)) | (value >> ((64 - count) % 64));
}

uint64_t enterprise_bit_rotate_right(uint64_t value, uint32_t count) {
    return (value >> (count % 64)) | (value << ((64 - count) % 64));
}

// ============================================================================
// Time & Clock Operations
// ============================================================================

uint64_t enterprise_clock_monotonic(void) {
    return enterprise_read_clock_monotonic_ns();
}

uint64_t enterprise_clock_realtime(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

enterprise_timer_t enterprise_timer_measure_operation(
    void (*operation)(void),
    uint32_t iterations
) {
    enterprise_timer_t result = {0, 0, 0};
    if (!operation || iterations == 0) {
        return result;
    }

    uint64_t start_ns = enterprise_clock_monotonic();
    uint64_t start_cycles = enterprise_rdtsc();

    for (uint32_t i = 0; i < iterations; i++) {
        operation();
    }

    uint64_t end_cycles = enterprise_rdtsc();
    uint64_t end_ns = enterprise_clock_monotonic();

    result.cycles = (end_cycles - start_cycles) / iterations;
    result.nanoseconds = end_ns - start_ns;
    result.microseconds = result.nanoseconds / 1000;
    return result;
}

// ============================================================================
// Fast String Operations
// ============================================================================

int enterprise_memcmp_fast(
    const void *s1,
    const void *s2,
    size_t len
) {
    // Use SIMD for large memcmp
    if (len >= 32) {
        const uint64_t *a = (const uint64_t *)s1;
        const uint64_t *b = (const uint64_t *)s2;

        for (size_t i = 0; i < len / 8; i++) {
            if (a[i] != b[i]) {
                return a[i] < b[i] ? -1 : 1;
            }
        }

        len %= 8;
    }

    // Fall back to C for remainder
    return memcmp(s1, s2, len);
}

void* enterprise_memcpy_fast(
    void *dest,
    const void *src,
    size_t len
) {
    return memcpy(dest, src, len);
}

void* enterprise_memset_fast(
    void *ptr,
    int value,
    size_t len
) {
    return memset(ptr, value, len);
}

uint32_t enterprise_strlen_fast(const char *str) {
    uint32_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

uint32_t enterprise_strchr_fast(const char *str, char c) {
    const char *found = strchr(str, c);
    if (!found) {
        return UINT32_MAX;
    }
    return (uint32_t)(found - str);
}

// ============================================================================
// Logging & Diagnostics
// ============================================================================

enterprise_logger_t* enterprise_logger_create(size_t buffer_size) {
    enterprise_logger_t *logger = malloc(sizeof(enterprise_logger_t));
    if (!logger) return NULL;

    logger->buffer = malloc(buffer_size);
    if (!logger->buffer) {
        free(logger);
        return NULL;
    }

    logger->log_level = ENTERPRISE_LOG_INFO;
    logger->enabled = 1;
    logger->buffer_size = buffer_size;
    logger->buffer_pos = 0;

    return logger;
}

void enterprise_logger_destroy(enterprise_logger_t *logger) {
    if (!logger) return;
    free(logger->buffer);
    free(logger);
}

void enterprise_logger_log(
    enterprise_logger_t *logger,
    uint32_t level,
    const char *format,
    ...
) {
    if (!logger || logger->enabled == 0 || level < logger->log_level) {
        return;
    }

    char temp[512];
    va_list args;
    va_start(args, format);
    int written = vsnprintf(temp, sizeof(temp), format, args);
    va_end(args);

    if (written <= 0) {
        return;
    }

    size_t msg_len = (size_t)written;
    if (msg_len >= sizeof(temp)) {
        msg_len = sizeof(temp) - 1;
    }

    if (logger->buffer_pos >= logger->buffer_size - 1) {
        return;
    }

    size_t available = logger->buffer_size - logger->buffer_pos - 1;
    if (msg_len > available) {
        msg_len = available;
    }

    if (msg_len == 0) {
        return;
    }

    memcpy(logger->buffer + logger->buffer_pos, temp, msg_len);
    logger->buffer_pos += msg_len;
    logger->buffer[logger->buffer_pos] = '\0';
}

uint32_t enterprise_stack_walk(
    enterprise_stack_frame_t *frames,
    uint32_t max_frames
) {
    if (frames && max_frames > 0) {
        memset(frames, 0, sizeof(enterprise_stack_frame_t) * max_frames);
    }
    return 0;
}

uint64_t enterprise_get_caller_address(void) {
#if defined(__GNUC__)
    return (uint64_t)(uintptr_t)__builtin_return_address(0);
#else
    return 0;
#endif
}

uint64_t enterprise_get_return_address(void) {
    return 0;
}

// ============================================================================
// Vector Operations
// ============================================================================

enterprise_vector_f32_t* enterprise_vector_f32_create(size_t length) {
    enterprise_vector_f32_t *vec = malloc(sizeof(enterprise_vector_f32_t));
    if (!vec) return NULL;

    vec->data = calloc(length, sizeof(float));
    if (!vec->data) {
        free(vec);
        return NULL;
    }

    vec->length = length;
    return vec;
}

void enterprise_vector_f32_destroy(enterprise_vector_f32_t *vec) {
    if (!vec) return;
    free(vec->data);
    free(vec);
}

float enterprise_vector_f32_dot(
    const enterprise_vector_f32_t *a,
    const enterprise_vector_f32_t *b
) {
    if (!a || !b || a->length != b->length) {
        return 0.0f;
    }

    float sum = 0.0f;
    for (size_t i = 0; i < a->length; i++) {
        sum += a->data[i] * b->data[i];
    }
    return sum;
}

void enterprise_vector_f32_add(
    enterprise_vector_f32_t *result,
    const enterprise_vector_f32_t *a,
    const enterprise_vector_f32_t *b
) {
    if (!result || !a || !b || result->length != a->length || a->length != b->length) {
        return;
    }

    for (size_t i = 0; i < a->length; i++) {
        result->data[i] = a->data[i] + b->data[i];
    }
}

void enterprise_vector_f32_scale(
    enterprise_vector_f32_t *vec,
    float scalar
) {
    if (!vec) return;
    for (size_t i = 0; i < vec->length; i++) {
        vec->data[i] *= scalar;
    }
}

// ============================================================================
// Hash Functions
// ============================================================================

static inline uint64_t enterprise_rotl64(uint64_t value, uint32_t count) {
    return (value << count) | (value >> (64 - count));
}

static inline uint64_t enterprise_fmix64(uint64_t k) {
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccdULL;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53ULL;
    k ^= k >> 33;
    return k;
}

static inline uint64_t enterprise_read64(const void *p) {
    uint64_t v;
    memcpy(&v, p, sizeof(v));
    return v;
}

static inline uint32_t enterprise_read32(const void *p) {
    uint32_t v;
    memcpy(&v, p, sizeof(v));
    return v;
}

uint64_t enterprise_hash_fnv1a(const void *data, size_t length) {
    uint64_t hash = 0xcbf29ce484222325ULL;
    const uint8_t *bytes = (const uint8_t *)data;

    for (size_t i = 0; i < length; i++) {
        hash ^= bytes[i];
        hash *= 0x100000001b3ULL;
    }

    return hash;
}

uint64_t enterprise_hash_murmur3(const void *data, size_t length) {
    const uint8_t *bytes = (const uint8_t *)data;
    const size_t nblocks = length / 16;

    uint64_t h1 = 0;
    uint64_t h2 = 0;

    const uint64_t c1 = 0x87c37b91114253d5ULL;
    const uint64_t c2 = 0x4cf5ad432745937fULL;

    for (size_t i = 0; i < nblocks; i++) {
        uint64_t k1 = enterprise_read64(bytes + i * 16);
        uint64_t k2 = enterprise_read64(bytes + i * 16 + 8);

        k1 *= c1;
        k1 = enterprise_rotl64(k1, 31);
        k1 *= c2;
        h1 ^= k1;

        h1 = enterprise_rotl64(h1, 27);
        h1 += h2;
        h1 = h1 * 5 + 0x52dce729;

        k2 *= c2;
        k2 = enterprise_rotl64(k2, 33);
        k2 *= c1;
        h2 ^= k2;

        h2 = enterprise_rotl64(h2, 31);
        h2 += h1;
        h2 = h2 * 5 + 0x38495ab5;
    }

    uint64_t k1 = 0;
    uint64_t k2 = 0;
    const uint8_t *tail = bytes + (nblocks * 16);

    switch (length & 15) {
        case 15: k2 ^= (uint64_t)tail[14] << 48;
        case 14: k2 ^= (uint64_t)tail[13] << 40;
        case 13: k2 ^= (uint64_t)tail[12] << 32;
        case 12: k2 ^= (uint64_t)tail[11] << 24;
        case 11: k2 ^= (uint64_t)tail[10] << 16;
        case 10: k2 ^= (uint64_t)tail[9] << 8;
        case 9:
            k2 ^= (uint64_t)tail[8];
            k2 *= c2;
            k2 = enterprise_rotl64(k2, 33);
            k2 *= c1;
            h2 ^= k2;
        case 8: k1 ^= (uint64_t)tail[7] << 56;
        case 7: k1 ^= (uint64_t)tail[6] << 48;
        case 6: k1 ^= (uint64_t)tail[5] << 40;
        case 5: k1 ^= (uint64_t)tail[4] << 32;
        case 4: k1 ^= (uint64_t)tail[3] << 24;
        case 3: k1 ^= (uint64_t)tail[2] << 16;
        case 2: k1 ^= (uint64_t)tail[1] << 8;
        case 1:
            k1 ^= (uint64_t)tail[0];
            k1 *= c1;
            k1 = enterprise_rotl64(k1, 31);
            k1 *= c2;
            h1 ^= k1;
    }

    h1 ^= length;
    h2 ^= length;

    h1 += h2;
    h2 += h1;

    h1 = enterprise_fmix64(h1);
    h2 = enterprise_fmix64(h2);

    h1 += h2;
    h2 += h1;

    return h1;
}

uint32_t enterprise_hash_crc32(const void *data, size_t length) {
    uint32_t crc = 0xffffffff;
    const uint8_t *bytes = (const uint8_t *)data;

    for (size_t i = 0; i < length; i++) {
        uint8_t byte = bytes[i];
        crc ^= byte;

        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xedb88320;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc ^ 0xffffffff;
}

uint64_t enterprise_hash_xxhash64(const void *data, size_t length) {
    const uint8_t *p = (const uint8_t *)data;
    const uint8_t *end = p + length;

    const uint64_t prime1 = 11400714785074694791ULL;
    const uint64_t prime2 = 14029467366897019727ULL;
    const uint64_t prime3 = 1609587929392839161ULL;
    const uint64_t prime4 = 9650029242287828579ULL;
    const uint64_t prime5 = 2870177450012600261ULL;

    uint64_t hash;

    if (length >= 32) {
        uint64_t v1 = prime1 + prime2;
        uint64_t v2 = prime2;
        uint64_t v3 = 0;
        uint64_t v4 = 0 - prime1;

        const uint8_t *limit = end - 32;
        do {
            v1 = enterprise_rotl64(v1 + enterprise_read64(p) * prime2, 31) * prime1;
            p += 8;
            v2 = enterprise_rotl64(v2 + enterprise_read64(p) * prime2, 31) * prime1;
            p += 8;
            v3 = enterprise_rotl64(v3 + enterprise_read64(p) * prime2, 31) * prime1;
            p += 8;
            v4 = enterprise_rotl64(v4 + enterprise_read64(p) * prime2, 31) * prime1;
            p += 8;
        } while (p <= limit);

        hash = enterprise_rotl64(v1, 1) + enterprise_rotl64(v2, 7)
            + enterprise_rotl64(v3, 12) + enterprise_rotl64(v4, 18);

        v1 = enterprise_rotl64(v1 * prime2, 31) * prime1;
        hash ^= v1;
        hash = hash * prime1 + prime4;

        v2 = enterprise_rotl64(v2 * prime2, 31) * prime1;
        hash ^= v2;
        hash = hash * prime1 + prime4;

        v3 = enterprise_rotl64(v3 * prime2, 31) * prime1;
        hash ^= v3;
        hash = hash * prime1 + prime4;

        v4 = enterprise_rotl64(v4 * prime2, 31) * prime1;
        hash ^= v4;
        hash = hash * prime1 + prime4;
    } else {
        hash = prime5;
    }

    hash += length;

    while ((size_t)(end - p) >= 8) {
        uint64_t k1 = enterprise_read64(p);
        k1 *= prime2;
        k1 = enterprise_rotl64(k1, 31);
        k1 *= prime1;
        hash ^= k1;
        hash = enterprise_rotl64(hash, 27) * prime1 + prime4;
        p += 8;
    }

    if ((size_t)(end - p) >= 4) {
        uint32_t k1 = enterprise_read32(p);
        hash ^= (uint64_t)k1 * prime1;
        hash = enterprise_rotl64(hash, 23) * prime2 + prime3;
        p += 4;
    }

    while (p < end) {
        hash ^= (*p) * prime5;
        hash = enterprise_rotl64(hash, 11) * prime1;
        p++;
    }

    hash ^= hash >> 33;
    hash *= prime2;
    hash ^= hash >> 29;
    hash *= prime3;
    hash ^= hash >> 32;

    return hash;
}

// ============================================================================
// CPU Feature Detection
// ============================================================================

enterprise_cpu_features_t enterprise_detect_cpu_features(void) {
    enterprise_cpu_features_t features = {0};
#if ENTERPRISE_USE_X86_INTRINSICS
    uint32_t eax, ebx, ecx, edx;

    __asm__ __volatile__(
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "0"(1));

    features.sse4_2 = (ecx >> 20) & 1;
    features.aes_ni = (ecx >> 25) & 1;
    features.rdrand = (ecx >> 30) & 1;
    features.rdtsc = (edx >> 4) & 1;

    __asm__ __volatile__(
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "0"(7), "2"(0));

    features.avx2 = (ebx >> 5) & 1;
    features.avx512 = (ebx >> 16) & 1;

    __asm__ __volatile__(
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "0"(1));

    features.avx = (ecx >> 28) & 1;
#else
    features.rdtsc = true;
#endif

    return features;
}

// ============================================================================
// Version
// ============================================================================

const char* enterprise_lib_version(void) {
    return ENTERPRISE_LIB_VERSION;
}
