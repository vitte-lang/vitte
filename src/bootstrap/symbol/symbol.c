#include "symbol.h"

#include <stdlib.h>
#include <string.h>

static void
vitte_symbol_set_error(
    vitte_symbol_context_t *context,
    vitte_symbol_error_t error)
{
    if (context != NULL) {
        context->last_error = error;
        if (error != VITTE_SYMBOL_ERROR_NONE) {
            context->stats.failures++;
        }
    }
}

static bool
vitte_symbol_context_is_usable(
    const vitte_symbol_context_t *context)
{
    return context != NULL &&
        context->magic == VITTE_SYMBOL_MAGIC &&
        context->state != VITTE_SYMBOL_STATE_DESTROYED &&
        context->state != VITTE_SYMBOL_STATE_INVALID;
}

static bool
vitte_symbol_grow_capacity(
    size_t current,
    size_t required,
    size_t *out_capacity)
{
    size_t capacity = current == 0u ? 16u : current;

    if (out_capacity == NULL) {
        return false;
    }
    while (capacity < required) {
        if (capacity > SIZE_MAX / 2u) {
            return false;
        }
        capacity *= 2u;
    }
    *out_capacity = capacity;
    return true;
}

static bool
vitte_symbol_reserve_symbols(
    vitte_symbol_context_t *context,
    size_t required)
{
    size_t capacity;
    vitte_symbol_t *symbols;

    if (required <= context->symbol_capacity) {
        return true;
    }
    if (!vitte_symbol_grow_capacity(
            context->symbol_capacity,
            required,
            &capacity) ||
        capacity > SIZE_MAX / sizeof(*symbols)) {
        vitte_symbol_set_error(context, VITTE_SYMBOL_ERROR_OVERFLOW);
        return false;
    }
    symbols = realloc(context->symbols, capacity * sizeof(*symbols));
    if (symbols == NULL) {
        vitte_symbol_set_error(context, VITTE_SYMBOL_ERROR_OUT_OF_MEMORY);
        return false;
    }
    context->symbols = symbols;
    context->symbol_capacity = capacity;
    context->stats.reallocations++;
    return true;
}

static bool
vitte_symbol_reserve_strings(
    vitte_symbol_context_t *context,
    size_t required)
{
    size_t capacity;
    size_t index;
    char *strings;

    if (required <= context->string_capacity) {
        return true;
    }
    if (required > context->max_string_bytes ||
        !vitte_symbol_grow_capacity(
            context->string_capacity,
            required,
            &capacity) ||
        capacity > context->max_string_bytes) {
        vitte_symbol_set_error(context, VITTE_SYMBOL_ERROR_STRING_LIMIT);
        return false;
    }
    strings = realloc(context->strings, capacity);
    if (strings == NULL) {
        vitte_symbol_set_error(context, VITTE_SYMBOL_ERROR_OUT_OF_MEMORY);
        return false;
    }
    for (index = 0u; index < context->symbol_count; index++) {
        size_t offset = (size_t)(context->symbols[index].name - context->strings);
        context->symbols[index].name = strings + offset;
    }
    context->strings = strings;
    context->string_capacity = capacity;
    context->stats.reallocations++;
    return true;
}

static uint64_t
vitte_symbol_lookup_hash(
    const char *name,
    size_t length,
    vitte_symbol_namespace_t name_space,
    vitte_symbol_scope_id_t scope_id)
{
    uint64_t hash = vitte_symbol_hash_name(name, length);
    size_t index;

    hash ^= (uint64_t)name_space;
    hash *= VITTE_SYMBOL_FNV_PRIME;
    for (index = 0u; index < sizeof(scope_id); index++) {
        hash ^= (scope_id >> (index * 8u)) & UINT64_C(0xff);
        hash *= VITTE_SYMBOL_FNV_PRIME;
    }
    return hash;
}

