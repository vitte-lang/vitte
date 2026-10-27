#include "source_map.h"

#include "diagnostic.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

#ifndef VITTE_SOURCE_MAP_INITIAL_CAPACITY
#define VITTE_SOURCE_MAP_INITIAL_CAPACITY ((size_t)64u)
#endif

#ifndef VITTE_SOURCE_MAP_MAX_DEPTH
#define VITTE_SOURCE_MAP_MAX_DEPTH ((size_t)128u)
#endif

#ifndef VITTE_SOURCE_MAP_INDEX_NONE
#define VITTE_SOURCE_MAP_INDEX_NONE SIZE_MAX
#endif

/* ========================================================================= */
/* Internal helpers                                                          */
/* ========================================================================= */

static bool
vitte_source_map_source_id_valid(
    vitte_source_id_t source_id
)
{
#ifdef VITTE_SOURCE_ID_INVALID
    return source_id != VITTE_SOURCE_ID_INVALID;
#else
    /*
     * Replace this fallback with the canonical source-id validity helper
     * once source.h exposes one.
     */
    return source_id != (vitte_source_id_t)0;
#endif
}

static bool
vitte_source_map_text_present(
    const char *text
)
{
    return text != NULL &&
           text[0] != '\0';
}

static size_t
vitte_source_map_span_length(
    const vitte_ast_span_t *span
)
{
    if (span == NULL ||
        !vitte_ast_span_is_valid(*span)) {

        return SIZE_MAX;
    }

    if (span->end_offset <
        span->start_offset) {

        return SIZE_MAX;
    }

    return span->end_offset -
           span->start_offset;
}

static bool
vitte_source_map_same_source(
    const vitte_ast_span_t *left,
    const vitte_ast_span_t *right
)
{
    if (left == NULL ||
        right == NULL) {

        return false;
    }

    if (vitte_source_map_source_id_valid(
            left->source_id
        ) &&
        vitte_source_map_source_id_valid(
            right->source_id
        )) {

        return left->source_id ==
               right->source_id;
    }

    if (vitte_source_map_text_present(
            left->source_name
        ) &&
        vitte_source_map_text_present(
            right->source_name
        )) {

        return strcmp(
            left->source_name,
            right->source_name
        ) == 0;
    }

    return false;
}

static bool
vitte_source_map_span_contains_offset(
    const vitte_ast_span_t *span,
    size_t offset
)
{
    if (span == NULL ||
        !vitte_ast_span_is_valid(*span)) {

        return false;
    }

    /*
     * Half-open interval:
     *
     *   [start_offset, end_offset)
     *
     * Zero-width spans are treated as point mappings.
     */
    if (span->start_offset ==
        span->end_offset) {

        return offset ==
               span->start_offset;
    }

    return offset >= span->start_offset &&
           offset < span->end_offset;
}

static bool
vitte_source_map_span_contains_span(
    const vitte_ast_span_t *outer,
    const vitte_ast_span_t *inner
)
{
    if (outer == NULL ||
        inner == NULL ||
        !vitte_ast_span_is_valid(*outer) ||
        !vitte_ast_span_is_valid(*inner) ||
        !vitte_source_map_same_source(
            outer,
            inner
        )) {

        return false;
    }

    return inner->start_offset >=
               outer->start_offset &&
           inner->end_offset <=
               outer->end_offset;
}

static bool
vitte_source_map_span_overlaps(
    const vitte_ast_span_t *left,
    const vitte_ast_span_t *right
)
{
    if (left == NULL ||
        right == NULL ||
        !vitte_ast_span_is_valid(*left) ||
        !vitte_ast_span_is_valid(*right) ||
        !vitte_source_map_same_source(
            left,
            right
        )) {

        return false;
    }

    if (left->start_offset ==
            left->end_offset &&
        right->start_offset ==
            right->end_offset) {

        return left->start_offset ==
               right->start_offset;
    }

    return left->start_offset <
               right->end_offset &&
           right->start_offset <
               left->end_offset;
}

static bool
vitte_source_map_span_equal(
    const vitte_ast_span_t *left,
    const vitte_ast_span_t *right
)
{
    if (left == NULL ||
        right == NULL ||
        !vitte_source_map_same_source(
            left,
            right
        )) {

        return false;
    }

    return left->start_offset ==
               right->start_offset &&
           left->end_offset ==
               right->end_offset;
}

static void
vitte_source_map_copy_span(
    vitte_ast_span_t *destination,
    const vitte_ast_span_t *source
)
{
    if (destination == NULL) {
        return;
    }

    memset(
        destination,
        0,
        sizeof(*destination)
    );

    if (source != NULL) {
        *destination = *source;
    }
}

/* ========================================================================= */
/* Entry validation                                                          */
/* ========================================================================= */

static bool
vitte_source_map_entry_valid(
    const vitte_source_map_entry_t *entry
)
{
    if (entry == NULL) {
        return false;
    }

    if (!vitte_ast_span_is_valid(entry->generated)) {

        return false;
    }

    if (!vitte_ast_span_is_valid(entry->original)) {

        return false;
    }

    if (entry->generated.end_offset <
        entry->generated.start_offset) {

        return false;
    }

    if (entry->original.end_offset <
        entry->original.start_offset) {

        return false;
    }

    return true;
}

