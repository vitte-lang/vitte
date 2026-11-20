/*
 * Vitte Compiler
 * src/bootstrap/cli/entry.c
 *
 * Bootstrap command-line entry implementation.
 *
 * This translation unit owns only the process-facing source/module loading
 * and command dispatch. The other executable responsibilities are split into
 * dedicated private modules:
 *
 *   - options.c       command-line/configuration parsing;
 *   - diagnostics.c   process-level diagnostic rendering;
 *   - pipeline.c      driver phase callbacks;
 *   - execution.c     driver lifecycle and exit status.
 *
 * Compiler implementation belongs to the dedicated subsystems:
 *
 *   source
 *   unicode
 *   scanner
 *   lexer
 *   parser
 *   scope
 *   symbol
 *   type
 *   sema
 *   constant_fold
 *   hir
 *   ir
 *   backend
 *   driver
 *   filesystem
 *   module
 *   import
 *   printer
 *   util
 *
 * ISO C17.
 */

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "entry.h"
#include "command_spec.h"
#include "internal.h"

/*
 * Keep includes rooted in their subsystem directories.
 *
 * Do not add include/vitte directly to the compiler include search path:
 * that tree can contain names overlapping with libc headers.
 */
#include "api/version.h"
#include "driver/driver.h"
#include "backend/c17/ast_emitter.h"
#include "config/config.h"
#include "diagnostic/cli_bridge.h"
#include "filesystem/filesystem.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "platform/platform.h"
#include "printer/printer.h"
#include "sema/sema.h"
#include "source/source.h"
#include "unicode/unicode.h"
#include "util/util.h"

static _Thread_local char
    vitte_main_executable_path[VITTE_MAIN_MODULE_PATH_MAX];

/* ========================================================================= */
bool
vitte_main_buffer_append(
    unsigned char **buffer,
    size_t *length,
    size_t *capacity,
    const unsigned char *data,
    size_t data_length)
{
    size_t required;
    size_t next_capacity;
    unsigned char *next_buffer;

    if (buffer == NULL || length == NULL || capacity == NULL ||
        (data == NULL && data_length != 0u) ||
        data_length > SIZE_MAX - *length) {
        return false;
    }

    required = *length + data_length;
    if (required > *capacity) {
        next_capacity = *capacity != 0u ? *capacity : 4096u;
        while (next_capacity < required) {
            if (next_capacity > SIZE_MAX / 2u) {
                next_capacity = required;
                break;
            }
            next_capacity *= 2u;
        }

        next_buffer = (unsigned char *)realloc(*buffer, next_capacity);
        if (next_buffer == NULL) {
            return false;
        }

        *buffer = next_buffer;
        *capacity = next_capacity;
    }

    if (data_length != 0u) {
        memcpy(*buffer + *length, data, data_length);
    }
    *length = required;
    return true;
}

static size_t
vitte_main_module_path_index(
    const vitte_main_module_set_t *modules,
    const char *path)
{
    char *identity;
    const char *identity_key;
    size_t index;

    identity = NULL;
    identity_key = path;
    if (vitte_filesystem_canonicalize(path, &identity).error ==
            VITTE_FILESYSTEM_ERROR_NONE &&
        identity != NULL) {
        identity_key = identity;
    }

    /* The import graph is the authoritative physical-path cache.  The
     * parallel path array below remains source-order storage for mapping
     * combined bytes back to diagnostics. */
    if (modules != NULL && modules->import_graph_initialized) {
        size_t module_index;

        for (module_index = 0u;
             module_index < modules->import_graph.module_count;
             ++module_index) {
            const vitte_import_module_t *module =
                &modules->import_graph.modules[module_index];
            const char *module_path =
                vitte_import_string_data(
                    &modules->import_graph,
                    module->path);

            if (module_path != NULL &&
                strcmp(module_path, identity_key) == 0) {
                const vitte_import_module_id_t module_id = module->id;

                for (index = 0u;
                     index < modules->count;
                     ++index) {
                    if (modules->graph_module_ids[index] == module_id) {
                        free(identity);
                        return index;
                    }
                }
            }
        }
    }

    for (index = 0u; index < modules->count; ++index) {
        if (strcmp(
                modules->identity_paths[index] != NULL
                    ? modules->identity_paths[index]
                    : modules->paths[index],
                identity_key) == 0) {
            free(identity);
            return index;
        }
    }
    free(identity);
    return SIZE_MAX;
}

static bool
vitte_main_module_record_path(
    vitte_main_module_set_t *modules,
    const char *path)
{
    size_t next_capacity;
    char **next_paths;
    char **next_identity_paths;
    vitte_import_module_id_t *next_graph_module_ids;
    bool *next_active_paths;
    bool *next_loaded_paths;
    char *copy;
    char *identity_copy;
    vitte_filesystem_result_t canonical_result;

    if (modules->count == modules->capacity) {
        next_capacity = modules->capacity != 0u
            ? modules->capacity * 2u
            : 16u;
        if (next_capacity < modules->capacity ||
            next_capacity > SIZE_MAX / sizeof(*next_paths)) {
            return false;
        }

        next_paths = (char **)realloc(
            modules->paths,
            next_capacity * sizeof(*next_paths));
        if (next_paths == NULL) {
            return false;
        }
        next_identity_paths = (char **)realloc(
            modules->identity_paths,
            next_capacity * sizeof(*next_identity_paths));
        if (next_identity_paths == NULL) {
            modules->paths = next_paths;
            return false;
        }
        next_graph_module_ids = (vitte_import_module_id_t *)realloc(
            modules->graph_module_ids,
            next_capacity * sizeof(*next_graph_module_ids));
        if (next_graph_module_ids == NULL) {
            modules->paths = next_paths;
            modules->identity_paths = next_identity_paths;
            return false;
        }
        next_active_paths = (bool *)realloc(
            modules->active_paths,
            next_capacity * sizeof(*next_active_paths));
        if (next_active_paths == NULL) {
            /* Keep the successfully reallocated paths pointer reachable. */
            modules->paths = next_paths;
            modules->identity_paths = next_identity_paths;
            modules->graph_module_ids = next_graph_module_ids;
            return false;
        }
        next_loaded_paths = (bool *)realloc(
            modules->loaded_paths,
            next_capacity * sizeof(*next_loaded_paths));
        if (next_loaded_paths == NULL) {
            modules->paths = next_paths;
            modules->identity_paths = next_identity_paths;
            modules->graph_module_ids = next_graph_module_ids;
            modules->active_paths = next_active_paths;
            return false;
        }
        modules->paths = next_paths;
        modules->identity_paths = next_identity_paths;
        modules->graph_module_ids = next_graph_module_ids;
        modules->active_paths = next_active_paths;
        modules->loaded_paths = next_loaded_paths;
        modules->capacity = next_capacity;
    }

    copy = (char *)malloc(strlen(path) + 1u);
    if (copy == NULL) {
        return false;
    }
    memcpy(copy, path, strlen(path) + 1u);
    identity_copy = NULL;
    canonical_result = vitte_filesystem_canonicalize(path, &identity_copy);
    if (canonical_result.error != VITTE_FILESYSTEM_ERROR_NONE ||
        identity_copy == NULL) {
        free(identity_copy);
        identity_copy = (char *)malloc(strlen(path) + 1u);
        if (identity_copy == NULL) {
            free(copy);
            return false;
        }
        memcpy(identity_copy, path, strlen(path) + 1u);
    }
    modules->paths[modules->count] = copy;
    modules->identity_paths[modules->count] = identity_copy;
    modules->graph_module_ids[modules->count] = VITTE_IMPORT_INVALID_ID;
    modules->active_paths[modules->count] = false;
    modules->loaded_paths[modules->count] = false;
    ++modules->count;
    return true;
}

static bool
vitte_main_module_register_graph_module(
    vitte_main_module_set_t *modules,
    size_t path_index,
    const char *namespace_name,
    bool is_root)
{
    vitte_import_module_desc_t description;
    vitte_import_string_id_t name_id;
    vitte_import_string_id_t path_id;
    const char *name;
    const char *path;
    vitte_import_module_id_t module_id;

    if (modules == NULL ||
        !modules->import_graph_initialized ||
        path_index >= modules->count) {
        return false;
    }

    name = namespace_name != NULL && namespace_name[0] != '\0'
        ? namespace_name
        : modules->paths[path_index];
    path = modules->identity_paths[path_index] != NULL
        ? modules->identity_paths[path_index]
        : modules->paths[path_index];

    name_id = vitte_import_intern_string(
        &modules->import_graph,
        name,
        strlen(name));
    path_id = vitte_import_intern_string(
        &modules->import_graph,
        path,
        strlen(path));
    if (name_id == VITTE_IMPORT_INVALID_ID ||
        path_id == VITTE_IMPORT_INVALID_ID) {
        return false;
    }

    description = vitte_import_module_desc_empty();
    description.name = name_id;
    description.path = path_id;
    description.source_id = (vitte_import_source_id_t)(path_index + 1u);
    description.is_root = is_root;
    module_id = vitte_import_add_module(
        &modules->import_graph,
        &description);
    if (module_id == VITTE_IMPORT_INVALID_ID) {
        return false;
    }

    modules->graph_module_ids[path_index] = module_id;
    return true;
}

static size_t
vitte_main_module_graph_path_index(
    const vitte_main_module_set_t *modules,
    vitte_import_module_id_t module_id)
{
    size_t index;

    if (modules == NULL || module_id == VITTE_IMPORT_INVALID_ID) {
        return SIZE_MAX;
    }
    for (index = 0u; index < modules->count; ++index) {
        if (modules->graph_module_ids[index] == module_id) {
            return index;
        }
    }
    return SIZE_MAX;
}

static bool
vitte_main_import_edge_exists(
    const vitte_main_module_set_t *modules,
    vitte_import_module_id_t source_module,
    vitte_import_string_id_t target_name,
    vitte_import_string_id_t alias)
{
    const vitte_import_module_t *source;
    size_t index;

    if (modules == NULL || !modules->import_graph_initialized) {
        return false;
    }
    source = vitte_import_get_module(
        &modules->import_graph,
        source_module);
    if (source == NULL) {
        return false;
    }
    for (index = 0u; index < source->import_count; ++index) {
        const vitte_import_edge_t *edge = vitte_import_get(
            &modules->import_graph,
            source->imports[index]);
        if (edge != NULL &&
            edge->module_name == target_name &&
            edge->alias == alias) {
            return true;
        }
    }
    return false;
}

static bool
vitte_main_import_edge_location(
    const vitte_main_module_set_t *modules,
    const vitte_import_edge_t *edge,
    const char *label,
    vitte_cli_diagnostic_location_t *location)
{
    size_t path_index;

    if (modules == NULL || edge == NULL || location == NULL ||
        !edge->span.valid || edge->span.begin > edge->span.end) {
        return false;
    }
    path_index = vitte_main_module_graph_path_index(
        modules,
        edge->source_module);
    if (path_index == SIZE_MAX || path_index >= modules->count) {
        return false;
    }

    memset(location, 0, sizeof(*location));
    location->kind = VITTE_CLI_DIAGNOSTIC_LOCATION_RELATED;
    location->label = label;
    location->span.valid = true;
    location->span.file_id = edge->span.file_id;
    location->span.source_id = VITTE_SOURCE_INVALID_ID;
    location->span.source_name = modules->paths[path_index];
    location->span.start_offset = edge->span.begin;
    location->span.end_offset = edge->span.end;
    location->span.start_line = edge->span.line != 0u
        ? edge->span.line - 1u
        : 0u;
    location->span.start_column = edge->span.column != 0u
        ? edge->span.column - 1u
        : 0u;
    location->span.end_line = edge->span.end_line != 0u
        ? edge->span.end_line - 1u
        : location->span.start_line;
    location->span.end_column = edge->span.end_column != 0u
        ? edge->span.end_column - 1u
        : location->span.start_column;
    return true;
}

static bool
vitte_main_import_alias(
    const unsigned char *data,
    size_t begin,
    size_t end,
    size_t *alias_begin,
    size_t *alias_end)
{
    size_t cursor;

    if (data == NULL || alias_begin == NULL || alias_end == NULL ||
        begin > end) {
        return false;
    }
    cursor = begin;
    while (cursor < end && isspace(data[cursor])) {
        ++cursor;
    }
    if (cursor + 2u > end ||
        data[cursor] != (unsigned char)'a' ||
        data[cursor + 1u] != (unsigned char)'s' ||
        (cursor + 2u < end &&
         !isspace(data[cursor + 2u]))) {
        return false;
    }
    cursor += 2u;
    while (cursor < end && isspace(data[cursor])) {
        ++cursor;
    }
    *alias_begin = cursor;
    while (cursor < end &&
           (isalnum(data[cursor]) || data[cursor] == (unsigned char)'_')) {
        ++cursor;
    }
    *alias_end = cursor;
    return *alias_begin != *alias_end && cursor == end;
}

static void
vitte_main_import_offset_location(
    const unsigned char *data,
    size_t length,
    size_t offset,
    size_t *line,
    size_t *column)
{
    size_t index;
    size_t current_line;
    size_t current_column;

    if (line == NULL || column == NULL) {
        return;
    }

    current_line = 1u;
    current_column = 1u;
    if (data == NULL) {
        *line = current_line;
        *column = current_column;
        return;
    }

    if (offset > length) {
        offset = length;
    }
    for (index = 0u; index < offset;) {
        if (data[index] == (unsigned char)'\r') {
            ++current_line;
            current_column = 1u;
            if (index + 1u < offset &&
                data[index + 1u] == (unsigned char)'\n') {
                index += 2u;
            } else {
                ++index;
            }
        } else if (data[index] == (unsigned char)'\n') {
            ++current_line;
            current_column = 1u;
            ++index;
        } else {
            size_t next = index;
            uint32_t codepoint;

            if (vitte_unicode_next_utf8(
                    (const char *)data,
                    length,
                    &next,
                    &codepoint) &&
                next <= offset) {
                index = next;
            } else {
                ++index;
            }
            (void)codepoint;
            ++current_column;
        }
    }
    *line = current_line;
    *column = current_column;
}

static vitte_import_span_t
vitte_main_import_span_make(
    uint32_t file_id,
    const unsigned char *data,
    size_t data_length,
    size_t begin,
    size_t end)
{
    vitte_import_span_t span;

    span = vitte_import_span_make(file_id, begin, end);
    vitte_main_import_offset_location(
        data,
        data_length,
        begin,
        &span.line,
        &span.column);
    vitte_main_import_offset_location(
        data,
        data_length,
        end,
        &span.end_line,
        &span.end_column);
    return span;
}

static bool
vitte_main_import_append_text(
    char *buffer,
    size_t capacity,
    size_t *length,
    const char *text)
{
    size_t text_length;

    if (buffer == NULL || length == NULL || text == NULL ||
        *length >= capacity) {
        return false;
    }
    text_length = strlen(text);
    if (text_length >= capacity - *length) {
        return false;
    }
    memcpy(buffer + *length, text, text_length);
    *length += text_length;
    buffer[*length] = '\0';
    return true;
}

static bool
vitte_main_import_format_edge_details(
    const vitte_import_context_t *graph,
    const vitte_import_desc_t *description,
    const char *prefix,
    char *buffer,
    size_t capacity)
{
    const char *module_name;
    const char *alias_name;
    int written;
    size_t length;

    if (graph == NULL || description == NULL || prefix == NULL ||
        buffer == NULL || capacity == 0u) {
        return false;
    }
    module_name = vitte_import_string_data(graph, description->module_name);
    alias_name = description->alias != VITTE_IMPORT_INVALID_ID
        ? vitte_import_string_data(graph, description->alias)
        : NULL;
    if (module_name == NULL) {
        module_name = "<module>";
    }
    if (alias_name == NULL &&
        description->alias != VITTE_IMPORT_INVALID_ID) {
        alias_name = "<alias>";
    }

    written = snprintf(
        buffer,
        capacity,
        "%s module `%s`",
        prefix,
        module_name);
    if (written < 0 || (size_t)written >= capacity) {
        return false;
    }
    length = (size_t)written;
    if (alias_name != NULL) {
        written = snprintf(
            buffer + length,
            capacity - length,
            " as alias `%s`",
            alias_name);
        if (written < 0 || (size_t)written >= capacity - length) {
            return false;
        }
        length += (size_t)written;
    }
    return vitte_main_import_append_text(
        buffer,
        capacity,
        &length,
        "; this `use` declaration is the relevant import");
}

