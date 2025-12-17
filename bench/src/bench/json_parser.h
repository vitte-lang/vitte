#pragma once

/*
  json_parser.h

  Lightweight, allocation-friendly JSON parser intended for benchmarking.

  Public API:
    - Arena allocator for predictable, low-noise allocations in benchmarks
    - Parse into a compact DOM (values stored in arena)
    - Compute a stable digest/hash of the DOM to prevent dead-code elimination

  Notes:
    - Strings are slices into the input when no escaping is needed; otherwise
      they are unescaped into arena memory.
    - Numbers are stored as double.
*/

#ifndef VITTE_BENCH_JSON_PARSER_H
#define VITTE_BENCH_JSON_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Types
// -----------------------------------------------------------------------------

typedef enum JsonType {
    JSON_NULL = 0,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT,
} JsonType;

typedef struct JsonStr {
    const char* ptr;
    uint32_t    len;
    uint32_t    owns_copy; // 0 = slice into input, 1 = arena copy
} JsonStr;

typedef struct JsonValue JsonValue;

typedef struct JsonArray {
    JsonValue* items;
    uint32_t   count;
    uint32_t   cap;
} JsonArray;

typedef struct JsonMember {
    JsonStr    key;
    JsonValue* value;
} JsonMember;

typedef struct JsonObject {
    JsonMember* members;
    uint32_t    count;
    uint32_t    cap;
} JsonObject;

struct JsonValue {
    JsonType type;
    union {
        bool       b;
        double     num;
        JsonStr    str;
        JsonArray  arr;
        JsonObject obj;
    } as;
};

typedef struct JsonError {
    size_t      offset;   // byte offset into source
    const char* message;  // static string
} JsonError;

typedef struct JsonArena {
    uint8_t* data;
    size_t   size;
    size_t   used;
} JsonArena;

typedef struct JsonDoc {
    JsonValue* root;
    size_t     consumed; // bytes consumed (for embedded JSON)
} JsonDoc;

// -----------------------------------------------------------------------------
// Arena
// -----------------------------------------------------------------------------

void  json_arena_init(JsonArena* a, size_t initial_size);
void  json_arena_reset(JsonArena* a);
void  json_arena_free(JsonArena* a);
void* json_arena_alloc(JsonArena* a, size_t size, size_t align);

// -----------------------------------------------------------------------------
// Parse + digest
// -----------------------------------------------------------------------------

JsonDoc   json_parse(const char* src, size_t len, JsonArena* arena, JsonError* err);
uint64_t  json_digest_u64(const JsonValue* v);
uint64_t  json_parse_and_digest(const char* src, size_t len, JsonArena* arena, JsonError* err);

// Debug-only self tests (no-op in release).
void json_parser_debug_init(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* VITTE_BENCH_JSON_PARSER_H */
