// Enterprise Library Implementation
// High-performance utilities for complex business applications

#include "compiler/enterprise_lib.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
    uint64_t write_pos, next_pos, read_pos;

    // Atomically get write position
    asm volatile (
        "mov (%1), %%rax; "
        "mov %%rax, %0; "
        : "=r"(write_pos)
        : "r"(&rb->write_pos)
        : "rax", "memory"
    );

    // Load read position
    asm volatile (
        "mov (%1), %%rax; "
        "mov %%rax, %0; "
        : "=r"(read_pos)
        : "r"(&rb->read_pos)
        : "rax", "memory"
    );

    // Calculate next position
    next_pos = (write_pos + 1) % rb->capacity;

    // Check if buffer is full
    if (next_pos == read_pos) {
        return false;
    }

    // Copy element
    memcpy(
        (uint8_t *)rb->data + (write_pos * rb->element_size),
        element,
        rb->element_size
    );

    // Atomically increment write position
    asm volatile (
        "lock; cmpxchgq %1, (%2); "
        : : "a"(write_pos), "r"(next_pos), "r"(&rb->write_pos)
        : "memory"
    );

    return true;
}

bool enterprise_ring_buffer_pop(
    enterprise_ring_buffer_t *rb,
    void *element
) {
    uint64_t read_pos, next_pos, write_pos;

    // Load read position
    asm volatile (
        "mov (%1), %%rax; "
        "mov %%rax, %0; "
        : "=r"(read_pos)
        : "r"(&rb->read_pos)
        : "rax", "memory"
    );

    // Load write position
    asm volatile (
        "mov (%1), %%rax; "
        "mov %%rax, %0; "
        : "=r"(write_pos)
        : "r"(&rb->write_pos)
        : "rax", "memory"
    );

    // Check if buffer is empty
    if (read_pos == write_pos) {
        return false;
    }

    // Copy element
    memcpy(
        element,
        (uint8_t *)rb->data + (read_pos * rb->element_size),
        rb->element_size
    );

    // Calculate next position
    next_pos = (read_pos + 1) % rb->capacity;

    // Atomically increment read position
    asm volatile (
        "lock; cmpxchgq %1, (%2); "
        : : "a"(read_pos), "r"(next_pos), "r"(&rb->read_pos)
        : "memory"
    );

    return true;
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
    lock->state = 0;
    lock->owner_thread = 0;
}

void enterprise_spinlock_acquire(enterprise_spinlock_t *lock) {
    uint32_t backoff = 1;

    while (1) {
        // Try to acquire
        uint32_t old_state;
        asm volatile (
            "mov $1, %%eax; "
            "lock; cmpxchgl %%eax, (%1); "
            "mov %%eax, %0; "
            : "=r"(old_state)
            : "r"(&lock->state)
            : "rax", "memory"
        );

        if (old_state == 0) {
            return;  // Acquired
        }

        // Exponential backoff
        for (uint32_t i = 0; i < backoff; i++) {
            asm volatile ("pause");
        }

        backoff = (backoff < 4096) ? backoff * 2 : 4096;
    }
}

void enterprise_spinlock_release(enterprise_spinlock_t *lock) {
    asm volatile (
        "movl $0, (%0); "
        "mfence; "
        : : "r"(&lock->state)
        : "memory"
    );
}

bool enterprise_spinlock_trylock(enterprise_spinlock_t *lock) {
    uint32_t old_state;
    asm volatile (
        "mov $1, %%eax; "
        "lock; cmpxchgl %%eax, (%1); "
        "mov %%eax, %0; "
        : "=r"(old_state)
        : "r"(&lock->state)
        : "rax", "memory"
    );

    return old_state == 0;
}

// ============================================================================
// Atomic Operations
// ============================================================================

void enterprise_atomic_init(enterprise_atomic_t *atom, uint64_t value) {
    atom->value = value;
}

uint64_t enterprise_atomic_load(const enterprise_atomic_t *atom) {
    uint64_t value;
    asm volatile (
        "mov (%1), %0; "
        : "=r"(value)
        : "r"(&atom->value)
        : "memory"
    );
    return value;
}

void enterprise_atomic_store(enterprise_atomic_t *atom, uint64_t value) {
    asm volatile (
        "mov %1, (%0); "
        "mfence; "
        : : "r"(&atom->value), "r"(value)
        : "memory"
    );
}

uint64_t enterprise_atomic_add_fetch(enterprise_atomic_t *atom, uint64_t delta) {
    uint64_t result;
    asm volatile (
        "lock; addq %1, (%0); "
        "mov (%0), %2; "
        : : "r"(&atom->value), "r"(delta), "r"(result)
        : "memory"
    );
    return result;
}

bool enterprise_atomic_compare_exchange(
    enterprise_atomic_t *atom,
    uint64_t expected,
    uint64_t new_value
) {
    uint64_t old_value;
    asm volatile (
        "lock; cmpxchgq %2, (%1); "
        : "=a"(old_value)
        : "r"(&atom->value), "r"(new_value), "0"(expected)
        : "memory"
    );
    return old_value == expected;
}

// ============================================================================
// Performance Monitoring
// ============================================================================

uint64_t enterprise_rdtsc(void) {
    uint64_t result;
    asm volatile (
        "rdtsc; "
        "shl $32, %%rdx; "
        "or %%rdx, %%rax; "
        "mov %%rax, %0; "
        : "=r"(result)
        : : "rax", "rdx"
    );
    return result;
}

uint64_t enterprise_rdpmc(uint32_t counter_id) {
    uint64_t result;
    asm volatile (
        "rdpmc; "
        "shl $32, %%rdx; "
        "or %%rdx, %%rax; "
        "mov %%rax, %0; "
        : "=r"(result)
        : "c"(counter_id)
        : "rax", "rdx"
    );
    return result;
}