static bool
vitte_main_import_register_edge(
    vitte_main_module_set_t *modules,
    size_t source_path_index,
    size_t target_path_index,
    const unsigned char *data,
    size_t directive_start,
    size_t directive_end,
    size_t name_start,
    size_t name_end)
{
    const vitte_import_module_t *target;
    vitte_import_desc_t description;
    vitte_import_string_id_t alias_id;
    size_t alias_begin;
    size_t alias_end;

    if (modules == NULL || data == NULL ||
        !modules->import_graph_initialized ||
        source_path_index >= modules->count ||
        target_path_index >= modules->count ||
        directive_end == SIZE_MAX ||
        directive_end < directive_start) {
        return false;
    }
    if (modules->graph_module_ids[source_path_index] ==
            VITTE_IMPORT_INVALID_ID ||
        modules->graph_module_ids[target_path_index] ==
            VITTE_IMPORT_INVALID_ID) {
        return false;
    }

    target = vitte_import_get_module(
        &modules->import_graph,
        modules->graph_module_ids[target_path_index]);
    if (target == NULL) {
        return false;
    }

    alias_id = VITTE_IMPORT_INVALID_ID;
    alias_begin = 0u;
    alias_end = 0u;
    if (vitte_main_import_alias(
            data,
            name_end,
            directive_end,
            &alias_begin,
            &alias_end)) {
        alias_id = vitte_import_intern_string(
            &modules->import_graph,
            (const char *)data + alias_begin,
            alias_end - alias_begin);
        if (alias_id == VITTE_IMPORT_INVALID_ID) {
            return false;
        }
    }

    description = vitte_import_desc_empty();
    description.source_module = modules->graph_module_ids[source_path_index];
    description.module_name = target->name;
    description.alias = alias_id;
    description.span = vitte_main_import_span_make(
        (uint32_t)(source_path_index + 1u),
        data,
        directive_end + 1u,
        directive_start,
        directive_end + 1u);

    /* A direct import followed by a glob import is intentionally one graph
     * dependency. The AST retains both declarations for name resolution,
     * while the graph keeps one edge and one authoritative span. */
    if (vitte_main_import_edge_exists(
            modules,
            description.source_module,
            description.module_name,
            description.alias)) {
        return true;
    }

    if (vitte_import_add(
            &modules->import_graph,
            &description) != VITTE_IMPORT_INVALID_ID) {
        return true;
    }

    {
        char details[1024];
        char primary_label[512];
        vitte_import_span_t diagnostic_span;
        vitte_cli_diagnostic_location_t related_location;
        const vitte_cli_diagnostic_location_t *related_pointer;
        size_t related_count;
        const vitte_import_module_t *source_module;
        const vitte_import_edge_t *conflicting_edge;
        size_t import_index;
        const char *module_name = vitte_import_string_data(
            &modules->import_graph,
            description.module_name);
        const char *alias_name = description.alias !=
                VITTE_IMPORT_INVALID_ID
            ? vitte_import_string_data(
                  &modules->import_graph,
                  description.alias)
            : NULL;
        const char *prefix =
            vitte_import_last_error(&modules->import_graph) ==
                    VITTE_IMPORT_ERROR_ALIAS_CONFLICT
                ? "conflicting import"
                : "duplicate import";

        if (vitte_import_last_error(&modules->import_graph) ==
            VITTE_IMPORT_ERROR_ALIAS_CONFLICT) {
            (void)snprintf(
                primary_label,
                sizeof(primary_label),
                "alias `%s` is already bound here",
                alias_name != NULL ? alias_name : "<alias>");
        } else {
            (void)snprintf(
                primary_label,
                sizeof(primary_label),
                "this import duplicates an earlier dependency");
        }

        diagnostic_span = description.span;
        related_pointer = NULL;
        related_count = 0u;
        conflicting_edge = NULL;
        source_module = vitte_import_get_module(
            &modules->import_graph,
            description.source_module);
        if (source_module != NULL) {
            for (import_index = 0u;
                 import_index < source_module->import_count;
                 ++import_index) {
                const vitte_import_edge_t *candidate = vitte_import_get(
                    &modules->import_graph,
                    source_module->imports[import_index]);
                if (candidate != NULL &&
                    ((description.alias != VITTE_IMPORT_INVALID_ID &&
                      candidate->alias == description.alias) ||
                     (candidate->module_name == description.module_name &&
                      candidate->alias == description.alias))) {
                    conflicting_edge = candidate;
                    break;
                }
            }
        }
        if (vitte_import_last_error(&modules->import_graph) ==
                VITTE_IMPORT_ERROR_ALIAS_CONFLICT &&
            alias_begin < alias_end) {
            diagnostic_span = vitte_main_import_span_make(
                (uint32_t)(source_path_index + 1u),
                data,
                directive_end + 1u,
                alias_begin,
                alias_end);
        } else if (name_start < name_end) {
            diagnostic_span = vitte_main_import_span_make(
                (uint32_t)(source_path_index + 1u),
                data,
                directive_end + 1u,
                name_start,
                name_end);
        }

        if (vitte_import_last_error(&modules->import_graph) ==
            VITTE_IMPORT_ERROR_ALIAS_CONFLICT) {
            (void)snprintf(
                details,
                sizeof(details),
                "conflicting import module `%s` as alias `%s`; an earlier import already binds this alias",
                module_name != NULL ? module_name : "<module>",
                alias_name != NULL ? alias_name : "<alias>");
        } else if (!vitte_main_import_format_edge_details(
                       &modules->import_graph,
                       &description,
                       prefix,
                       details,
                       sizeof(details))) {
            (void)snprintf(
                details,
                sizeof(details),
                "%s; choose a distinct alias",
                prefix);
        }
        if (conflicting_edge != NULL &&
            vitte_main_import_edge_location(
                modules,
                conflicting_edge,
                vitte_import_last_error(&modules->import_graph) ==
                        VITTE_IMPORT_ERROR_ALIAS_CONFLICT
                    ? "earlier import binds this alias"
                    : "earlier import is declared here",
                &related_location)) {
            related_pointer = &related_location;
            related_count = 1u;
        }
        vitte_main_report_process_error_span_related(
            "VITTE_IMPORT_E_DUPLICATE",
            vitte_import_last_error(&modules->import_graph) ==
                    VITTE_IMPORT_ERROR_ALIAS_CONFLICT
                ? "conflicting import alias"
                : "duplicate import",
            details,
            modules->paths[source_path_index],
            &diagnostic_span,
            "use declaration",
            module_name,
            alias_name,
            "import graph",
            primary_label,
            related_pointer,
            related_count);
    }
    return false;
}

static const char *
vitte_main_import_display_path(
    const vitte_main_module_set_t *modules,
    vitte_import_module_id_t module_id)
{
    const vitte_import_module_t *module;
    size_t path_index;

    if (modules == NULL || !modules->import_graph_initialized) {
        return NULL;
    }
    path_index = vitte_main_module_graph_path_index(modules, module_id);
    if (path_index != SIZE_MAX) {
        return modules->paths[path_index];
    }
    module = vitte_import_get_module(
        &modules->import_graph,
        module_id);
    return module != NULL
        ? vitte_import_string_data(
              &modules->import_graph,
              module->path)
        : NULL;
}

static const vitte_import_edge_t *
vitte_main_import_find_edge(
    const vitte_import_context_t *graph,
    vitte_import_module_id_t source_module,
    vitte_import_module_id_t target_module)
{
    const vitte_import_module_t *source;
    size_t index;

    if (graph == NULL || source_module == VITTE_IMPORT_INVALID_ID) {
        return NULL;
    }
    source = vitte_import_get_module(graph, source_module);
    if (source == NULL) {
        return NULL;
    }
    for (index = 0u; index < source->import_count; ++index) {
        const vitte_import_edge_t *edge = vitte_import_get(
            graph,
            source->imports[index]);
        if (edge != NULL &&
            (target_module == VITTE_IMPORT_INVALID_ID ||
             edge->target_module == target_module)) {
            return edge;
        }
    }
    return NULL;
}

static const vitte_import_edge_t *
vitte_main_import_find_failed_edge(
    const vitte_import_context_t *graph,
    vitte_import_error_t error)
{
    size_t index;

    if (graph == NULL) {
        return NULL;
    }
    for (index = 0u; index < graph->edge_count; ++index) {
        const vitte_import_edge_t *edge = &graph->edges[index];
        if (edge->state != VITTE_IMPORT_EDGE_FAILED) {
            continue;
        }
        if (error == VITTE_IMPORT_ERROR_SELF_IMPORT &&
            edge->source_module == edge->target_module) {
            return edge;
        }
        if (error == VITTE_IMPORT_ERROR_IMPORT_NOT_FOUND &&
            edge->target_module == VITTE_IMPORT_INVALID_ID) {
            return edge;
        }
    }
    return NULL;
}

bool
vitte_main_import_graph_resolve(
    vitte_main_module_set_t *modules)
{
    vitte_import_error_t error;
    char details[VITTE_MAIN_MODULE_PATH_MAX * 2u];
    size_t detail_length;
    size_t index;

    if (modules == NULL || !modules->import_graph_initialized) {
        return false;
    }
    if (vitte_import_resolve(&modules->import_graph)) {
        return true;
    }

    error = vitte_import_last_error(&modules->import_graph);
    details[0] = '\0';
    detail_length = 0u;
    if (error == VITTE_IMPORT_ERROR_CYCLE) {
        const size_t cycle_count = vitte_import_cycle_path_count(
            &modules->import_graph);
        const vitte_import_edge_t *cycle_edge = NULL;
        vitte_cli_diagnostic_location_t related_locations[
            VITTE_CLI_DIAGNOSTIC_MAX_RELATED];
        size_t related_count = 0u;
        int written;

        written = snprintf(
            details,
            sizeof(details),
            "this import participates in the cycle; import chain: ");
        if (written > 0) {
            detail_length = (size_t)written;
        }
        for (index = 0u;
             index < cycle_count && detail_length < sizeof(details);
             ++index) {
            if (index + 1u < cycle_count && cycle_edge == NULL) {
                cycle_edge = vitte_main_import_find_edge(
                    &modules->import_graph,
                    vitte_import_cycle_path_at(
                        &modules->import_graph,
                        index),
                    vitte_import_cycle_path_at(
                        &modules->import_graph,
                        index + 1u));
            }
            const char *path = vitte_main_import_display_path(
                modules,
                vitte_import_cycle_path_at(
                    &modules->import_graph,
                    index));

            written = snprintf(
                details + detail_length,
                sizeof(details) - detail_length,
                "%s%s",
                index == 0u ? "" : " -> ",
                path != NULL ? path : "<module>");
            if (written <= 0 ||
                (size_t)written >= sizeof(details) - detail_length) {
                break;
            }
            detail_length += (size_t)written;
        }
        if (cycle_edge != NULL) {
            for (index = 0u;
                 index + 1u < cycle_count &&
                     related_count < VITTE_CLI_DIAGNOSTIC_MAX_RELATED;
                 ++index) {
                const vitte_import_edge_t *path_edge =
                    vitte_main_import_find_edge(
                        &modules->import_graph,
                        vitte_import_cycle_path_at(
                            &modules->import_graph,
                            index),
                        vitte_import_cycle_path_at(
                            &modules->import_graph,
                            index + 1u));
                if (path_edge == NULL || path_edge == cycle_edge) {
                    continue;
                }
                if (vitte_main_import_edge_location(
                        modules,
                        path_edge,
                        "another import in this cycle is here",
                        &related_locations[related_count])) {
                    ++related_count;
                }
            }
            const char *module_name = vitte_import_string_data(
                &modules->import_graph,
                cycle_edge->module_name);
            const char *alias_name = cycle_edge->alias !=
                    VITTE_IMPORT_INVALID_ID
                ? vitte_import_string_data(
                      &modules->import_graph,
                      cycle_edge->alias)
                : NULL;
            char import_context[512];

            if (alias_name != NULL) {
                (void)snprintf(
                    import_context,
                    sizeof(import_context),
                    "use declaration imports `%s` as alias `%s`",
                    module_name != NULL ? module_name : "<module>",
                    alias_name);
            } else {
                (void)snprintf(
                    import_context,
                    sizeof(import_context),
                    "use declaration imports `%s`",
                    module_name != NULL ? module_name : "<module>");
            }
            vitte_main_report_process_error_span_related(
                "VITTE_IMPORT_E_CYCLE",
                "cyclic module import",
                details,
                vitte_main_import_display_path(
                    modules,
                    cycle_edge->source_module),
                &cycle_edge->span,
                import_context,
                module_name,
                alias_name,
                "import graph",
                details,
                related_locations,
                related_count);
        } else {
            vitte_main_report_process_error(
                "VITTE_IMPORT_E_CYCLE",
                "cyclic module import",
                details);
        }
    } else if (error == VITTE_IMPORT_ERROR_SELF_IMPORT) {
        const vitte_import_edge_t *edge = vitte_main_import_find_failed_edge(
            &modules->import_graph,
            error);
        if (edge != NULL) {
            char edge_details[1024];
            if (!vitte_main_import_format_edge_details(
                    &modules->import_graph,
                    &(vitte_import_desc_t){
                        edge->source_module,
                        edge->module_name,
                        edge->alias,
                        edge->visibility,
                        edge->span},
                    "self-import",
                    edge_details,
                    sizeof(edge_details))) {
                (void)snprintf(
                    edge_details,
                    sizeof(edge_details),
                    "remove the self-import");
            }
            vitte_main_report_process_error_span(
                "VITTE_IMPORT_E_CYCLE",
                "module imports itself",
                edge_details,
                vitte_main_import_display_path(
                    modules,
                    edge->source_module),
                &edge->span,
                "use declaration",
                vitte_import_string_data(
                    &modules->import_graph,
                    edge->module_name),
                edge->alias != VITTE_IMPORT_INVALID_ID
                    ? vitte_import_string_data(
                          &modules->import_graph,
                          edge->alias)
                    : NULL,
                "import graph");
        } else {
            vitte_main_report_process_error(
                "VITTE_IMPORT_E_CYCLE",
                "module imports itself",
                "remove the self-import or move the shared declaration to another module");
        }
    } else if (error == VITTE_IMPORT_ERROR_IMPORT_NOT_FOUND ||
               error == VITTE_IMPORT_ERROR_MODULE_NOT_FOUND) {
        const vitte_import_edge_t *edge = vitte_main_import_find_failed_edge(
            &modules->import_graph,
            VITTE_IMPORT_ERROR_IMPORT_NOT_FOUND);
        if (edge != NULL) {
            char edge_details[1024];
            if (!vitte_main_import_format_edge_details(
                    &modules->import_graph,
                    &(vitte_import_desc_t){
                        edge->source_module,
                        edge->module_name,
                        edge->alias,
                        edge->visibility,
                        edge->span},
                    "unresolved import",
                    edge_details,
                    sizeof(edge_details))) {
                (void)snprintf(
                    edge_details,
                    sizeof(edge_details),
                    "the import graph contains an unresolved module");
            }
            vitte_main_report_process_error_span(
                "VITTE_IMPORT_E_MODULE_NOT_FOUND",
                "cannot resolve imported module",
                edge_details,
                vitte_main_import_display_path(
                    modules,
                    edge->source_module),
                &edge->span,
                "use declaration",
                vitte_import_string_data(
                    &modules->import_graph,
                    edge->module_name),
                edge->alias != VITTE_IMPORT_INVALID_ID
                    ? vitte_import_string_data(
                          &modules->import_graph,
                          edge->alias)
                    : NULL,
                "import graph");
        } else {
            vitte_main_report_process_error(
                "VITTE_IMPORT_E_MODULE_NOT_FOUND",
                "cannot resolve imported module",
                "the import graph contains an unresolved module");
        }
    } else {
        vitte_main_report_process_error(
            "VITTE_IMPORT_E_DUPLICATE",
            "import graph resolution failed",
            vitte_import_error_name(error));
    }
    return false;
}

bool
vitte_main_import_graph_load(
    vitte_main_module_set_t *modules,
    const char *root_path)
{
    if (modules == NULL || root_path == NULL ||
        !modules->import_graph_initialized) {
        return false;
    }
    if (!vitte_main_expand_module(modules, root_path, 0u, NULL) ||
        !vitte_main_import_graph_resolve(modules)) {
        return false;
    }
    if (!vitte_import_validate(&modules->import_graph)) {
        vitte_main_report_process_error(
            "VITTE_IMPORT_E_INVALID_GRAPH",
            "import graph validation failed",
            vitte_import_error_name(
                vitte_import_last_error(&modules->import_graph)));
        return false;
    }
    return modules->source != NULL && modules->source_length != SIZE_MAX;
}