static bool
vitte_symbol_rebuild_buckets(
    vitte_symbol_context_t *context,
    size_t bucket_count)
{
    size_t index;
    vitte_symbol_id_t *buckets;

    if (bucket_count < 16u ||
        bucket_count > SIZE_MAX / sizeof(*buckets)) {
        vitte_symbol_set_error(context, VITTE_SYMBOL_ERROR_OVERFLOW);
        return false;
    }
    buckets = calloc(bucket_count, sizeof(*buckets));
    if (buckets == NULL) {
        vitte_symbol_set_error(context, VITTE_SYMBOL_ERROR_OUT_OF_MEMORY);
        return false;
    }
    for (index = 0u; index < context->symbol_count; index++) {
        vitte_symbol_t *symbol = &context->symbols[index];
        size_t bucket = (size_t)(vitte_symbol_lookup_hash(
            symbol->name,
            symbol->name_length,
            symbol->name_space,
            symbol->scope_id) % (uint64_t)bucket_count);

        symbol->next_in_bucket = buckets[bucket];
        buckets[bucket] = symbol->id;
    }
    free(context->buckets);
    context->buckets = buckets;
    context->bucket_count = bucket_count;
    context->stats.table_rebuilds++;
    return true;
}

static bool
vitte_symbol_prepare_bucket(
    vitte_symbol_context_t *context)
{
    size_t required;
    size_t bucket_count;

    if (context->bucket_count == 0u) {
        return vitte_symbol_rebuild_buckets(
            context,
            VITTE_SYMBOL_DEFAULT_INITIAL_BUCKET_CAPACITY);
    }
    if (context->symbol_count + 1u <= context->bucket_count * 3u / 4u) {
        return true;
    }
    if (context->bucket_count > SIZE_MAX / 2u) {
        vitte_symbol_set_error(context, VITTE_SYMBOL_ERROR_OVERFLOW);
        return false;
    }
    bucket_count = context->bucket_count * 2u;
    required = context->symbol_count + 1u;
    if (bucket_count < required) {
        vitte_symbol_set_error(context, VITTE_SYMBOL_ERROR_OVERFLOW);
        return false;
    }
    return vitte_symbol_rebuild_buckets(context, bucket_count);
}

const char *
vitte_symbol_error_name(
    vitte_symbol_error_t error)
{
    switch (error) {
        case VITTE_SYMBOL_ERROR_NONE:
            return "none";
        case VITTE_SYMBOL_ERROR_INVALID_ARGUMENT:
            return "invalid-argument";
        case VITTE_SYMBOL_ERROR_INVALID_CONTEXT:
            return "invalid-context";
        case VITTE_SYMBOL_ERROR_INVALID_STATE:
            return "invalid-state";
        case VITTE_SYMBOL_ERROR_INVALID_SYMBOL:
            return "invalid-symbol";
        case VITTE_SYMBOL_ERROR_INVALID_SYMBOL_ID:
            return "invalid-symbol-id";
        case VITTE_SYMBOL_ERROR_INVALID_SCOPE:
            return "invalid-scope";
        case VITTE_SYMBOL_ERROR_INVALID_NAME:
            return "invalid-name";
        case VITTE_SYMBOL_ERROR_INVALID_NAMESPACE:
            return "invalid-namespace";
        case VITTE_SYMBOL_ERROR_INVALID_KIND:
            return "invalid-kind";
        case VITTE_SYMBOL_ERROR_DUPLICATE_SYMBOL:
            return "duplicate-symbol";
        case VITTE_SYMBOL_ERROR_NOT_FOUND:
            return "not-found";
        case VITTE_SYMBOL_ERROR_SYMBOL_LIMIT:
            return "symbol-limit";
        case VITTE_SYMBOL_ERROR_STRING_LIMIT:
            return "string-limit";
        case VITTE_SYMBOL_ERROR_OVERFLOW:
            return "overflow";
        case VITTE_SYMBOL_ERROR_OUT_OF_MEMORY:
            return "out-of-memory";
        case VITTE_SYMBOL_ERROR_VALIDATION:
            return "validation";
        case VITTE_SYMBOL_ERROR_CORRUPTION:
            return "corruption";
        case VITTE_SYMBOL_ERROR_INTERNAL:
            return "internal";
        case VITTE_SYMBOL_ERROR_COUNT:
            return "count";
    }
    return "unknown";
}

