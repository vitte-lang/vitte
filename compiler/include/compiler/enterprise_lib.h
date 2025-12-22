// Enterprise Library for Vitte Compiler
// High-performance utilities for complex business applications
// Copyright 2024 - Production Ready

#ifndef VITTE_ENTERPRISE_LIB_H
#define VITTE_ENTERPRISE_LIB_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// SECTION 1: Thread-Safe Data Structures
// ============================================================================

typedef struct {
    volatile uint64_t write_pos;
    volatile uint64_t read_pos;
    uint64_t *data;
    size_t capacity;
    size_t element_size;
    uint64_t padding[7];  // Ensure 64-byte cache line
} enterprise_ring_buffer_t;

typedef struct {
    volatile uint32_t state;  // 0=unlocked, 1=locked
    uint32_t owner_thread;
    uint64_t acquire_time;
    uint64_t padding[5];  // 64-byte cache line
} enterprise_spinlock_t;

typedef struct {
    volatile uint64_t readers;
    volatile uint64_t writers;
    volatile uint64_t write_waiters;
    uint64_t padding[5];  // 64-byte cache line
} enterprise_rwlock_t;

typedef struct {
    volatile uint64_t value;
    uint64_t padding[7];  // Prevent false sharing
} enterprise_atomic_t;

// ============================================================================
// SECTION 2: Ring Buffer Operations
// ============================================================================

enterprise_ring_buffer_t* enterprise_ring_buffer_create(
    size_t capacity,
    size_t element_size
);

void enterprise_ring_buffer_destroy(enterprise_ring_buffer_t *rb);

bool enterprise_ring_buffer_push(
    enterprise_ring_buffer_t *rb,
    const void *element
);

bool enterprise_ring_buffer_pop(
    enterprise_ring_buffer_t *rb,
    void *element
);

bool enterprise_ring_buffer_is_full(const enterprise_ring_buffer_t *rb);
bool enterprise_ring_buffer_is_empty(const enterprise_ring_buffer_t *rb);
size_t enterprise_ring_buffer_available(const enterprise_ring_buffer_t *rb);

// ============================================================================
// SECTION 3: Synchronization Primitives
// ============================================================================

void enterprise_spinlock_init(enterprise_spinlock_t *lock);
void enterprise_spinlock_acquire(enterprise_spinlock_t *lock);
void enterprise_spinlock_release(enterprise_spinlock_t *lock);
bool enterprise_spinlock_trylock(enterprise_spinlock_t *lock);
void enterprise_spinlock_acquire_with_timeout(
    enterprise_spinlock_t *lock,
    uint64_t timeout_cycles
);

void enterprise_rwlock_init(enterprise_rwlock_t *lock);
void enterprise_rwlock_read_lock(enterprise_rwlock_t *lock);
void enterprise_rwlock_read_unlock(enterprise_rwlock_t *lock);
void enterprise_rwlock_write_lock(enterprise_rwlock_t *lock);
void enterprise_rwlock_write_unlock(enterprise_rwlock_t *lock);
bool enterprise_rwlock_write_trylock(enterprise_rwlock_t *lock);

// ============================================================================
// SECTION 4: Atomic Operations
// ============================================================================

void enterprise_atomic_init(enterprise_atomic_t *atom, uint64_t value);
uint64_t enterprise_atomic_load(const enterprise_atomic_t *atom);
void enterprise_atomic_store(enterprise_atomic_t *atom, uint64_t value);
uint64_t enterprise_atomic_add_fetch(enterprise_atomic_t *atom, uint64_t delta);
uint64_t enterprise_atomic_sub_fetch(enterprise_atomic_t *atom, uint64_t delta);
bool enterprise_atomic_compare_exchange(
    enterprise_atomic_t *atom,
    uint64_t expected,
    uint64_t new_value
);

// ============================================================================
// SECTION 5: Memory Management
// ============================================================================

typedef struct {
    uint8_t *memory;
    size_t total_size;
    size_t allocated;
    size_t block_size;
    volatile uint32_t lock;
    void *free_list;
    size_t free_blocks;
} enterprise_memory_pool_t;

enterprise_memory_pool_t* enterprise_memory_pool_create(
    size_t total_size,
    size_t block_size
);