static bool
vitte_main_module_push_active(
    vitte_main_module_set_t *modules,
    size_t path_index)
{
    size_t next_capacity;
    size_t *next_stack;

    if (modules == NULL) {
        return false;
    }
    if (modules->active_count == modules->active_capacity) {
        next_capacity = modules->active_capacity != 0u
            ? modules->active_capacity * 2u
            : 16u;
        if (next_capacity < modules->active_capacity ||
            next_capacity > SIZE_MAX / sizeof(*next_stack)) {
            return false;
        }
        next_stack = (size_t *)realloc(
            modules->active_stack,
            next_capacity * sizeof(*next_stack));
        if (next_stack == NULL) {
            return false;
        }
        modules->active_stack = next_stack;
        modules->active_capacity = next_capacity;
    }
    modules->active_stack[modules->active_count++] = path_index;
    return true;
}

static void
vitte_main_module_pop_active(
    vitte_main_module_set_t *modules,
    size_t path_index)
{
    size_t index;

    if (modules == NULL || modules->active_count == 0u) {
        return;
    }
    if (modules->active_stack[modules->active_count - 1u] == path_index) {
        --modules->active_count;
        return;
    }
    for (index = modules->active_count; index != 0u; --index) {
        if (modules->active_stack[index - 1u] == path_index) {
            memmove(
                &modules->active_stack[index - 1u],
                &modules->active_stack[index],
                (modules->active_count - index) * sizeof(*modules->active_stack));
            --modules->active_count;
            return;
        }
    }
}

static bool
vitte_main_module_record_origin(
    vitte_main_module_set_t *modules,
    const char *path,
    size_t combined_begin,
    size_t source_begin,
    size_t length)
{
    size_t path_index;
    size_t next_capacity;
    vitte_main_source_origin_t *next_origins;

    if (modules == NULL || path == NULL || length == 0u ||
        combined_begin > SIZE_MAX - length ||
        source_begin > SIZE_MAX - length) {
        return length == 0u;
    }

    for (path_index = 0u; path_index < modules->count; ++path_index) {
        if (strcmp(modules->paths[path_index], path) == 0) {
            break;
        }
    }
    if (path_index == modules->count) {
        return false;
    }

    if (modules->origin_count == modules->origin_capacity) {
        next_capacity = modules->origin_capacity != 0u
            ? modules->origin_capacity * 2u
            : 64u;
        if (next_capacity < modules->origin_capacity ||
            next_capacity > SIZE_MAX / sizeof(*next_origins)) {
            return false;
        }
        next_origins = (vitte_main_source_origin_t *)realloc(
            modules->origins,
            next_capacity * sizeof(*next_origins));
        if (next_origins == NULL) {
            return false;
        }
        modules->origins = next_origins;
        modules->origin_capacity = next_capacity;
    }

    modules->origins[modules->origin_count].combined_begin = combined_begin;
    modules->origins[modules->origin_count].combined_end =
        combined_begin + length;
    modules->origins[modules->origin_count].source_begin = source_begin;
    modules->origins[modules->origin_count].source_end = source_begin + length;
    modules->origins[modules->origin_count].path_index = path_index;
    modules->origins[modules->origin_count].source_id =
        VITTE_SOURCE_INVALID_ID;
    ++modules->origin_count;
    return true;
}

static bool
vitte_main_module_root_header_range(
    const unsigned char *source,
    size_t source_length,
    size_t *header_begin,
    size_t *header_end)
{
    size_t offset;

    if (source == NULL || header_begin == NULL || header_end == NULL) {
        return false;
    }
    offset = vitte_unicode_skip_utf8_bom(
        (const char *)source,
        source_length);

    while (offset < source_length) {
        size_t line_end = offset;
        size_t token_begin;
        size_t token_length;

        while (line_end < source_length && source[line_end] != '\n') {
            ++line_end;
        }
        token_begin = offset;
        while (token_begin < line_end &&
               (source[token_begin] == ' ' || source[token_begin] == '\t' ||
                source[token_begin] == '\r')) {
            ++token_begin;
        }
        token_length = line_end - token_begin;
        if (token_length == 0u ||
            (token_length >= 2u && source[token_begin] == '/' &&
             (source[token_begin + 1u] == '/' ||
              source[token_begin + 1u] == '*')) ||
            (token_length >= 2u && source[token_begin] == '#' &&
             source[token_begin + 1u] == '!')) {
            offset = line_end < source_length ? line_end + 1u : line_end;
            continue;
        }

        if (token_length >= 5u &&
            memcmp(source + token_begin, "space", 5u) == 0 &&
            (token_length == 5u || source[token_begin + 5u] == ' ' ||
             source[token_begin + 5u] == '\t' ||
             source[token_begin + 5u] == ';' ||
             source[token_begin + 5u] == '{')) {
            *header_begin = offset;
            *header_end = line_end < source_length ? line_end + 1u : line_end;
            return true;
        }

        return false;
    }

    return false;
}

static bool
vitte_main_module_store_canonical_path(
    const char *candidate,
    char *resolved,
    size_t resolved_capacity)
{
    char *canonical;
    char absolute_candidate[VITTE_MAIN_MODULE_PATH_MAX];
    const char *path;
    vitte_filesystem_result_t result;
    size_t length;
    int written;

    if (candidate == NULL || resolved == NULL || resolved_capacity == 0u) {
        return false;
    }

    path = candidate;
    if (!vitte_filesystem_path_is_absolute(candidate)) {
        char working_directory[VITTE_MAIN_MODULE_PATH_MAX];

        if (!vitte_platform_getcwd(
                working_directory,
                sizeof(working_directory))) {
            return false;
        }
        written = snprintf(
            absolute_candidate,
            sizeof(absolute_candidate),
            "%s/%s",
            working_directory,
            candidate);
        if (written < 0 || (size_t)written >= sizeof(absolute_candidate)) {
            return false;
        }
        path = absolute_candidate;
    }

    canonical = NULL;
    /* Keep the logical absolute path stable in diagnostics while collapsing
       '.', '..', and duplicate separators before graph identity checks. */
    result = vitte_filesystem_normalize(path, &canonical);
    if (result.error != VITTE_FILESYSTEM_ERROR_NONE || canonical == NULL) {
        return false;
    }
    length = strlen(canonical);
    if (length >= resolved_capacity) {
        free(canonical);
        return false;
    }
    memcpy(resolved, canonical, length + 1u);
    free(canonical);
    return true;
}

static bool
vitte_main_module_try_candidate(
    const char *base,
    const char *module_path,
    char *resolved,
    size_t resolved_capacity)
{
    char candidate[VITTE_MAIN_MODULE_PATH_MAX];
    const char *separator;
    size_t base_length;
    int written;

    if (base == NULL || module_path == NULL) {
        return false;
    }

    base_length = strlen(base);
    separator = base_length != 0u && base[base_length - 1u] == '/'
        ? ""
        : "/";

    written = snprintf(
        candidate,
        sizeof(candidate),
        "%s%s%s.vit",
        base,
        separator,
        module_path);
    if (written < 0 || (size_t)written >= sizeof(candidate)) {
        return false;
    }
    if (vitte_filesystem_is_file(candidate)) {
        return vitte_main_module_store_canonical_path(
            candidate,
            resolved,
            resolved_capacity);
    }

    written = snprintf(
        candidate,
        sizeof(candidate),
        "%s%s%s.vitte",
        base,
        separator,
        module_path);
    if (written < 0 || (size_t)written >= sizeof(candidate)) {
        return false;
    }
    if (vitte_filesystem_is_file(candidate)) {
        return vitte_main_module_store_canonical_path(
            candidate,
            resolved,
            resolved_capacity);
    }

    written = snprintf(
        candidate,
        sizeof(candidate),
        "%s%s%s/mod.vit",
        base,
        separator,
        module_path);
    if (written < 0 || (size_t)written >= sizeof(candidate)) {
        return false;
    }
    if (!vitte_filesystem_is_file(candidate)) {
        written = snprintf(
            candidate,
            sizeof(candidate),
            "%s%s%s/mod.vitte",
            base,
            separator,
            module_path);
        if (written < 0 || (size_t)written >= sizeof(candidate)) {
            return false;
        }
        if (!vitte_filesystem_is_file(candidate)) {
            return false;
        }
    }
    return vitte_main_module_store_canonical_path(
        candidate,
        resolved,
        resolved_capacity);
}

static bool
vitte_main_module_try_root(
    const char *root,
    const char *module_path,
    char *resolved,
    size_t resolved_capacity)
{
    char manifest_path[VITTE_MAIN_MODULE_PATH_MAX];
    char package_name[128];
    vitte_main_file_t manifest;
    size_t package_length;
    size_t line_start;
    size_t line_end;
    int written;

    if (root == NULL || module_path == NULL) {
        return false;
    }

    if (vitte_main_module_try_candidate(
            root,
            module_path,
            resolved,
            resolved_capacity)) {
        return true;
    }

    /*
     * A configured module root may point at a package's `src` directory.
     * In that case, accept the package name as an import prefix while looking
     * up its source-relative module path. For example, with
     * `modules/core/src` on VITTE_MODULE_PATH, `core::intrinsics::memory`
     * resolves to `modules/core/src/intrinsics/memory.vit`.
     */
    written = snprintf(
        manifest_path,
        sizeof(manifest_path),
        "%s/Vitte.toml",
        root);
    if (written < 0 || (size_t)written >= sizeof(manifest_path) ||
        !vitte_filesystem_is_file(manifest_path)) {
        written = snprintf(
            manifest_path,
            sizeof(manifest_path),
            "%s/../Vitte.toml",
            root);
        if (written < 0 ||
            (size_t)written >= sizeof(manifest_path) ||
            !vitte_filesystem_is_file(manifest_path)) {
            return false;
        }
    }

    memset(&manifest, 0, sizeof(manifest));
    if (!vitte_main_read_file(manifest_path, &manifest) ||
        manifest.data == NULL) {
        free(manifest.data);
        return false;
    }

    package_length = 0u;
    line_start = 0u;
    while (line_start < manifest.length) {
        size_t cursor;

        line_end = line_start;
        while (line_end < manifest.length &&
               manifest.data[line_end] != '\n') {
            ++line_end;
        }
        cursor = line_start;
        while (cursor < line_end &&
               (manifest.data[cursor] == ' ' ||
                manifest.data[cursor] == '\t' ||
                manifest.data[cursor] == '\r')) {
            ++cursor;
        }
        if (line_end - cursor >= 4u &&
            memcmp(manifest.data + cursor, "name", 4u) == 0) {
            cursor += 4u;
            while (cursor < line_end &&
                   (manifest.data[cursor] == ' ' ||
                    manifest.data[cursor] == '\t')) {
                ++cursor;
            }
            if (cursor < line_end && manifest.data[cursor] == '=') {
                ++cursor;
                while (cursor < line_end &&
                       (manifest.data[cursor] == ' ' ||
                        manifest.data[cursor] == '\t')) {
                    ++cursor;
                }
                if (cursor < line_end && manifest.data[cursor] == '"') {
                    size_t value_start = ++cursor;

                    while (cursor < line_end &&
                           manifest.data[cursor] != '"') {
                        ++cursor;
                    }
                    package_length = cursor - value_start;
                    if (package_length != 0u &&
                        package_length < sizeof(package_name)) {
                        memcpy(
                            package_name,
                            manifest.data + value_start,
                            package_length);
                        package_name[package_length] = '\0';
                    } else {
                        package_length = 0u;
                    }
                    break;
                }
            }
        }
        line_start = line_end < manifest.length ? line_end + 1u : line_end;
    }
    free(manifest.data);

    if (package_length == 0u ||
        strncmp(module_path, package_name, package_length) != 0 ||
        module_path[package_length] != '/') {
        return false;
    }

    return vitte_main_module_try_candidate(
        root,
        module_path + package_length + 1u,
        resolved,
        resolved_capacity);
}

static void
vitte_main_module_trace_root(
    char *trace,
    size_t trace_capacity,
    const char *root)
{
    size_t length;
    size_t root_length;
    const char *separator;
    int written;

    if (trace == NULL || trace_capacity == 0u || root == NULL ||
        root[0] == '\0') {
        return;
    }
    length = strlen(trace);
    root_length = strlen(root);
    separator = length == 0u ? "" : ", ";
    if (trace_capacity - length <= root_length + strlen(separator) + 2u) {
        static const char truncated[] = ", …";
        size_t truncated_length = sizeof(truncated) - 1u;
        if (length + truncated_length < trace_capacity &&
            (length == 0u ||
             (length >= truncated_length &&
              strcmp(trace + length - truncated_length, truncated) != 0))) {
            memcpy(trace + length, truncated, truncated_length + 1u);
        }
        return;
    }
    written = snprintf(
        trace + length,
        trace_capacity - length,
        "%s`%s`",
        separator,
        root);
    (void)written;
}

static bool
vitte_main_module_resolve_installed(
    const char *module_path,
    char *resolved,
    size_t resolved_capacity,
    char *lookup_roots,
    size_t lookup_roots_capacity)
{
    const char *configured_paths;
    const char *cursor;
    char root[VITTE_MAIN_MODULE_PATH_MAX];
    char executable_directory[VITTE_MAIN_MODULE_PATH_MAX];
    char *slash;
    size_t length;
    int written;

    configured_paths = getenv("VITTE_MODULE_PATH");
    cursor = configured_paths;
    while (cursor != NULL && *cursor != '\0') {
        const char *separator;

        separator = strchr(cursor, vitte_platform_path_list_separator());
        length = separator != NULL
            ? (size_t)(separator - cursor)
            : strlen(cursor);
        if (length != 0u && length < sizeof(root)) {
            memcpy(root, cursor, length);
            root[length] = '\0';
            vitte_main_module_trace_root(
                lookup_roots,
                lookup_roots_capacity,
                root);
            if (vitte_main_module_try_root(
                    root,
                    module_path,
                    resolved,
                    resolved_capacity)) {
                return true;
            }
        }
        if (separator == NULL) {
            break;
        }
        cursor = separator + 1;
    }

#if defined(_WIN32)
    {
        const char *user_data = getenv("LOCALAPPDATA");
        written = user_data != NULL && user_data[0] != '\0'
            ? snprintf(root, sizeof(root), "%s/Vitte/modules", user_data)
            : -1;
    }
#else
    {
        const char *user_data = getenv("XDG_DATA_HOME");
        if (user_data != NULL && user_data[0] != '\0') {
            written = snprintf(root, sizeof(root), "%s/vitte/modules", user_data);
        } else {
            user_data = getenv("HOME");
            written = user_data != NULL && user_data[0] != '\0'
                ? snprintf(root, sizeof(root), "%s/.local/share/vitte/modules",
                           user_data)
                : -1;
        }
    }
#endif
    if (written >= 0 && (size_t)written < sizeof(root)) {
        vitte_main_module_trace_root(
            lookup_roots, lookup_roots_capacity, root);
        if (vitte_main_module_try_root(
                root, module_path, resolved, resolved_capacity)) {
            return true;
        }
    }

    if (vitte_main_executable_path[0] != '\0' &&
        strlen(vitte_main_executable_path) < sizeof(executable_directory)) {
        memcpy(
            executable_directory,
            vitte_main_executable_path,
            strlen(vitte_main_executable_path) + 1u);
        slash = strrchr(executable_directory, '/');
        if (slash != NULL) {
            *slash = '\0';
            slash = strrchr(executable_directory, '/');
            if (slash != NULL &&
                (strcmp(slash + 1, "bin") == 0 ||
                 strcmp(slash + 1, "sbin") == 0)) {
                *slash = '\0';
                written = snprintf(
                    root,
                    sizeof(root),
                    "%s/share/vitte/modules",
                    executable_directory);
                if (written >= 0 &&
                    (size_t)written < sizeof(root)) {
                    vitte_main_module_trace_root(
                        lookup_roots,
                        lookup_roots_capacity,
                        root);
                }
                if (written >= 0 &&
                    (size_t)written < sizeof(root) &&
                    vitte_main_module_try_root(
                        root,
                        module_path,
                        resolved,
                        resolved_capacity)) {
                    return true;
                }
            }
        }
    }

    vitte_main_module_trace_root(
        lookup_roots,
        lookup_roots_capacity,
        "/usr/local/share/vitte/modules");
    if (vitte_main_module_try_root(
            "/usr/local/share/vitte/modules",
            module_path,
            resolved,
            resolved_capacity)) {
        return true;
    }
    vitte_main_module_trace_root(
        lookup_roots,
        lookup_roots_capacity,
        "/usr/share/vitte/modules");
    if (vitte_main_module_try_root(
            "/usr/share/vitte/modules",
            module_path,
            resolved,
            resolved_capacity)) {
        return true;
    }

    return false;
}