const char *
vitte_symbol_state_name(
    vitte_symbol_state_t state)
{
    switch (state) {
        case VITTE_SYMBOL_STATE_INVALID:
            return "invalid";
        case VITTE_SYMBOL_STATE_READY:
            return "ready";
        case VITTE_SYMBOL_STATE_BUILDING:
            return "building";
        case VITTE_SYMBOL_STATE_FROZEN:
            return "frozen";
        case VITTE_SYMBOL_STATE_FAILED:
            return "failed";
        case VITTE_SYMBOL_STATE_DESTROYED:
            return "destroyed";
        case VITTE_SYMBOL_STATE_COUNT:
            return "count";
    }
    return "unknown";
}

const char *
vitte_symbol_namespace_name(
    vitte_symbol_namespace_t name_space)
{
    switch (name_space) {
        case VITTE_SYMBOL_NAMESPACE_INVALID:
            return "invalid";
        case VITTE_SYMBOL_NAMESPACE_VALUE:
            return "value";
        case VITTE_SYMBOL_NAMESPACE_TYPE:
            return "type";
        case VITTE_SYMBOL_NAMESPACE_TRAIT:
            return "trait";
        case VITTE_SYMBOL_NAMESPACE_MACRO:
            return "macro";
        case VITTE_SYMBOL_NAMESPACE_SPACE:
            return "space";
        case VITTE_SYMBOL_NAMESPACE_COUNT:
            return "count";
    }
    return "unknown";
}

const char *
vitte_symbol_storage_name(
    vitte_symbol_storage_t storage)
{
    switch (storage) {
        case VITTE_SYMBOL_STORAGE_INVALID:
            return "invalid";
        case VITTE_SYMBOL_STORAGE_NONE:
            return "none";
        case VITTE_SYMBOL_STORAGE_LOCAL:
            return "local";
        case VITTE_SYMBOL_STORAGE_PARAMETER:
            return "parameter";
        case VITTE_SYMBOL_STORAGE_STATIC:
            return "static";
        case VITTE_SYMBOL_STORAGE_GLOBAL:
            return "global";
        case VITTE_SYMBOL_STORAGE_EXTERN:
            return "extern";
        case VITTE_SYMBOL_STORAGE_COMPTIME:
            return "comptime";
        case VITTE_SYMBOL_STORAGE_COUNT:
            return "count";
    }
    return "unknown";
}

bool
vitte_symbol_init(
    vitte_symbol_context_t *context)
{
    if (context == NULL) {
        return false;
    }
    memset(context, 0, sizeof(*context));
    context->magic = VITTE_SYMBOL_MAGIC;
    context->state = VITTE_SYMBOL_STATE_READY;
    context->max_symbols = VITTE_SYMBOL_DEFAULT_MAX_SYMBOLS;
    context->max_string_bytes = VITTE_SYMBOL_DEFAULT_MAX_STRING_BYTES;
    context->generation = 1u;
    context->last_error = VITTE_SYMBOL_ERROR_NONE;
    return true;
}

bool
vitte_symbol_reset(
    vitte_symbol_context_t *context)
{
    if (!vitte_symbol_context_is_usable(context)) {
        return false;
    }
    context->symbol_count = 0u;
    context->string_size = 0u;
    if (context->bucket_count > 0u) {
        memset(
            context->buckets,
            0,
            context->bucket_count * sizeof(*context->buckets));
    }
    if (context->strings != NULL) {
        context->strings[0] = '\0';
    }
    memset(&context->stats, 0, sizeof(context->stats));
    context->stats.resets++;
    context->state = VITTE_SYMBOL_STATE_READY;
    context->last_error = VITTE_SYMBOL_ERROR_NONE;
    context->generation++;
    return true;
}

void
vitte_symbol_destroy(
    vitte_symbol_context_t *context)
{
    if (context == NULL) {
        return;
    }
    if (context->magic == VITTE_SYMBOL_MAGIC) {
        free(context->symbols);
        free(context->buckets);
        free(context->strings);
    }
    memset(context, 0, sizeof(*context));
    context->magic = VITTE_SYMBOL_DEAD_MAGIC;
    context->state = VITTE_SYMBOL_STATE_DESTROYED;
}