void enterprise_memory_pool_destroy(enterprise_memory_pool_t *pool);
void* enterprise_memory_pool_alloc(enterprise_memory_pool_t *pool);
void enterprise_memory_pool_free(enterprise_memory_pool_t *pool, void *ptr);
size_t enterprise_memory_pool_available(const enterprise_memory_pool_t *pool);

// NUMA-aware allocation
void* enterprise_malloc_numa(size_t size, uint32_t node);
void enterprise_free_numa(void *ptr);

// ============================================================================
// SECTION 6: Performance Monitoring
// ============================================================================

typedef struct {
    uint64_t timestamp;
    uint64_t cycles;
    uint64_t instructions;
    uint64_t cache_misses;
    uint64_t branch_misses;
} enterprise_perf_sample_t;

typedef struct {
    volatile uint64_t enabled;
    uint32_t counter_id;
    uint64_t *samples;
    size_t sample_count;
    volatile size_t current_sample;
} enterprise_perf_monitor_t;

enterprise_perf_monitor_t* enterprise_perf_monitor_create(
    uint32_t counter_id,
    size_t max_samples
);

void enterprise_perf_monitor_destroy(enterprise_perf_monitor_t *monitor);
void enterprise_perf_monitor_start(enterprise_perf_monitor_t *monitor);
void enterprise_perf_monitor_stop(enterprise_perf_monitor_t *monitor);
void enterprise_perf_monitor_sample(enterprise_perf_monitor_t *monitor);
enterprise_perf_sample_t enterprise_perf_monitor_get_sample(
    const enterprise_perf_monitor_t *monitor,
    size_t index
);

uint64_t enterprise_rdtsc(void);
uint64_t enterprise_rdpmc(uint32_t counter_id);
uint64_t enterprise_measure_operation_cycles(
    void (*operation)(void),
    uint32_t iterations
);

// ============================================================================
// SECTION 7: Cache Optimization
// ============================================================================

void enterprise_cache_prefetch_t0(const void *ptr);
void enterprise_cache_prefetch_t1(const void *ptr);
void enterprise_cache_prefetch_t2(const void *ptr);
void enterprise_cache_clflush(void *ptr);
void enterprise_cache_mfence(void);
void enterprise_cache_lfence(void);
void enterprise_cache_sfence(void);

// ============================================================================
// SECTION 8: SIMD Operations
// ============================================================================

typedef struct {
    float elements[4];
} enterprise_simd_float4_t;

typedef struct {
    double elements[2];
} enterprise_simd_float2_t;

typedef struct {
    int32_t elements[4];
} enterprise_simd_int32_4_t;

// Float operations
enterprise_simd_float4_t enterprise_simd_float4_add(
    const enterprise_simd_float4_t *a,
    const enterprise_simd_float4_t *b
);

enterprise_simd_float4_t enterprise_simd_float4_multiply(
    const enterprise_simd_float4_t *a,
    const enterprise_simd_float4_t *b
);

enterprise_simd_float4_t enterprise_simd_float4_dot(
    const enterprise_simd_float4_t *a,
    const enterprise_simd_float4_t *b
);

float enterprise_simd_float4_reduce_sum(const enterprise_simd_float4_t *v);

// Integer operations
enterprise_simd_int32_4_t enterprise_simd_int32_4_add(
    const enterprise_simd_int32_4_t *a,
    const enterprise_simd_int32_4_t *b
);

enterprise_simd_int32_4_t enterprise_simd_int32_4_multiply(
    const enterprise_simd_int32_4_t *a,
    const enterprise_simd_int32_4_t *b
);

// ============================================================================
// SECTION 9: String Operations
// ============================================================================

int enterprise_memcmp_fast(
    const void *s1,
    const void *s2,
    size_t len
);

void* enterprise_memcpy_fast(
    void *dest,
    const void *src,
    size_t len
);

void* enterprise_memset_fast(
    void *ptr,
    int value,
    size_t len
);

uint32_t enterprise_strlen_fast(const char *str);
uint32_t enterprise_strchr_fast(const char *str, char c);

// ============================================================================
// SECTION 10: Logging & Diagnostics
// ============================================================================

typedef struct {
    volatile uint32_t log_level;  // 0=debug, 1=info, 2=warn, 3=error
    volatile uint32_t enabled;
    char *buffer;
    size_t buffer_size;
    volatile size_t buffer_pos;
} enterprise_logger_t;