static void
vitte_main_set_executable_path(
    const char *argument_zero)
{
    char candidate[VITTE_MAIN_MODULE_PATH_MAX];
    char working_directory[VITTE_MAIN_MODULE_PATH_MAX];
    const char *path;
    const char *cursor;
    const char *separator;
    size_t directory_length;
    int written;

    vitte_main_executable_path[0] = '\0';
    if (argument_zero == NULL || argument_zero[0] == '\0') {
        return;
    }

    if (strchr(argument_zero, '/') != NULL) {
        if (vitte_filesystem_path_is_absolute(argument_zero)) {
            written = snprintf(
                vitte_main_executable_path,
                sizeof(vitte_main_executable_path),
                "%s",
                argument_zero);
        } else if (vitte_platform_getcwd(
                       working_directory,
                       sizeof(working_directory))) {
            written = snprintf(
                vitte_main_executable_path,
                sizeof(vitte_main_executable_path),
                "%s/%s",
                working_directory,
                argument_zero);
        } else {
            return;
        }
        if (written < 0 ||
            (size_t)written >= sizeof(vitte_main_executable_path)) {
            vitte_main_executable_path[0] = '\0';
        }
        return;
    }

    path = getenv("PATH");
    cursor = path;
    while (cursor != NULL && *cursor != '\0') {
        separator = strchr(cursor, vitte_platform_path_list_separator());
        directory_length = separator != NULL
            ? (size_t)(separator - cursor)
            : strlen(cursor);
        if (directory_length != 0u &&
            directory_length < sizeof(candidate)) {
            memcpy(candidate, cursor, directory_length);
            candidate[directory_length] = '\0';
            written = snprintf(
                vitte_main_executable_path,
                sizeof(vitte_main_executable_path),
                "%s/%s",
                candidate,
                argument_zero);
            if (written >= 0 &&
                (size_t)written < sizeof(vitte_main_executable_path) &&
                vitte_platform_is_executable(
                    vitte_main_executable_path)) {
                return;
            }
        }
        if (separator == NULL) {
            break;
        }
        cursor = separator + 1;
    }
    vitte_main_executable_path[0] = '\0';
}

typedef enum vitte_main_module_path_status {
    VITTE_MAIN_MODULE_PATH_VALID = 0,
    VITTE_MAIN_MODULE_PATH_MALFORMED,
    VITTE_MAIN_MODULE_PATH_INVALID_UTF8,
    VITTE_MAIN_MODULE_PATH_INVALID_IDENTIFIER
} vitte_main_module_path_status_t;

static vitte_main_module_path_status_t
vitte_main_module_import_path_is_well_formed(
    const char *path,
    size_t *error_begin,
    size_t *error_end,
    uint32_t *error_codepoint,
    vitte_unicode_error_t *unicode_error);

static bool
vitte_main_module_resolve_path(
    const char *importer_path,
    const char *requested,
    char *resolved,
    size_t resolved_capacity,
    char *lookup_roots,
    size_t lookup_roots_capacity)
{
    char module_path[VITTE_MAIN_MODULE_PATH_MAX];
    char ancestor[VITTE_MAIN_MODULE_PATH_MAX];
    char absolute_importer[VITTE_MAIN_MODULE_PATH_MAX];
    char project_modules[VITTE_MAIN_MODULE_PATH_MAX];
    char working_directory[VITTE_MAIN_MODULE_PATH_MAX];
    const char *cursor;
    char *slash;
    size_t path_length;
    size_t ancestor_length;
    size_t depth;
    size_t end;
    const char *path_separator;
    int written;

    if (lookup_roots != NULL && lookup_roots_capacity != 0u) {
        lookup_roots[0] = '\0';
    }
    if (importer_path == NULL || requested == NULL ||
        resolved == NULL || resolved_capacity == 0u) {
        return false;
    }

    path_length = strlen(requested);
    if (path_length == 0u || path_length >= sizeof(module_path)) {
        return false;
    }

    end = 0u;
    cursor = requested;
    while (*cursor != '\0') {
        if (cursor[0] == ':' && cursor[1] == ':') {
            if (end == 0u || module_path[end - 1u] == '/') {
                return false;
            }
            module_path[end++] = '/';
            cursor += 2;
        } else if (isalnum((unsigned char)*cursor) ||
                   (unsigned char)*cursor >= 0x80u ||
                   *cursor == '_' || *cursor == '/') {
            module_path[end++] = *cursor++;
        } else {
            return false;
        }
    }
    while (end != 0u && module_path[end - 1u] == '/') {
        --end;
    }
    if (end == 0u) {
        return false;
    }
    module_path[end] = '\0';

    if (vitte_main_module_import_path_is_well_formed(
            requested, NULL, NULL, NULL, NULL) !=
        VITTE_MAIN_MODULE_PATH_VALID) {
        return false;
    }

    if (vitte_filesystem_path_is_absolute(importer_path)) {
        written = snprintf(
            absolute_importer,
            sizeof(absolute_importer),
            "%s",
            importer_path);
    } else if (!vitte_platform_getcwd(
                   working_directory,
                   sizeof(working_directory))) {
        return false;
    } else {
        written = snprintf(
            absolute_importer,
            sizeof(absolute_importer),
            "%s/%s",
            working_directory,
            importer_path);
    }
    if (written < 0 ||
        (size_t)written >= sizeof(absolute_importer)) {
        return false;
    }

    memcpy(
        ancestor,
        absolute_importer,
        strlen(absolute_importer) + 1u);
    slash = strrchr(ancestor, '/');
    if (slash == NULL) {
        memcpy(ancestor, ".", 2u);
    } else if (slash == ancestor) {
        ancestor[1] = '\0';
    } else {
        *slash = '\0';
    }

    for (depth = 0u; depth < 12u; ++depth) {
        vitte_main_module_trace_root(
            lookup_roots,
            lookup_roots_capacity,
            ancestor);
        if (vitte_main_module_try_candidate(
                ancestor,
                module_path,
                resolved,
                resolved_capacity)) {
            return true;
        }
        ancestor_length = strlen(ancestor);
        path_separator =
            ancestor_length != 0u &&
            ancestor[ancestor_length - 1u] == '/'
                ? ""
                : "/";
        written = snprintf(
            project_modules,
            sizeof(project_modules),
            "%s%s.vitte/modules",
            ancestor,
            path_separator);
        if (written >= 0 &&
            (size_t)written < sizeof(project_modules)) {
            vitte_main_module_trace_root(
                lookup_roots,
                lookup_roots_capacity,
                project_modules);
        }
        if (written >= 0 &&
            (size_t)written < sizeof(project_modules) &&
            vitte_main_module_try_candidate(
                project_modules,
                module_path,
                resolved,
                resolved_capacity)) {
            return true;
        }
        if (strcmp(ancestor, "/") == 0 ||
            strcmp(ancestor, ".") == 0) {
            break;
        }

        slash = strrchr(ancestor, '/');
        if (slash == NULL) {
            memcpy(ancestor, ".", 2u);
        } else if (slash == ancestor) {
            ancestor[1] = '\0';
        } else {
            *slash = '\0';
        }
    }

    return vitte_main_module_resolve_installed(
        module_path,
        resolved,
        resolved_capacity,
        lookup_roots,
        lookup_roots_capacity);
}

#define VITTE_MAIN_MODULE_SUGGESTION_COUNT 3u
#define VITTE_MAIN_MODULE_SCAN_ENTRY_LIMIT 40000u
#define VITTE_MAIN_MODULE_SCAN_DEPTH_LIMIT 6u
#define VITTE_MAIN_MODULE_NAME_MAX 256u

typedef struct vitte_main_module_suggestion {
    char name[VITTE_MAIN_MODULE_NAME_MAX];
    size_t distance;
} vitte_main_module_suggestion_t;

typedef struct vitte_main_module_scan {
    const char *requested;
    size_t requested_length;
    size_t max_distance;
    size_t entries_left;
    vitte_main_module_suggestion_t candidates[
        VITTE_MAIN_MODULE_SUGGESTION_COUNT];
    size_t candidate_count;
} vitte_main_module_scan_t;

static bool
vitte_main_module_normalize_name(
    const char *input,
    char *output,
    size_t output_capacity)
{
    size_t input_index;
    size_t output_length;

    if (input == NULL || output == NULL || output_capacity == 0u) {
        return false;
    }

    input_index = 0u;
    output_length = 0u;
    while (input[input_index] != '\0') {
        if (input[input_index] == ':' &&
            input[input_index + 1u] == ':') {
            if (output_length == 0u ||
                output_length + 2u >= output_capacity ||
                output[output_length - 1u] == ':') {
                return false;
            }
            output[output_length++] = ':';
            output[output_length++] = ':';
            input_index += 2u;
        } else if (input[input_index] == '/') {
            if (output_length == 0u ||
                output_length + 2u >= output_capacity ||
                output[output_length - 1u] == ':') {
                return false;
            }
            output[output_length++] = ':';
            output[output_length++] = ':';
            ++input_index;
        } else if (isalnum((unsigned char)input[input_index]) ||
                   input[input_index] == '_' ||
                   (unsigned char)input[input_index] >= 0x80u) {
            if (output_length + 1u >= output_capacity) {
                return false;
            }
            output[output_length++] = input[input_index++];
        } else {
            return false;
        }
    }

    if (output_length == 0u || output[output_length - 1u] == ':') {
        return false;
    }
    output[output_length] = '\0';
    return true;
}

static vitte_main_module_path_status_t
vitte_main_module_import_path_is_well_formed(
    const char *path,
    size_t *error_begin,
    size_t *error_end,
    uint32_t *error_codepoint,
    vitte_unicode_error_t *unicode_error)
{
    size_t length;
    size_t index;
    bool expect_segment;

    if (error_begin != NULL) {
        *error_begin = 0u;
    }
    if (error_end != NULL) {
        *error_end = 0u;
    }
    if (error_codepoint != NULL) {
        *error_codepoint = 0u;
    }
    if (unicode_error != NULL) {
        *unicode_error = VITTE_UNICODE_ERROR_NONE;
    }
    if (path == NULL || path[0] == '\0') {
        return VITTE_MAIN_MODULE_PATH_MALFORMED;
    }

    length = strlen(path);
    index = 0u;
    expect_segment = true;
    while (index < length) {
        if (expect_segment) {
            size_t segment_start = index;
            vitte_unicode_result_t decoded = vitte_unicode_decode_utf8(
                (const unsigned char *)path + index,
                length - index);
            if (decoded.error != VITTE_UNICODE_ERROR_NONE) {
                if (error_begin != NULL) {
                    *error_begin = index;
                }
                if (error_end != NULL) {
                    *error_end = index + (decoded.length != 0u
                        ? decoded.length : 1u);
                }
                if (unicode_error != NULL) {
                    *unicode_error = decoded.error;
                }
                return VITTE_MAIN_MODULE_PATH_INVALID_UTF8;
            }
            if (!vitte_unicode_is_identifier_start(decoded.codepoint)) {
                if (error_begin != NULL) {
                    *error_begin = index;
                }
                if (error_end != NULL) {
                    *error_end = index + decoded.length;
                }
                if (error_codepoint != NULL) {
                    *error_codepoint = decoded.codepoint;
                }
                return (decoded.codepoint >= 0x80u ||
                        vitte_unicode_is_identifier_continue(decoded.codepoint))
                    ? VITTE_MAIN_MODULE_PATH_INVALID_IDENTIFIER
                    : VITTE_MAIN_MODULE_PATH_MALFORMED;
            }
            index += decoded.length;

            while (index < length) {
                if (path[index] == '/' || path[index] == ':') {
                    break;
                }
                decoded = vitte_unicode_decode_utf8(
                    (const unsigned char *)path + index,
                    length - index);
                if (decoded.error != VITTE_UNICODE_ERROR_NONE) {
                    if (error_begin != NULL) {
                        *error_begin = index;
                    }
                    if (error_end != NULL) {
                        *error_end = index + (decoded.length != 0u
                            ? decoded.length : 1u);
                    }
                    if (unicode_error != NULL) {
                        *unicode_error = decoded.error;
                    }
                    return VITTE_MAIN_MODULE_PATH_INVALID_UTF8;
                }
                if (!vitte_unicode_is_identifier_continue(decoded.codepoint)) {
                    if (error_begin != NULL) {
                        *error_begin = index;
                    }
                    if (error_end != NULL) {
                        *error_end = index + decoded.length;
                    }
                    if (error_codepoint != NULL) {
                        *error_codepoint = decoded.codepoint;
                    }
                    return decoded.codepoint >= 0x80u
                        ? VITTE_MAIN_MODULE_PATH_INVALID_IDENTIFIER
                        : VITTE_MAIN_MODULE_PATH_MALFORMED;
                }
                index += decoded.length;
            }

            {
                vitte_token_kind_t keyword_kind = vitte_lexer_keyword_kind(
                    path + segment_start,
                    index - segment_start);
                bool permitted_reserved = keyword_kind ==
                        VITTE_TOKEN_KW_RESERVED &&
                    ((index - segment_start == 8u &&
                      memcmp(path + segment_start, "compiler", 8u) == 0) ||
                     (index - segment_start == 10u &&
                      memcmp(path + segment_start, "diagnostic", 10u) == 0));
                if (keyword_kind != VITTE_TOKEN_IDENTIFIER &&
                    !permitted_reserved) {
                    if (error_begin != NULL) {
                        *error_begin = segment_start;
                    }
                    if (error_end != NULL) {
                        *error_end = index;
                    }
                    return VITTE_MAIN_MODULE_PATH_MALFORMED;
                }
            }
            expect_segment = false;
            continue;
        }

        if (path[index] == '/') {
            size_t separator_start = index++;
            expect_segment = true;
            if (index == length) {
                if (error_begin != NULL) {
                    *error_begin = separator_start;
                }
                if (error_end != NULL) {
                    *error_end = index;
                }
                return VITTE_MAIN_MODULE_PATH_MALFORMED;
            }
        } else if (path[index] == ':' && index + 1u < length &&
                   path[index + 1u] == ':') {
            size_t separator_start = index;
            index += 2u;
            expect_segment = true;
            if (index == length) {
                if (error_begin != NULL) {
                    *error_begin = separator_start;
                }
                if (error_end != NULL) {
                    *error_end = index;
                }
                return VITTE_MAIN_MODULE_PATH_MALFORMED;
            }
        } else {
            if (error_begin != NULL) {
                *error_begin = index;
            }
            if (error_end != NULL) {
                *error_end = index + 1u;
            }
            return VITTE_MAIN_MODULE_PATH_MALFORMED;
        }
    }
    return expect_segment
        ? VITTE_MAIN_MODULE_PATH_MALFORMED
        : VITTE_MAIN_MODULE_PATH_VALID;
}

static bool
vitte_main_module_package_dependency(
    const char *importer_path,
    const char *package_name,
    char *version,
    size_t version_capacity)
{
    char directory[VITTE_MAIN_MODULE_PATH_MAX];
    char manifest_path[VITTE_MAIN_MODULE_PATH_MAX];
    char working_directory[VITTE_MAIN_MODULE_PATH_MAX];
    size_t depth;
    int written;

    if (importer_path == NULL || package_name == NULL ||
        package_name[0] == '\0' || version == NULL ||
        version_capacity == 0u) {
        return false;
    }
    version[0] = '\0';
    if (vitte_filesystem_path_is_absolute(importer_path)) {
        written = snprintf(
            directory,
            sizeof(directory),
            "%s",
            importer_path);
    } else if (vitte_platform_getcwd(
                   working_directory,
                   sizeof(working_directory))) {
        written = snprintf(
            directory,
            sizeof(directory),
            "%s/%s",
            working_directory,
            importer_path);
    } else {
        return false;
    }
    if (written < 0 || (size_t)written >= sizeof(directory)) {
        return false;
    }
    {
        char *slash = strrchr(directory, '/');
        if (slash == NULL) {
            memcpy(directory, ".", 2u);
        } else if (slash == directory) {
            directory[1] = '\0';
        } else {
            *slash = '\0';
        }
    }

    for (depth = 0u; depth < 64u; ++depth) {
        vitte_main_file_t manifest;
        int manifest_written = snprintf(
            manifest_path,
            sizeof(manifest_path),
            "%s/vitte.packages.toml",
            directory);
        if (manifest_written >= 0 &&
            (size_t)manifest_written < sizeof(manifest_path) &&
            vitte_filesystem_is_file(manifest_path)) {
            size_t line_start = 0u;
            memset(&manifest, 0, sizeof(manifest));
            if (vitte_main_read_file(manifest_path, &manifest) &&
                manifest.data != NULL) {
                while (line_start < manifest.length) {
                    size_t line_end = line_start;
                    size_t cursor;
                    size_t name_length = strlen(package_name);
                    while (line_end < manifest.length &&
                           manifest.data[line_end] != '\n') {
                        ++line_end;
                    }
                    cursor = line_start;
                    while (cursor < line_end &&
                           (manifest.data[cursor] == ' ' ||
                            manifest.data[cursor] == '\t' ||
                            manifest.data[cursor] == '\r')) {
                        ++cursor;
                    }
                    if (line_end - cursor >= 11u + name_length &&
                        memcmp(manifest.data + cursor, "dependency.", 11u) == 0 &&
                        memcmp(manifest.data + cursor + 11u,
                               package_name,
                               name_length) == 0) {
                        size_t after_name = cursor + 11u + name_length;
                        while (after_name < line_end &&
                               (manifest.data[after_name] == ' ' ||
                                manifest.data[after_name] == '\t')) {
                            ++after_name;
                        }
                        if (after_name < line_end &&
                            manifest.data[after_name] == '=') {
                            size_t value_start;
                            size_t value_end;
                            ++after_name;
                            while (after_name < line_end &&
                                   (manifest.data[after_name] == ' ' ||
                                    manifest.data[after_name] == '\t')) {
                                ++after_name;
                            }
                            if (after_name < line_end &&
                                manifest.data[after_name] == '"') {
                                value_start = ++after_name;
                                value_end = value_start;
                                while (value_end < line_end &&
                                       manifest.data[value_end] != '"') {
                                    ++value_end;
                                }
                                if (value_end - value_start < version_capacity) {
                                    memcpy(version,
                                           manifest.data + value_start,
                                           value_end - value_start);
                                    version[value_end - value_start] = '\0';
                                }
                            }
                            free(manifest.data);
                            return true;
                        }
                    }
                    line_start = line_end < manifest.length
                        ? line_end + 1u : line_end;
                }
                free(manifest.data);
            } else {
                free(manifest.data);
            }
        }
        if (strcmp(directory, "/") == 0 || strcmp(directory, ".") == 0) {
            break;
        }
        {
            char *slash = strrchr(directory, '/');
            if (slash == NULL) {
                memcpy(directory, ".", 2u);
            } else if (slash == directory) {
                directory[1] = '\0';
            } else {
                *slash = '\0';
            }
        }
    }
    return false;
}

