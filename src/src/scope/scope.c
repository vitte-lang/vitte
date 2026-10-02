/*
 * Vitte Compiler
 * src/scope/scope.c
 *
 * Scope, symbol table and lexical name-resolution implementation.
 *
 * Public contract: scope.h
 *
 * Responsibilities:
 *   - lexical scope creation/destruction;
 *   - hierarchical parent/child relationships;
 *   - deterministic symbol insertion;
 *   - duplicate detection;
 *   - local and recursive name lookup;
 *   - symbol visibility;
 *   - declaration/source metadata;
 *   - shadowing detection;
 *   - scope depth tracking;
 *   - stable IDs;
 *   - deterministic fingerprints;
 *   - structural validation;
 *   - bounded resource usage;
 *   - statistics;
 *   - explicit lifecycle.
 *
 * Design:
 *   - ISO C17;
 *   - scope.h is the single public contract;
 *   - no parser/HIR/IR dependency;
 *   - names are copied and owned by the context;
 *   - caller payloads are opaque and never freed by this subsystem;
 *   - stable numeric scope/symbol IDs;
 *   - deterministic insertion order;
 *   - open-addressed hash tables for local name lookup;
 *   - explicit overflow checks;
 *   - no global mutable state;
 *   - validation suitable for compiler debug/testing builds.
 */

#include "scope.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* Private constants                                                         */
/* ========================================================================= */

#define VITTE_SCOPE_PRIVATE_EMPTY_SLOT ((size_t)0u)

#define VITTE_SCOPE_PRIVATE_MIN_HASH_CAPACITY ((size_t)16u)

#define VITTE_SCOPE_PRIVATE_HASH_LOAD_NUMERATOR ((size_t)7u)
#define VITTE_SCOPE_PRIVATE_HASH_LOAD_DENOMINATOR ((size_t)10u)

/* ========================================================================= */
/* Private arithmetic                                                        */
/* ========================================================================= */

static bool
vitte_scope_size_add(
    size_t left,
    size_t right,
    size_t *result)
{
    if (result == NULL) {
        return false;
    }

    if (left > SIZE_MAX - right) {
        return false;
    }

    *result = left + right;
    return true;
}

static bool
vitte_scope_size_mul(
    size_t left,
    size_t right,
    size_t *result)
{
    if (result == NULL) {
        return false;
    }

    if (left != 0u &&
        right > SIZE_MAX / left) {
        return false;
    }

    *result = left * right;
    return true;
}

static uint64_t
vitte_scope_u64_add_sat(
    uint64_t left,
    uint64_t right)
{
    if (UINT64_MAX - left < right) {
        return UINT64_MAX;
    }

    return left + right;
}

/* ========================================================================= */
/* Private hashing                                                           */
/* ========================================================================= */

static uint64_t
vitte_scope_hash_byte(
    uint64_t hash,
    unsigned char value)
{
    hash ^= (uint64_t)value;
    hash *= VITTE_SCOPE_FNV_PRIME;

    return hash;
}

static uint64_t
vitte_scope_hash_bytes(
    uint64_t hash,
    const unsigned char *data,
    size_t length)
{
    size_t index;

    if (data == NULL &&
        length != 0u) {
        return hash;
    }

    for (index = 0u;
         index < length;
         ++index) {
        hash =
            vitte_scope_hash_byte(
                hash,
                data[index]);
    }

    return hash;
}

static uint64_t
vitte_scope_hash_u64(
    uint64_t hash,
    uint64_t value)
{
    unsigned int shift;

    for (shift = 0u;
         shift < 64u;
         shift += 8u) {
        hash =
            vitte_scope_hash_byte(
                hash,
                (unsigned char)(
                    (value >> shift) &
                    UINT64_C(0xff)));
    }

    return hash;
}

static uint64_t
vitte_scope_name_hash(
    const char *name,
    size_t length)
{
    uint64_t hash;

    hash = VITTE_SCOPE_FNV_OFFSET;

    hash =
        vitte_scope_hash_bytes(
            hash,
            (const unsigned char *)name,
            length);

    /*
     * Zero is kept available as an invalid/sentinel hash.
     */
    if (hash == UINT64_C(0)) {
        hash = UINT64_C(1);
    }

    return hash;
}

/* ========================================================================= */
/* Private allocation                                                        */
/* ========================================================================= */

static void *
vitte_scope_allocate(
    vitte_scope_t *context,
    size_t count,
    size_t element_size)
{
    size_t bytes;
    void *memory;

    if (context == NULL) {
        return NULL;
    }

    if (!vitte_scope_size_mul(
            count,
            element_size,
            &bytes)) {
        context->last_error =
            VITTE_SCOPE_ERROR_OVERFLOW;

        return NULL;
    }

    if (bytes == 0u) {
        bytes = 1u;
    }

    memory = calloc(1u, bytes);

    if (memory == NULL) {
        context->stats.allocation_failures =
            vitte_scope_u64_add_sat(
                context->stats.allocation_failures,
                UINT64_C(1));

        context->last_error =
            VITTE_SCOPE_ERROR_OUT_OF_MEMORY;

        return NULL;
    }

    context->stats.allocations =
        vitte_scope_u64_add_sat(
            context->stats.allocations,
            UINT64_C(1));

    return memory;
}

static void *
vitte_scope_reallocate(
    vitte_scope_t *context,
    void *memory,
    size_t count,
    size_t element_size)
{
    size_t bytes;
    void *result;

    if (context == NULL) {
        return NULL;
    }

    if (!vitte_scope_size_mul(
            count,
            element_size,
            &bytes)) {
        context->last_error =
            VITTE_SCOPE_ERROR_OVERFLOW;

        return NULL;
    }

    if (bytes == 0u) {
        bytes = 1u;
    }

    result = realloc(memory, bytes);

    if (result == NULL) {
        context->stats.allocation_failures =
            vitte_scope_u64_add_sat(
                context->stats.allocation_failures,
                UINT64_C(1));

        context->last_error =
            VITTE_SCOPE_ERROR_OUT_OF_MEMORY;

        return NULL;
    }

    context->stats.reallocations =
        vitte_scope_u64_add_sat(
            context->stats.reallocations,
            UINT64_C(1));

    return result;
}

static char *
vitte_scope_copy_string(
    vitte_scope_t *context,
    const char *source,
    size_t length)
{
    size_t bytes;
    char *copy;

    if (context == NULL ||
        source == NULL) {
        return NULL;
    }

    if (!vitte_scope_size_add(
            length,
            1u,
            &bytes)) {
        context->last_error =
            VITTE_SCOPE_ERROR_OVERFLOW;

        return NULL;
    }

    copy =
        (char *)vitte_scope_allocate(
            context,
            bytes,
            sizeof(char));

    if (copy == NULL) {
        return NULL;
    }

    if (length != 0u) {
        memcpy(copy, source, length);
    }

    copy[length] = '\0';

    return copy;
}

/* ========================================================================= */
/* Private context helpers                                                   */
/* ========================================================================= */

static bool
vitte_scope_context_valid_private(
    const vitte_scope_t *context)
{
    if (context == NULL) {
        return false;
    }

    if (context->magic !=
        VITTE_SCOPE_MAGIC) {
        return false;
    }

    if (context->state <=
            VITTE_SCOPE_STATE_INVALID ||
        context->state >=
            VITTE_SCOPE_STATE_DESTROYED) {
        return false;
    }

    if (context->scope_count >
        context->scope_capacity) {
        return false;
    }

    if (context->symbol_count >
        context->symbol_capacity) {
        return false;
    }

    if (context->scope_count != 0u &&
        context->scopes == NULL) {
        return false;
    }

    if (context->symbol_count != 0u &&
        context->symbols == NULL) {
        return false;
    }

    if (context->max_scopes == 0u ||
        context->max_symbols == 0u ||
        context->max_name_bytes == 0u ||
        context->max_depth == 0u) {
        return false;
    }

    if (context->scope_count >
        context->max_scopes ||
        context->symbol_count >
        context->max_symbols) {
        return false;
    }

    if (context->root_scope !=
            VITTE_SCOPE_INVALID_SCOPE_ID &&
        context->root_scope >
            (vitte_scope_id_t)context->scope_count) {
        return false;
    }

    return true;
}

static bool
vitte_scope_fail(
    vitte_scope_t *context,
    vitte_scope_error_t error)
{
    if (context != NULL &&
        context->magic ==
            VITTE_SCOPE_MAGIC) {
        context->last_error = error;

        if (context->state !=
            VITTE_SCOPE_STATE_DESTROYED) {
            context->state =
                VITTE_SCOPE_STATE_FAILED;
        }

        context->stats.failures =
            vitte_scope_u64_add_sat(
                context->stats.failures,
                UINT64_C(1));
    }

    return false;
}

static void
vitte_scope_set_error(
    vitte_scope_t *context,
    vitte_scope_error_t error)
{
    if (context != NULL &&
        context->magic ==
            VITTE_SCOPE_MAGIC) {
        context->last_error = error;
    }
}

/* ========================================================================= */
/* Private ID helpers                                                        */
/* ========================================================================= */

static bool
vitte_scope_scope_id_to_index(
    const vitte_scope_t *context,
    vitte_scope_id_t id,
    size_t *index)
{
    uint64_t raw;

    if (context == NULL ||
        index == NULL ||
        id == VITTE_SCOPE_INVALID_SCOPE_ID) {
        return false;
    }

    raw = (uint64_t)id;

    if (raw == UINT64_C(0) ||
        raw > (uint64_t)context->scope_count) {
        return false;
    }

    raw -= UINT64_C(1);

    if (raw > (uint64_t)SIZE_MAX) {
        return false;
    }

    *index = (size_t)raw;

    return true;
}