/* ========================================================================= */
/* Entry comparison                                                          */
/* ========================================================================= */

static int
vitte_source_map_compare_source(
    const vitte_ast_span_t *left,
    const vitte_ast_span_t *right
)
{
    if (left == NULL &&
        right == NULL) {

        return 0;
    }

    if (left == NULL) {
        return -1;
    }

    if (right == NULL) {
        return 1;
    }

    if (vitte_source_map_source_id_valid(
            left->source_id
        ) &&
        vitte_source_map_source_id_valid(
            right->source_id
        )) {

        if (left->source_id <
            right->source_id) {

            return -1;
        }

        if (left->source_id >
            right->source_id) {

            return 1;
        }
    }

    if (vitte_source_map_text_present(
            left->source_name
        ) &&
        vitte_source_map_text_present(
            right->source_name
        )) {

        int result;

        result =
            strcmp(
                left->source_name,
                right->source_name
            );

        if (result != 0) {
            return result;
        }
    } else if (vitte_source_map_text_present(
                   left->source_name
               )) {

        return 1;
    } else if (vitte_source_map_text_present(
                   right->source_name
               )) {

        return -1;
    }

    return 0;
}

static int
vitte_source_map_entry_compare(
    const vitte_source_map_entry_t *left,
    const vitte_source_map_entry_t *right
)
{
    int source_result;

    if (left == NULL &&
        right == NULL) {

        return 0;
    }

    if (left == NULL) {
        return -1;
    }

    if (right == NULL) {
        return 1;
    }

    source_result =
        vitte_source_map_compare_source(
            &left->generated,
            &right->generated
        );

    if (source_result != 0) {
        return source_result;
    }

    if (left->generated.start_offset <
        right->generated.start_offset) {

        return -1;
    }

    if (left->generated.start_offset >
        right->generated.start_offset) {

        return 1;
    }

    /*
     * For equal starts, the smaller/more specific span comes first.
     */
    if (left->generated.end_offset <
        right->generated.end_offset) {

        return -1;
    }

    if (left->generated.end_offset >
        right->generated.end_offset) {

        return 1;
    }

    source_result =
        vitte_source_map_compare_source(
            &left->original,
            &right->original
        );

    if (source_result != 0) {
        return source_result;
    }

    if (left->original.start_offset <
        right->original.start_offset) {

        return -1;
    }

    if (left->original.start_offset >
        right->original.start_offset) {

        return 1;
    }

    if (left->original.end_offset <
        right->original.end_offset) {

        return -1;
    }

    if (left->original.end_offset >
        right->original.end_offset) {

        return 1;
    }

    if (left->kind <
        right->kind) {

        return -1;
    }

    if (left->kind >
        right->kind) {

        return 1;
    }

    return 0;
}

static int
vitte_source_map_qsort_compare(
    const void *left,
    const void *right
)
{
    return vitte_source_map_entry_compare(
        (const vitte_source_map_entry_t *)left,
        (const vitte_source_map_entry_t *)right
    );
}

/* ========================================================================= */
/* Capacity                                                                  */
/* ========================================================================= */