static bool
vitte_main_module_package_root_at(
    const char *root,
    const char *package_name)
{
    char package_root[VITTE_MAIN_MODULE_PATH_MAX];
    int written;

    if (root == NULL || root[0] == '\0' || package_name == NULL ||
        package_name[0] == '\0') {
        return false;
    }
    written = snprintf(
        package_root,
        sizeof(package_root),
        "%s/%s",
        root,
        package_name);
    return written >= 0 && (size_t)written < sizeof(package_root) &&
        vitte_filesystem_is_directory(package_root);
}

static bool
vitte_main_module_package_exists(
    const char *importer_path,
    const char *package_name)
{
    char directory[VITTE_MAIN_MODULE_PATH_MAX];
    char root[VITTE_MAIN_MODULE_PATH_MAX];
    char package_modules[VITTE_MAIN_MODULE_PATH_MAX];
    char executable_directory[VITTE_MAIN_MODULE_PATH_MAX];
    char *slash;
    const char *configured_paths;
    const char *cursor;
    size_t depth;
    int written;

    if (importer_path == NULL || package_name == NULL) {
        return false;
    }
    if (vitte_filesystem_path_is_absolute(importer_path)) {
        written = snprintf(directory, sizeof(directory), "%s", importer_path);
    } else {
        char working_directory[VITTE_MAIN_MODULE_PATH_MAX];
        if (!vitte_platform_getcwd(working_directory, sizeof(working_directory))) {
            return false;
        }
        written = snprintf(directory, sizeof(directory), "%s/%s",
                           working_directory, importer_path);
    }
    if (written < 0 || (size_t)written >= sizeof(directory)) {
        return false;
    }
    slash = strrchr(directory, '/');
    if (slash == NULL) {
        memcpy(directory, ".", 2u);
    } else if (slash == directory) {
        directory[1] = '\0';
    } else {
        *slash = '\0';
    }

    for (depth = 0u; depth < 12u; ++depth) {
        if (vitte_main_module_package_root_at(directory, package_name)) {
            return true;
        }
        written = snprintf(package_modules, sizeof(package_modules),
                           "%s/.vitte/modules", directory);
        if (written >= 0 && (size_t)written < sizeof(package_modules) &&
            vitte_main_module_package_root_at(package_modules, package_name)) {
            return true;
        }
        if (strcmp(directory, "/") == 0 || strcmp(directory, ".") == 0) {
            break;
        }
        slash = strrchr(directory, '/');
        if (slash == NULL) {
            memcpy(directory, ".", 2u);
        } else if (slash == directory) {
            directory[1] = '\0';
        } else {
            *slash = '\0';
        }
    }

    configured_paths = getenv("VITTE_MODULE_PATH");
    cursor = configured_paths;
    while (cursor != NULL && *cursor != '\0') {
        const char *separator = strchr(
            cursor, vitte_platform_path_list_separator());
        size_t length = separator != NULL
            ? (size_t)(separator - cursor) : strlen(cursor);
        if (length != 0u && length < sizeof(root)) {
            memcpy(root, cursor, length);
            root[length] = '\0';
            if (vitte_main_module_package_root_at(root, package_name)) {
                return true;
            }
        }
        if (separator == NULL) {
            break;
        }
        cursor = separator + 1;
    }

#if defined(_WIN32)
    {
        const char *user_data = getenv("LOCALAPPDATA");
        if (user_data != NULL && user_data[0] != '\0') {
            written = snprintf(root, sizeof(root), "%s/Vitte/modules", user_data);
        } else {
            written = -1;
        }
    }
#else
    {
        const char *user_data = getenv("XDG_DATA_HOME");
        if (user_data != NULL && user_data[0] != '\0') {
            written = snprintf(root, sizeof(root), "%s/vitte/modules", user_data);
        } else {
            user_data = getenv("HOME");
            written = user_data != NULL && user_data[0] != '\0'
                ? snprintf(root, sizeof(root), "%s/.local/share/vitte/modules",
                           user_data)
                : -1;
        }
    }
#endif
    if (written >= 0 && (size_t)written < sizeof(root) &&
        vitte_main_module_package_root_at(root, package_name)) {
        return true;
    }

    if (vitte_main_executable_path[0] != '\0' &&
        strlen(vitte_main_executable_path) < sizeof(executable_directory)) {
        memcpy(executable_directory, vitte_main_executable_path,
               strlen(vitte_main_executable_path) + 1u);
        slash = strrchr(executable_directory, '/');
        if (slash != NULL) {
            *slash = '\0';
            slash = strrchr(executable_directory, '/');
            if (slash != NULL &&
                (strcmp(slash + 1, "bin") == 0 ||
                 strcmp(slash + 1, "sbin") == 0)) {
                *slash = '\0';
            }
            written = snprintf(root, sizeof(root),
                               "%s/share/vitte/modules", executable_directory);
            if (written >= 0 && (size_t)written < sizeof(root) &&
                vitte_main_module_package_root_at(root, package_name)) {
                return true;
            }
        }
    }
    return vitte_main_module_package_root_at(
               "/usr/local/share/vitte/modules", package_name) ||
        vitte_main_module_package_root_at(
               "/usr/share/vitte/modules", package_name);
}

static bool
vitte_main_module_declared_name(
    const unsigned char *source,
    size_t source_length,
    char *name,
    size_t name_capacity,
    size_t *name_begin,
    size_t *name_end)
{
    size_t header_begin;
    size_t header_end;
    size_t cursor;
    size_t end;

    if (source == NULL || name == NULL || name_capacity == 0u ||
        name_begin == NULL || name_end == NULL ||
        !vitte_main_module_root_header_range(
            source, source_length, &header_begin, &header_end)) {
        return false;
    }
    cursor = header_begin;
    while (cursor < header_end &&
           (source[cursor] == ' ' || source[cursor] == '\t' ||
            source[cursor] == '\r')) {
        ++cursor;
    }
    if (header_end - cursor < 5u ||
        memcmp(source + cursor, "space", 5u) != 0) {
        return false;
    }
    cursor += 5u;
    while (cursor < header_end &&
           (source[cursor] == ' ' || source[cursor] == '\t')) {
        ++cursor;
    }
    *name_begin = cursor;
    end = cursor;
    while (end < header_end && source[end] != ';' &&
           source[end] != '{' && source[end] != '\n' &&
           source[end] != '\r' && source[end] != ' ' &&
           source[end] != '\t') {
        ++end;
    }
    if (end == cursor || end - cursor >= name_capacity) {
        return false;
    }
    cursor = end;
    while (cursor < header_end &&
           (source[cursor] == ' ' || source[cursor] == '\t')) {
        ++cursor;
    }
    if (cursor >= header_end ||
        (source[cursor] != ';' && source[cursor] != '{')) {
        return false;
    }
    memcpy(name, source + *name_begin, end - *name_begin);
    name[end - *name_begin] = '\0';
    *name_end = end;
    return true;
}

static bool
vitte_main_module_validate_import_name(
    const char *importer_path,
    const unsigned char *importer_source,
    size_t importer_source_length,
    uint32_t importer_file_id,
    size_t import_path_begin,
    size_t import_path_end,
    const char *resolved_path,
    const char *requested_name)
{
    vitte_main_file_t imported;
    char declared_name[VITTE_MAIN_MODULE_NAME_MAX];
    char normalized_declared[VITTE_MAIN_MODULE_NAME_MAX];
    char normalized_requested[VITTE_MAIN_MODULE_NAME_MAX];
    size_t declared_begin;
    size_t declared_end;
    if (importer_path == NULL || importer_source == NULL ||
        resolved_path == NULL || requested_name == NULL) {
        return true;
    }
    memset(&imported, 0, sizeof(imported));
    if (!vitte_main_read_file(resolved_path, &imported) ||
        imported.data == NULL) {
        free(imported.data);
        return true;
    }
    if (!vitte_main_module_declared_name(
            imported.data,
            imported.length,
            declared_name,
            sizeof(declared_name),
            &declared_begin,
            &declared_end) ||
        !vitte_main_module_normalize_name(
            declared_name,
            normalized_declared,
            sizeof(normalized_declared)) ||
        !vitte_main_module_normalize_name(
            requested_name,
            normalized_requested,
            sizeof(normalized_requested)) ||
        strcmp(normalized_declared, normalized_requested) == 0) {
        free(imported.data);
        return true;
    }

    {
        char details[VITTE_CLI_DIAGNOSTIC_DETAILS_CAPACITY];
        char primary_label[512];
        vitte_import_span_t primary_span;
        vitte_import_span_t related_span;
        vitte_cli_diagnostic_location_t related;
        int written;

        written = snprintf(
            details,
            sizeof(details),
            "the resolved source declares module `%s`, but this import requests `%s`",
            declared_name,
            requested_name);
        if (written < 0 || (size_t)written >= sizeof(details)) {
            free(imported.data);
            return false;
        }
        (void)snprintf(
            primary_label,
            sizeof(primary_label),
            "declares `%s`, but the import expects `%s`",
            declared_name,
            requested_name);
        primary_span = vitte_main_import_span_make(
            0u,
            imported.data,
            imported.length,
            declared_begin,
            declared_end);
        related_span = vitte_main_import_span_make(
            importer_file_id,
            importer_source,
            importer_source_length,
            import_path_begin,
            import_path_end);
        memset(&related, 0, sizeof(related));
        related.kind = VITTE_CLI_DIAGNOSTIC_LOCATION_RELATED;
        related.label = "import requests this module path";
        related.span.valid = related_span.valid;
        related.span.file_id = related_span.file_id;
        related.span.source_id = VITTE_SOURCE_INVALID_ID;
        related.span.source_name = importer_path;
        related.span.start_offset = related_span.begin;
        related.span.end_offset = related_span.end;
        related.span.start_line = related_span.line != 0u
            ? related_span.line - 1u : 0u;
        related.span.start_column = related_span.column != 0u
            ? related_span.column - 1u : 0u;
        related.span.end_line = related_span.end_line != 0u
            ? related_span.end_line - 1u : related.span.start_line;
        related.span.end_column = related_span.end_column != 0u
            ? related_span.end_column - 1u : related.span.start_column;
        vitte_main_report_process_error_span_related(
            "VITTE_IMPORT_E_MODULE_PATH_MISMATCH",
            "module name does not match import path",
            details,
            resolved_path,
            &primary_span,
            "module import path mismatch",
            requested_name,
            declared_name,
            "import graph",
            primary_label,
            &related,
            1u);
    }
    free(imported.data);
    return false;
}

static void
vitte_main_report_invalid_import_path(
    const char *source_name,
    const unsigned char *source,
    size_t source_length,
    uint32_t file_id,
    size_t begin,
    size_t end,
    const char *path)
{
    vitte_import_span_t span = vitte_main_import_span_make(
        file_id, source, source_length, begin, end);
    char label[512];
    (void)snprintf(label, sizeof(label), "invalid module path `%s`",
                   path != NULL ? path : "<path>");
    vitte_main_report_process_error_span_related(
        "VITTE_PARSER_E_INVALID_MODULE_PATH",
        "malformed module path",
        "Use identifier path segments separated by `::` or `/`; remove leading, repeated or trailing separators and unsupported punctuation.",
        source_name,
        &span,
        "use declaration",
        path,
        NULL,
        "import graph",
        label,
        NULL,
        0u);
}

static void
vitte_main_report_invalid_import_unicode(
    const char *source_name,
    const unsigned char *source,
    size_t source_length,
    uint32_t file_id,
    size_t begin,
    size_t end,
    vitte_main_module_path_status_t status,
    uint32_t codepoint,
    vitte_unicode_error_t unicode_error)
{
    vitte_import_span_t span = vitte_main_import_span_make(
        file_id, source, source_length, begin, end);
    char details[512];
    char label[128];
    const char *code;
    const char *title;

    if (status == VITTE_MAIN_MODULE_PATH_INVALID_UTF8) {
        const char *encoding_name = vitte_unicode_error_name(unicode_error);
        code = "VITTE_LEXER_E_INVALID_UNICODE";
        title = "invalid Unicode sequence";
        (void)snprintf(
            details,
            sizeof(details),
            "malformed UTF-8 in module path (%s); offending byte range [%zu, %zu)",
            encoding_name != NULL ? encoding_name : "encoding error",
            begin,
            end);
        (void)snprintf(label, sizeof(label), "malformed UTF-8 sequence");
    } else {
        code = "VITTE_LEXER_E_INVALID_IDENTIFIER";
        title = "invalid identifier";
        (void)snprintf(
            details,
            sizeof(details),
            "Unicode scalar U+%04X is not allowed in this module-path identifier; offending byte range [%zu, %zu)",
            (unsigned int)codepoint,
            begin,
            end);
        (void)snprintf(
            label,
            sizeof(label),
            "invalid identifier scalar U+%04X",
            (unsigned int)codepoint);
    }

    vitte_main_report_process_error_span_related(
        code,
        title,
        details,
        source_name,
        &span,
        "module path",
        NULL,
        NULL,
        "lexer",
        label,
        NULL,
        0u);
}

static void
vitte_main_report_empty_use_path(
    const char *source_name,
    const unsigned char *source,
    size_t source_length,
    uint32_t file_id,
    size_t directive_begin,
    size_t directive_end)
{
    vitte_import_span_t span = vitte_main_import_span_make(
        file_id, source, source_length, directive_begin, directive_end);
    vitte_main_report_process_error_span_related(
        "VITTE_PARSER_E_INVALID_USE_DECLARATION",
        "malformed use declaration",
        "A `use` declaration needs a module path after the keyword.",
        source_name,
        &span,
        "use declaration",
        NULL,
        NULL,
        "import graph",
        "expected a module path",
        NULL,
        0u);
}