static bool
vitte_scope_symbol_id_to_index(
    const vitte_scope_t *context,
    vitte_symbol_id_t id,
    size_t *index)
{
    uint64_t raw;

    if (context == NULL ||
        index == NULL ||
        id == VITTE_SCOPE_INVALID_SYMBOL_ID) {
        return false;
    }

    raw = (uint64_t)id;

    if (raw == UINT64_C(0) ||
        raw > (uint64_t)context->symbol_count) {
        return false;
    }

    raw -= UINT64_C(1);

    if (raw > (uint64_t)SIZE_MAX) {
        return false;
    }

    *index = (size_t)raw;

    return true;
}

/* ========================================================================= */
/* Private capacity helpers                                                  */
/* ========================================================================= */

static bool
vitte_scope_reserve_scopes(
    vitte_scope_t *context,
    size_t required)
{
    size_t capacity;
    vitte_scope_entry_t *entries;

    if (context == NULL) {
        return false;
    }

    if (required <=
        context->scope_capacity) {
        return true;
    }

    if (required >
        context->max_scopes) {
        context->last_error =
            VITTE_SCOPE_ERROR_SCOPE_LIMIT;

        return false;
    }

    capacity =
        context->scope_capacity;

    if (capacity == 0u) {
        capacity =
            VITTE_SCOPE_DEFAULT_INITIAL_SCOPE_CAPACITY;
    }

    while (capacity < required) {
        size_t next;

        if (capacity >=
            context->max_scopes) {
            capacity =
                context->max_scopes;

            break;
        }

        if (capacity >
            SIZE_MAX / 2u) {
            next =
                context->max_scopes;
        } else {
            next =
                capacity * 2u;
        }

        if (next >
            context->max_scopes) {
            next =
                context->max_scopes;
        }

        if (next <= capacity) {
            context->last_error =
                VITTE_SCOPE_ERROR_OVERFLOW;

            return false;
        }

        capacity = next;
    }

    entries =
        (vitte_scope_entry_t *)
        vitte_scope_reallocate(
            context,
            context->scopes,
            capacity,
            sizeof(*entries));

    if (entries == NULL) {
        return false;
    }

    if (capacity >
        context->scope_capacity) {
        size_t old_bytes;
        size_t new_bytes;

        if (!vitte_scope_size_mul(
                context->scope_capacity,
                sizeof(*entries),
                &old_bytes) ||
            !vitte_scope_size_mul(
                capacity,
                sizeof(*entries),
                &new_bytes)) {
            context->last_error =
                VITTE_SCOPE_ERROR_OVERFLOW;

            return false;
        }

        if (new_bytes > old_bytes) {
            memset(
                ((unsigned char *)entries) +
                    old_bytes,
                0,
                new_bytes - old_bytes);
        }
    }

    context->scopes = entries;
    context->scope_capacity = capacity;

    return true;
}

static bool
vitte_scope_reserve_symbols(
    vitte_scope_t *context,
    size_t required)
{
    size_t capacity;
    vitte_symbol_t *symbols;

    if (context == NULL) {
        return false;
    }

    if (required <=
        context->symbol_capacity) {
        return true;
    }

    if (required >
        context->max_symbols) {
        context->last_error =
            VITTE_SCOPE_ERROR_SYMBOL_LIMIT;

        return false;
    }

    capacity =
        context->symbol_capacity;

    if (capacity == 0u) {
        capacity =
            VITTE_SCOPE_DEFAULT_INITIAL_SYMBOL_CAPACITY;
    }

    while (capacity < required) {
        size_t next;

        if (capacity >=
            context->max_symbols) {
            capacity =
                context->max_symbols;

            break;
        }

        if (capacity >
            SIZE_MAX / 2u) {
            next =
                context->max_symbols;
        } else {
            next =
                capacity * 2u;
        }

        if (next >
            context->max_symbols) {
            next =
                context->max_symbols;
        }

        if (next <= capacity) {
            context->last_error =
                VITTE_SCOPE_ERROR_OVERFLOW;

            return false;
        }

        capacity = next;
    }

    symbols =
        (vitte_symbol_t *)
        vitte_scope_reallocate(
            context,
            context->symbols,
            capacity,
            sizeof(*symbols));

    if (symbols == NULL) {
        return false;
    }

    if (capacity >
        context->symbol_capacity) {
        size_t old_bytes;
        size_t new_bytes;

        if (!vitte_scope_size_mul(
                context->symbol_capacity,
                sizeof(*symbols),
                &old_bytes) ||
            !vitte_scope_size_mul(
                capacity,
                sizeof(*symbols),
                &new_bytes)) {
            context->last_error =
                VITTE_SCOPE_ERROR_OVERFLOW;

            return false;
        }

        if (new_bytes > old_bytes) {
            memset(
                ((unsigned char *)symbols) +
                    old_bytes,
                0,
                new_bytes - old_bytes);
        }
    }

    context->symbols = symbols;
    context->symbol_capacity = capacity;

    return true;
}

static bool
vitte_scope_reserve_children(
    vitte_scope_t *context,
    vitte_scope_entry_t *scope,
    size_t required)
{
    size_t capacity;
    vitte_scope_id_t *children;

    if (context == NULL ||
        scope == NULL) {
        return false;
    }

    if (required <=
        scope->child_capacity) {
        return true;
    }

    capacity = scope->child_capacity;

    if (capacity == 0u) {
        capacity = 4u;
    }

    while (capacity < required) {
        if (capacity >
            SIZE_MAX / 2u) {
            context->last_error =
                VITTE_SCOPE_ERROR_OVERFLOW;

            return false;
        }

        capacity *= 2u;
    }

    children =
        (vitte_scope_id_t *)
        vitte_scope_reallocate(
            context,
            scope->children,
            capacity,
            sizeof(*children));

    if (children == NULL) {
        return false;
    }

    scope->children = children;
    scope->child_capacity = capacity;

    return true;
}

static bool
vitte_scope_reserve_local_symbols(
    vitte_scope_t *context,
    vitte_scope_entry_t *scope,
    size_t required)
{
    size_t capacity;
    vitte_symbol_id_t *symbols;

    if (context == NULL ||
        scope == NULL) {
        return false;
    }

    if (required <=
        scope->symbol_capacity) {
        return true;
    }

    capacity = scope->symbol_capacity;

    if (capacity == 0u) {
        capacity = 8u;
    }

    while (capacity < required) {
        if (capacity >
            SIZE_MAX / 2u) {
            context->last_error =
                VITTE_SCOPE_ERROR_OVERFLOW;

            return false;
        }

        capacity *= 2u;
    }

    symbols =
        (vitte_symbol_id_t *)
        vitte_scope_reallocate(
            context,
            scope->symbols,
            capacity,
            sizeof(*symbols));

    if (symbols == NULL) {
        return false;
    }

    scope->symbols = symbols;
    scope->symbol_capacity = capacity;

    return true;
}

/* ========================================================================= */
/* Private scope lookup                                                      */
/* ========================================================================= */

static vitte_scope_entry_t *
vitte_scope_get_scope_mut_private(
    vitte_scope_t *context,
    vitte_scope_id_t id)
{
    size_t index;

    if (context == NULL ||
        !vitte_scope_scope_id_to_index(
            context,
            id,
            &index)) {
        return NULL;
    }

    return &context->scopes[index];
}

static const vitte_scope_entry_t *
vitte_scope_get_scope_private(
    const vitte_scope_t *context,
    vitte_scope_id_t id)
{
    size_t index;

    if (context == NULL ||
        !vitte_scope_scope_id_to_index(
            context,
            id,
            &index)) {
        return NULL;
    }

    return &context->scopes[index];
}

static vitte_symbol_t *
vitte_scope_get_symbol_mut_private(
    vitte_scope_t *context,
    vitte_symbol_id_t id)
{
    size_t index;

    if (context == NULL ||
        !vitte_scope_symbol_id_to_index(
            context,
            id,
            &index)) {
        return NULL;
    }

    return &context->symbols[index];
}

static const vitte_symbol_t *
vitte_scope_get_symbol_private(
    const vitte_scope_t *context,
    vitte_symbol_id_t id)
{
    size_t index;

    if (context == NULL ||
        !vitte_scope_symbol_id_to_index(
            context,
            id,
            &index)) {
        return NULL;
    }

    return &context->symbols[index];
}

/* ========================================================================= */
/* Private local hash table                                                  */
/* ========================================================================= */

static bool
vitte_scope_hash_capacity_valid(
    size_t capacity)
{
    return capacity == 0u ||
           (capacity >=
                VITTE_SCOPE_PRIVATE_MIN_HASH_CAPACITY &&
            (capacity & (capacity - 1u)) == 0u);
}

static bool
vitte_scope_hash_should_grow(
    size_t count,
    size_t capacity)
{
    size_t threshold;

    if (capacity == 0u) {
        return true;
    }

    if (capacity >
        SIZE_MAX /
            VITTE_SCOPE_PRIVATE_HASH_LOAD_NUMERATOR) {
        return count >=
               capacity -
                   (capacity /
                    VITTE_SCOPE_PRIVATE_HASH_LOAD_DENOMINATOR);
    }

    threshold =
        (capacity *
         VITTE_SCOPE_PRIVATE_HASH_LOAD_NUMERATOR) /
        VITTE_SCOPE_PRIVATE_HASH_LOAD_DENOMINATOR;

    return count >= threshold;
}