bool
vitte_symbol_is_valid(
    const vitte_symbol_context_t *context)
{
    return vitte_symbol_context_is_usable(context);
}

vitte_symbol_error_t
vitte_symbol_last_error(
    const vitte_symbol_context_t *context)
{
    return vitte_symbol_context_is_usable(context)
        ? context->last_error
        : VITTE_SYMBOL_ERROR_INVALID_CONTEXT;
}

uint64_t
vitte_symbol_generation(
    const vitte_symbol_context_t *context)
{
    return vitte_symbol_context_is_usable(context)
        ? context->generation
        : 0u;
}

bool
vitte_symbol_set_limits(
    vitte_symbol_context_t *context,
    size_t max_symbols,
    size_t max_string_bytes)
{
    if (!vitte_symbol_context_is_usable(context)) {
        return false;
    }
    if (context->symbol_count > max_symbols ||
        context->string_size > max_string_bytes) {
        vitte_symbol_set_error(context, VITTE_SYMBOL_ERROR_INVALID_ARGUMENT);
        return false;
    }
    context->max_symbols = max_symbols;
    context->max_string_bytes = max_string_bytes;
    context->last_error = VITTE_SYMBOL_ERROR_NONE;
    return true;
}

bool
vitte_symbol_begin_build(
    vitte_symbol_context_t *context)
{
    if (!vitte_symbol_context_is_usable(context) ||
        (context->state != VITTE_SYMBOL_STATE_READY &&
         context->state != VITTE_SYMBOL_STATE_BUILDING)) {
        vitte_symbol_set_error(context, VITTE_SYMBOL_ERROR_INVALID_STATE);
        return false;
    }
    context->state = VITTE_SYMBOL_STATE_BUILDING;
    context->last_error = VITTE_SYMBOL_ERROR_NONE;
    return true;
}

bool
vitte_symbol_freeze(
    vitte_symbol_context_t *context)
{
    if (!vitte_symbol_context_is_usable(context) ||
        context->state != VITTE_SYMBOL_STATE_BUILDING) {
        vitte_symbol_set_error(context, VITTE_SYMBOL_ERROR_INVALID_STATE);
        return false;
    }
    context->state = VITTE_SYMBOL_STATE_FROZEN;
    context->stats.freezes++;
    context->last_error = VITTE_SYMBOL_ERROR_NONE;
    return true;
}

const vitte_symbol_t *
vitte_symbol_lookup(
    const vitte_symbol_context_t *context,
    const char *name,
    size_t name_length,
    vitte_symbol_namespace_t name_space,
    vitte_symbol_scope_id_t scope_id)
{
    uint64_t hash;
    vitte_symbol_id_t id;

    if (!vitte_symbol_context_is_usable(context) ||
        name == NULL ||
        name_length == 0u ||
        context->bucket_count == 0u) {
        return NULL;
    }
    hash = vitte_symbol_lookup_hash(name, name_length, name_space, scope_id);
    id = context->buckets[(size_t)(hash % (uint64_t)context->bucket_count)];
    while (id != VITTE_SYMBOL_INVALID_ID) {
        const vitte_symbol_t *symbol = vitte_symbol_get(context, id);
        if (symbol == NULL) {
            return NULL;
        }
        if (symbol->name_hash == vitte_symbol_hash_name(name, name_length) &&
            symbol->name_length == name_length &&
            symbol->name_space == name_space &&
            symbol->scope_id == scope_id &&
            memcmp(symbol->name, name, name_length) == 0) {
            return symbol;
        }
        id = symbol->next_in_bucket;
    }
    return NULL;
}

const vitte_symbol_t *
vitte_symbol_lookup_cstr(
    const vitte_symbol_context_t *context,
    const char *name,
    vitte_symbol_namespace_t name_space,
    vitte_symbol_scope_id_t scope_id)
{
    return name != NULL
        ? vitte_symbol_lookup(context, name, strlen(name), name_space, scope_id)
        : NULL;
}

bool
vitte_symbol_contains(
    const vitte_symbol_context_t *context,
    const char *name,
    size_t name_length,
    vitte_symbol_namespace_t name_space,
    vitte_symbol_scope_id_t scope_id)
{
    return vitte_symbol_lookup(
        context,
        name,
        name_length,
        name_space,
        scope_id) != NULL;
}