static vitte_status_t
vitte_source_map_reserve(
    vitte_source_map_t *map,
    size_t required
)
{
    vitte_source_map_entry_t *storage;
    size_t capacity;

    if (map == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (required <=
        map->capacity) {

        return VITTE_STATUS_OK;
    }

    capacity =
        map->capacity != 0u
            ? map->capacity
            : VITTE_SOURCE_MAP_INITIAL_CAPACITY;

    while (capacity < required) {
        size_t next;

        if (capacity >
            SIZE_MAX / 2u) {

            capacity = required;
            break;
        }

        next =
            capacity * 2u;

        if (next < capacity) {
            capacity = required;
            break;
        }

        capacity = next;
    }

    if (capacity >
        SIZE_MAX /
        sizeof(*storage)) {

        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }

    storage =
        (vitte_source_map_entry_t *)realloc(
            map->entries,
            capacity *
            sizeof(*storage)
        );

    if (storage == NULL) {
        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }

    map->entries = storage;
    map->capacity = capacity;

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Initialization                                                            */
/* ========================================================================= */

void
vitte_source_map_init(
    vitte_source_map_t *map
)
{
    if (map == NULL) {
        return;
    }

    memset(
        map,
        0,
        sizeof(*map)
    );

    map->normalized = true;
}

void
vitte_source_map_destroy(
    vitte_source_map_t *map
)
{
    if (map == NULL) {
        return;
    }

    free(
        map->entries
    );

    memset(
        map,
        0,
        sizeof(*map)
    );
}

void
vitte_source_map_clear(
    vitte_source_map_t *map
)
{
    if (map == NULL) {
        return;
    }

    map->count = 0u;
    map->normalized = true;
}

/* ========================================================================= */
/* Queries                                                                   */
/* ========================================================================= */

size_t
vitte_source_map_count(
    const vitte_source_map_t *map
)
{
    return map != NULL
        ? map->count
        : 0u;
}

bool
vitte_source_map_empty(
    const vitte_source_map_t *map
)
{
    return map == NULL ||
           map->count == 0u;
}

const vitte_source_map_entry_t *
vitte_source_map_at(
    const vitte_source_map_t *map,
    size_t index
)
{
    if (map == NULL ||
        index >= map->count) {

        return NULL;
    }

    return &map->entries[index];
}

vitte_source_map_entry_t *
vitte_source_map_at_mut(
    vitte_source_map_t *map,
    size_t index
)
{
    if (map == NULL ||
        index >= map->count) {

        return NULL;
    }

    map->normalized = false;

    return &map->entries[index];
}

/* ========================================================================= */
/* Duplicate detection                                                       */
/* ========================================================================= */

static bool
vitte_source_map_entry_equal(
    const vitte_source_map_entry_t *left,
    const vitte_source_map_entry_t *right
)
{
    if (left == NULL ||
        right == NULL) {

        return false;
    }

    return
        left->kind ==
            right->kind &&
        vitte_source_map_span_equal(
            &left->generated,
            &right->generated
        ) &&
        vitte_source_map_span_equal(
            &left->original,
            &right->original
        );
}

static size_t
vitte_source_map_find_exact_entry(
    const vitte_source_map_t *map,
    const vitte_source_map_entry_t *entry
)
{
    size_t index;

    if (map == NULL ||
        entry == NULL) {

        return VITTE_SOURCE_MAP_INDEX_NONE;
    }

    for (index = 0u;
         index < map->count;
         index++) {

        if (vitte_source_map_entry_equal(
                &map->entries[index],
                entry
            )) {

            return index;
        }
    }

    return VITTE_SOURCE_MAP_INDEX_NONE;
}

/* ========================================================================= */
/* Add                                                                       */
/* ========================================================================= */

vitte_status_t
vitte_source_map_add_entry(
    vitte_source_map_t *map,
    const vitte_source_map_entry_t *entry
)
{
    vitte_status_t status;

    if (map == NULL ||
        entry == NULL) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (!vitte_source_map_entry_valid(
            entry
        )) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    /*
     * Exact duplicates add no provenance information.
     */
    if (vitte_source_map_find_exact_entry(
            map,
            entry
        ) !=
        VITTE_SOURCE_MAP_INDEX_NONE) {

        return VITTE_STATUS_OK;
    }

    status =
        vitte_source_map_reserve(
            map,
            map->count + 1u
        );

    if (status !=
        VITTE_STATUS_OK) {

        return status;
    }

    map->entries[map->count] =
        *entry;

    map->count++;
    map->normalized = false;

    return VITTE_STATUS_OK;
}

vitte_status_t
vitte_source_map_add(
    vitte_source_map_t *map,
    vitte_source_map_kind_t kind,
    const vitte_ast_span_t *generated,
    const vitte_ast_span_t *original
)
{
    vitte_source_map_entry_t entry;

    if (map == NULL ||
        generated == NULL ||
        original == NULL) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    memset(
        &entry,
        0,
        sizeof(entry)
    );

    entry.kind = kind;

    vitte_source_map_copy_span(
        &entry.generated,
        generated
    );

    vitte_source_map_copy_span(
        &entry.original,
        original
    );

    return vitte_source_map_add_entry(
        map,
        &entry
    );
}

/* ========================================================================= */
/* Remove                                                                    */
/* ========================================================================= */

vitte_status_t
vitte_source_map_remove(
    vitte_source_map_t *map,
    size_t index
)
{
    if (map == NULL ||
        index >= map->count) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (index + 1u <
        map->count) {

        memmove(
            &map->entries[index],
            &map->entries[index + 1u],
            (map->count - index - 1u) *
            sizeof(map->entries[0])
        );
    }

    map->count--;

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Normalize                                                                 */
/* ========================================================================= */

vitte_status_t
vitte_source_map_normalize(
    vitte_source_map_t *map
)
{
    size_t read_index;
    size_t write_index;

    if (map == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (map->count <= 1u) {
        map->normalized = true;
        return VITTE_STATUS_OK;
    }

    qsort(
        map->entries,
        map->count,
        sizeof(map->entries[0]),
        vitte_source_map_qsort_compare
    );

    /*
     * Remove exact duplicates after deterministic sorting.
     */
    write_index = 1u;

    for (read_index = 1u;
         read_index < map->count;
         read_index++) {

        if (vitte_source_map_entry_equal(
                &map->entries[write_index - 1u],
                &map->entries[read_index]
            )) {

            continue;
        }

        if (write_index !=
            read_index) {

            map->entries[write_index] =
                map->entries[read_index];
        }

        write_index++;
    }

    map->count = write_index;
    map->normalized = true;

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Mapping specificity                                                       */
/* ========================================================================= */

static bool
vitte_source_map_entry_better_for_offset(
    const vitte_source_map_entry_t *candidate,
    const vitte_source_map_entry_t *current
)
{
    size_t candidate_length;
    size_t current_length;

    if (candidate == NULL) {
        return false;
    }

    if (current == NULL) {
        return true;
    }

    candidate_length =
        vitte_source_map_span_length(
            &candidate->generated
        );

    current_length =
        vitte_source_map_span_length(
            &current->generated
        );

    /*
     * Prefer the smallest generated span because it represents the most
     * specific provenance available for the generated location.
     */
    if (candidate_length <
        current_length) {

        return true;
    }

    if (candidate_length >
        current_length) {

        return false;
    }

    /*
     * Exact mapping is preferred over coarser synthesized mappings.
     *
     * The enum order should therefore be defined from strongest to weakest
     * mapping semantics.
     */
    if (candidate->kind <
        current->kind) {

        return true;
    }

    if (candidate->kind >
        current->kind) {

        return false;
    }

    /*
     * Stable deterministic tie-break.
     */
    if (candidate->generated.start_offset >
        current->generated.start_offset) {

        return true;
    }

    return false;
}

/* ========================================================================= */
/* Find mapping by generated offset                                          */
/* ========================================================================= */

const vitte_source_map_entry_t *
vitte_source_map_find_offset(
    const vitte_source_map_t *map,
    vitte_source_id_t generated_source_id,
    const char *generated_source_name,
    size_t generated_offset
)
{
    const vitte_source_map_entry_t *best;
    size_t index;

    if (map == NULL) {
        return NULL;
    }

    best = NULL;

    for (index = 0u;
         index < map->count;
         index++) {

        const vitte_source_map_entry_t *entry;
        bool same_source;

        entry =
            &map->entries[index];

        same_source = false;

        if (vitte_source_map_source_id_valid(
                generated_source_id
            ) &&
            vitte_source_map_source_id_valid(
                entry->generated.source_id
            )) {

            same_source =
                generated_source_id ==
                entry->generated.source_id;
        } else if (
            vitte_source_map_text_present(
                generated_source_name
            ) &&
            vitte_source_map_text_present(
                entry->generated.source_name
            )) {

            same_source =
                strcmp(
                    generated_source_name,
                    entry->generated.source_name
                ) == 0;
        }

        if (!same_source) {
            continue;
        }

        if (!vitte_source_map_span_contains_offset(
                &entry->generated,
                generated_offset
            )) {

            continue;
        }

        if (vitte_source_map_entry_better_for_offset(
                entry,
                best
            )) {

            best = entry;
        }
    }

    return best;
}

/* ========================================================================= */
/* Find mapping by generated span                                            */
/* ========================================================================= */

const vitte_source_map_entry_t *
vitte_source_map_find_span(
    const vitte_source_map_t *map,
    const vitte_ast_span_t *generated_span
)
{
    const vitte_source_map_entry_t *best;
    size_t index;

    if (map == NULL ||
        generated_span == NULL ||
        !vitte_ast_span_is_valid(*generated_span)) {

        return NULL;
    }

    best = NULL;

    /*
     * First preference:
     * mapping that completely contains the requested generated span.
     */
    for (index = 0u;
         index < map->count;
         index++) {

        const vitte_source_map_entry_t *entry;

        entry =
            &map->entries[index];

        if (!vitte_source_map_span_contains_span(
                &entry->generated,
                generated_span
            )) {

            continue;
        }

        if (vitte_source_map_entry_better_for_offset(
                entry,
                best
            )) {

            best = entry;
        }
    }

    if (best != NULL) {
        return best;
    }

    /*
     * Fallback:
     * choose the most specific overlapping mapping.
     *
     * This is useful for diagnostics emitted by an external compiler whose
     * span may not align exactly with generated-token boundaries.
     */
    for (index = 0u;
         index < map->count;
         index++) {

        const vitte_source_map_entry_t *entry;

        entry =
            &map->entries[index];

        if (!vitte_source_map_span_overlaps(
                &entry->generated,
                generated_span
            )) {

            continue;
        }

        if (vitte_source_map_entry_better_for_offset(
                entry,
                best
            )) {

            best = entry;
        }
    }

    return best;
}

/* ========================================================================= */
/* Offset projection                                                         */
/* ========================================================================= */

static size_t
vitte_source_map_project_offset(
    const vitte_source_map_entry_t *entry,
    size_t generated_offset
)
{
    size_t generated_length;
    size_t original_length;
    size_t relative;

    if (entry == NULL) {
        return 0u;
    }

    generated_length =
        vitte_source_map_span_length(
            &entry->generated
        );

    original_length =
        vitte_source_map_span_length(
            &entry->original
        );

    if (generated_length ==
            SIZE_MAX ||
        original_length ==
            SIZE_MAX) {

        return entry->original.start_offset;
    }

    if (generated_offset <=
        entry->generated.start_offset) {

        return entry->original.start_offset;
    }

    if (generated_offset >=
        entry->generated.end_offset) {

        return entry->original.end_offset;
    }

    relative =
        generated_offset -
        entry->generated.start_offset;

    /*
     * Exact 1:1 byte projection where possible.
     *
     * This is preferable for direct textual emission.
     */
    if (generated_length ==
        original_length) {

        return entry->original.start_offset +
               relative;
    }

    /*
     * Non-1:1 transformations cannot generally preserve exact byte identity.
     *
     * Use proportional projection as a best-effort location only.
     *
     * Semantic mappings should normally use sufficiently fine-grained entries
     * so this path is rare.
     */
    if (generated_length != 0u &&
        original_length != 0u) {

        uint64_t numerator;
        uint64_t projected;

        numerator =
            (uint64_t)relative *
            (uint64_t)original_length;

        projected =
            numerator /
            (uint64_t)generated_length;

        if (projected >
            (uint64_t)original_length) {

            projected =
                (uint64_t)original_length;
        }

        return entry->original.start_offset +
               (size_t)projected;
    }

    return entry->original.start_offset;
}

/* ========================================================================= */
/* Remap generated offset                                                    */
/* ========================================================================= */

bool
vitte_source_map_remap_offset(
    const vitte_source_map_t *map,
    vitte_source_id_t generated_source_id,
    const char *generated_source_name,
    size_t generated_offset,
    vitte_source_map_position_t *result
)
{
    const vitte_source_map_entry_t *entry;

    if (result == NULL) {
        return false;
    }

    memset(
        result,
        0,
        sizeof(*result)
    );

    entry =
        vitte_source_map_find_offset(
            map,
            generated_source_id,
            generated_source_name,
            generated_offset
        );

    if (entry == NULL) {
        return false;
    }

    result->source_id =
        entry->original.source_id;

    result->source_name =
        entry->original.source_name;

    result->offset =
        vitte_source_map_project_offset(
            entry,
            generated_offset
        );

    result->mapping_kind =
        entry->kind;

    result->mapped = true;

    return true;
}

/* ========================================================================= */
/* Remap span                                                                */
/* ========================================================================= */

bool
vitte_source_map_remap_span(
    const vitte_source_map_t *map,
    const vitte_ast_span_t *generated_span,
    vitte_ast_span_t *original_span
)
{
    const vitte_source_map_entry_t *entry;
    size_t start;
    size_t end;

    if (map == NULL ||
        generated_span == NULL ||
        original_span == NULL ||
        !vitte_ast_span_is_valid(*generated_span)) {

        return false;
    }

    entry =
        vitte_source_map_find_span(
            map,
            generated_span
        );

    if (entry == NULL) {
        return false;
    }

    *original_span =
        entry->original;

    start =
        vitte_source_map_project_offset(
            entry,
            generated_span->start_offset
        );

    end =
        vitte_source_map_project_offset(
            entry,
            generated_span->end_offset
        );

    if (end < start) {
        end = start;
    }

    original_span->start_offset =
        start;

    original_span->end_offset =
        end;

    /*
     * Do not fabricate line/column information for projected byte offsets.
     *
     * Exact line/column resolution belongs to vitte_source_manager_t.
     *
     * The original mapping's cached coordinates remain useful as a fallback,
     * but consumers requiring exact projected positions should resolve the
     * projected offsets through the source manager.
     */

    return true;
}

/* ========================================================================= */
/* Exact original mapping                                                    */
/* ========================================================================= */

bool
vitte_source_map_original_span(
    const vitte_source_map_t *map,
    const vitte_ast_span_t *generated_span,
    vitte_ast_span_t *original_span
)
{
    const vitte_source_map_entry_t *entry;

    if (map == NULL ||
        generated_span == NULL ||
        original_span == NULL) {

        return false;
    }

    entry =
        vitte_source_map_find_span(
            map,
            generated_span
        );

    if (entry == NULL) {
        return false;
    }

    *original_span =
        entry->original;

    return true;
}

/* ========================================================================= */
/* Reverse lookup                                                            */
/* ========================================================================= */

const vitte_source_map_entry_t *
vitte_source_map_find_original_offset(
    const vitte_source_map_t *map,
    vitte_source_id_t original_source_id,
    const char *original_source_name,
    size_t original_offset
)
{
    const vitte_source_map_entry_t *best;
    size_t index;

    if (map == NULL) {
        return NULL;
    }

    best = NULL;

    for (index = 0u;
         index < map->count;
         index++) {

        const vitte_source_map_entry_t *entry;
        bool same_source;

        entry =
            &map->entries[index];

        same_source = false;

        if (vitte_source_map_source_id_valid(
                original_source_id
            ) &&
            vitte_source_map_source_id_valid(
                entry->original.source_id
            )) {

            same_source =
                original_source_id ==
                entry->original.source_id;
        } else if (
            vitte_source_map_text_present(
                original_source_name
            ) &&
            vitte_source_map_text_present(
                entry->original.source_name
            )) {

            same_source =
                strcmp(
                    original_source_name,
                    entry->original.source_name
                ) == 0;
        }

        if (!same_source) {
            continue;
        }

        if (!vitte_source_map_span_contains_offset(
                &entry->original,
                original_offset
            )) {

            continue;
        }

        if (best == NULL ||
            vitte_source_map_span_length(
                &entry->original
            ) <
            vitte_source_map_span_length(
                &best->original
            )) {

            best = entry;
        }
    }

    return best;
}

/* ========================================================================= */
/* Mapping chain                                                             */
/* ========================================================================= */

size_t
vitte_source_map_trace(
    const vitte_source_map_t *map,
    const vitte_ast_span_t *start,
    vitte_source_map_trace_t *trace
)
{
    vitte_ast_span_t current;
    size_t depth;

    if (trace != NULL) {
        memset(
            trace,
            0,
            sizeof(*trace)
        );
    }

    if (map == NULL ||
        start == NULL ||
        !vitte_ast_span_is_valid(*start)) {

        return 0u;
    }

    current = *start;
    depth = 0u;

    while (depth <
           VITTE_SOURCE_MAP_MAX_DEPTH) {

        const vitte_source_map_entry_t *entry;
        vitte_ast_span_t next;

        entry =
            vitte_source_map_find_span(
                map,
                &current
            );

        if (entry == NULL) {
            break;
        }

        next =
            entry->original;

        /*
         * Prevent trivial cycles.
         */
        if (vitte_source_map_span_equal(
                &current,
                &next
            )) {

            break;
        }

        if (trace != NULL &&
            depth <
                VITTE_SOURCE_MAP_TRACE_CAPACITY) {

            trace->steps[depth].kind =
                entry->kind;

            trace->steps[depth].generated =
                current;

            trace->steps[depth].original =
                next;

            trace->count =
                depth + 1u;
        }

        current = next;
        depth++;
    }

    if (trace != NULL) {
        trace->truncated =
            depth >=
            VITTE_SOURCE_MAP_MAX_DEPTH;

        trace->final_span =
            current;

        trace->has_final_span = true;
    }

    return depth;
}

/* ========================================================================= */
/* Remap to root source                                                      */
/* ========================================================================= */

bool
vitte_source_map_remap_to_root(
    const vitte_source_map_t *map,
    const vitte_ast_span_t *generated_span,
    vitte_ast_span_t *root_span
)
{
    vitte_ast_span_t current;
    size_t depth;
    bool changed;

    if (map == NULL ||
        generated_span == NULL ||
        root_span == NULL ||
        !vitte_ast_span_is_valid(*generated_span)) {

        return false;
    }

    current =
        *generated_span;

    changed = false;

    for (depth = 0u;
         depth < VITTE_SOURCE_MAP_MAX_DEPTH;
         depth++) {

        vitte_ast_span_t next;

        if (!vitte_source_map_remap_span(
                map,
                &current,
                &next
            )) {

            break;
        }

        if (vitte_source_map_span_equal(
                &current,
                &next
            )) {

            break;
        }

        current = next;
        changed = true;
    }

    *root_span =
        current;

    return changed;
}

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

vitte_source_map_validation_t
vitte_source_map_validate(
    const vitte_source_map_t *map
)
{
    vitte_source_map_validation_t result;
    size_t index;

    memset(
        &result,
        0,
        sizeof(result)
    );

    result.valid = true;
    result.first_invalid_index =
        VITTE_SOURCE_MAP_INDEX_NONE;

    if (map == NULL) {
        result.valid = false;
        return result;
    }

    result.entry_count =
        map->count;

    for (index = 0u;
         index < map->count;
         index++) {

        if (!vitte_source_map_entry_valid(
                &map->entries[index]
            )) {

            result.valid = false;
            result.invalid_entry_count++;

            if (result.first_invalid_index ==
                VITTE_SOURCE_MAP_INDEX_NONE) {

                result.first_invalid_index =
                    index;
            }
        }
    }

    /*
     * Count exact duplicates.
     */
    for (index = 0u;
         index < map->count;
         index++) {

        size_t other;

        for (other = index + 1u;
             other < map->count;
             other++) {

            if (vitte_source_map_entry_equal(
                    &map->entries[index],
                    &map->entries[other]
                )) {

                result.duplicate_count++;
            }
        }
    }

    return result;
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_source_map_statistics_t
vitte_source_map_statistics(
    const vitte_source_map_t *map
)
{
    vitte_source_map_statistics_t result;
    size_t index;

    memset(
        &result,
        0,
        sizeof(result)
    );

    if (map == NULL) {
        return result;
    }

    result.entry_count =
        map->count;

    result.capacity =
        map->capacity;

    result.normalized =
        map->normalized;

    for (index = 0u;
         index < map->count;
         index++) {

        const vitte_source_map_entry_t *entry;
        size_t generated_length;
        size_t original_length;

        entry =
            &map->entries[index];

        generated_length =
            vitte_source_map_span_length(
                &entry->generated
            );

        original_length =
            vitte_source_map_span_length(
                &entry->original
            );

        if (generated_length != SIZE_MAX) {
            result.generated_bytes +=
                generated_length;
        }

        if (original_length != SIZE_MAX) {
            result.original_bytes +=
                original_length;
        }

        if (generated_length ==
            original_length) {

            result.exact_length_count++;
        } else {
            result.transformed_length_count++;
        }

        if (entry->generated.start_offset ==
            entry->generated.end_offset) {

            result.zero_width_count++;
        }
    }

    return result;
}

/* ========================================================================= */
/* Fingerprinting                                                            */
/* ========================================================================= */

static uint64_t
vitte_source_map_hash_byte(
    uint64_t hash,
    uint8_t byte
)
{
    hash ^= (uint64_t)byte;
    hash *= UINT64_C(1099511628211);

    return hash;
}

static uint64_t
vitte_source_map_hash_size(
    uint64_t hash,
    size_t value
)
{
    size_t index;

    for (index = 0u;
         index < sizeof(value);
         index++) {

        hash =
            vitte_source_map_hash_byte(
                hash,
                (uint8_t)(
                    (value >>
                     (index * 8u)) &
                    0xffu
                )
            );
    }

    return hash;
}

static uint64_t
vitte_source_map_hash_text(
    uint64_t hash,
    const char *text
)
{
    if (text == NULL) {
        return vitte_source_map_hash_byte(
            hash,
            0u
        );
    }

    while (*text != '\0') {
        hash =
            vitte_source_map_hash_byte(
                hash,
                (uint8_t)*text
            );

        text++;
    }

    return vitte_source_map_hash_byte(
        hash,
        0u
    );
}

static uint64_t
vitte_source_map_hash_span(
    uint64_t hash,
    const vitte_ast_span_t *span
)
{
    if (span == NULL) {
        return vitte_source_map_hash_byte(
            hash,
            0u
        );
    }

    hash =
        vitte_source_map_hash_size(
            hash,
            (size_t)span->source_id
        );

    hash =
        vitte_source_map_hash_text(
            hash,
            span->source_name
        );

    hash =
        vitte_source_map_hash_size(
            hash,
            span->start_offset
        );

    hash =
        vitte_source_map_hash_size(
            hash,
            span->end_offset
        );

    return hash;
}

uint64_t
vitte_source_map_entry_fingerprint(
    const vitte_source_map_entry_t *entry
)
{
    uint64_t hash;

    if (entry == NULL) {
        return UINT64_C(0);
    }

    hash =
        UINT64_C(1469598103934665603);

    hash =
        vitte_source_map_hash_size(
            hash,
            (size_t)entry->kind
        );

    hash =
        vitte_source_map_hash_span(
            hash,
            &entry->generated
        );

    hash =
        vitte_source_map_hash_span(
            hash,
            &entry->original
        );

    return hash;
}

uint64_t
vitte_source_map_fingerprint(
    const vitte_source_map_t *map
)
{
    uint64_t hash;
    size_t index;

    if (map == NULL) {
        return UINT64_C(0);
    }

    hash =
        UINT64_C(1469598103934665603);

    for (index = 0u;
         index < map->count;
         index++) {

        uint64_t entry_hash;
        size_t byte_index;

        entry_hash =
            vitte_source_map_entry_fingerprint(
                &map->entries[index]
            );

        for (byte_index = 0u;
             byte_index < sizeof(entry_hash);
             byte_index++) {

            hash =
                vitte_source_map_hash_byte(
                    hash,
                    (uint8_t)(
                        (entry_hash >>
                         (byte_index * 8u)) &
                        UINT64_C(0xff)
                    )
                );
        }
    }

    return hash;
}

/* ========================================================================= */
/* Mapping kind                                                              */
/* ========================================================================= */

const char *
vitte_source_map_kind_name(
    vitte_source_map_kind_t kind
)
{
    switch (kind) {
        case VITTE_SOURCE_MAP_EXACT:
            return "exact";

        case VITTE_SOURCE_MAP_TOKEN:
            return "token";

        case VITTE_SOURCE_MAP_EXPRESSION:
            return "expression";

        case VITTE_SOURCE_MAP_STATEMENT:
            return "statement";

        case VITTE_SOURCE_MAP_DECLARATION:
            return "declaration";

        case VITTE_SOURCE_MAP_SYNTHETIC:
            return "synthetic";

        case VITTE_SOURCE_MAP_LOWERED:
            return "lowered";

        case VITTE_SOURCE_MAP_GENERATED:
            return "generated";

        default:
            return "unknown";
    }
}

/* ========================================================================= */
/* Merge                                                                     */
/* ========================================================================= */

vitte_status_t
vitte_source_map_merge(
    vitte_source_map_t *destination,
    const vitte_source_map_t *source
)
{
    size_t index;
    vitte_status_t status;

    if (destination == NULL ||
        source == NULL) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (destination == source) {
        return VITTE_STATUS_OK;
    }

    status =
        vitte_source_map_reserve(
            destination,
            destination->count +
            source->count
        );

    if (status !=
        VITTE_STATUS_OK) {

        return status;
    }

    for (index = 0u;
         index < source->count;
         index++) {

        status =
            vitte_source_map_add_entry(
                destination,
                &source->entries[index]
            );

        if (status !=
            VITTE_STATUS_OK) {

            return status;
        }
    }

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Copy                                                                      */
/* ========================================================================= */

vitte_status_t
vitte_source_map_copy(
    vitte_source_map_t *destination,
    const vitte_source_map_t *source
)
{
    vitte_source_map_entry_t *entries;

    if (destination == NULL ||
        source == NULL) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (destination == source) {
        return VITTE_STATUS_OK;
    }

    entries = NULL;

    if (source->count != 0u) {
        if (source->count >
            SIZE_MAX /
            sizeof(*entries)) {

            return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
        }

        entries =
            (vitte_source_map_entry_t *)malloc(
                source->count *
                sizeof(*entries)
            );

        if (entries == NULL) {
            return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
        }

        memcpy(
            entries,
            source->entries,
            source->count *
            sizeof(*entries)
        );
    }

    free(
        destination->entries
    );

    destination->entries =
        entries;

    destination->count =
        source->count;

    destination->capacity =
        source->count;

    destination->normalized =
        source->normalized;

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Swap                                                                      */
/* ========================================================================= */

void
vitte_source_map_swap(
    vitte_source_map_t *left,
    vitte_source_map_t *right
)
{
    vitte_source_map_t temporary;

    if (left == NULL ||
        right == NULL ||
        left == right) {

        return;
    }

    temporary = *left;
    *left = *right;
    *right = temporary;
}

/* ========================================================================= */
/* Generated-source count                                                    */
/* ========================================================================= */

size_t
vitte_source_map_generated_source_count(
    const vitte_source_map_t *map
)
{
    size_t count;
    size_t index;

    if (map == NULL) {
        return 0u;
    }

    count = 0u;

    for (index = 0u;
         index < map->count;
         index++) {

        size_t previous;
        bool seen;

        seen = false;

        for (previous = 0u;
             previous < index;
             previous++) {

            if (vitte_source_map_same_source(
                    &map->entries[index].generated,
                    &map->entries[previous].generated
                )) {

                seen = true;
                break;
            }
        }

        if (!seen) {
            count++;
        }
    }

    return count;
}

/* ========================================================================= */
/* Original-source count                                                     */
/* ========================================================================= */

size_t
vitte_source_map_original_source_count(
    const vitte_source_map_t *map
)
{
    size_t count;
    size_t index;

    if (map == NULL) {
        return 0u;
    }

    count = 0u;

    for (index = 0u;
         index < map->count;
         index++) {

        size_t previous;
        bool seen;

        seen = false;

        for (previous = 0u;
             previous < index;
             previous++) {

            if (vitte_source_map_same_source(
                    &map->entries[index].original,
                    &map->entries[previous].original
                )) {

                seen = true;
                break;
            }
        }

        if (!seen) {
            count++;
        }
    }

    return count;
}

/* ========================================================================= */
/* Visit                                                                     */
/* ========================================================================= */

vitte_status_t
vitte_source_map_visit(
    const vitte_source_map_t *map,
    vitte_source_map_visitor_t visitor,
    void *user_data
)
{
    size_t index;

    if (map == NULL ||
        visitor == NULL) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    for (index = 0u;
         index < map->count;
         index++) {

        if (!visitor(
                &map->entries[index],
                index,
                user_data
            )) {

            break;
        }
    }

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Root-source test                                                          */
/* ========================================================================= */

bool
vitte_source_map_has_mapping_for_span(
    const vitte_source_map_t *map,
    const vitte_ast_span_t *span
)
{
    return vitte_source_map_find_span(
        map,
        span
    ) != NULL;
}

bool
vitte_source_map_has_mapping_for_offset(
    const vitte_source_map_t *map,
    vitte_source_id_t source_id,
    const char *source_name,
    size_t offset
)
{
    return vitte_source_map_find_offset(
        map,
        source_id,
        source_name,
        offset
    ) != NULL;
}

/* ========================================================================= */
/* Shrink                                                                    */
/* ========================================================================= */

vitte_status_t
vitte_source_map_shrink_to_fit(
    vitte_source_map_t *map
)
{
    vitte_source_map_entry_t *storage;

    if (map == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (map->count ==
        map->capacity) {

        return VITTE_STATUS_OK;
    }

    if (map->count == 0u) {
        free(
            map->entries
        );

        map->entries = NULL;
        map->capacity = 0u;

        return VITTE_STATUS_OK;
    }

    storage =
        (vitte_source_map_entry_t *)realloc(
            map->entries,
            map->count *
            sizeof(*storage)
        );

    if (storage == NULL) {
        /*
         * realloc failure leaves the existing allocation valid.
         */
        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }

    map->entries = storage;
    map->capacity = map->count;

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Metadata                                                                  */
/* ========================================================================= */

const char *
vitte_source_map_component_name(void)
{
    return "diagnostic-source-map";
}

size_t
vitte_source_map_max_trace_depth(void)
{
    return VITTE_SOURCE_MAP_MAX_DEPTH;
}