static bool
vitte_scope_hash_insert_into(
    const vitte_scope_t *context,
    size_t *table,
    size_t capacity,
    vitte_symbol_id_t symbol_id)
{
    const vitte_symbol_t *symbol;
    size_t mask;
    size_t slot;
    size_t probes;

    if (context == NULL ||
        table == NULL ||
        capacity == 0u ||
        !vitte_scope_hash_capacity_valid(capacity)) {
        return false;
    }

    symbol =
        vitte_scope_get_symbol_private(
            context,
            symbol_id);

    if (symbol == NULL ||
        symbol->name == NULL ||
        symbol->name_hash == UINT64_C(0)) {
        return false;
    }

    mask = capacity - 1u;

    slot =
        (size_t)(
            symbol->name_hash &
            (uint64_t)mask);

    for (probes = 0u;
         probes < capacity;
         ++probes) {
        if (table[slot] ==
            VITTE_SCOPE_PRIVATE_EMPTY_SLOT) {
            size_t symbol_index;

            if (!vitte_scope_symbol_id_to_index(
                    context,
                    symbol_id,
                    &symbol_index)) {
                return false;
            }

            if (symbol_index == SIZE_MAX) {
                return false;
            }

            /*
             * Stored table values are index + 1 so zero remains empty.
             */
            table[slot] =
                symbol_index + 1u;

            return true;
        }

        slot =
            (slot + 1u) & mask;
    }

    return false;
}

static bool
vitte_scope_rebuild_hash(
    vitte_scope_t *context,
    vitte_scope_entry_t *scope,
    size_t new_capacity)
{
    size_t *new_table;
    size_t index;

    if (context == NULL ||
        scope == NULL ||
        !vitte_scope_hash_capacity_valid(
            new_capacity) ||
        new_capacity == 0u) {
        return false;
    }

    new_table =
        (size_t *)vitte_scope_allocate(
            context,
            new_capacity,
            sizeof(*new_table));

    if (new_table == NULL) {
        return false;
    }

    for (index = 0u;
         index < scope->symbol_count;
         ++index) {
        if (!vitte_scope_hash_insert_into(
                context,
                new_table,
                new_capacity,
                scope->symbols[index])) {
            free(new_table);

            context->last_error =
                VITTE_SCOPE_ERROR_CORRUPTION;

            return false;
        }
    }

    free(scope->hash_slots);

    scope->hash_slots = new_table;
    scope->hash_capacity = new_capacity;

    return true;
}

static bool
vitte_scope_ensure_hash_capacity(
    vitte_scope_t *context,
    vitte_scope_entry_t *scope,
    size_t future_count)
{
    size_t capacity;

    if (context == NULL ||
        scope == NULL) {
        return false;
    }

    if (!vitte_scope_hash_should_grow(
            future_count,
            scope->hash_capacity)) {
        return true;
    }

    capacity = scope->hash_capacity;

    if (capacity == 0u) {
        capacity =
            VITTE_SCOPE_PRIVATE_MIN_HASH_CAPACITY;
    } else {
        if (capacity > SIZE_MAX / 2u) {
            context->last_error =
                VITTE_SCOPE_ERROR_OVERFLOW;

            return false;
        }

        capacity *= 2u;
    }

    return vitte_scope_rebuild_hash(
        context,
        scope,
        capacity);
}