bool
vitte_symbol_declare(
    vitte_symbol_context_t *context,
    const vitte_symbol_declaration_t *declaration,
    vitte_symbol_id_t *out_symbol_id)
{
    size_t string_required;
    size_t bucket;
    vitte_symbol_t *symbol;
    vitte_symbol_id_t id;
    uint64_t name_hash;

    if (out_symbol_id != NULL) {
        *out_symbol_id = VITTE_SYMBOL_INVALID_ID;
    }
    if (!vitte_symbol_context_is_usable(context) ||
        context->state != VITTE_SYMBOL_STATE_BUILDING ||
        declaration == NULL ||
        declaration->name == NULL ||
        declaration->name_length == 0u ||
        declaration->kind <= VITTE_SYMBOL_KIND_INVALID ||
        declaration->kind >= VITTE_SYMBOL_KIND_COUNT ||
        declaration->name_space <= VITTE_SYMBOL_NAMESPACE_INVALID ||
        declaration->name_space >= VITTE_SYMBOL_NAMESPACE_COUNT ||
        declaration->visibility <= VITTE_SYMBOL_VISIBILITY_INVALID ||
        declaration->visibility >= VITTE_SYMBOL_VISIBILITY_COUNT ||
        declaration->storage <= VITTE_SYMBOL_STORAGE_INVALID ||
        declaration->storage >= VITTE_SYMBOL_STORAGE_COUNT) {
        vitte_symbol_set_error(context, VITTE_SYMBOL_ERROR_INVALID_ARGUMENT);
        return false;
    }
    if (context->symbol_count >= context->max_symbols) {
        vitte_symbol_set_error(context, VITTE_SYMBOL_ERROR_SYMBOL_LIMIT);
        return false;
    }
    if (vitte_symbol_contains(
            context,
            declaration->name,
            declaration->name_length,
            declaration->name_space,
            declaration->scope_id)) {
        context->stats.duplicate_rejections++;
        vitte_symbol_set_error(context, VITTE_SYMBOL_ERROR_DUPLICATE_SYMBOL);
        return false;
    }
    if (declaration->name_length == SIZE_MAX ||
        context->string_size > SIZE_MAX - declaration->name_length - 1u) {
        vitte_symbol_set_error(context, VITTE_SYMBOL_ERROR_OVERFLOW);
        return false;
    }
    string_required = context->string_size + declaration->name_length + 1u;
    if (!vitte_symbol_prepare_bucket(context) ||
        !vitte_symbol_reserve_symbols(context, context->symbol_count + 1u) ||
        !vitte_symbol_reserve_strings(context, string_required)) {
        return false;
    }
    memcpy(
        context->strings + context->string_size,
        declaration->name,
        declaration->name_length);
    context->strings[string_required - 1u] = '\0';
    id = (vitte_symbol_id_t)context->symbol_count + 1u;
    name_hash = vitte_symbol_hash_name(
        declaration->name,
        declaration->name_length);
    symbol = &context->symbols[context->symbol_count];
    memset(symbol, 0, sizeof(*symbol));
    symbol->id = id;
    symbol->kind = declaration->kind;
    symbol->name_space = declaration->name_space;
    symbol->visibility = declaration->visibility;
    symbol->storage = declaration->storage;
    symbol->flags = declaration->flags;
    symbol->name = context->strings + context->string_size;
    symbol->name_length = declaration->name_length;
    symbol->name_hash = name_hash;
    symbol->scope_id = declaration->scope_id;
    symbol->parent_symbol_id = declaration->parent_symbol_id;
    symbol->shadowed_symbol_id = declaration->shadowed_symbol_id;
    symbol->type_id = declaration->type_id;
    symbol->declaration_node_id = declaration->declaration_node_id;
    symbol->location = declaration->location;
    bucket = (size_t)(vitte_symbol_lookup_hash(
        symbol->name,
        symbol->name_length,
        symbol->name_space,
        symbol->scope_id) % (uint64_t)context->bucket_count);
    symbol->next_in_bucket = context->buckets[bucket];
    context->buckets[bucket] = id;
    context->symbol_count++;
    context->string_size = string_required;
    context->stats.symbols_created++;
    context->stats.names_interned++;
    context->stats.string_bytes +=
        (uint64_t)(declaration->name_length + 1u);
    context->generation++;
    context->last_error = VITTE_SYMBOL_ERROR_NONE;
    if (out_symbol_id != NULL) {
        *out_symbol_id = id;
    }
    return true;
}