static size_t
vitte_main_module_bounded_distance(
    const char *left,
    size_t left_length,
    const char *right,
    size_t right_length,
    size_t limit)
{
    size_t previous_previous[VITTE_MAIN_MODULE_NAME_MAX];
    size_t previous[VITTE_MAIN_MODULE_NAME_MAX];
    size_t current[VITTE_MAIN_MODULE_NAME_MAX];
    size_t index;

    if (left_length >= VITTE_MAIN_MODULE_NAME_MAX ||
        right_length >= VITTE_MAIN_MODULE_NAME_MAX) {
        return limit + 1u;
    }
    if (left_length > right_length + limit ||
        right_length > left_length + limit) {
        return limit + 1u;
    }

    for (index = 0u; index <= right_length; ++index) {
        previous[index] = index <= limit ? index : limit + 1u;
        previous_previous[index] = limit + 1u;
    }

    for (index = 1u; index <= left_length; ++index) {
        size_t column;
        size_t first_column;
        size_t last_column;
        size_t row_minimum;

        first_column = index > limit ? index - limit : 1u;
        last_column = index + limit < right_length
            ? index + limit
            : right_length;
        current[0] = index <= limit ? index : limit + 1u;
        for (column = 1u; column < first_column; ++column) {
            current[column] = limit + 1u;
        }
        row_minimum = limit + 1u;
        for (column = first_column; column <= last_column; ++column) {
            size_t deletion;
            size_t insertion;
            size_t substitution;
            size_t transposition;
            size_t best;

            deletion = previous[column] < limit + 1u
                ? previous[column] + 1u
                : limit + 1u;
            insertion = current[column - 1u] < limit + 1u
                ? current[column - 1u] + 1u
                : limit + 1u;
            substitution = previous[column - 1u] +
                (left[index - 1u] == right[column - 1u] ? 0u : 1u);
            transposition = limit + 1u;
            if (index > 1u && column > 1u &&
                left[index - 1u] == right[column - 2u] &&
                left[index - 2u] == right[column - 1u] &&
                previous_previous[column - 2u] < limit + 1u) {
                transposition = previous_previous[column - 2u] + 1u;
            }
            best = deletion < insertion ? deletion : insertion;
            if (substitution < best) {
                best = substitution;
            }
            if (transposition < best) {
                best = transposition;
            }
            current[column] = best;
            if (best < row_minimum) {
                row_minimum = best;
            }
        }
        for (column = last_column + 1u;
             column <= right_length;
             ++column) {
            current[column] = limit + 1u;
        }
        if (row_minimum > limit) {
            return limit + 1u;
        }
        memcpy(
            previous_previous,
            previous,
            (right_length + 1u) * sizeof(*previous_previous));
        memcpy(previous, current, (right_length + 1u) * sizeof(*previous));
    }

    return previous[right_length] <= limit
        ? previous[right_length]
        : limit + 1u;
}

static void
vitte_main_module_consider_suggestion(
    vitte_main_module_scan_t *scan,
    const char *candidate)
{
    size_t candidate_length;
    size_t distance;
    size_t position;
    size_t index;

    if (scan == NULL || candidate == NULL) {
        return;
    }
    candidate_length = strlen(candidate);
    if (candidate_length == 0u ||
        candidate_length >= VITTE_MAIN_MODULE_NAME_MAX) {
        return;
    }
    for (index = 0u; index < scan->candidate_count; ++index) {
        if (strcmp(scan->candidates[index].name, candidate) == 0) {
            return;
        }
    }

    distance = vitte_main_module_bounded_distance(
        scan->requested,
        scan->requested_length,
        candidate,
        candidate_length,
        scan->max_distance);
    if (distance > scan->max_distance) {
        return;
    }

    position = 0u;
    while (position < scan->candidate_count &&
           (scan->candidates[position].distance < distance ||
            (scan->candidates[position].distance == distance &&
             strcmp(scan->candidates[position].name, candidate) < 0))) {
        ++position;
    }
    if (position >= VITTE_MAIN_MODULE_SUGGESTION_COUNT) {
        return;
    }
    if (scan->candidate_count < VITTE_MAIN_MODULE_SUGGESTION_COUNT) {
        ++scan->candidate_count;
    }
    for (index = scan->candidate_count - 1u;
         index > position;
         --index) {
        scan->candidates[index] = scan->candidates[index - 1u];
    }
    memcpy(scan->candidates[position].name, candidate, candidate_length + 1u);
    scan->candidates[position].distance = distance;
}

static void
vitte_main_module_consider_file(
    vitte_main_module_scan_t *scan,
    const char *relative_path)
{
    char candidate[VITTE_MAIN_MODULE_NAME_MAX];
    size_t length;
    size_t suffix_length;
    size_t index;

    if (scan == NULL || relative_path == NULL) {
        return;
    }
    length = strlen(relative_path);
    suffix_length = 0u;
    if (length > 4u && strcmp(relative_path + length - 4u, ".vit") == 0) {
        suffix_length = 4u;
    } else if (length > 6u &&
               strcmp(relative_path + length - 6u, ".vitte") == 0) {
        suffix_length = 6u;
    } else {
        return;
    }
    length -= suffix_length;
    if (length >= sizeof(candidate)) {
        return;
    }
    memcpy(candidate, relative_path, length);
    candidate[length] = '\0';
    if (length >= 4u && strcmp(candidate + length - 4u, "/mod") == 0) {
        length -= 4u;
        candidate[length] = '\0';
    } else if (length == 3u && strcmp(candidate, "mod") == 0) {
        return;
    }
    if (length == 0u || length >= sizeof(candidate)) {
        return;
    }
    for (index = 0u; index < length; ++index) {
        if (candidate[index] == '/') {
            if (index == 0u || index + 1u == length ||
                candidate[index - 1u] == '/' ||
                length + 1u >= sizeof(candidate)) {
                return;
            }
            candidate[index] = ':';
            if (index + 1u >= sizeof(candidate) - 1u) {
                return;
            }
            memmove(
                candidate + index + 2u,
                candidate + index + 1u,
                length - index);
            candidate[index + 1u] = ':';
            ++length;
            ++index;
        } else if (!isalnum((unsigned char)candidate[index]) &&
                   candidate[index] != '_') {
            return;
        }
    }
    candidate[length] = '\0';
    vitte_main_module_consider_suggestion(scan, candidate);
}

static bool
vitte_main_module_scan_directory(
    vitte_main_module_scan_t *scan,
    const char *root,
    const char *relative_directory,
    size_t depth)
{
    char *directory_path;
    vitte_filesystem_directory_list_t directory;
    size_t index;
    vitte_filesystem_result_t result;

    if (scan == NULL || root == NULL ||
        depth > VITTE_MAIN_MODULE_SCAN_DEPTH_LIMIT ||
        scan->entries_left == 0u) {
        return true;
    }
    directory_path = NULL;
    result = vitte_filesystem_join(
        root,
        relative_directory != NULL ? relative_directory : "",
        &directory_path);
    if (result.error != VITTE_FILESYSTEM_ERROR_NONE ||
        directory_path == NULL) {
        return true;
    }
    memset(&directory, 0, sizeof(directory));
    result = vitte_filesystem_list_directory(
        directory_path,
        scan->entries_left,
        &directory);
    free(directory_path);
    if (result.error != VITTE_FILESYSTEM_ERROR_NONE) {
        return true;
    }

    for (index = 0u;
         index < directory.count && scan->entries_left != 0u;
         ++index) {
        const vitte_filesystem_directory_entry_t *entry =
            &directory.entries[index];
        char *child_relative = NULL;

        if (entry->name == NULL || entry->name[0] == '.') {
            continue;
        }
        --scan->entries_left;
        result = vitte_filesystem_join(
            relative_directory != NULL ? relative_directory : "",
            entry->name,
            &child_relative);
        if (result.error != VITTE_FILESYSTEM_ERROR_NONE ||
            child_relative == NULL) {
            continue;
        }
        if (entry->kind == VITTE_FILESYSTEM_KIND_DIRECTORY) {
            if (strcmp(entry->name, "target") != 0 &&
                strcmp(entry->name, "build") != 0 &&
                strcmp(entry->name, "dist") != 0 &&
                strcmp(entry->name, "node_modules") != 0) {
                (void)vitte_main_module_scan_directory(
                    scan,
                    root,
                    child_relative,
                    depth + 1u);
            }
        } else if (entry->kind == VITTE_FILESYSTEM_KIND_REGULAR) {
            vitte_main_module_consider_file(scan, child_relative);
        }
        free(child_relative);
    }

    vitte_filesystem_directory_list_destroy(&directory);
    return true;
}

static void
vitte_main_module_scan_root(
    vitte_main_module_scan_t *scan,
    const char *root)
{
    if (scan == NULL || root == NULL || root[0] == '\0' ||
        scan->entries_left == 0u) {
        return;
    }
    (void)vitte_main_module_scan_directory(scan, root, "", 0u);
}

static void
vitte_main_report_missing_module(
    const char *importer_path,
    const char *requested,
    const vitte_import_span_t *span,
    const char *lookup_roots)
{
    vitte_main_module_scan_t scan;
    char normalized_requested[VITTE_MAIN_MODULE_NAME_MAX];
    char requested_package[128];
    char package_version[128];
    char absolute_importer[VITTE_MAIN_MODULE_PATH_MAX];
    char root[VITTE_MAIN_MODULE_PATH_MAX];
    char installed_root[VITTE_MAIN_MODULE_PATH_MAX];
    char executable_directory[VITTE_MAIN_MODULE_PATH_MAX];
    char local_modules[VITTE_MAIN_MODULE_PATH_MAX];
    char global_modules[VITTE_MAIN_MODULE_PATH_MAX];
    char details[VITTE_CLI_DIAGNOSTIC_DETAILS_CAPACITY];
    char primary_label[512];
    char *slash;
    const char *configured_paths;
    const char *cursor;
    size_t depth;
    size_t package_length;
    const char *package_separator;
    bool package_declared;
    int written;

    if (importer_path == NULL || requested == NULL ||
        !vitte_main_module_normalize_name(
            requested,
            normalized_requested,
            sizeof(normalized_requested))) {
        vitte_main_error_argument("cannot resolve imported module", requested);
        return;
    }

    package_separator = strstr(requested, "::");
    {
        const char *slash_separator = strchr(requested, '/');
        if (slash_separator != NULL &&
            (package_separator == NULL || slash_separator < package_separator)) {
            package_separator = slash_separator;
        }
    }
    package_length = package_separator != NULL
        ? (size_t)(package_separator - requested)
        : strlen(requested);
    package_declared = false;
    requested_package[0] = '\0';
    package_version[0] = '\0';
    if (package_length != 0u && package_length < sizeof(requested_package)) {
        memcpy(requested_package, requested, package_length);
        requested_package[package_length] = '\0';
        package_declared = vitte_main_module_package_dependency(
            importer_path,
            requested_package,
            package_version,
            sizeof(package_version));
        if (package_declared &&
            vitte_main_module_package_exists(
                importer_path,
                requested_package)) {
            package_declared = false;
        }
    }

    memset(&scan, 0, sizeof(scan));
    scan.requested = normalized_requested;
    scan.requested_length = strlen(normalized_requested);
    scan.max_distance = scan.requested_length <= 8u
        ? 1u
        : scan.requested_length <= 18u
            ? 2u
            : 3u;
    scan.entries_left = VITTE_MAIN_MODULE_SCAN_ENTRY_LIMIT;

    if (vitte_filesystem_path_is_absolute(importer_path)) {
        written = snprintf(
            absolute_importer,
            sizeof(absolute_importer),
            "%s",
            importer_path);
    } else {
        char working_directory[VITTE_MAIN_MODULE_PATH_MAX];

        if (!vitte_platform_getcwd(
                working_directory,
                sizeof(working_directory))) {
            vitte_main_error_argument("cannot resolve imported module", requested);
            return;
        }
        written = snprintf(
            absolute_importer,
            sizeof(absolute_importer),
            "%s/%s",
            working_directory,
            importer_path);
    }
    if (written < 0 || (size_t)written >= sizeof(absolute_importer)) {
        vitte_main_error_argument("cannot resolve imported module", requested);
        return;
    }

    slash = strrchr(absolute_importer, '/');
    if (slash == NULL) {
        memcpy(root, ".", 2u);
    } else if (slash == absolute_importer) {
        absolute_importer[1] = '\0';
        memcpy(root, absolute_importer, 2u);
    } else {
        *slash = '\0';
        memcpy(root, absolute_importer, strlen(absolute_importer) + 1u);
    }
    for (depth = 0u; depth < 12u && scan.entries_left != 0u; ++depth) {
        char *parent;
        written = snprintf(local_modules, sizeof(local_modules),
                           "%s/.vitte/modules", root);
        if (written >= 0 && (size_t)written < sizeof(local_modules)) {
            vitte_main_module_scan_root(&scan, local_modules);
        }

        vitte_main_module_scan_root(&scan, root);
        if (strcmp(root, "/") == 0 || strcmp(root, ".") == 0) {
            break;
        }
        parent = strrchr(root, '/');
        if (parent == NULL) {
            memcpy(root, ".", 2u);
        } else if (parent == root) {
            root[1] = '\0';
        } else {
            *parent = '\0';
        }
    }

    configured_paths = getenv("VITTE_MODULE_PATH");
    cursor = configured_paths;
    while (cursor != NULL && *cursor != '\0' && scan.entries_left != 0u) {
        const char *separator;
        size_t root_length;

        separator = strchr(cursor, vitte_platform_path_list_separator());
        root_length = separator != NULL
            ? (size_t)(separator - cursor)
            : strlen(cursor);
        if (root_length != 0u && root_length < sizeof(root)) {
            memcpy(root, cursor, root_length);
            root[root_length] = '\0';
            vitte_main_module_scan_root(&scan, root);
        }
        if (separator == NULL) {
            break;
        }
        cursor = separator + 1u;
    }
#if defined(_WIN32)
    {
        const char *user_data = getenv("LOCALAPPDATA");
        if (user_data != NULL && user_data[0] != '\0') {
            written = snprintf(global_modules, sizeof(global_modules),
                               "%s/Vitte/modules", user_data);
            if (written >= 0 && (size_t)written < sizeof(global_modules)) {
                vitte_main_module_scan_root(&scan, global_modules);
            }
        }
    }
#else
    {
        const char *user_data = getenv("XDG_DATA_HOME");
        if (user_data != NULL && user_data[0] != '\0') {
            written = snprintf(global_modules, sizeof(global_modules),
                               "%s/vitte/modules", user_data);
        } else {
            user_data = getenv("HOME");
            written = user_data != NULL && user_data[0] != '\0'
                ? snprintf(global_modules, sizeof(global_modules),
                           "%s/.local/share/vitte/modules", user_data)
                : -1;
        }
        if (written >= 0 && (size_t)written < sizeof(global_modules)) {
            vitte_main_module_scan_root(&scan, global_modules);
        }
    }
#endif
    if (vitte_main_executable_path[0] != '\0' &&
        strlen(vitte_main_executable_path) < sizeof(executable_directory)) {
        char *executable_slash;

        memcpy(
            executable_directory,
            vitte_main_executable_path,
            strlen(vitte_main_executable_path) + 1u);
        executable_slash = strrchr(executable_directory, '/');
        if (executable_slash != NULL) {
            *executable_slash = '\0';
            executable_slash = strrchr(executable_directory, '/');
            if (executable_slash != NULL &&
                (strcmp(executable_slash + 1, "bin") == 0 ||
                 strcmp(executable_slash + 1, "sbin") == 0)) {
                *executable_slash = '\0';
            }
            written = snprintf(
                installed_root,
                sizeof(installed_root),
                "%s/share/vitte/modules",
                executable_directory);
            if (written >= 0 &&
                (size_t)written < sizeof(installed_root)) {
                vitte_main_module_scan_root(&scan, installed_root);
            }
        }
    }
    vitte_main_module_scan_root(&scan, "/usr/local/share/vitte/modules");
    vitte_main_module_scan_root(&scan, "/usr/share/vitte/modules");

    if (package_declared && package_version[0] != '\0') {
        written = snprintf(
            details,
            sizeof(details),
            "declared package `%s` version `%s` is unavailable; requested module path `%s` from `%s`; candidate module paths: `<module path>.vit`, `<module path>.vitte`, `<module path>/mod.vit`, and `<module path>/mod.vitte`; package directories checked as `<root>/<package>`; search roots checked in order: %s",
            requested_package,
            package_version,
            requested,
            importer_path,
            lookup_roots != NULL && lookup_roots[0] != '\0'
                ? lookup_roots : "none");
    } else if (package_declared) {
        written = snprintf(
            details,
            sizeof(details),
            "declared package `%s` is unavailable; requested module path `%s` from `%s`; candidate module paths: `<module path>.vit`, `<module path>.vitte`, `<module path>/mod.vit`, and `<module path>/mod.vitte`; package directories checked as `<root>/<package>`; search roots checked in order: %s",
            requested_package,
            requested,
            importer_path,
            lookup_roots != NULL && lookup_roots[0] != '\0'
                ? lookup_roots : "none");
    } else {
        written = snprintf(
            details,
            sizeof(details),
            "module path `%s` imported from `%s` was not found; candidate paths under each root: `<module path>.vit`, `<module path>.vitte`, `<module path>/mod.vit`, and `<module path>/mod.vitte`; when a package manifest matches the first path segment, the resolver also checks the remaining path under that package source root; search roots checked in order: %s",
            requested,
            importer_path,
            lookup_roots != NULL && lookup_roots[0] != '\0'
                ? lookup_roots : "none");
    }
    if (written < 0 || (size_t)written >= sizeof(details)) {
        (void)snprintf(details, sizeof(details),
                       "module path `%s` from `%s` could not be resolved; candidate suffixes: `.vit`, `.vitte`, `/mod.vit`, `/mod.vitte`; search roots: %s",
                       requested, importer_path,
                       lookup_roots != NULL && lookup_roots[0] != '\0'
                           ? lookup_roots : "none");
    }
    if (scan.candidate_count != 0u) {
        size_t used = strlen(details);
        size_t candidate_index;
        written = snprintf(details + used, sizeof(details) - used,
                           "; nearby module paths: ");
        if (written > 0 && (size_t)written < sizeof(details) - used) {
            used += (size_t)written;
        for (candidate_index = 0u;
             candidate_index < scan.candidate_count;
             ++candidate_index) {
            written = snprintf(
                details + used,
                sizeof(details) - used,
                "%s`%s`",
                candidate_index == 0u ? "" : ", ",
                scan.candidates[candidate_index].name);
            if (written < 0 || (size_t)written >= sizeof(details) - used) {
                break;
            }
            used += (size_t)written;
        }
        }
    }
    (void)snprintf(
        primary_label,
        sizeof(primary_label),
        package_declared
            ? "package `%s` source directory was not found"
            : "module path `%s` was not found",
        package_declared ? requested_package : requested);
    vitte_main_report_process_error_span_related(
        package_declared
            ? "VITTE_IMPORT_E_PACKAGE_NOT_FOUND"
            : "VITTE_IMPORT_E_MODULE_NOT_FOUND",
        package_declared
            ? "package not found"
            : "cannot resolve imported module",
        details,
        importer_path,
        span,
        package_declared ? "package dependency" : "use declaration",
        requested,
        NULL,
        "import graph",
        primary_label,
        NULL,
        0u);
}