static vitte_symbol_id_t
vitte_scope_lookup_local_private(
    const vitte_scope_t *context,
    const vitte_scope_entry_t *scope,
    const char *name,
    size_t name_length,
    uint64_t name_hash)
{
    size_t mask;
    size_t slot;
    size_t probes;

    if (context == NULL ||
        scope == NULL ||
        name == NULL) {
        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    if (scope->hash_capacity == 0u ||
        scope->hash_slots == NULL) {
        size_t index;

        /*
         * Fallback keeps lookup correct even before the first hash-table
         * allocation or while validating partially built contexts.
         */
        for (index = 0u;
             index < scope->symbol_count;
             ++index) {
            const vitte_symbol_t *symbol;

            symbol =
                vitte_scope_get_symbol_private(
                    context,
                    scope->symbols[index]);

            if (symbol == NULL) {
                continue;
            }

            if (symbol->name_hash ==
                    name_hash &&
                symbol->name_length ==
                    name_length &&
                memcmp(
                    symbol->name,
                    name,
                    name_length) == 0) {
                return symbol->id;
            }
        }

        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    if (!vitte_scope_hash_capacity_valid(
            scope->hash_capacity)) {
        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    mask =
        scope->hash_capacity - 1u;

    slot =
        (size_t)(
            name_hash &
            (uint64_t)mask);

    for (probes = 0u;
         probes < scope->hash_capacity;
         ++probes) {
        size_t encoded;
        size_t symbol_index;
        const vitte_symbol_t *symbol;

        encoded =
            scope->hash_slots[slot];

        if (encoded ==
            VITTE_SCOPE_PRIVATE_EMPTY_SLOT) {
            return VITTE_SCOPE_INVALID_SYMBOL_ID;
        }

        symbol_index = encoded - 1u;

        if (symbol_index >=
            context->symbol_count) {
            return VITTE_SCOPE_INVALID_SYMBOL_ID;
        }

        symbol =
            &context->symbols[symbol_index];

        if (symbol->scope_id ==
                scope->id &&
            symbol->name_hash ==
                name_hash &&
            symbol->name_length ==
                name_length &&
            memcmp(
                symbol->name,
                name,
                name_length) == 0) {
            return symbol->id;
        }

        slot =
            (slot + 1u) & mask;
    }

    return VITTE_SCOPE_INVALID_SYMBOL_ID;
}

/* ========================================================================= */
/* Private scope hierarchy helpers                                           */
/* ========================================================================= */

static bool
vitte_scope_is_ancestor_private(
    const vitte_scope_t *context,
    vitte_scope_id_t ancestor,
    vitte_scope_id_t descendant)
{
    vitte_scope_id_t current;
    size_t guard;

    if (context == NULL ||
        ancestor ==
            VITTE_SCOPE_INVALID_SCOPE_ID ||
        descendant ==
            VITTE_SCOPE_INVALID_SCOPE_ID) {
        return false;
    }

    current = descendant;
    guard = 0u;

    while (current !=
           VITTE_SCOPE_INVALID_SCOPE_ID) {
        const vitte_scope_entry_t *scope;

        if (current == ancestor) {
            return true;
        }

        if (guard >=
            context->scope_count) {
            return false;
        }

        ++guard;

        scope =
            vitte_scope_get_scope_private(
                context,
                current);

        if (scope == NULL) {
            return false;
        }

        current =
            scope->parent_id;
    }

    return false;
}

static vitte_symbol_id_t
vitte_scope_find_shadowed_private(
    const vitte_scope_t *context,
    vitte_scope_id_t scope_id,
    const char *name,
    size_t name_length,
    uint64_t name_hash)
{
    const vitte_scope_entry_t *scope;
    vitte_scope_id_t current;
    size_t guard;

    scope =
        vitte_scope_get_scope_private(
            context,
            scope_id);

    if (scope == NULL) {
        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    current = scope->parent_id;
    guard = 0u;

    while (current !=
           VITTE_SCOPE_INVALID_SCOPE_ID) {
        const vitte_scope_entry_t *parent;
        vitte_symbol_id_t found;

        if (guard >=
            context->scope_count) {
            return VITTE_SCOPE_INVALID_SYMBOL_ID;
        }

        ++guard;

        parent =
            vitte_scope_get_scope_private(
                context,
                current);

        if (parent == NULL) {
            return VITTE_SCOPE_INVALID_SYMBOL_ID;
        }

        found =
            vitte_scope_lookup_local_private(
                context,
                parent,
                name,
                name_length,
                name_hash);

        if (found !=
            VITTE_SCOPE_INVALID_SYMBOL_ID) {
            return found;
        }

        current =
            parent->parent_id;
    }

    return VITTE_SCOPE_INVALID_SYMBOL_ID;
}

/* ========================================================================= */
/* Name helpers                                                              */
/* ========================================================================= */

static bool
vitte_scope_name_valid_private(
    const char *name,
    size_t name_length)
{
    size_t index;

    if (name == NULL ||
        name_length == 0u) {
        return false;
    }

    /*
     * Symbol names are stored as explicit byte sequences but embedded NULs
     * are rejected so the public NUL-terminated view remains unambiguous.
     */
    for (index = 0u;
         index < name_length;
         ++index) {
        if (name[index] == '\0') {
            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* Public names                                                              */
/* ========================================================================= */

const char *
vitte_scope_error_name(
    vitte_scope_error_t error)
{
    switch (error) {
        case VITTE_SCOPE_ERROR_NONE:
            return "none";

        case VITTE_SCOPE_ERROR_INVALID_ARGUMENT:
            return "invalid_argument";

        case VITTE_SCOPE_ERROR_INVALID_CONTEXT:
            return "invalid_context";

        case VITTE_SCOPE_ERROR_INVALID_STATE:
            return "invalid_state";

        case VITTE_SCOPE_ERROR_INVALID_SCOPE:
            return "invalid_scope";

        case VITTE_SCOPE_ERROR_INVALID_SYMBOL:
            return "invalid_symbol";

        case VITTE_SCOPE_ERROR_INVALID_NAME:
            return "invalid_name";

        case VITTE_SCOPE_ERROR_DUPLICATE_SYMBOL:
            return "duplicate_symbol";

        case VITTE_SCOPE_ERROR_SCOPE_LIMIT:
            return "scope_limit";

        case VITTE_SCOPE_ERROR_SYMBOL_LIMIT:
            return "symbol_limit";

        case VITTE_SCOPE_ERROR_NAME_LIMIT:
            return "name_limit";

        case VITTE_SCOPE_ERROR_DEPTH_LIMIT:
            return "depth_limit";

        case VITTE_SCOPE_ERROR_OVERFLOW:
            return "overflow";

        case VITTE_SCOPE_ERROR_OUT_OF_MEMORY:
            return "out_of_memory";

        case VITTE_SCOPE_ERROR_VALIDATION:
            return "validation";

        case VITTE_SCOPE_ERROR_CORRUPTION:
            return "corruption";

        case VITTE_SCOPE_ERROR_INTERNAL:
            return "internal";

        case VITTE_SCOPE_ERROR_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_scope_state_name(
    vitte_scope_state_t state)
{
    switch (state) {
        case VITTE_SCOPE_STATE_INVALID:
            return "invalid";

        case VITTE_SCOPE_STATE_BUILDING:
            return "building";

        case VITTE_SCOPE_STATE_SEALED:
            return "sealed";

        case VITTE_SCOPE_STATE_FAILED:
            return "failed";

        case VITTE_SCOPE_STATE_DESTROYED:
            return "destroyed";

        case VITTE_SCOPE_STATE_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_scope_kind_name(
    vitte_scope_kind_t kind)
{
    switch (kind) {
        case VITTE_SCOPE_KIND_INVALID:
            return "invalid";

        case VITTE_SCOPE_KIND_ROOT:
            return "root";

        case VITTE_SCOPE_KIND_SPACE:
            return "space";

        case VITTE_SCOPE_KIND_FORM:
            return "form";

        case VITTE_SCOPE_KIND_PICK:
            return "pick";

        case VITTE_SCOPE_KIND_TRAIT:
            return "trait";

        case VITTE_SCOPE_KIND_IMPL:
            return "impl";

        case VITTE_SCOPE_KIND_PROC:
            return "proc";

        case VITTE_SCOPE_KIND_BLOCK:
            return "block";

        case VITTE_SCOPE_KIND_LOOP:
            return "loop";

        case VITTE_SCOPE_KIND_MATCH:
            return "match";

        case VITTE_SCOPE_KIND_MATCH_ARM:
            return "match_arm";

        case VITTE_SCOPE_KIND_MACRO:
            return "macro";

        case VITTE_SCOPE_KIND_TEST:
            return "test";

        case VITTE_SCOPE_KIND_COMPTIME:
            return "comptime";

        case VITTE_SCOPE_KIND_UNSAFE:
            return "unsafe";

        case VITTE_SCOPE_KIND_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_symbol_kind_name(
    vitte_symbol_kind_t kind)
{
    switch (kind) {
        case VITTE_SYMBOL_KIND_INVALID:
            return "invalid";

        case VITTE_SYMBOL_KIND_SPACE:
            return "space";

        case VITTE_SYMBOL_KIND_CONST:
            return "const";

        case VITTE_SYMBOL_KIND_STATIC:
            return "static";

        case VITTE_SYMBOL_KIND_TYPE:
            return "type";

        case VITTE_SYMBOL_KIND_OPAQUE:
            return "opaque";

        case VITTE_SYMBOL_KIND_FORM:
            return "form";

        case VITTE_SYMBOL_KIND_FIELD:
            return "field";

        case VITTE_SYMBOL_KIND_PICK:
            return "pick";

        case VITTE_SYMBOL_KIND_VARIANT:
            return "variant";

        case VITTE_SYMBOL_KIND_TRAIT:
            return "trait";

        case VITTE_SYMBOL_KIND_IMPL:
            return "impl";

        case VITTE_SYMBOL_KIND_PROC:
            return "proc";

        case VITTE_SYMBOL_KIND_PARAMETER:
            return "parameter";

        case VITTE_SYMBOL_KIND_GENERIC_PARAMETER:
            return "generic_parameter";

        case VITTE_SYMBOL_KIND_LOCAL:
            return "local";

        case VITTE_SYMBOL_KIND_MACRO:
            return "macro";

        case VITTE_SYMBOL_KIND_TEST:
            return "test";

        case VITTE_SYMBOL_KIND_IMPORT:
            return "import";

        case VITTE_SYMBOL_KIND_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_symbol_visibility_name(
    vitte_symbol_visibility_t visibility)
{
    switch (visibility) {
        case VITTE_SYMBOL_VISIBILITY_PRIVATE:
            return "private";

        case VITTE_SYMBOL_VISIBILITY_PUBLIC:
            return "public";

        case VITTE_SYMBOL_VISIBILITY_COUNT:
            return "count";
    }

    return "unknown";
}

/* ========================================================================= */
/* Initialization                                                            */
/* ========================================================================= */

bool
vitte_scope_init(
    vitte_scope_t *context)
{
    vitte_scope_id_t root;

    if (context == NULL) {
        return false;
    }

    memset(context, 0, sizeof(*context));

    context->magic =
        VITTE_SCOPE_MAGIC;

    context->state =
        VITTE_SCOPE_STATE_BUILDING;

    context->last_error =
        VITTE_SCOPE_ERROR_NONE;

    context->root_scope =
        VITTE_SCOPE_INVALID_SCOPE_ID;

    context->max_scopes =
        VITTE_SCOPE_DEFAULT_MAX_SCOPES;

    context->max_symbols =
        VITTE_SCOPE_DEFAULT_MAX_SYMBOLS;

    context->max_name_bytes =
        VITTE_SCOPE_DEFAULT_MAX_NAME_BYTES;

    context->max_depth =
        VITTE_SCOPE_DEFAULT_MAX_DEPTH;

    context->generation =
        UINT64_C(1);

    root =
        vitte_scope_create(
            context,
            VITTE_SCOPE_INVALID_SCOPE_ID,
            VITTE_SCOPE_KIND_ROOT,
            vitte_scope_span_invalid(),
            NULL);

    if (root ==
        VITTE_SCOPE_INVALID_SCOPE_ID) {
        return false;
    }

    context->root_scope = root;

    return true;
}

/* ========================================================================= */
/* Reset                                                                     */
/* ========================================================================= */

bool
vitte_scope_reset(
    vitte_scope_t *context)
{
    size_t index;
    uint64_t generation;
    size_t max_scopes;
    size_t max_symbols;
    size_t max_name_bytes;
    size_t max_depth;
    vitte_scope_id_t root;

    if (!vitte_scope_context_valid_private(context)) {
        return false;
    }

    generation =
        context->generation;

    max_scopes =
        context->max_scopes;

    max_symbols =
        context->max_symbols;

    max_name_bytes =
        context->max_name_bytes;

    max_depth =
        context->max_depth;

    for (index = 0u;
         index < context->symbol_count;
         ++index) {
        free(context->symbols[index].name);
        context->symbols[index].name = NULL;
    }

    for (index = 0u;
         index < context->scope_count;
         ++index) {
        free(context->scopes[index].children);
        free(context->scopes[index].symbols);
        free(context->scopes[index].hash_slots);

        context->scopes[index].children = NULL;
        context->scopes[index].symbols = NULL;
        context->scopes[index].hash_slots = NULL;
    }

    if (context->scopes != NULL &&
        context->scope_capacity != 0u) {
        memset(
            context->scopes,
            0,
            context->scope_capacity *
                sizeof(*context->scopes));
    }

    if (context->symbols != NULL &&
        context->symbol_capacity != 0u) {
        memset(
            context->symbols,
            0,
            context->symbol_capacity *
                sizeof(*context->symbols));
    }

    context->scope_count = 0u;
    context->symbol_count = 0u;

    context->root_scope =
        VITTE_SCOPE_INVALID_SCOPE_ID;

    context->state =
        VITTE_SCOPE_STATE_BUILDING;

    context->last_error =
        VITTE_SCOPE_ERROR_NONE;

    context->max_scopes =
        max_scopes;

    context->max_symbols =
        max_symbols;

    context->max_name_bytes =
        max_name_bytes;

    context->max_depth =
        max_depth;

    memset(
        &context->stats,
        0,
        sizeof(context->stats));

    context->generation =
        generation == UINT64_MAX
            ? UINT64_MAX
            : generation + UINT64_C(1);

    root =
        vitte_scope_create(
            context,
            VITTE_SCOPE_INVALID_SCOPE_ID,
            VITTE_SCOPE_KIND_ROOT,
            vitte_scope_span_invalid(),
            NULL);

    if (root ==
        VITTE_SCOPE_INVALID_SCOPE_ID) {
        return false;
    }

    context->root_scope = root;

    return true;
}

/* ========================================================================= */
/* Destroy                                                                   */
/* ========================================================================= */

void
vitte_scope_destroy(
    vitte_scope_t *context)
{
    size_t index;

    if (context == NULL) {
        return;
    }

    if (context->magic ==
        VITTE_SCOPE_MAGIC) {
        for (index = 0u;
             index < context->symbol_count;
             ++index) {
            free(context->symbols[index].name);
        }

        for (index = 0u;
             index < context->scope_count;
             ++index) {
            free(context->scopes[index].children);
            free(context->scopes[index].symbols);
            free(context->scopes[index].hash_slots);
        }

        free(context->scopes);
        free(context->symbols);
    }

    memset(context, 0, sizeof(*context));

    context->magic =
        VITTE_SCOPE_DEAD_MAGIC;

    context->state =
        VITTE_SCOPE_STATE_DESTROYED;
}

/* ========================================================================= */
/* Limits                                                                    */
/* ========================================================================= */

bool
vitte_scope_set_limits(
    vitte_scope_t *context,
    size_t max_scopes,
    size_t max_symbols,
    size_t max_name_bytes,
    size_t max_depth)
{
    if (!vitte_scope_context_valid_private(context)) {
        return false;
    }

    if (context->state !=
        VITTE_SCOPE_STATE_BUILDING) {
        return vitte_scope_fail(
            context,
            VITTE_SCOPE_ERROR_INVALID_STATE);
    }

    if (max_scopes == 0u ||
        max_symbols == 0u ||
        max_name_bytes == 0u ||
        max_depth == 0u ||
        max_scopes <
            context->scope_count ||
        max_symbols <
            context->symbol_count) {
        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_INVALID_ARGUMENT);

        return false;
    }

    context->max_scopes =
        max_scopes;

    context->max_symbols =
        max_symbols;

    context->max_name_bytes =
        max_name_bytes;

    context->max_depth =
        max_depth;

    context->last_error =
        VITTE_SCOPE_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Scope creation                                                            */
/* ========================================================================= */

vitte_scope_id_t
vitte_scope_create(
    vitte_scope_t *context,
    vitte_scope_id_t parent_id,
    vitte_scope_kind_t kind,
    vitte_scope_span_t span,
    void *payload)
{
    size_t required;
    size_t index;
    vitte_scope_entry_t *entry;
    vitte_scope_entry_t *parent;
    size_t depth;

    if (!vitte_scope_context_valid_private(context)) {
        return VITTE_SCOPE_INVALID_SCOPE_ID;
    }

    if (context->state !=
        VITTE_SCOPE_STATE_BUILDING) {
        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_INVALID_STATE);

        return VITTE_SCOPE_INVALID_SCOPE_ID;
    }

    if (kind <= VITTE_SCOPE_KIND_INVALID ||
        kind >= VITTE_SCOPE_KIND_COUNT) {
        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_INVALID_ARGUMENT);

        return VITTE_SCOPE_INVALID_SCOPE_ID;
    }

    if (context->scope_count >=
        context->max_scopes) {
        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_SCOPE_LIMIT);

        return VITTE_SCOPE_INVALID_SCOPE_ID;
    }

    parent = NULL;
    depth = 0u;

    if (parent_id !=
        VITTE_SCOPE_INVALID_SCOPE_ID) {
        parent =
            vitte_scope_get_scope_mut_private(
                context,
                parent_id);

        if (parent == NULL) {
            vitte_scope_set_error(
                context,
                VITTE_SCOPE_ERROR_INVALID_SCOPE);

            return VITTE_SCOPE_INVALID_SCOPE_ID;
        }

        if (parent->depth == SIZE_MAX) {
            vitte_scope_set_error(
                context,
                VITTE_SCOPE_ERROR_OVERFLOW);

            return VITTE_SCOPE_INVALID_SCOPE_ID;
        }

        depth = parent->depth + 1u;

        if (depth >
            context->max_depth) {
            vitte_scope_set_error(
                context,
                VITTE_SCOPE_ERROR_DEPTH_LIMIT);

            return VITTE_SCOPE_INVALID_SCOPE_ID;
        }
    } else if (context->scope_count != 0u) {
        /*
         * Exactly one parentless root scope is allowed.
         */
        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_INVALID_SCOPE);

        return VITTE_SCOPE_INVALID_SCOPE_ID;
    }

    if (context->scope_count ==
        (size_t)UINT64_MAX) {
        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_OVERFLOW);

        return VITTE_SCOPE_INVALID_SCOPE_ID;
    }

    if (!vitte_scope_size_add(
            context->scope_count,
            1u,
            &required)) {
        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_OVERFLOW);

        return VITTE_SCOPE_INVALID_SCOPE_ID;
    }

    /*
     * Reserve the parent child array before publishing the new scope. This
     * keeps creation transactional with respect to allocation failures.
     */
    if (parent != NULL) {
        size_t child_required;

        if (!vitte_scope_size_add(
                parent->child_count,
                1u,
                &child_required)) {
            vitte_scope_set_error(
                context,
                VITTE_SCOPE_ERROR_OVERFLOW);

            return VITTE_SCOPE_INVALID_SCOPE_ID;
        }

        if (!vitte_scope_reserve_children(
                context,
                parent,
                child_required)) {
            return VITTE_SCOPE_INVALID_SCOPE_ID;
        }
    }

    if (!vitte_scope_reserve_scopes(
            context,
            required)) {
        return VITTE_SCOPE_INVALID_SCOPE_ID;
    }

    /*
     * scopes may have moved during reserve, reacquire parent.
     */
    if (parent_id !=
        VITTE_SCOPE_INVALID_SCOPE_ID) {
        parent =
            vitte_scope_get_scope_mut_private(
                context,
                parent_id);

        if (parent == NULL) {
            vitte_scope_set_error(
                context,
                VITTE_SCOPE_ERROR_CORRUPTION);

            return VITTE_SCOPE_INVALID_SCOPE_ID;
        }
    }

    index = context->scope_count;
    entry = &context->scopes[index];

    memset(entry, 0, sizeof(*entry));

    entry->id =
        (vitte_scope_id_t)(index + 1u);

    entry->parent_id =
        parent_id;

    entry->kind =
        kind;

    entry->depth =
        depth;

    entry->span =
        span;

    entry->payload =
        payload;

    entry->sealed = false;

    ++context->scope_count;

    if (parent != NULL) {
        parent->children[parent->child_count] =
            entry->id;

        ++parent->child_count;
    }

    context->stats.scopes_created =
        vitte_scope_u64_add_sat(
            context->stats.scopes_created,
            UINT64_C(1));

    if ((uint64_t)depth >
        context->stats.max_depth) {
        context->stats.max_depth =
            (uint64_t)depth;
    }

    context->last_error =
        VITTE_SCOPE_ERROR_NONE;

    return entry->id;
}

/* ========================================================================= */
/* Scope sealing                                                             */
/* ========================================================================= */

bool
vitte_scope_seal_scope(
    vitte_scope_t *context,
    vitte_scope_id_t scope_id)
{
    vitte_scope_entry_t *scope;

    if (!vitte_scope_context_valid_private(context)) {
        return false;
    }

    if (context->state !=
        VITTE_SCOPE_STATE_BUILDING) {
        return vitte_scope_fail(
            context,
            VITTE_SCOPE_ERROR_INVALID_STATE);
    }

    scope =
        vitte_scope_get_scope_mut_private(
            context,
            scope_id);

    if (scope == NULL) {
        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_INVALID_SCOPE);

        return false;
    }

    scope->sealed = true;

    context->last_error =
        VITTE_SCOPE_ERROR_NONE;

    return true;
}

bool
vitte_scope_seal(
    vitte_scope_t *context)
{
    size_t index;

    if (!vitte_scope_context_valid_private(context)) {
        return false;
    }

    if (context->state !=
        VITTE_SCOPE_STATE_BUILDING) {
        return vitte_scope_fail(
            context,
            VITTE_SCOPE_ERROR_INVALID_STATE);
    }

    if (!vitte_scope_validate(context)) {
        return false;
    }

    for (index = 0u;
         index < context->scope_count;
         ++index) {
        context->scopes[index].sealed = true;
    }

    context->state =
        VITTE_SCOPE_STATE_SEALED;

    context->last_error =
        VITTE_SCOPE_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Symbol declaration                                                        */
/* ========================================================================= */

vitte_symbol_id_t
vitte_scope_declare(
    vitte_scope_t *context,
    vitte_scope_id_t scope_id,
    const char *name,
    size_t name_length,
    vitte_symbol_kind_t kind,
    vitte_symbol_visibility_t visibility,
    uint32_t flags,
    vitte_scope_span_t span,
    void *payload)
{
    vitte_scope_entry_t *scope;
    uint64_t name_hash;
    vitte_symbol_id_t duplicate;
    vitte_symbol_id_t shadowed;
    size_t required;
    size_t local_required;
    char *name_copy;
    size_t index;
    vitte_symbol_t *symbol;

    if (!vitte_scope_context_valid_private(context)) {
        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    if (context->state !=
        VITTE_SCOPE_STATE_BUILDING) {
        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_INVALID_STATE);

        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    scope =
        vitte_scope_get_scope_mut_private(
            context,
            scope_id);

    if (scope == NULL) {
        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_INVALID_SCOPE);

        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    if (scope->sealed) {
        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_INVALID_STATE);

        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    if (!vitte_scope_name_valid_private(
            name,
            name_length)) {
        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_INVALID_NAME);

        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    if (name_length >
        context->max_name_bytes) {
        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_NAME_LIMIT);

        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    if (kind <= VITTE_SYMBOL_KIND_INVALID ||
        kind >= VITTE_SYMBOL_KIND_COUNT ||
        visibility >=
            VITTE_SYMBOL_VISIBILITY_COUNT) {
        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_INVALID_ARGUMENT);

        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    if (context->symbol_count >=
        context->max_symbols) {
        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_SYMBOL_LIMIT);

        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    name_hash =
        vitte_scope_name_hash(
            name,
            name_length);

    duplicate =
        vitte_scope_lookup_local_private(
            context,
            scope,
            name,
            name_length,
            name_hash);

    if (duplicate !=
        VITTE_SCOPE_INVALID_SYMBOL_ID) {
        context->stats.duplicate_declarations =
            vitte_scope_u64_add_sat(
                context->stats.duplicate_declarations,
                UINT64_C(1));

        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_DUPLICATE_SYMBOL);

        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    shadowed =
        vitte_scope_find_shadowed_private(
            context,
            scope_id,
            name,
            name_length,
            name_hash);

    if (!vitte_scope_size_add(
            context->symbol_count,
            1u,
            &required) ||
        !vitte_scope_size_add(
            scope->symbol_count,
            1u,
            &local_required)) {
        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_OVERFLOW);

        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    /*
     * Reserve all mutable storage before copying/publishing the symbol.
     */
    if (!vitte_scope_reserve_symbols(
            context,
            required)) {
        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    /*
     * Global symbol reserve can move context->symbols but not scopes.
     */
    scope =
        vitte_scope_get_scope_mut_private(
            context,
            scope_id);

    if (scope == NULL) {
        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_CORRUPTION);

        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    if (!vitte_scope_reserve_local_symbols(
            context,
            scope,
            local_required)) {
        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    if (!vitte_scope_ensure_hash_capacity(
            context,
            scope,
            local_required)) {
        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    name_copy =
        vitte_scope_copy_string(
            context,
            name,
            name_length);

    if (name_copy == NULL) {
        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    index = context->symbol_count;

    if (index == SIZE_MAX ||
        (uint64_t)index >= UINT64_MAX) {
        free(name_copy);

        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_OVERFLOW);

        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    symbol =
        &context->symbols[index];

    memset(symbol, 0, sizeof(*symbol));

    symbol->id =
        (vitte_symbol_id_t)(index + 1u);

    symbol->scope_id =
        scope_id;

    symbol->name =
        name_copy;

    symbol->name_length =
        name_length;

    symbol->name_hash =
        name_hash;

    symbol->kind =
        kind;

    symbol->visibility =
        visibility;

    symbol->flags =
        flags;

    symbol->span =
        span;

    symbol->payload =
        payload;

    symbol->shadowed_symbol =
        shadowed;

    ++context->symbol_count;

    scope->symbols[scope->symbol_count] =
        symbol->id;

    ++scope->symbol_count;

    if (!vitte_scope_hash_insert_into(
            context,
            scope->hash_slots,
            scope->hash_capacity,
            symbol->id)) {
        /*
         * Roll back the publication completely.
         */
        --scope->symbol_count;
        --context->symbol_count;

        free(symbol->name);
        memset(symbol, 0, sizeof(*symbol));

        /*
         * The hash table did not accept the just-published symbol, so no
         * table entry exists to roll back.
         */
        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_CORRUPTION);

        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    context->stats.symbols_declared =
        vitte_scope_u64_add_sat(
            context->stats.symbols_declared,
            UINT64_C(1));

    if (shadowed !=
        VITTE_SCOPE_INVALID_SYMBOL_ID) {
        context->stats.shadowing_declarations =
            vitte_scope_u64_add_sat(
                context->stats.shadowing_declarations,
                UINT64_C(1));
    }

    context->last_error =
        VITTE_SCOPE_ERROR_NONE;

    return symbol->id;
}

/* ========================================================================= */
/* Lookup                                                                    */
/* ========================================================================= */

vitte_symbol_id_t
vitte_scope_lookup_local_n(
    vitte_scope_t *context,
    vitte_scope_id_t scope_id,
    const char *name,
    size_t name_length)
{
    const vitte_scope_entry_t *scope;
    uint64_t name_hash;
    vitte_symbol_id_t result;

    if (!vitte_scope_context_valid_private(context)) {
        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    if (!vitte_scope_name_valid_private(
            name,
            name_length)) {
        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_INVALID_NAME);

        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    scope =
        vitte_scope_get_scope_private(
            context,
            scope_id);

    if (scope == NULL) {
        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_INVALID_SCOPE);

        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    name_hash =
        vitte_scope_name_hash(
            name,
            name_length);

    result =
        vitte_scope_lookup_local_private(
            context,
            scope,
            name,
            name_length,
            name_hash);

    context->stats.local_lookups =
        vitte_scope_u64_add_sat(
            context->stats.local_lookups,
            UINT64_C(1));

    if (result ==
        VITTE_SCOPE_INVALID_SYMBOL_ID) {
        context->stats.lookup_misses =
            vitte_scope_u64_add_sat(
                context->stats.lookup_misses,
                UINT64_C(1));
    } else {
        context->stats.lookup_hits =
            vitte_scope_u64_add_sat(
                context->stats.lookup_hits,
                UINT64_C(1));
    }

    context->last_error =
        VITTE_SCOPE_ERROR_NONE;

    return result;
}

vitte_symbol_id_t
vitte_scope_lookup_local(
    vitte_scope_t *context,
    vitte_scope_id_t scope_id,
    const char *name)
{
    if (name == NULL) {
        if (vitte_scope_context_valid_private(context)) {
            vitte_scope_set_error(
                context,
                VITTE_SCOPE_ERROR_INVALID_NAME);
        }

        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    return vitte_scope_lookup_local_n(
        context,
        scope_id,
        name,
        strlen(name));
}

vitte_symbol_id_t
vitte_scope_lookup_n(
    vitte_scope_t *context,
    vitte_scope_id_t start_scope,
    const char *name,
    size_t name_length)
{
    vitte_scope_id_t current;
    uint64_t name_hash;
    size_t guard;

    if (!vitte_scope_context_valid_private(context)) {
        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    if (!vitte_scope_name_valid_private(
            name,
            name_length)) {
        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_INVALID_NAME);

        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    if (vitte_scope_get_scope_private(
            context,
            start_scope) == NULL) {
        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_INVALID_SCOPE);

        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    name_hash =
        vitte_scope_name_hash(
            name,
            name_length);

    current = start_scope;
    guard = 0u;

    context->stats.recursive_lookups =
        vitte_scope_u64_add_sat(
            context->stats.recursive_lookups,
            UINT64_C(1));

    while (current !=
           VITTE_SCOPE_INVALID_SCOPE_ID) {
        const vitte_scope_entry_t *scope;
        vitte_symbol_id_t result;

        if (guard >=
            context->scope_count) {
            vitte_scope_set_error(
                context,
                VITTE_SCOPE_ERROR_CORRUPTION);

            return VITTE_SCOPE_INVALID_SYMBOL_ID;
        }

        ++guard;

        scope =
            vitte_scope_get_scope_private(
                context,
                current);

        if (scope == NULL) {
            vitte_scope_set_error(
                context,
                VITTE_SCOPE_ERROR_CORRUPTION);

            return VITTE_SCOPE_INVALID_SYMBOL_ID;
        }

        result =
            vitte_scope_lookup_local_private(
                context,
                scope,
                name,
                name_length,
                name_hash);

        if (result !=
            VITTE_SCOPE_INVALID_SYMBOL_ID) {
            context->stats.lookup_hits =
                vitte_scope_u64_add_sat(
                    context->stats.lookup_hits,
                    UINT64_C(1));

            context->last_error =
                VITTE_SCOPE_ERROR_NONE;

            return result;
        }

        current =
            scope->parent_id;
    }

    context->stats.lookup_misses =
        vitte_scope_u64_add_sat(
            context->stats.lookup_misses,
            UINT64_C(1));

    context->last_error =
        VITTE_SCOPE_ERROR_NONE;

    return VITTE_SCOPE_INVALID_SYMBOL_ID;
}

vitte_symbol_id_t
vitte_scope_lookup(
    vitte_scope_t *context,
    vitte_scope_id_t start_scope,
    const char *name)
{
    if (name == NULL) {
        if (vitte_scope_context_valid_private(context)) {
            vitte_scope_set_error(
                context,
                VITTE_SCOPE_ERROR_INVALID_NAME);
        }

        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    return vitte_scope_lookup_n(
        context,
        start_scope,
        name,
        strlen(name));
}

/* ========================================================================= */
/* Visibility-aware lookup                                                   */
/* ========================================================================= */

vitte_symbol_id_t
vitte_scope_lookup_visible_n(
    vitte_scope_t *context,
    vitte_scope_id_t start_scope,
    vitte_scope_id_t requester_scope,
    const char *name,
    size_t name_length)
{
    vitte_symbol_id_t symbol_id;
    const vitte_symbol_t *symbol;

    symbol_id =
        vitte_scope_lookup_n(
            context,
            start_scope,
            name,
            name_length);

    if (symbol_id ==
        VITTE_SCOPE_INVALID_SYMBOL_ID) {
        return symbol_id;
    }

    symbol =
        vitte_scope_get_symbol_private(
            context,
            symbol_id);

    if (symbol == NULL) {
        vitte_scope_set_error(
            context,
            VITTE_SCOPE_ERROR_CORRUPTION);

        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    if (symbol->visibility ==
        VITTE_SYMBOL_VISIBILITY_PUBLIC) {
        return symbol_id;
    }

    /*
     * Private declarations are visible from their declaration scope and
     * lexical descendants.
     */
    if (vitte_scope_is_ancestor_private(
            context,
            symbol->scope_id,
            requester_scope)) {
        return symbol_id;
    }

    context->stats.visibility_rejections =
        vitte_scope_u64_add_sat(
            context->stats.visibility_rejections,
            UINT64_C(1));

    return VITTE_SCOPE_INVALID_SYMBOL_ID;
}

vitte_symbol_id_t
vitte_scope_lookup_visible(
    vitte_scope_t *context,
    vitte_scope_id_t start_scope,
    vitte_scope_id_t requester_scope,
    const char *name)
{
    if (name == NULL) {
        if (vitte_scope_context_valid_private(context)) {
            vitte_scope_set_error(
                context,
                VITTE_SCOPE_ERROR_INVALID_NAME);
        }

        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    return vitte_scope_lookup_visible_n(
        context,
        start_scope,
        requester_scope,
        name,
        strlen(name));
}

/* ========================================================================= */
/* Public object access                                                      */
/* ========================================================================= */

const vitte_scope_entry_t *
vitte_scope_get_scope(
    const vitte_scope_t *context,
    vitte_scope_id_t id)
{
    if (!vitte_scope_context_valid_private(context)) {
        return NULL;
    }

    return vitte_scope_get_scope_private(
        context,
        id);
}

vitte_scope_entry_t *
vitte_scope_get_scope_mut(
    vitte_scope_t *context,
    vitte_scope_id_t id)
{
    if (!vitte_scope_context_valid_private(context)) {
        return NULL;
    }

    if (context->state !=
        VITTE_SCOPE_STATE_BUILDING) {
        return NULL;
    }

    return vitte_scope_get_scope_mut_private(
        context,
        id);
}

const vitte_symbol_t *
vitte_scope_get_symbol(
    const vitte_scope_t *context,
    vitte_symbol_id_t id)
{
    if (!vitte_scope_context_valid_private(context)) {
        return NULL;
    }

    return vitte_scope_get_symbol_private(
        context,
        id);
}

vitte_symbol_t *
vitte_scope_get_symbol_mut(
    vitte_scope_t *context,
    vitte_symbol_id_t id)
{
    if (!vitte_scope_context_valid_private(context)) {
        return NULL;
    }

    if (context->state !=
        VITTE_SCOPE_STATE_BUILDING) {
        return NULL;
    }

    return vitte_scope_get_symbol_mut_private(
        context,
        id);
}

/* ========================================================================= */
/* Hierarchy queries                                                         */
/* ========================================================================= */

bool
vitte_scope_is_ancestor(
    const vitte_scope_t *context,
    vitte_scope_id_t ancestor,
    vitte_scope_id_t descendant)
{
    if (!vitte_scope_context_valid_private(context)) {
        return false;
    }

    return vitte_scope_is_ancestor_private(
        context,
        ancestor,
        descendant);
}

vitte_scope_id_t
vitte_scope_common_ancestor(
    const vitte_scope_t *context,
    vitte_scope_id_t left,
    vitte_scope_id_t right)
{
    const vitte_scope_entry_t *left_scope;
    const vitte_scope_entry_t *right_scope;
    vitte_scope_id_t left_id;
    vitte_scope_id_t right_id;
    size_t guard;

    if (!vitte_scope_context_valid_private(context)) {
        return VITTE_SCOPE_INVALID_SCOPE_ID;
    }

    left_scope =
        vitte_scope_get_scope_private(
            context,
            left);

    right_scope =
        vitte_scope_get_scope_private(
            context,
            right);

    if (left_scope == NULL ||
        right_scope == NULL) {
        return VITTE_SCOPE_INVALID_SCOPE_ID;
    }

    left_id = left;
    right_id = right;
    guard = 0u;

    while (left_scope->depth >
           right_scope->depth) {
        if (guard++ >=
            context->scope_count) {
            return VITTE_SCOPE_INVALID_SCOPE_ID;
        }

        left_id =
            left_scope->parent_id;

        left_scope =
            vitte_scope_get_scope_private(
                context,
                left_id);

        if (left_scope == NULL) {
            return VITTE_SCOPE_INVALID_SCOPE_ID;
        }
    }

    while (right_scope->depth >
           left_scope->depth) {
        if (guard++ >=
            context->scope_count) {
            return VITTE_SCOPE_INVALID_SCOPE_ID;
        }

        right_id =
            right_scope->parent_id;

        right_scope =
            vitte_scope_get_scope_private(
                context,
                right_id);

        if (right_scope == NULL) {
            return VITTE_SCOPE_INVALID_SCOPE_ID;
        }
    }

    while (left_id != right_id) {
        if (guard++ >=
            context->scope_count) {
            return VITTE_SCOPE_INVALID_SCOPE_ID;
        }

        left_id =
            left_scope->parent_id;

        right_id =
            right_scope->parent_id;

        if (left_id ==
                VITTE_SCOPE_INVALID_SCOPE_ID ||
            right_id ==
                VITTE_SCOPE_INVALID_SCOPE_ID) {
            return VITTE_SCOPE_INVALID_SCOPE_ID;
        }

        left_scope =
            vitte_scope_get_scope_private(
                context,
                left_id);

        right_scope =
            vitte_scope_get_scope_private(
                context,
                right_id);

        if (left_scope == NULL ||
            right_scope == NULL) {
            return VITTE_SCOPE_INVALID_SCOPE_ID;
        }
    }

    return left_id;
}

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

static bool
vitte_scope_validate_scope_private(
    vitte_scope_t *context,
    const vitte_scope_entry_t *scope,
    size_t expected_index)
{
    size_t index;

    if (context == NULL ||
        scope == NULL) {
        return false;
    }

    if (scope->id !=
        (vitte_scope_id_t)(expected_index + 1u)) {
        return false;
    }

    if (scope->kind <=
            VITTE_SCOPE_KIND_INVALID ||
        scope->kind >=
            VITTE_SCOPE_KIND_COUNT) {
        return false;
    }

    if (scope->child_count >
            scope->child_capacity ||
        scope->symbol_count >
            scope->symbol_capacity) {
        return false;
    }

    if (scope->child_count != 0u &&
        scope->children == NULL) {
        return false;
    }

    if (scope->symbol_count != 0u &&
        scope->symbols == NULL) {
        return false;
    }

    if (!vitte_scope_hash_capacity_valid(
            scope->hash_capacity)) {
        return false;
    }

    if ((scope->hash_capacity == 0u) !=
        (scope->hash_slots == NULL)) {
        return false;
    }

    if (scope->parent_id ==
        VITTE_SCOPE_INVALID_SCOPE_ID) {
        if (scope->id !=
                context->root_scope ||
            scope->kind !=
                VITTE_SCOPE_KIND_ROOT ||
            scope->depth != 0u) {
            return false;
        }
    } else {
        const vitte_scope_entry_t *parent;
        bool found;
        size_t parent_index;

        parent =
            vitte_scope_get_scope_private(
                context,
                scope->parent_id);

        if (parent == NULL ||
            parent->depth == SIZE_MAX ||
            scope->depth !=
                parent->depth + 1u) {
            return false;
        }

        found = false;

        for (parent_index = 0u;
             parent_index <
                 parent->child_count;
             ++parent_index) {
            if (parent->children[parent_index] ==
                scope->id) {
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    if (scope->depth >
        context->max_depth) {
        return false;
    }

    for (index = 0u;
         index < scope->child_count;
         ++index) {
        const vitte_scope_entry_t *child;
        size_t duplicate_index;

        child =
            vitte_scope_get_scope_private(
                context,
                scope->children[index]);

        if (child == NULL ||
            child->parent_id !=
                scope->id) {
            return false;
        }

        for (duplicate_index = index + 1u;
             duplicate_index <
                 scope->child_count;
             ++duplicate_index) {
            if (scope->children[index] ==
                scope->children[duplicate_index]) {
                return false;
            }
        }
    }

    for (index = 0u;
         index < scope->symbol_count;
         ++index) {
        const vitte_symbol_t *symbol;
        vitte_symbol_id_t lookup;
        size_t duplicate_index;

        symbol =
            vitte_scope_get_symbol_private(
                context,
                scope->symbols[index]);

        if (symbol == NULL ||
            symbol->scope_id !=
                scope->id) {
            return false;
        }

        lookup =
            vitte_scope_lookup_local_private(
                context,
                scope,
                symbol->name,
                symbol->name_length,
                symbol->name_hash);

        if (lookup != symbol->id) {
            return false;
        }

        for (duplicate_index = index + 1u;
             duplicate_index <
                 scope->symbol_count;
             ++duplicate_index) {
            if (scope->symbols[index] ==
                scope->symbols[duplicate_index]) {
                return false;
            }
        }
    }

    return true;
}

static bool
vitte_scope_validate_symbol_private(
    const vitte_scope_t *context,
    const vitte_symbol_t *symbol,
    size_t expected_index)
{
    const vitte_scope_entry_t *scope;
    uint64_t expected_hash;
    bool found;
    size_t index;

    if (context == NULL ||
        symbol == NULL) {
        return false;
    }

    if (symbol->id !=
        (vitte_symbol_id_t)(expected_index + 1u)) {
        return false;
    }

    if (symbol->scope_id ==
        VITTE_SCOPE_INVALID_SCOPE_ID) {
        return false;
    }

    if (!vitte_scope_name_valid_private(
            symbol->name,
            symbol->name_length)) {
        return false;
    }

    if (symbol->name_length >
        context->max_name_bytes) {
        return false;
    }

    expected_hash =
        vitte_scope_name_hash(
            symbol->name,
            symbol->name_length);

    if (expected_hash !=
        symbol->name_hash) {
        return false;
    }

    if (symbol->kind <=
            VITTE_SYMBOL_KIND_INVALID ||
        symbol->kind >=
            VITTE_SYMBOL_KIND_COUNT) {
        return false;
    }

    if (symbol->visibility >=
        VITTE_SYMBOL_VISIBILITY_COUNT) {
        return false;
    }

    scope =
        vitte_scope_get_scope_private(
            context,
            symbol->scope_id);

    if (scope == NULL) {
        return false;
    }

    found = false;

    for (index = 0u;
         index < scope->symbol_count;
         ++index) {
        if (scope->symbols[index] ==
            symbol->id) {
            found = true;
            break;
        }
    }

    if (!found) {
        return false;
    }

    if (symbol->shadowed_symbol !=
        VITTE_SCOPE_INVALID_SYMBOL_ID) {
        const vitte_symbol_t *shadowed;

        shadowed =
            vitte_scope_get_symbol_private(
                context,
                symbol->shadowed_symbol);

        if (shadowed == NULL ||
            shadowed->id == symbol->id ||
            shadowed->name_length !=
                symbol->name_length ||
            shadowed->name_hash !=
                symbol->name_hash ||
            memcmp(
                shadowed->name,
                symbol->name,
                symbol->name_length) != 0 ||
            !vitte_scope_is_ancestor_private(
                context,
                shadowed->scope_id,
                symbol->scope_id) ||
            shadowed->scope_id ==
                symbol->scope_id) {
            return false;
        }
    }

    return true;
}

bool
vitte_scope_validate(
    vitte_scope_t *context)
{
    size_t index;
    size_t root_count;

    if (!vitte_scope_context_valid_private(context)) {
        return false;
    }

    context->stats.validation_runs =
        vitte_scope_u64_add_sat(
            context->stats.validation_runs,
            UINT64_C(1));

    if (context->scope_count == 0u ||
        context->root_scope ==
            VITTE_SCOPE_INVALID_SCOPE_ID) {
        goto validation_failure;
    }

    root_count = 0u;

    for (index = 0u;
         index < context->scope_count;
         ++index) {
        const vitte_scope_entry_t *scope;

        scope = &context->scopes[index];

        if (scope->parent_id ==
            VITTE_SCOPE_INVALID_SCOPE_ID) {
            ++root_count;
        }

        if (!vitte_scope_validate_scope_private(
                context,
                scope,
                index)) {
            goto validation_failure;
        }
    }

    if (root_count != 1u) {
        goto validation_failure;
    }

    for (index = 0u;
         index < context->symbol_count;
         ++index) {
        if (!vitte_scope_validate_symbol_private(
                context,
                &context->symbols[index],
                index)) {
            goto validation_failure;
        }
    }

    /*
     * Verify every scope reaches the unique root without a cycle.
     */
    for (index = 0u;
         index < context->scope_count;
         ++index) {
        const vitte_scope_entry_t *scope;
        vitte_scope_id_t current;
        size_t guard;

        scope = &context->scopes[index];
        current = scope->id;
        guard = 0u;

        while (current !=
               VITTE_SCOPE_INVALID_SCOPE_ID) {
            const vitte_scope_entry_t *current_scope;

            if (guard >=
                context->scope_count) {
                goto validation_failure;
            }

            ++guard;

            current_scope =
                vitte_scope_get_scope_private(
                    context,
                    current);

            if (current_scope == NULL) {
                goto validation_failure;
            }

            if (current_scope->parent_id ==
                VITTE_SCOPE_INVALID_SCOPE_ID) {
                if (current_scope->id !=
                    context->root_scope) {
                    goto validation_failure;
                }

                break;
            }

            current =
                current_scope->parent_id;
        }
    }

    context->last_error =
        VITTE_SCOPE_ERROR_NONE;

    return true;

validation_failure:

    context->stats.validation_failures =
        vitte_scope_u64_add_sat(
            context->stats.validation_failures,
            UINT64_C(1));

    context->last_error =
        VITTE_SCOPE_ERROR_VALIDATION;

    return false;
}

/* ========================================================================= */
/* Fingerprinting                                                            */
/* ========================================================================= */

uint64_t
vitte_scope_fingerprint(
    vitte_scope_t *context)
{
    uint64_t hash;
    size_t index;

    if (!vitte_scope_context_valid_private(context)) {
        return UINT64_C(0);
    }

    hash =
        VITTE_SCOPE_FNV_OFFSET;

    hash =
        vitte_scope_hash_u64(
            hash,
            (uint64_t)context->scope_count);

    hash =
        vitte_scope_hash_u64(
            hash,
            (uint64_t)context->symbol_count);

    hash =
        vitte_scope_hash_u64(
            hash,
            (uint64_t)context->root_scope);

    for (index = 0u;
         index < context->scope_count;
         ++index) {
        const vitte_scope_entry_t *scope;
        size_t child_index;
        size_t symbol_index;

        scope = &context->scopes[index];

        hash =
            vitte_scope_hash_u64(
                hash,
                (uint64_t)scope->id);

        hash =
            vitte_scope_hash_u64(
                hash,
                (uint64_t)scope->parent_id);

        hash =
            vitte_scope_hash_u64(
                hash,
                (uint64_t)scope->kind);

        hash =
            vitte_scope_hash_u64(
                hash,
                (uint64_t)scope->depth);

        hash =
            vitte_scope_hash_u64(
                hash,
                scope->sealed
                    ? UINT64_C(1)
                    : UINT64_C(0));

        hash =
            vitte_scope_hash_u64(
                hash,
                (uint64_t)scope->child_count);

        for (child_index = 0u;
             child_index <
                 scope->child_count;
             ++child_index) {
            hash =
                vitte_scope_hash_u64(
                    hash,
                    (uint64_t)
                        scope->children[
                            child_index]);
        }

        hash =
            vitte_scope_hash_u64(
                hash,
                (uint64_t)scope->symbol_count);

        for (symbol_index = 0u;
             symbol_index <
                 scope->symbol_count;
             ++symbol_index) {
            hash =
                vitte_scope_hash_u64(
                    hash,
                    (uint64_t)
                        scope->symbols[
                            symbol_index]);
        }
    }

    for (index = 0u;
         index < context->symbol_count;
         ++index) {
        const vitte_symbol_t *symbol;

        symbol = &context->symbols[index];

        hash =
            vitte_scope_hash_u64(
                hash,
                (uint64_t)symbol->id);

        hash =
            vitte_scope_hash_u64(
                hash,
                (uint64_t)symbol->scope_id);

        hash =
            vitte_scope_hash_u64(
                hash,
                (uint64_t)symbol->kind);

        hash =
            vitte_scope_hash_u64(
                hash,
                (uint64_t)symbol->visibility);

        hash =
            vitte_scope_hash_u64(
                hash,
                (uint64_t)symbol->flags);

        hash =
            vitte_scope_hash_u64(
                hash,
                (uint64_t)symbol->name_length);

        hash =
            vitte_scope_hash_bytes(
                hash,
                (const unsigned char *)
                    symbol->name,
                symbol->name_length);

        hash =
            vitte_scope_hash_u64(
                hash,
                (uint64_t)
                    symbol->shadowed_symbol);
    }

    context->stats.hash_runs =
        vitte_scope_u64_add_sat(
            context->stats.hash_runs,
            UINT64_C(1));

    return hash;
}

/* ========================================================================= */
/* Semantic fingerprint                                                      */
/* ========================================================================= */

uint64_t
vitte_scope_semantic_fingerprint(
    vitte_scope_t *context)
{
    uint64_t hash;
    size_t index;

    if (!vitte_scope_context_valid_private(context)) {
        return UINT64_C(0);
    }

    /*
     * Deliberately excludes source spans, payload pointers, capacities,
     * statistics and generation. This fingerprint represents the lexical
     * namespace structure rather than process/runtime identity.
     */
    hash =
        VITTE_SCOPE_FNV_OFFSET;

    for (index = 0u;
         index < context->scope_count;
         ++index) {
        const vitte_scope_entry_t *scope;

        scope = &context->scopes[index];

        hash =
            vitte_scope_hash_u64(
                hash,
                (uint64_t)scope->kind);

        hash =
            vitte_scope_hash_u64(
                hash,
                (uint64_t)scope->parent_id);

        hash =
            vitte_scope_hash_u64(
                hash,
                (uint64_t)scope->depth);
    }

    for (index = 0u;
         index < context->symbol_count;
         ++index) {
        const vitte_symbol_t *symbol;

        symbol = &context->symbols[index];

        hash =
            vitte_scope_hash_u64(
                hash,
                (uint64_t)symbol->scope_id);

        hash =
            vitte_scope_hash_u64(
                hash,
                (uint64_t)symbol->kind);

        hash =
            vitte_scope_hash_u64(
                hash,
                (uint64_t)symbol->visibility);

        hash =
            vitte_scope_hash_u64(
                hash,
                (uint64_t)symbol->flags);

        hash =
            vitte_scope_hash_bytes(
                hash,
                (const unsigned char *)
                    symbol->name,
                symbol->name_length);
    }

    context->stats.hash_runs =
        vitte_scope_u64_add_sat(
            context->stats.hash_runs,
            UINT64_C(1));

    return hash;
}

/* ========================================================================= */
/* Counts / root                                                             */
/* ========================================================================= */

vitte_scope_id_t
vitte_scope_root(
    const vitte_scope_t *context)
{
    if (!vitte_scope_context_valid_private(context)) {
        return VITTE_SCOPE_INVALID_SCOPE_ID;
    }

    return context->root_scope;
}

size_t
vitte_scope_count(
    const vitte_scope_t *context)
{
    if (!vitte_scope_context_valid_private(context)) {
        return 0u;
    }

    return context->scope_count;
}

size_t
vitte_scope_symbol_count(
    const vitte_scope_t *context)
{
    if (!vitte_scope_context_valid_private(context)) {
        return 0u;
    }

    return context->symbol_count;
}

/* ========================================================================= */
/* Symbol predicates                                                         */
/* ========================================================================= */

bool
vitte_scope_symbol_shadows(
    const vitte_scope_t *context,
    vitte_symbol_id_t symbol_id,
    vitte_symbol_id_t *shadowed_symbol)
{
    const vitte_symbol_t *symbol;

    if (!vitte_scope_context_valid_private(context)) {
        return false;
    }

    symbol =
        vitte_scope_get_symbol_private(
            context,
            symbol_id);

    if (symbol == NULL ||
        symbol->shadowed_symbol ==
            VITTE_SCOPE_INVALID_SYMBOL_ID) {
        return false;
    }

    if (shadowed_symbol != NULL) {
        *shadowed_symbol =
            symbol->shadowed_symbol;
    }

    return true;
}

bool
vitte_scope_symbol_is_visible_from(
    const vitte_scope_t *context,
    vitte_symbol_id_t symbol_id,
    vitte_scope_id_t requester_scope)
{
    const vitte_symbol_t *symbol;

    if (!vitte_scope_context_valid_private(context)) {
        return false;
    }

    symbol =
        vitte_scope_get_symbol_private(
            context,
            symbol_id);

    if (symbol == NULL ||
        vitte_scope_get_scope_private(
            context,
            requester_scope) == NULL) {
        return false;
    }

    if (symbol->visibility ==
        VITTE_SYMBOL_VISIBILITY_PUBLIC) {
        return true;
    }

    return vitte_scope_is_ancestor_private(
        context,
        symbol->scope_id,
        requester_scope);
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_scope_stats_t
vitte_scope_stats(
    const vitte_scope_t *context)
{
    vitte_scope_stats_t stats;

    memset(&stats, 0, sizeof(stats));

    if (!vitte_scope_context_valid_private(context)) {
        return stats;
    }

    return context->stats;
}

/* ========================================================================= */
/* Public context queries                                                    */
/* ========================================================================= */

bool
vitte_scope_is_valid(
    const vitte_scope_t *context)
{
    return vitte_scope_context_valid_private(
        context);
}

vitte_scope_error_t
vitte_scope_last_error(
    const vitte_scope_t *context)
{
    if (context == NULL ||
        context->magic !=
            VITTE_SCOPE_MAGIC) {
        return VITTE_SCOPE_ERROR_INVALID_CONTEXT;
    }

    return context->last_error;
}

uint64_t
vitte_scope_generation(
    const vitte_scope_t *context)
{
    if (!vitte_scope_context_valid_private(context)) {
        return UINT64_C(0);
    }

    return context->generation;
}

/* ========================================================================= */
/* Translation-unit anchor                                                   */
/* ========================================================================= */

void
vitte_scope_translation_unit_anchor(void)
{
    /*
     * Stable symbol for build-system and static-library linkage probes.
     */
}