const vitte_symbol_t *
vitte_symbol_get(
    const vitte_symbol_context_t *context,
    vitte_symbol_id_t symbol_id)
{
    if (!vitte_symbol_context_is_usable(context) ||
        symbol_id == VITTE_SYMBOL_INVALID_ID ||
        symbol_id > (vitte_symbol_id_t)context->symbol_count) {
        return NULL;
    }
    return &context->symbols[(size_t)(symbol_id - 1u)];
}

vitte_symbol_t *
vitte_symbol_get_mut(
    vitte_symbol_context_t *context,
    vitte_symbol_id_t symbol_id)
{
    if (!vitte_symbol_context_is_usable(context) ||
        context->state != VITTE_SYMBOL_STATE_BUILDING ||
        symbol_id == VITTE_SYMBOL_INVALID_ID ||
        symbol_id > (vitte_symbol_id_t)context->symbol_count) {
        return NULL;
    }
    return &context->symbols[(size_t)(symbol_id - 1u)];
}

bool
vitte_symbol_set_type(
    vitte_symbol_context_t *context,
    vitte_symbol_id_t symbol_id,
    vitte_symbol_type_id_t type_id)
{
    vitte_symbol_t *symbol = vitte_symbol_get_mut(context, symbol_id);
    if (symbol == NULL) {
        vitte_symbol_set_error(context, VITTE_SYMBOL_ERROR_INVALID_SYMBOL_ID);
        return false;
    }
    symbol->type_id = type_id;
    context->last_error = VITTE_SYMBOL_ERROR_NONE;
    return true;
}

bool
vitte_symbol_add_flags(
    vitte_symbol_context_t *context,
    vitte_symbol_id_t symbol_id,
    vitte_symbol_flags_t flags)
{
    vitte_symbol_t *symbol = vitte_symbol_get_mut(context, symbol_id);
    if (symbol == NULL) {
        vitte_symbol_set_error(context, VITTE_SYMBOL_ERROR_INVALID_SYMBOL_ID);
        return false;
    }
    symbol->flags |= flags;
    context->last_error = VITTE_SYMBOL_ERROR_NONE;
    return true;
}

bool
vitte_symbol_remove_flags(
    vitte_symbol_context_t *context,
    vitte_symbol_id_t symbol_id,
    vitte_symbol_flags_t flags)
{
    vitte_symbol_t *symbol = vitte_symbol_get_mut(context, symbol_id);
    if (symbol == NULL) {
        vitte_symbol_set_error(context, VITTE_SYMBOL_ERROR_INVALID_SYMBOL_ID);
        return false;
    }
    symbol->flags &= ~flags;
    context->last_error = VITTE_SYMBOL_ERROR_NONE;
    return true;
}

static bool
vitte_symbol_record_flag(
    vitte_symbol_context_t *context,
    vitte_symbol_id_t symbol_id,
    vitte_symbol_flags_t flag)
{
    vitte_symbol_t *symbol = vitte_symbol_get_mut(context, symbol_id);
    if (symbol == NULL) {
        vitte_symbol_set_error(context, VITTE_SYMBOL_ERROR_INVALID_SYMBOL_ID);
        return false;
    }
    symbol->flags |= flag;
    symbol->reference_count++;
    context->stats.references_recorded++;
    context->last_error = VITTE_SYMBOL_ERROR_NONE;
    return true;
}

bool
vitte_symbol_record_reference(
    vitte_symbol_context_t *context,
    vitte_symbol_id_t symbol_id)
{
    return vitte_symbol_record_flag(
        context,
        symbol_id,
        VITTE_SYMBOL_FLAG_REFERENCED);
}