bool
vitte_main_expand_module(
    vitte_main_module_set_t *modules,
    const char *path,
    size_t depth,
    const char *namespace_name);

static bool
vitte_main_append_namespace_open(
    vitte_main_module_set_t *modules,
    const char *namespace_name,
    size_t *namespace_depth)
{
    const char *segment;
    size_t depth;

    if (modules == NULL ||
        namespace_name == NULL ||
        namespace_depth == NULL) {
        return false;
    }

    segment = namespace_name;
    depth = 0u;
    while (*segment != '\0') {
        const char *separator;
        size_t segment_length;

        separator = strstr(segment, "::");
        segment_length = separator != NULL
            ? (size_t)(separator - segment)
            : strlen(segment);
        if (segment_length == 0u ||
            !vitte_main_buffer_append(
                &modules->source,
                &modules->source_length,
                &modules->source_capacity,
                (const unsigned char *)"export space ",
                13u) ||
            !vitte_main_buffer_append(
                &modules->source,
                &modules->source_length,
                &modules->source_capacity,
                (const unsigned char *)segment,
                segment_length) ||
            !vitte_main_buffer_append(
                &modules->source,
                &modules->source_length,
                &modules->source_capacity,
                (const unsigned char *)" {\n",
                3u)) {
            return false;
        }

        ++depth;
        if (separator == NULL) {
            break;
        }
        segment = separator + 2u;
        if (*segment == '\0') {
            return false;
        }
    }

    *namespace_depth = depth;
    return depth != 0u;
}

static bool
vitte_main_append_namespace_close(
    vitte_main_module_set_t *modules,
    size_t namespace_depth)
{
    while (namespace_depth != 0u) {
        if (!vitte_main_buffer_append(
                &modules->source,
                &modules->source_length,
                &modules->source_capacity,
                (const unsigned char *)"}\n",
                2u)) {
            return false;
        }
        --namespace_depth;
    }
    return true;
}

bool
vitte_main_expand_module(
    vitte_main_module_set_t *modules,
    const char *path,
    size_t depth,
    const char *namespace_name)
{
    vitte_main_file_t file;
    size_t path_index;
    size_t offset;
    size_t namespace_depth;
    size_t root_header_begin = SIZE_MAX;
    size_t root_header_end = SIZE_MAX;
    bool root_header_copied = false;

    if (modules == NULL || path == NULL ||
        depth > VITTE_MAIN_MODULE_DEPTH_MAX) {
        return false;
    }
    path_index = vitte_main_module_path_index(modules, path);
    if (path_index != SIZE_MAX) {
        /* The graph owns physical identity and cycle edges. An active module
         * is not expanded recursively; its back-edge is registered by the
         * caller and reported by graph resolution after discovery finishes. */
        if (modules->active_paths[path_index]) {
            return true;
        }
        /* A module that completed expansion is a cache hit. */
        return modules->loaded_paths[path_index];
    }
    if (!vitte_main_module_record_path(modules, path)) {
        return false;
    }
    path_index = modules->count - 1u;
    if (!vitte_main_module_register_graph_module(
            modules,
            path_index,
            namespace_name,
            namespace_name == NULL)) {
        vitte_main_report_process_error(
            "VITTE_IMPORT_E_DUPLICATE",
            "cannot register imported module",
            "the module path or logical module name conflicts in the import graph");
        return false;
    }
    modules->active_paths[path_index] = true;
    if (!vitte_main_module_push_active(modules, path_index)) {
        modules->active_paths[path_index] = false;
        return false;
    }

    memset(&file, 0, sizeof(file));
    namespace_depth = 0u;
    if (!vitte_main_read_file(path, &file)) {
        vitte_main_error_argument("cannot load imported module", path);
        return false;
    }

    if (namespace_name == NULL &&
        vitte_main_module_root_header_range(
            file.data,
            file.length,
            &root_header_begin,
            &root_header_end)) {
        size_t combined_begin = modules->source_length;
        size_t header_length = root_header_end - root_header_begin;

        if (!vitte_main_buffer_append(
                &modules->source,
                &modules->source_length,
                &modules->source_capacity,
                file.data + root_header_begin,
                header_length) ||
            !vitte_main_module_record_origin(
                modules,
                path,
                combined_begin,
                root_header_begin,
                header_length)) {
            free(file.data);
            return false;
        }
        root_header_copied = true;
    }

    /*
     * Expand dependencies before emitting this module's declarations so
     * imported module scopes remain siblings rather than nested scopes.
     */
    offset = 0u;
    while (offset < file.length) {
        size_t line_end;
        size_t directive_start;
        size_t directive_end;

        line_end = offset;
        while (line_end < file.length && file.data[line_end] != '\n') {
            ++line_end;
        }
        directive_start = offset;
        while (directive_start < line_end &&
               (file.data[directive_start] == ' ' ||
                file.data[directive_start] == '\t')) {
            ++directive_start;
        }

        if (line_end - directive_start < 4u ||
            memcmp(file.data + directive_start, "use", 3u) != 0 ||
            !isspace(file.data[directive_start + 3u])) {
            offset = line_end < file.length ? line_end + 1u : line_end;
            continue;
        }

        directive_end = offset;
        while (directive_end < file.length &&
               file.data[directive_end] != ';') {
            ++directive_end;
        }
        if (directive_end == file.length) {
            vitte_main_report_empty_use_path(
                path,
                file.data,
                file.length,
                (uint32_t)(path_index + 1u),
                line_end,
                line_end);
            free(file.data);
            return false;
        }

        {
            char requested[VITTE_MAIN_MODULE_PATH_MAX];
            char imported_path[VITTE_MAIN_MODULE_PATH_MAX];
            char lookup_roots[VITTE_CLI_DIAGNOSTIC_DETAILS_CAPACITY];
            size_t name_start;
            size_t name_end;
            size_t name_length;
            size_t path_error_begin;
            size_t path_error_end;
            uint32_t path_error_codepoint;
            vitte_unicode_error_t path_unicode_error;
            vitte_main_module_path_status_t path_status;
            size_t imported_path_index;
            vitte_import_span_t path_span;

            name_start = directive_start + 3u;
            while (name_start < directive_end &&
                   isspace(file.data[name_start])) {
                ++name_start;
            }
            name_end = name_start;
            while (name_end < directive_end &&
                   file.data[name_end] != '{' &&
                   file.data[name_end] != ';' &&
                   !isspace(file.data[name_end])) {
                ++name_end;
            }
            if (name_end < directive_end && file.data[name_end] == '{' &&
                name_end >= name_start + 2u &&
                file.data[name_end - 1u] == ':' &&
                file.data[name_end - 2u] == ':') {
                name_end -= 2u;
            } else if (name_end >= name_start + 3u &&
                       file.data[name_end - 1u] == '*' &&
                       file.data[name_end - 2u] == ':' &&
                       file.data[name_end - 3u] == ':') {
                name_end -= 3u;
            }
            name_length = name_end - name_start;
            if (name_length == 0u) {
                vitte_main_report_empty_use_path(
                    path,
                    file.data,
                    file.length,
                    (uint32_t)(path_index + 1u),
                    directive_end,
                    directive_end + 1u);
                free(file.data);
                return false;
            }
            path_span = vitte_main_import_span_make(
                (uint32_t)(path_index + 1u),
                file.data,
                file.length,
                name_start,
                name_end);
            if (name_length >= sizeof(requested)) {
                vitte_main_report_invalid_import_path(
                    path,
                    file.data,
                    file.length,
                    (uint32_t)(path_index + 1u),
                    name_start,
                    name_end,
                    "<path exceeds supported length>");
                free(file.data);
                return false;
            }
            {
                const unsigned char *nul_byte = (const unsigned char *)memchr(
                    file.data + name_start,
                    '\0',
                    name_length);
                if (nul_byte != NULL) {
                    size_t nul_offset = (size_t)(nul_byte - file.data);
                    vitte_main_report_invalid_import_path(
                        path,
                        file.data,
                        file.length,
                        (uint32_t)(path_index + 1u),
                        nul_offset,
                        nul_offset + 1u,
                        "NUL byte");
                    free(file.data);
                    return false;
                }
            }

            memcpy(requested, file.data + name_start, name_length);
            requested[name_length] = '\0';
            path_status = vitte_main_module_import_path_is_well_formed(
                requested,
                &path_error_begin,
                &path_error_end,
                &path_error_codepoint,
                &path_unicode_error);
            if (path_status != VITTE_MAIN_MODULE_PATH_VALID) {
                if (path_error_end <= path_error_begin ||
                    path_error_end > name_length) {
                    path_error_begin = 0u;
                    path_error_end = name_length;
                }
                if (path_status == VITTE_MAIN_MODULE_PATH_INVALID_UTF8 ||
                    path_status == VITTE_MAIN_MODULE_PATH_INVALID_IDENTIFIER) {
                    vitte_main_report_invalid_import_unicode(
                        path,
                        file.data,
                        file.length,
                        (uint32_t)(path_index + 1u),
                        name_start + path_error_begin,
                        name_start + path_error_end,
                        path_status,
                        path_error_codepoint,
                        path_unicode_error);
                    free(file.data);
                    return false;
                }
                char invalid_token[16];
                size_t invalid_length;
                invalid_length = path_error_end - path_error_begin;
                if (invalid_length >= sizeof(invalid_token)) {
                    invalid_length = sizeof(invalid_token) - 1u;
                }
                memcpy(invalid_token, requested + path_error_begin,
                       invalid_length);
                invalid_token[invalid_length] = '\0';
                vitte_main_report_invalid_import_path(
                    path,
                    file.data,
                    file.length,
                    (uint32_t)(path_index + 1u),
                    name_start + path_error_begin,
                    name_start + path_error_end,
                    invalid_token);
                free(file.data);
                return false;
            }

            if (!vitte_main_module_resolve_path(
                    path,
                    requested,
                    imported_path,
                    sizeof(imported_path),
                    lookup_roots,
                    sizeof(lookup_roots))) {
                vitte_main_report_missing_module(
                    path,
                    requested,
                    &path_span,
                    lookup_roots);
                free(file.data);
                return false;
            }
            if (!vitte_main_module_validate_import_name(
                    path,
                    file.data,
                    file.length,
                    (uint32_t)(path_index + 1u),
                    name_start,
                    name_end,
                    imported_path,
                    requested)) {
                free(file.data);
                return false;
            }
            if (!vitte_main_expand_module(
                    modules,
                    imported_path,
                    depth + 1u,
                    requested)) {
                free(file.data);
                return false;
            }

            imported_path_index = vitte_main_module_path_index(
                modules,
                imported_path);
            if (imported_path_index == SIZE_MAX ||
                !vitte_main_import_register_edge(
                    modules,
                    path_index,
                    imported_path_index,
                    file.data,
                    directive_start,
                    directive_end,
                    name_start,
                    name_end)) {
                free(file.data);
                return false;
            }
        }

        /* Continue at the next byte so adjacent `use` directives on the
         * same line are discovered as separate imports. */
        offset = directive_end + 1u;
    }

    if (namespace_name != NULL) {
        if (!vitte_main_append_namespace_open(
                modules,
                namespace_name,
                &namespace_depth)) {
            free(file.data);
            return false;
        }
    }

    offset = 0u;
    while (offset < file.length) {
        size_t line_end;
        size_t directive_start;
        size_t directive_end;
        const char *keyword;

        line_end = offset;
        while (line_end < file.length && file.data[line_end] != '\n') {
            ++line_end;
        }
        directive_start = offset;
        while (directive_start < line_end &&
               (file.data[directive_start] == ' ' ||
                file.data[directive_start] == '\t')) {
            ++directive_start;
        }

        keyword = NULL;
        if (line_end - directive_start >= 4u &&
            memcmp(file.data + directive_start, "use ", 4u) == 0) {
            keyword = "use";
        } else if (line_end - directive_start >= 6u &&
                   memcmp(
                       file.data + directive_start,
                       "space ",
                       6u) == 0) {
            keyword = "space";
        }

        if (keyword != NULL && strcmp(keyword, "space") == 0 &&
            namespace_name == NULL &&
            (!root_header_copied || offset != root_header_begin)) {
            size_t combined_begin = modules->source_length;
            size_t line_length = line_end - offset;

            if (!vitte_main_buffer_append(
                    &modules->source,
                    &modules->source_length,
                    &modules->source_capacity,
                    file.data + offset,
                    line_length) ||
                !vitte_main_module_record_origin(
                    modules,
                    path,
                    combined_begin,
                    offset,
                    line_length)) {
                free(file.data);
                return false;
            }
            if (line_end < file.length) {
                combined_begin = modules->source_length;
                if (!vitte_main_buffer_append(
                        &modules->source,
                        &modules->source_length,
                        &modules->source_capacity,
                        (const unsigned char *)"\n",
                        1u) ||
                    !vitte_main_module_record_origin(
                        modules,
                        path,
                        combined_begin,
                        line_end,
                        1u)) {
                    free(file.data);
                    return false;
                }
            }
            offset = line_end < file.length ? line_end + 1u : line_end;
            continue;
        }

        if (keyword == NULL) {
            size_t line_length;
            size_t combined_begin;
            line_length = line_end - offset;
            combined_begin = modules->source_length;
            if (!vitte_main_buffer_append(
                    &modules->source,
                    &modules->source_length,
                    &modules->source_capacity,
                    file.data + offset,
                    line_length) ||
                !vitte_main_module_record_origin(
                    modules,
                    path,
                    combined_begin,
                    offset,
                    line_length)) {
                free(file.data);
                return false;
            }
            if (line_end < file.length) {
                combined_begin = modules->source_length;
                if (!vitte_main_buffer_append(
                        &modules->source,
                        &modules->source_length,
                        &modules->source_capacity,
                        (const unsigned char *)"\n",
                        1u) ||
                    !vitte_main_module_record_origin(
                        modules,
                        path,
                        combined_begin,
                        line_end,
                        1u)) {
                    free(file.data);
                    return false;
                }
            }
            offset = line_end < file.length ? line_end + 1u : line_end;
            continue;
        }

        if (strcmp(keyword, "use") == 0) {
            size_t directive_length;
            size_t combined_begin;

            directive_end = offset;
            while (directive_end < file.length &&
                   file.data[directive_end] != ';') {
                ++directive_end;
            }
            if (directive_end == file.length) {
                free(file.data);
                vitte_main_error_argument(
                    "unterminated module directive in",
                    path);
                return false;
            }

            directive_length = directive_end - offset + 1u;
            combined_begin = modules->source_length;
            if (!vitte_main_buffer_append(
                    &modules->source,
                    &modules->source_length,
                    &modules->source_capacity,
                    file.data + offset,
                    directive_length) ||
                !vitte_main_module_record_origin(
                    modules,
                    path,
                    combined_begin,
                    offset,
                    directive_length)) {
                free(file.data);
                return false;
            }
            combined_begin = modules->source_length;
            if (!vitte_main_buffer_append(
                    &modules->source,
                    &modules->source_length,
                    &modules->source_capacity,
                    (const unsigned char *)"\n",
                    1u) ||
                (directive_end + 1u < file.length &&
                 file.data[directive_end + 1u] == '\n' &&
                 !vitte_main_module_record_origin(
                    modules,
                    path,
                    combined_begin,
                    directive_end + 1u,
                    1u))) {
                free(file.data);
                return false;
            }

            /* Keep scanning the current line: compact sources may contain
             * several `use` directives separated by semicolons. */
            offset = directive_end + 1u;
            continue;
        }

        directive_end = offset;
        if (strcmp(keyword, "space") == 0) {
            while (directive_end < line_end &&
                   file.data[directive_end] != ';') {
                ++directive_end;
            }
            if (directive_end == line_end) {
                directive_end = line_end;
            }
        } else {
            while (directive_end < file.length &&
                   file.data[directive_end] != ';') {
                ++directive_end;
            }
        }
        if (directive_end == file.length &&
            strcmp(keyword, "use") == 0) {
            free(file.data);
            vitte_main_error_argument(
                "unterminated module directive in",
                path);
            return false;
        }

        while (offset <= directive_end) {
            if (file.data[offset] == '\n' &&
                !vitte_main_buffer_append(
                    &modules->source,
                    &modules->source_length,
                    &modules->source_capacity,
                    (const unsigned char *)"\n",
                    1u)) {
                free(file.data);
                return false;
            }
            ++offset;
        }
    }

    if (modules->source_length == 0u ||
        modules->source[modules->source_length - 1u] != '\n') {
        if (!vitte_main_buffer_append(
                &modules->source,
                &modules->source_length,
                &modules->source_capacity,
                (const unsigned char *)"\n",
                1u)) {
            free(file.data);
            return false;
        }
    }

    if (namespace_name != NULL &&
        !vitte_main_append_namespace_close(
            modules,
            namespace_depth)) {
        free(file.data);
        return false;
    }

    free(file.data);
    vitte_main_module_pop_active(modules, path_index);
    modules->active_paths[path_index] = false;
    modules->loaded_paths[path_index] = true;
    return true;
}