uint64_t enterprise_measure_operation_cycles(
    void (*operation)(void),
    uint32_t iterations
) {
    uint64_t start, end;

    // Serialize and start timing
    asm volatile (
        "cpuid; rdtsc; "
        "shl $32, %%rdx; or %%rdx, %%rax; "
        "mov %%rax, %0; "
        : "=m"(start)
        : : "rax", "rbx", "rcx", "rdx"
    );

    // Execute operation
    for (uint32_t i = 0; i < iterations; i++) {
        operation();
    }

    // Serialize and end timing
    asm volatile (
        "rdtsc; shl $32, %%rdx; or %%rdx, %%rax; "
        "mov %%rax, %0; cpuid; "
        : "=m"(end)
        : : "rax", "rbx", "rcx", "rdx"
    );

    return (end - start) / iterations;
}

// ============================================================================
// Cache Operations
// ============================================================================

void enterprise_cache_prefetch_t0(const void *ptr) {
    asm volatile ("prefetcht0 (%0)" : : "r"(ptr));
}

void enterprise_cache_prefetch_t1(const void *ptr) {
    asm volatile ("prefetcht1 (%0)" : : "r"(ptr));
}

void enterprise_cache_prefetch_t2(const void *ptr) {
    asm volatile ("prefetcht2 (%0)" : : "r"(ptr));
}

void enterprise_cache_clflush(void *ptr) {
    asm volatile ("clflush (%0)" : : "r"(ptr) : "memory");
}

void enterprise_cache_mfence(void) {
    asm volatile ("mfence" : : : "memory");
}

void enterprise_cache_lfence(void) {
    asm volatile ("lfence" : : : "memory");
}

void enterprise_cache_sfence(void) {
    asm volatile ("sfence" : : : "memory");
}

// ============================================================================
// Bit Operations
// ============================================================================

uint32_t enterprise_bit_scan_forward(uint64_t value) {
    uint32_t result;
    asm ("bsf %1, %0" : "=r"(result) : "r"(value));
    return result;
}

uint32_t enterprise_bit_scan_reverse(uint64_t value) {
    uint32_t result;
    asm ("bsr %1, %0" : "=r"(result) : "r"(value));
    return result;
}

uint32_t enterprise_bit_count(uint64_t value) {
    uint32_t result;
    asm ("popcnt %1, %0" : "=r"(result) : "r"(value));
    return result;
}

uint32_t enterprise_bit_count_leading_zeros(uint64_t value) {
    uint32_t result;
    asm ("lzcnt %1, %0" : "=r"(result) : "r"(value));
    return result;
}

uint32_t enterprise_bit_count_trailing_zeros(uint64_t value) {
    uint32_t result;
    asm ("tzcnt %1, %0" : "=r"(result) : "r"(value));
    return result;
}

uint64_t enterprise_bit_rotate_left(uint64_t value, uint32_t count) {
    uint64_t result;
    asm ("rol %b1, %0" : "=r"(result) : "r"(count), "0"(value));
    return result;
}

uint64_t enterprise_bit_rotate_right(uint64_t value, uint32_t count) {
    uint64_t result;
    asm ("ror %b1, %0" : "=r"(result) : "r"(count), "0"(value));
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
    // Use rep movsq for large copies
    if (len >= 64) {
        asm volatile (
            "cld; rep movsq; "
            : : "D"(dest), "S"(src), "c"(len >> 3)
            : "memory"
        );
        len %= 8;
    }

    // Handle remainder
    if (len > 0) {
        memcpy(dest, src, len);
    }

    return dest;
}

void* enterprise_memset_fast(
    void *ptr,
    int value,
    size_t len
) {
    // Use rep stosl for large fills
    if (len >= 64) {
        uint32_t pattern = (uint32_t)value;
        pattern |= (pattern << 8);
        pattern |= (pattern << 16);

        asm volatile (
            "cld; rep stosl; "
            : : "D"(ptr), "a"(pattern), "c"(len >> 2)
            : "memory"
        );
        len %= 4;
    }

    // Handle remainder
    if (len > 0) {
        memset(ptr, value, len);
    }

    return ptr;
}

uint32_t enterprise_strlen_fast(const char *str) {
    uint32_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

// ============================================================================
// Hash Functions
// ============================================================================

uint64_t enterprise_hash_fnv1a(const void *data, size_t length) {
    uint64_t hash = 0xcbf29ce484222325ULL;
    const uint8_t *bytes = (const uint8_t *)data;

    for (size_t i = 0; i < length; i++) {
        hash ^= bytes[i];
        hash *= 0x100000001b3ULL;
    }

    return hash;
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

// ============================================================================
// CPU Feature Detection
// ============================================================================

enterprise_cpu_features_t enterprise_detect_cpu_features(void) {
    enterprise_cpu_features_t features = {0};
    uint32_t eax, ebx, ecx, edx;

    // CPUID leaf 1: Feature flags
    asm volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "0"(1)
    );

    features.sse4_2 = (ecx >> 20) & 1;
    features.aes_ni = (ecx >> 25) & 1;
    features.rdrand = (ecx >> 30) & 1;
    features.rdtsc = (edx >> 4) & 1;

    // CPUID leaf 7: Extended features
    asm volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "0"(7), "2"(0)
    );

    features.avx2 = (ebx >> 5) & 1;
    features.avx512 = (ebx >> 16) & 1;

    // CPUID leaf 1, again for AVX
    asm volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "0"(1)
    );

    features.avx = (ecx >> 28) & 1;

    return features;
}

// ============================================================================
// Version
// ============================================================================

const char* enterprise_lib_version(void) {
    return ENTERPRISE_LIB_VERSION;
}