bool
vitte_symbol_record_read(
    vitte_symbol_context_t *context,
    vitte_symbol_id_t symbol_id)
{
    return vitte_symbol_record_flag(
        context,
        symbol_id,
        VITTE_SYMBOL_FLAG_READ | VITTE_SYMBOL_FLAG_REFERENCED);
}

bool
vitte_symbol_record_write(
    vitte_symbol_context_t *context,
    vitte_symbol_id_t symbol_id)
{
    return vitte_symbol_record_flag(
        context,
        symbol_id,
        VITTE_SYMBOL_FLAG_WRITTEN | VITTE_SYMBOL_FLAG_REFERENCED);
}

bool
vitte_symbol_record_address_taken(
    vitte_symbol_context_t *context,
    vitte_symbol_id_t symbol_id)
{
    return vitte_symbol_record_flag(
        context,
        symbol_id,
        VITTE_SYMBOL_FLAG_ADDRESS_TAKEN);
}

uint64_t
vitte_symbol_hash_name(
    const char *name,
    size_t length)
{
    size_t index;
    uint64_t hash = VITTE_SYMBOL_FNV_OFFSET;

    if (name == NULL) {
        return 0u;
    }
    for (index = 0u; index < length; index++) {
        hash ^= (uint64_t)(unsigned char)name[index];
        hash *= VITTE_SYMBOL_FNV_PRIME;
    }
    return hash;
}

bool
vitte_symbol_validate(
    vitte_symbol_context_t *context)
{
    size_t index;
    bool valid = true;

    if (!vitte_symbol_context_is_usable(context)) {
        return false;
    }
    context->stats.validation_runs++;
    if ((context->symbol_count > 0u &&
         (context->symbols == NULL || context->buckets == NULL ||
          context->strings == NULL)) ||
        context->symbol_count > context->symbol_capacity ||
        context->string_size > context->string_capacity) {
        valid = false;
    }
    for (index = 0u; valid && index < context->symbol_count; index++) {
        const vitte_symbol_t *symbol = &context->symbols[index];
        if (symbol->id != (vitte_symbol_id_t)(index + 1u) ||
            symbol->name == NULL ||
            symbol->name_length == 0u ||
            symbol->name < context->strings ||
            symbol->name + symbol->name_length >=
                context->strings + context->string_size ||
            symbol->name[symbol->name_length] != '\0' ||
            symbol->name_hash != vitte_symbol_hash_name(
                symbol->name,
                symbol->name_length)) {
            valid = false;
        }
    }
    if (!valid) {
        context->stats.validation_failures++;
        vitte_symbol_set_error(context, VITTE_SYMBOL_ERROR_VALIDATION);
        return false;
    }
    context->last_error = VITTE_SYMBOL_ERROR_NONE;
    return true;
}

uint64_t
vitte_symbol_fingerprint(
    vitte_symbol_context_t *context)
{
    size_t index;
    uint64_t hash = VITTE_SYMBOL_FNV_OFFSET;

    if (!vitte_symbol_context_is_usable(context)) {
        return 0u;
    }
    context->stats.hash_runs++;
    for (index = 0u; index < context->symbol_count; index++) {
        const vitte_symbol_t *symbol = &context->symbols[index];
        size_t name_index;
        hash ^= symbol->id;
        hash *= VITTE_SYMBOL_FNV_PRIME;
        hash ^= (uint64_t)symbol->kind;
        hash *= VITTE_SYMBOL_FNV_PRIME;
        hash ^= (uint64_t)symbol->name_space;
        hash *= VITTE_SYMBOL_FNV_PRIME;
        hash ^= symbol->scope_id;
        hash *= VITTE_SYMBOL_FNV_PRIME;
        for (name_index = 0u; name_index < symbol->name_length; name_index++) {
            hash ^= (uint64_t)(unsigned char)symbol->name[name_index];
            hash *= VITTE_SYMBOL_FNV_PRIME;
        }
    }
    return hash;
}

vitte_symbol_stats_t
vitte_symbol_stats(
    const vitte_symbol_context_t *context)
{
    vitte_symbol_stats_t stats = {0};
    return vitte_symbol_context_is_usable(context) ? context->stats : stats;
}

void
vitte_symbol_translation_unit_anchor(void)
{
}