void
vitte_main_module_set_destroy(
    vitte_main_module_set_t *modules)
{
    size_t index;

    if (modules == NULL) {
        return;
    }
    for (index = 0u; index < modules->count; ++index) {
        free(modules->paths[index]);
        free(modules->identity_paths[index]);
    }
    free(modules->paths);
    free(modules->identity_paths);
    free(modules->graph_module_ids);
    free(modules->active_paths);
    free(modules->loaded_paths);
    free(modules->active_stack);
    modules->paths = NULL;
    modules->identity_paths = NULL;
    modules->graph_module_ids = NULL;
    modules->active_paths = NULL;
    modules->loaded_paths = NULL;
    modules->active_stack = NULL;
    modules->count = 0u;
    modules->capacity = 0u;
    modules->active_count = 0u;
    modules->active_capacity = 0u;
    if (modules->import_graph_initialized) {
        vitte_import_destroy(&modules->import_graph);
        modules->import_graph_initialized = false;
    }
    free(modules->origins);
    modules->origins = NULL;
    modules->origin_count = 0u;
    modules->origin_capacity = 0u;
}

/* ========================================================================= */
/* File loading                                                              */
/* ========================================================================= */

bool
vitte_main_read_file(
    const char *path,
    vitte_main_file_t *file)
{
    vitte_filesystem_buffer_t buffer = {0};
    vitte_filesystem_result_t result;

    if (path == NULL ||
        file == NULL) {
        return false;
    }

    file->data = NULL;
    file->length = 0u;

    result = vitte_filesystem_read_file(path, 0u, &buffer);
    if (result.error != VITTE_FILESYSTEM_ERROR_NONE) {
        vitte_main_report_path_error(
            "VITTE_INFRA_E_SOURCE_READ",
            "source file could not be read",
            path,
            vitte_filesystem_error_name(result.error));
        vitte_filesystem_buffer_destroy(&buffer);
        return false;
    }

    file->data = buffer.data;
    file->length = buffer.length;

    return true;
}

void
vitte_main_file_destroy(
    vitte_main_file_t *file)
{
    if (file == NULL) {
        return;
    }

    free(file->data);

    file->data = NULL;
    file->length = 0u;
}

/* ========================================================================= */
/* Source name                                                               */
/* ========================================================================= */

const char *
vitte_main_basename(
    const char *path)
{
    const char *result;
    const char *cursor;

    if (path == NULL) {
        return "";
    }

    result = path;

    for (cursor = path;
         *cursor != '\0';
         ++cursor) {
        if (*cursor == '/' ||
            *cursor == '\\') {
            result = cursor + 1;
        }
    }

    return result;
}

/* ========================================================================= */
/* UTF-8 validation                                                          */
/* ========================================================================= */

bool
vitte_main_validate_utf8(
    const vitte_main_options_t *options,
    const vitte_main_file_t *file)
{
    size_t error_offset;

    if (options == NULL ||
        file == NULL) {
        return false;
    }

    if (!options->validate_utf8) {
        return true;
    }

    error_offset = 0u;

    if (!vitte_unicode_validate_utf8(
            (const char *)file->data,
            file->length,
            &error_offset)) {
        char details[256];
        int details_length;
        size_t error_end;
        vitte_import_span_t span;

        details_length = snprintf(
            details,
            sizeof(details),
            "invalid UTF-8 sequence at byte %zu",
            error_offset);
        error_end = error_offset < file->length
            ? error_offset + 1u
            : error_offset;
        span = vitte_main_import_span_make(
            0u,
            file->data,
            file->length,
            error_offset,
            error_end);
        if (details_length < 0 ||
            (size_t)details_length >= sizeof(details)) {
            vitte_main_report_process_error(
                "VITTE_INFRA_E_ENCODING",
                "source contains invalid UTF-8",
                NULL);
        } else {
            vitte_main_report_process_error_span(
                "VITTE_INFRA_E_ENCODING",
                "source contains invalid UTF-8",
                details,
                options->input_path,
                &span,
                "source encoding",
                NULL,
                NULL,
                "source file");
        }

        return false;
    }

    return true;
}

/* ========================================================================= */
/* Token output                                                              */
/* ========================================================================= */

void
vitte_main_print_tokens(
    const vitte_lexer_t *lexer)
{
    size_t index;

    if (lexer == NULL) {
        return;
    }

    for (index = 0u;
         index < lexer->token_count;
         ++index) {
        const vitte_token_t *token =
            &lexer->tokens[index];

        (void)printf(
            "%6zu  %-24s  [%zu, %zu)  %zu:%zu\n",
            index,
            vitte_token_kind_name(
                token->kind),
            token->span.begin,
            token->span.end,
            token->line,
            token->column);
    }
}

/* ========================================================================= */
/* Frontend                                                                  */
/* ========================================================================= */

static vitte_main_exit_code_t
vitte_main_frontend(
    const vitte_main_options_t *options)
{
    vitte_main_file_t file;

    vitte_source_context_t sources;
    vitte_source_id_t source_id;

    vitte_lexer_t lexer;
    vitte_parser_t parser;

    bool source_initialized;
    bool lexer_initialized;
    bool parser_initialized;

    vitte_main_exit_code_t exit_code;

    memset(
        &file,
        0,
        sizeof(file));

    memset(
        &sources,
        0,
        sizeof(sources));

    memset(
        &lexer,
        0,
        sizeof(lexer));

    memset(
        &parser,
        0,
        sizeof(parser));

    source_id =
        VITTE_SOURCE_INVALID_ID;

    source_initialized = false;
    lexer_initialized = false;
    parser_initialized = false;

    exit_code =
        VITTE_MAIN_EXIT_FAILURE;

    /* --------------------------------------------------------------------- */
    /* Load                                                                  */
    /* --------------------------------------------------------------------- */

    vitte_main_verbose(
        options,
        "loading source");

    if (!vitte_main_read_file(
            options->input_path,
            &file)) {
        goto cleanup;
    }

    /* --------------------------------------------------------------------- */
    /* Unicode                                                               */
    /* --------------------------------------------------------------------- */

    vitte_main_verbose(
        options,
        "validating UTF-8");

    if (!vitte_main_validate_utf8(
            options,
            &file)) {
        goto cleanup;
    }

    /* --------------------------------------------------------------------- */
    /* Source database                                                       */
    /* --------------------------------------------------------------------- */

    vitte_main_verbose(
        options,
        "registering source");

    if (!vitte_source_init(
            &sources)) {
        vitte_main_error(
            "failed to initialize source subsystem");

        exit_code =
            VITTE_MAIN_EXIT_INTERNAL;

        goto cleanup;
    }

    source_initialized = true;

    if (!vitte_source_add(
            &sources,
            vitte_main_basename(
                options->input_path),
            options->input_path,
            (const char *)file.data,
            file.length,
            &source_id)) {
        vitte_main_report_path_error(
            "VITTE_INFRA_E_SOURCE_MAP",
            "source file could not be registered",
            options->input_path,
            vitte_source_error_name(
                vitte_source_last_error(&sources)));

        goto cleanup;
    }

    /* --------------------------------------------------------------------- */
    /* Lexer                                                                 */
    /* --------------------------------------------------------------------- */

    vitte_main_verbose(
        options,
        "lexing");

    /*
     * The lexer contract generated for the compiler owns the exact source
     * binding API. Keep initialization explicit here.
     *
     * The frontend source bytes are stable for the complete lexer lifetime.
     */
    if (!vitte_lexer_init(
            &lexer,
            (const char *)file.data,
            file.length,
            (uint32_t)source_id)) {
        vitte_main_error(
            "failed to initialize lexer");

        exit_code =
            VITTE_MAIN_EXIT_INTERNAL;

        goto cleanup;
    }

    lexer_initialized = true;

    if (!vitte_lexer_run(
            &lexer)) {
        vitte_main_driver_context_t diagnostic_context;

        memset(
            &diagnostic_context,
            0,
            sizeof(diagnostic_context));
        diagnostic_context.options = options;
        diagnostic_context.file = file;
        diagnostic_context.sources = sources;
        diagnostic_context.source_id = source_id;
        diagnostic_context.lexer = lexer;
        diagnostic_context.lexer_initialized = true;

        if (!vitte_main_render_lexer_diagnostic(
                &diagnostic_context)) {
            vitte_main_error(
                "failed to render lexical diagnostic");
        }

        goto cleanup;
    }

    if (options->emit_tokens ||
        options->command ==
            VITTE_MAIN_COMMAND_LEX) {
        vitte_main_print_tokens(
            &lexer);
    }

    if (options->command ==
            VITTE_MAIN_COMMAND_LEX ||
        options->stop_after_lexer) {
        exit_code =
            VITTE_MAIN_EXIT_SUCCESS;

        goto cleanup;
    }

    /* --------------------------------------------------------------------- */
    /* Parser                                                                */
    /* --------------------------------------------------------------------- */

    vitte_main_verbose(
        options,
        "parsing");

    if (!vitte_parser_init(
            &parser,
            lexer.tokens,
            lexer.token_count)) {
        vitte_main_error(
            "failed to initialize parser");

        exit_code =
            VITTE_MAIN_EXIT_INTERNAL;

        goto cleanup;
    }

    parser_initialized = true;

    if (!vitte_parser_run(
            &parser)) {
        bool rendered;

        rendered = false;
        {
            vitte_main_driver_context_t diagnostic_context;

            memset(
                &diagnostic_context,
                0,
                sizeof(diagnostic_context));
            diagnostic_context.options = options;
            diagnostic_context.file = file;
            diagnostic_context.sources = sources;
            diagnostic_context.source_id = source_id;
            diagnostic_context.parser = parser;
            diagnostic_context.parser_initialized = true;

            if (!vitte_main_render_parser_diagnostics(
                    &diagnostic_context,
                    &rendered)) {
                vitte_main_error(
                    "failed to render parser diagnostics");
                goto cleanup;
            }
        }

        if (!rendered) {
            vitte_main_report_process_error(
                "VITTE_INFRA_E_PHASE",
                "parsing failed",
                vitte_parser_error_name(
                    vitte_parser_last_error(&parser)));
        }

        goto cleanup;
    }

    /*
     * AST printing is intentionally delegated to the printer subsystem.
     * Because printer API revisions have been evolving with the parser
     * contract, the main entry point does not duplicate AST traversal.
     */
    if (options->emit_ast ||
        options->command ==
            VITTE_MAIN_COMMAND_PARSE) {
        const vitte_ast_node_t *root = vitte_parser_get_node(
            &parser, vitte_parser_root(&parser));

        if (root == NULL) {
            vitte_main_error(
                "parser produced no translation unit");

            goto cleanup;
        }

        (void)printf(
            "AST root kind: %u\n"
            "nodes: %zu\n",
            (unsigned int)root->kind,
            parser.node_count);
    }

    if (options->command ==
            VITTE_MAIN_COMMAND_PARSE ||
        options->stop_after_parser) {
        exit_code =
            VITTE_MAIN_EXIT_SUCCESS;

        goto cleanup;
    }

    /*
     * Semantic analysis and the lower compiler pipeline are normally owned
     * by driver.c.
     *
     * Do not recreate scope/type/sema/HIR/IR/backend state here: doing so
     * would create two independent compiler orchestration implementations.
     *
     * The explicit lexer/parser path above exists for:
     *
     *   vitte lex
     *   vitte parse
     *   --emit-tokens
     *   --emit-ast
     *   --stop-after-lexer
     *   --stop-after-parser
     *
     * compile/check/run continue through the canonical driver below.
     */

    exit_code =
        VITTE_MAIN_EXIT_SUCCESS;

cleanup:

    if (parser_initialized) {
        vitte_parser_destroy(
            &parser);
    }

    if (lexer_initialized) {
        vitte_lexer_destroy(
            &lexer);
    }

    if (source_initialized) {
        vitte_source_destroy(
            &sources);
    }

    vitte_main_file_destroy(
        &file);

    return exit_code;
}

/* ========================================================================= */
/* Dispatch                                                                  */
/* ========================================================================= */

static vitte_main_exit_code_t
vitte_main_dispatch(
    const vitte_main_options_t *options)
{
    if (options == NULL) {
        return VITTE_MAIN_EXIT_INTERNAL;
    }

    switch (options->command) {
        case VITTE_MAIN_COMMAND_EXPLAIN:
            if (options->diagnostic_format == VITTE_MAIN_DIAGNOSTIC_SARIF) {
                vitte_main_error("explain supports terminal or JSON output; use --diagnostic=json");
                return VITTE_MAIN_EXIT_USAGE;
            }
            if (!vitte_diagnostic_cli_code_exists(options->input_path)) {
                vitte_main_report_process_error("VITTE_INFRA_E_CONFIG",
                    "unknown diagnostic code", options->input_path);
                return VITTE_MAIN_EXIT_USAGE;
            }
            return vitte_diagnostic_explain_cli(stdout, options->input_path,
                options->diagnostic_format == VITTE_MAIN_DIAGNOSTIC_JSON)
                    ? VITTE_MAIN_EXIT_SUCCESS : VITTE_MAIN_EXIT_FAILURE;

        case VITTE_MAIN_COMMAND_COMPLETION:
            return vitte_cli_render_completion(stdout, options->input_path)
                ? VITTE_MAIN_EXIT_SUCCESS
                : VITTE_MAIN_EXIT_FAILURE;

        case VITTE_MAIN_COMMAND_HELP:
            vitte_main_print_usage(
                stdout);

            return VITTE_MAIN_EXIT_SUCCESS;

        case VITTE_MAIN_COMMAND_VERSION:
            vitte_main_print_version();

            return VITTE_MAIN_EXIT_SUCCESS;

        case VITTE_MAIN_COMMAND_INSTALL:
            return vitte_main_install_package(
                options->input_path,
                options->package_version,
                options->package_global);

        case VITTE_MAIN_COMMAND_LEX:
        case VITTE_MAIN_COMMAND_PARSE:
            return vitte_main_frontend(
                options);

        case VITTE_MAIN_COMMAND_COMPILE:
        case VITTE_MAIN_COMMAND_CHECK:
        case VITTE_MAIN_COMMAND_RUN:
            break;
    }

    return vitte_main_driver_compile(
        options);
}

/* ========================================================================= */
/* Main                                                                      */
/* ========================================================================= */

int
vitte_main_entry(
    int argc,
    char **argv)
{
    vitte_main_options_t options;
    vitte_main_exit_code_t result;

    vitte_main_process_diagnostic_emitted = false;
    vitte_main_set_executable_path(argc > 0 ? argv[0] : NULL);

    if (!vitte_main_parse_options(
            argc,
            argv,
            &options)) {
        if (vitte_main_active_diagnostic_format ==
            VITTE_MAIN_DIAGNOSTIC_TERMINAL) {
            (void)fprintf(stderr, "\n");
            vitte_main_print_usage(stderr);
        }

        vitte_main_options_destroy(&options);
        return (int)
            VITTE_MAIN_EXIT_USAGE;
    }

    result =
        vitte_main_dispatch(
            &options);

    if (result ==
            VITTE_MAIN_EXIT_INTERNAL &&
        !options.quiet) {
        vitte_main_warning(
            &options,
            "compiler terminated because of an internal or infrastructure failure");
    }

    vitte_main_options_destroy(&options);
    return (int)result;
}