typedef struct {
    uint64_t frame_address;
    uint64_t return_address;
    uint32_t frame_size;
} enterprise_stack_frame_t;

enterprise_logger_t* enterprise_logger_create(size_t buffer_size);
void enterprise_logger_destroy(enterprise_logger_t *logger);
void enterprise_logger_log(
    enterprise_logger_t *logger,
    uint32_t level,
    const char *format,
    ...
);

uint32_t enterprise_stack_walk(
    enterprise_stack_frame_t *frames,
    uint32_t max_frames
);

uint64_t enterprise_get_caller_address(void);
uint64_t enterprise_get_return_address(void);

// ============================================================================
// SECTION 11: Bit Operations
// ============================================================================

uint32_t enterprise_bit_scan_forward(uint64_t value);
uint32_t enterprise_bit_scan_reverse(uint64_t value);
uint32_t enterprise_bit_count(uint64_t value);
uint32_t enterprise_bit_count_leading_zeros(uint64_t value);
uint32_t enterprise_bit_count_trailing_zeros(uint64_t value);
uint64_t enterprise_bit_rotate_left(uint64_t value, uint32_t count);
uint64_t enterprise_bit_rotate_right(uint64_t value, uint32_t count);

// ============================================================================
// SECTION 12: Time & Clock Operations
// ============================================================================

typedef struct {
    uint64_t cycles;
    uint64_t nanoseconds;
    uint64_t microseconds;
} enterprise_timer_t;

uint64_t enterprise_clock_monotonic(void);
uint64_t enterprise_clock_realtime(void);
enterprise_timer_t enterprise_timer_measure_operation(
    void (*operation)(void),
    uint32_t iterations
);

// ============================================================================
// SECTION 13: Vector/Matrix Operations (for data processing)
// ============================================================================

typedef struct {
    float *data;
    size_t length;
} enterprise_vector_f32_t;

typedef struct {
    double *data;
    size_t length;
} enterprise_vector_f64_t;

enterprise_vector_f32_t* enterprise_vector_f32_create(size_t length);
void enterprise_vector_f32_destroy(enterprise_vector_f32_t *vec);
float enterprise_vector_f32_dot(
    const enterprise_vector_f32_t *a,
    const enterprise_vector_f32_t *b
);
void enterprise_vector_f32_add(
    enterprise_vector_f32_t *result,
    const enterprise_vector_f32_t *a,
    const enterprise_vector_f32_t *b
);
void enterprise_vector_f32_scale(
    enterprise_vector_f32_t *vec,
    float scalar
);

// ============================================================================
// SECTION 14: Hash Functions (for database/cache operations)
// ============================================================================

uint64_t enterprise_hash_fnv1a(const void *data, size_t length);
uint64_t enterprise_hash_murmur3(const void *data, size_t length);
uint32_t enterprise_hash_crc32(const void *data, size_t length);
uint64_t enterprise_hash_xxhash64(const void *data, size_t length);

// ============================================================================
// SECTION 15: Configuration & Constants
// ============================================================================

#define ENTERPRISE_CACHE_LINE_SIZE 64
#define ENTERPRISE_PAGE_SIZE 4096
#define ENTERPRISE_ALIGN_CACHE_LINE __attribute__((aligned(64)))
#define ENTERPRISE_ALIGN_PAGE __attribute__((aligned(4096)))

#define ENTERPRISE_LOG_DEBUG 0
#define ENTERPRISE_LOG_INFO 1
#define ENTERPRISE_LOG_WARN 2
#define ENTERPRISE_LOG_ERROR 3

// ============================================================================
// Version and Feature Detection
// ============================================================================

#define ENTERPRISE_LIB_VERSION "1.0"
#define ENTERPRISE_LIB_MAJOR 1
#define ENTERPRISE_LIB_MINOR 0
#define ENTERPRISE_LIB_PATCH 0

typedef struct {
    bool sse4_2;
    bool avx;
    bool avx2;
    bool avx512;
    bool aes_ni;
    bool rdrand;
    bool rdtsc;
    bool rdpmc;
} enterprise_cpu_features_t;

enterprise_cpu_features_t enterprise_detect_cpu_features(void);

#ifdef __cplusplus
}
#endif

#endif // VITTE_ENTERPRISE_LIB_H
