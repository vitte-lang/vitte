#ifndef VITTE_SOURCE_SOURCE_H
#define VITTE_SOURCE_SOURCE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VITTE_SOURCE_MAGIC UINT64_C(0x5649545445535243)
#define VITTE_SOURCE_DEAD_MAGIC UINT64_C(0x4445414453524345)
#define VITTE_SOURCE_INVALID_ID ((vitte_source_id_t)0u)
#define VITTE_SOURCE_ID_INVALID VITTE_SOURCE_INVALID_ID
#define VITTE_SOURCE_DEFAULT_INITIAL_CAPACITY ((size_t)8u)
#define VITTE_SOURCE_DEFAULT_MAX_SOURCES ((size_t)1048576u)
#define VITTE_SOURCE_DEFAULT_MAX_SOURCE_BYTES ((size_t)1024u * 1024u * 1024u)
#define VITTE_SOURCE_FNV_OFFSET UINT64_C(14695981039346656037)
#define VITTE_SOURCE_FNV_PRIME UINT64_C(1099511628211)

typedef uint64_t vitte_source_id_t;

typedef enum vitte_source_state {
    VITTE_SOURCE_STATE_INVALID = 0,
    VITTE_SOURCE_STATE_READY,
    VITTE_SOURCE_STATE_FAILED,
    VITTE_SOURCE_STATE_DESTROYED,
    VITTE_SOURCE_STATE_COUNT
} vitte_source_state_t;

typedef enum vitte_source_error {
    VITTE_SOURCE_ERROR_NONE = 0,
    VITTE_SOURCE_ERROR_INVALID_CONTEXT,
    VITTE_SOURCE_ERROR_INVALID_ARGUMENT,
    VITTE_SOURCE_ERROR_INVALID_STATE,
    VITTE_SOURCE_ERROR_OUT_OF_MEMORY,
    VITTE_SOURCE_ERROR_OVERFLOW,
    VITTE_SOURCE_ERROR_SOURCE_LIMIT,
    VITTE_SOURCE_ERROR_SOURCE_TOO_LARGE,
    VITTE_SOURCE_ERROR_DUPLICATE_PATH,
    VITTE_SOURCE_ERROR_INVALID_UTF8,
    VITTE_SOURCE_ERROR_INVALID_SOURCE,
    VITTE_SOURCE_ERROR_INVALID_SOURCE_ID,
    VITTE_SOURCE_ERROR_INVALID_OFFSET,
    VITTE_SOURCE_ERROR_INVALID_LINE,
    VITTE_SOURCE_ERROR_INVALID_COLUMN,
    VITTE_SOURCE_ERROR_INVALID_SPAN,
    VITTE_SOURCE_ERROR_VALIDATION,
    VITTE_SOURCE_ERROR_CORRUPTION,
    VITTE_SOURCE_ERROR_INTERNAL,
    VITTE_SOURCE_ERROR_COUNT
} vitte_source_error_t;

typedef struct vitte_source_entry {
    vitte_source_id_t id;
    char *name;
    size_t name_length;
    char *path;
    size_t path_length;
    char *data;
    size_t length;
    size_t *line_offsets;
    size_t line_count;
} vitte_source_entry_t;

typedef struct vitte_source_stats {
    uint64_t allocations;
    uint64_t allocation_failures;
    uint64_t reallocations;
    uint64_t sources_added;
    uint64_t bytes_added;
    uint64_t lines_indexed;
    uint64_t resets;
    uint64_t hash_runs;
    uint64_t validation_runs;
    uint64_t validation_failures;
} vitte_source_stats_t;

typedef struct vitte_source_context {
    uint64_t magic;
    vitte_source_state_t state;
    vitte_source_error_t last_error;
    vitte_source_entry_t *sources;
    size_t source_count;
    size_t source_capacity;
    size_t max_sources;
    size_t max_source_bytes;
    uint64_t generation;
    vitte_source_stats_t stats;
} vitte_source_context_t;

typedef struct vitte_source_location {
    vitte_source_id_t source_id;
    size_t offset;
    size_t line;
    size_t column;
    bool valid;
} vitte_source_location_t;

typedef struct vitte_source_slice {
    const char *data;
    size_t length;
    vitte_source_id_t source_id;
    size_t begin;
    size_t end;
    bool valid;
} vitte_source_slice_t;

typedef struct vitte_source_span {
    vitte_source_id_t source_id;
    size_t begin;
    size_t end;
    bool valid;
} vitte_source_span_t;

const char *vitte_source_error_name(vitte_source_error_t error);
const char *vitte_source_state_name(vitte_source_state_t state);
bool vitte_source_init(vitte_source_context_t *context);
bool vitte_source_reset(vitte_source_context_t *context);
void vitte_source_destroy(vitte_source_context_t *context);
bool vitte_source_is_valid(const vitte_source_context_t *context);
vitte_source_error_t vitte_source_last_error(const vitte_source_context_t *context);
uint64_t vitte_source_generation(const vitte_source_context_t *context);
bool vitte_source_set_limits(vitte_source_context_t *context, size_t max_sources, size_t max_source_bytes);
bool vitte_source_add(vitte_source_context_t *context, const char *name, const char *path,
    const char *data, size_t length, vitte_source_id_t *out_source_id);
bool vitte_source_add_cstr(vitte_source_context_t *context, const char *name, const char *path,
    const char *data, vitte_source_id_t *out_source_id);
const vitte_source_entry_t *vitte_source_get(const vitte_source_context_t *context, vitte_source_id_t source_id);
const vitte_source_entry_t *vitte_source_find_path(const vitte_source_context_t *context, const char *path);
const vitte_source_entry_t *vitte_source_find_name(const vitte_source_context_t *context, const char *name);
bool vitte_source_location_from_offset(const vitte_source_context_t *context, vitte_source_id_t source_id,
    size_t offset, vitte_source_location_t *location);
bool vitte_source_offset_from_location(const vitte_source_context_t *context, vitte_source_id_t source_id,
    size_t line, size_t column, size_t *offset);
bool vitte_source_get_line(const vitte_source_context_t *context, vitte_source_id_t source_id,
    size_t line, vitte_source_slice_t *slice);
bool vitte_source_slice(const vitte_source_context_t *context, vitte_source_id_t source_id,
    size_t begin, size_t end, vitte_source_slice_t *slice);
bool vitte_source_make_span(const vitte_source_context_t *context, vitte_source_id_t source_id,
    size_t begin, size_t end, vitte_source_span_t *span);
bool vitte_source_span_locations(const vitte_source_context_t *context, vitte_source_span_t span,
    vitte_source_location_t *begin_location, vitte_source_location_t *end_location);
bool vitte_source_span_slice(const vitte_source_context_t *context, vitte_source_span_t span,
    vitte_source_slice_t *slice);
bool vitte_source_validate_utf8(const vitte_source_context_t *context, vitte_source_id_t source_id);
bool vitte_source_codepoint_at(const vitte_source_context_t *context, vitte_source_id_t source_id,
    size_t offset, uint32_t *codepoint, size_t *width);
bool vitte_source_validate(vitte_source_context_t *context);
uint64_t vitte_source_fingerprint(vitte_source_context_t *context);
vitte_source_stats_t vitte_source_stats(const vitte_source_context_t *context);
void vitte_source_translation_unit_anchor(void);

#ifdef __cplusplus
}
#endif

#endif
