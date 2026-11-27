/*
 * Vitte Compiler
 * src/bootstrap/cli/pipeline.c
 *
 * Private executable-facing implementation module.
 *
 * ISO C17.
 */

#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include "api/version.h"
#include "backend/c17/ast_emitter.h"
#include "backend/c17/ir_emitter.h"
#include "filesystem/filesystem.h"
#include "hir/lower.h"
#include "ir/lower.h"
#include "macro_expansion.h"
#include "platform/platform.h"
#include "printer/printer.h"
#include "unicode/unicode.h"
#include "util/util.h"
#include "internal.h"



static bool vitte_main_origin_offset(
    const vitte_main_driver_context_t *context,
    size_t combined_offset,
    bool end_position,
    const vitte_main_source_origin_t **origin,
    size_t *source_offset);

static bool vitte_main_origin_span(
    const vitte_main_driver_context_t *context,
    size_t combined_begin,
    size_t combined_end,
    vitte_source_id_t *source_id,
    size_t *source_begin,
    size_t *source_end);

static void vitte_main_diagnostic_map_span(
    const vitte_main_driver_context_t *context,
    vitte_cli_diagnostic_span_t *span);
#ifdef VITTE_NATIVE_FRONTEND_LINKED
static void vitte_main_diagnostic_map_native_batch(
    const vitte_main_driver_context_t *context,
    vitte_cli_diagnostic_t *diagnostics,
    size_t diagnostic_count);
#endif
static bool vitte_main_render_diagnostic(
    const vitte_main_driver_context_t *context,
    vitte_cli_diagnostic_severity_t severity,
    const char *code,
    const char *message,
    const char *details,
    const vitte_parser_span_t *source_span,
    const char *procedure,
    const char *diagnostic_context);

/* Driver bridge                                                             */
/* ========================================================================= */

bool
vitte_main_driver_load_source(
    vitte_driver_t *driver,
    vitte_driver_phase_t phase,
    const vitte_driver_input_t *input,
    void *user_data)
{
    vitte_main_driver_context_t *context;
    vitte_main_module_set_t modules;
    char root_path[VITTE_MAIN_MODULE_PATH_MAX];
    char absolute_input[VITTE_MAIN_MODULE_PATH_MAX];
    char expanded_source_path[VITTE_MAIN_MODULE_PATH_MAX];
    char *canonical_input;
    vitte_filesystem_result_t canonical_result;
    size_t expanded_length;
    unsigned char terminator;
    int written;

    (void)driver;
    (void)phase;

    context = (vitte_main_driver_context_t *)user_data;

    if (context == NULL ||
        context->options == NULL ||
        input == NULL ||
        input->name == NULL) {
        return false;
    }

    vitte_main_verbose(context->options, "loading source");

    if (!vitte_main_read_file(input->name, &context->file) ||
        !vitte_main_validate_utf8(context->options, &context->file)) {
        return false;
    }

    canonical_input = NULL;
    if (vitte_filesystem_path_is_absolute(input->name)) {
        if (snprintf(
                absolute_input,
                sizeof(absolute_input),
                "%s",
                input->name) < 0) {
            return false;
        }
    } else {
        char working_directory[VITTE_MAIN_MODULE_PATH_MAX];

        if (!vitte_platform_getcwd(
                working_directory,
                sizeof(working_directory)) ||
            snprintf(
                absolute_input,
                sizeof(absolute_input),
                "%s/%s",
                working_directory,
                input->name) < 0) {
            return false;
        }
    }
    canonical_input = NULL;
    canonical_result = vitte_filesystem_normalize(
        absolute_input,
        &canonical_input);
    if (canonical_result.error != VITTE_FILESYSTEM_ERROR_NONE ||
        canonical_input == NULL ||
        strlen(canonical_input) >= sizeof(root_path)) {
        free(canonical_input);
        vitte_main_error_argument("input path is too long", input->name);
        return false;
    }
    memcpy(root_path, canonical_input, strlen(canonical_input) + 1u);
    free(canonical_input);

    memset(&modules, 0, sizeof(modules));
    if (!vitte_import_init(&modules.import_graph)) {
        vitte_main_error("failed to initialize import graph");
        free(context->file.data);
        context->file.data = NULL;
        context->file.length = 0u;
        return false;
    }
    modules.import_graph_initialized = true;
    if (!vitte_main_import_graph_load(&modules, root_path) ||
        modules.source == NULL ||
        modules.source_length == SIZE_MAX) {
        vitte_main_module_set_destroy(&modules);
        free(modules.source);
        if (!vitte_main_process_diagnostic_emitted) {
            vitte_main_error_argument(
                "failed to assemble imported modules for",
                input->name);
        }
        return false;
    }
    expanded_length = modules.source_length;
    terminator = 0u;
    if (!vitte_main_buffer_append(
            &modules.source,
            &modules.source_length,
            &modules.source_capacity,
            &terminator,
            1u)) {
        vitte_main_module_set_destroy(&modules);
        free(modules.source);
        vitte_main_error("failed to allocate combined module source");
        return false;
    }
    vitte_main_verbose(context->options, "registering source");

    if (!vitte_source_init(&context->sources)) {
        vitte_main_error("failed to initialize source subsystem");
        vitte_main_module_set_destroy(&modules);
        return false;
    }

    context->source_initialized = true;

    if (!vitte_source_add(
            &context->sources,
            vitte_main_basename(input->name),
            input->name,
            (const char *)context->file.data,
            context->file.length,
            &context->root_source_id)) {
        vitte_main_report_path_error(
            "VITTE_INFRA_E_SOURCE_MAP",
            "root source file could not be registered",
            input->name,
            vitte_source_error_name(
                vitte_source_last_error(&context->sources)));
        vitte_main_module_set_destroy(&modules);
        free(modules.source);
        return false;
    }

    written = snprintf(
        expanded_source_path,
        sizeof(expanded_source_path),
        "vitte://expanded/%s",
        vitte_main_basename(input->name));
    if (written < 0 ||
        (size_t)written >= sizeof(expanded_source_path)) {
        vitte_main_module_set_destroy(&modules);
        free(modules.source);
        vitte_main_error_argument(
            "expanded source name is too long for",
            input->name);
        return false;
    }

    free(context->file.data);
    context->file.data = modules.source;
    context->file.length = expanded_length;
    modules.source = NULL;

    if (!vitte_source_add(
            &context->sources,
            "expanded module input",
            expanded_source_path,
            (const char *)context->file.data,
            context->file.length,
            &context->source_id)) {
        vitte_main_report_path_error(
            "VITTE_INFRA_E_SOURCE_MAP",
            "expanded source could not be registered",
            expanded_source_path,
            vitte_source_error_name(
                vitte_source_last_error(&context->sources)));
        vitte_main_module_set_destroy(&modules);
        return false;
    }

    {
        size_t path_index;
        size_t origin_index;

        for (path_index = 0u;
             path_index < modules.count;
             ++path_index) {
            vitte_source_id_t origin_source_id;

            if (strcmp(modules.paths[path_index], root_path) == 0) {
                origin_source_id = context->root_source_id;
            } else {
                vitte_main_file_t origin_file;

                memset(&origin_file, 0, sizeof(origin_file));
                if (!vitte_main_read_file(
                        modules.paths[path_index],
                        &origin_file) ||
                    !vitte_source_add(
                        &context->sources,
                        vitte_main_basename(modules.paths[path_index]),
                        modules.paths[path_index],
                        (const char *)origin_file.data,
                        origin_file.length,
                        &origin_source_id)) {
                    vitte_main_file_destroy(&origin_file);
                    vitte_main_report_path_error(
                        "VITTE_INFRA_E_SOURCE_MAP",
                        "imported source file could not be registered",
                        modules.paths[path_index],
                        vitte_source_error_name(
                            vitte_source_last_error(&context->sources)));
                    vitte_main_module_set_destroy(&modules);
                    return false;
                }
                vitte_main_file_destroy(&origin_file);
            }

            for (origin_index = 0u;
                 origin_index < modules.origin_count;
                 ++origin_index) {
                if (modules.origins[origin_index].path_index ==
                    path_index) {
                    modules.origins[origin_index].source_id =
                        origin_source_id;
                }
            }
        }

        context->origins = modules.origins;
        context->origin_count = modules.origin_count;
        modules.origins = NULL;
        modules.origin_count = 0u;
        modules.origin_capacity = 0u;
    }
    vitte_main_module_set_destroy(&modules);

    if (context->source_id > (vitte_source_id_t)UINT32_MAX) {
        vitte_main_error("source identifier exceeds the lexer limit");
        return false;
    }

    return true;
}

#ifdef VITTE_NATIVE_FRONTEND_LINKED
static bool
vitte_main_source_has_import_keyword(
    const unsigned char *source,
    size_t length)
{
    size_t index;

    if (source == NULL || length < 4u) {
        return false;
    }

    for (index = 0u; index + 3u < length; ++index) {
        unsigned char previous = index == 0u ? 0u : source[index - 1u];
        unsigned char following = source[index + 3u];
        bool previous_is_identifier =
            (previous >= (unsigned char)'a' && previous <= (unsigned char)'z') ||
            (previous >= (unsigned char)'A' && previous <= (unsigned char)'Z') ||
            (previous >= (unsigned char)'0' && previous <= (unsigned char)'9') ||
            previous == (unsigned char)'_';

        if (previous_is_identifier || source[index] != (unsigned char)'u' ||
            source[index + 1u] != (unsigned char)'s' ||
            source[index + 2u] != (unsigned char)'e') {
            continue;
        }

        if (following == (unsigned char)' ' ||
            following == (unsigned char)'\t' ||
            following == (unsigned char)'\r' ||
            following == (unsigned char)'\n') {
            return true;
        }
    }

    return false;
}
#endif

#ifdef VITTE_NATIVE_FRONTEND_LINKED
static bool
vitte_main_source_identifier_byte(unsigned char byte)
{
    return (byte >= (unsigned char)'a' && byte <= (unsigned char)'z') ||
           (byte >= (unsigned char)'A' && byte <= (unsigned char)'Z') ||
           (byte >= (unsigned char)'0' && byte <= (unsigned char)'9') ||
           byte == (unsigned char)'_';
}

static bool
vitte_main_source_has_word(
    const unsigned char *source,
    size_t length,
    const char *word)
{
    size_t word_length;
    size_t index;

    if (source == NULL || word == NULL) {
        return false;
    }
    word_length = strlen(word);
    if (word_length == 0u || word_length > length) {
        return false;
    }

    for (index = 0u; index + word_length <= length; ++index) {
        bool left_boundary = index == 0u ||
            !vitte_main_source_identifier_byte(source[index - 1u]);
        size_t end = index + word_length;
        bool right_boundary = end == length ||
            !vitte_main_source_identifier_byte(source[end]);

        if (left_boundary && right_boundary &&
            memcmp(source + index, word, word_length) == 0) {
            return true;
        }
    }
    return false;
}

static size_t
vitte_main_source_skip_space(const unsigned char *source, size_t length, size_t index)
{
    while (index < length &&
           (source[index] == (unsigned char)' ' ||
            source[index] == (unsigned char)'\t' ||
            source[index] == (unsigned char)'\r' ||
            source[index] == (unsigned char)'\n')) {
        ++index;
    }
    return index;
}

/* Native HIR lowers nongeneric picks with up to sixteen scalar payload fields. */
static bool
vitte_main_pick_payload_type_is_native(
    const unsigned char *source,
    size_t length)
{
#define VITTE_PICK_TYPE(name) \
    (length == sizeof(name) - 1u && \
     memcmp(source, name, sizeof(name) - 1u) == 0)
    return VITTE_PICK_TYPE("i32") || VITTE_PICK_TYPE("i64") ||
           VITTE_PICK_TYPE("int") || VITTE_PICK_TYPE("u8") ||
           VITTE_PICK_TYPE("u16") || VITTE_PICK_TYPE("u32") ||
           VITTE_PICK_TYPE("u64") || VITTE_PICK_TYPE("usize") ||
           VITTE_PICK_TYPE("bool") || VITTE_PICK_TYPE("char") ||
           VITTE_PICK_TYPE("f64") || VITTE_PICK_TYPE("double") ||
           VITTE_PICK_TYPE("string") || VITTE_PICK_TYPE("str");
#undef VITTE_PICK_TYPE
}

static bool
vitte_main_source_has_unsupported_pick(
    const unsigned char *source,
    size_t length)
{
    static const char pick_word[] = "pick";
    size_t index;

    if (source == NULL) {
        return false;
    }
    for (index = 0u; index + sizeof(pick_word) - 1u <= length; ++index) {
        size_t cursor;
        size_t body;
        size_t depth;

        if ((index != 0u &&
             vitte_main_source_identifier_byte(source[index - 1u])) ||
            memcmp(source + index, pick_word, sizeof(pick_word) - 1u) != 0 ||
            (index + sizeof(pick_word) - 1u < length &&
             vitte_main_source_identifier_byte(
                 source[index + sizeof(pick_word) - 1u]))) {
            continue;
        }
        cursor = vitte_main_source_skip_space(
            source,
            length,
            index + sizeof(pick_word) - 1u);
        while (cursor < length && source[cursor] != (unsigned char)'{' &&
               source[cursor] != (unsigned char)';' &&
               source[cursor] != (unsigned char)'}') {
            if (source[cursor] == (unsigned char)'<') {
                return true;
            }
            ++cursor;
        }
        if (cursor == length || source[cursor] != (unsigned char)'{') {
            continue;
        }
        body = cursor + 1u;
        depth = 1u;
        while (body < length && depth != 0u) {
            if (source[body] == (unsigned char)'(') {
#ifndef VITTE_NATIVE_FRONTEND_LINKED
                /* The bootstrap emitter still owns payload-pick source while
                 * the native frontend is being rebuilt. */
                return true;
#else
                size_t payload = body + 1u;
                size_t payload_fields = 0u;
                bool payload_valid = true;
                for (;;) {
                    size_t type_begin = vitte_main_source_skip_space(
                        source,
                        length,
                        payload);
                    size_t type_end = type_begin;
                    size_t after_type;
                    if (type_begin < length &&
                        source[type_begin] == (unsigned char)')') {
                        payload = type_begin + 1u;
                        break;
                    }
                    while (type_end < length &&
                           vitte_main_source_identifier_byte(source[type_end])) {
                        ++type_end;
                    }
                    after_type = vitte_main_source_skip_space(
                        source,
                        length,
                        type_end);
                    if (!vitte_main_pick_payload_type_is_native(
                            source + type_begin,
                            type_end - type_begin) ||
                        after_type >= length ||
                        (source[after_type] != (unsigned char)',' &&
                         source[after_type] != (unsigned char)')') ||
                        payload_fields >= 16u) {
                        payload_valid = false;
                        break;
                    }
                    ++payload_fields;
                    if (source[after_type] == (unsigned char)')') {
                        payload = after_type + 1u;
                        break;
                    }
                    payload = after_type + 1u;
                }
                if (!payload_valid || payload > length) {
                    return true;
                }
                body = payload - 1u;
#endif
            }
            if (source[body] == (unsigned char)'{') {
                ++depth;
            } else if (source[body] == (unsigned char)'}') {
                --depth;
            }
            ++body;
        }
    }
    return false;
}

static bool
vitte_main_source_has_export_alias(
    const unsigned char *source,
    size_t length)
{
    size_t index;
    static const char export_word[] = "export";
    static const char as_word[] = "as";

    if (source == NULL) {
        return false;
    }
    for (index = 0u; index + sizeof(export_word) - 1u <= length; ++index) {
        size_t cursor;
        size_t name_begin;
        size_t name_end;

        if ((index != 0u &&
             vitte_main_source_identifier_byte(source[index - 1u])) ||
            memcmp(source + index,
                   export_word,
                   sizeof(export_word) - 1u) != 0 ||
            (index + sizeof(export_word) - 1u < length &&
             vitte_main_source_identifier_byte(
                 source[index + sizeof(export_word) - 1u]))) {
            continue;
        }
        cursor = vitte_main_source_skip_space(
            source,
            length,
            index + sizeof(export_word) - 1u);
        name_begin = cursor;
        while (cursor < length &&
               vitte_main_source_identifier_byte(source[cursor])) {
            ++cursor;
        }
        name_end = cursor;
        if (name_begin == name_end) {
            continue;
        }
        cursor = vitte_main_source_skip_space(source, length, cursor);
        if (cursor + sizeof(as_word) - 1u <= length &&
            memcmp(source + cursor, as_word, sizeof(as_word) - 1u) == 0 &&
            (cursor + sizeof(as_word) - 1u == length ||
             !vitte_main_source_identifier_byte(
                 source[cursor + sizeof(as_word) - 1u]))) {
            return true;
        }
    }
    return false;
}

static bool
vitte_main_source_has_named_call(
    const unsigned char *source,
    size_t length,
    const char *name)
{
    size_t name_length;
    size_t index;

    if (source == NULL || name == NULL) {
        return false;
    }
    name_length = strlen(name);
    if (name_length == 0u || name_length > length) {
        return false;
    }
    for (index = 0u; index + name_length <= length; ++index) {
        size_t cursor;
        size_t end = index + name_length;

        if ((index != 0u &&
             vitte_main_source_identifier_byte(source[index - 1u])) ||
            (end < length &&
             vitte_main_source_identifier_byte(source[end])) ||
            memcmp(source + index, name, name_length) != 0) {
            continue;
        }
        cursor = vitte_main_source_skip_space(source, length, end);
        if (cursor < length && source[cursor] == (unsigned char)'(') {
            return true;
        }
    }
    return false;
}

static bool
vitte_main_source_has_borrowed_call(
    const unsigned char *source,
    size_t length)
{
    size_t index;

    if (source == NULL) {
        return false;
    }
    for (index = 0u; index < length; ++index) {
        size_t cursor;
        size_t name_begin;

        if (source[index] != (unsigned char)'&') {
            continue;
        }
        cursor = vitte_main_source_skip_space(source, length, index + 1u);
        name_begin = cursor;
        while (cursor < length &&
               vitte_main_source_identifier_byte(source[cursor])) {
            ++cursor;
        }
        if (cursor == name_begin) {
            continue;
        }
        cursor = vitte_main_source_skip_space(source, length, cursor);
        if (cursor < length && source[cursor] == (unsigned char)'(') {
            return true;
        }
    }
    return false;
}

static bool
vitte_main_source_has_if_block_local(
    const unsigned char *source,
    size_t length)
{
    size_t index;

    if (source == NULL) {
        return false;
    }
    for (index = 0u; index + 2u <= length; ++index) {
        size_t cursor;

        if (source[index] != (unsigned char)'i' ||
            source[index + 1u] != (unsigned char)'f' ||
            (index != 0u &&
             vitte_main_source_identifier_byte(source[index - 1u])) ||
            (index + 2u < length &&
             vitte_main_source_identifier_byte(source[index + 2u]))) {
            continue;
        }
        cursor = index + 2u;
        while (cursor < length && source[cursor] != (unsigned char)'{' &&
               source[cursor] != (unsigned char)';' &&
               source[cursor] != (unsigned char)'}') {
            ++cursor;
        }
        if (cursor == length || source[cursor] != (unsigned char)'{') {
            continue;
        }
        cursor = vitte_main_source_skip_space(source, length, cursor + 1u);
        if (cursor + 3u <= length &&
            memcmp(source + cursor, "let", 3u) == 0 &&
            (cursor + 3u == length ||
             !vitte_main_source_identifier_byte(source[cursor + 3u]))) {
            return true;
        }
    }
    return false;
}

/* Keep language features without complete native lowering on the compatibility
 * pipeline. Form fields, including fixed arrays, are selected by the native
 * frontend's AST-aware HIR/IR support check. */
static bool
vitte_main_source_requires_compatibility_frontend(
    const unsigned char *source,
    size_t length)
{
    return vitte_main_source_has_unsupported_pick(source, length) ||
           vitte_main_source_has_word(source, length, "requires") ||
           vitte_main_source_has_word(source, length, "ensures") ||
           vitte_main_source_has_export_alias(source, length) ||
           vitte_main_source_has_named_call(source, length, "trim") ||
           vitte_main_source_has_named_call(source, length, "starts_with") ||
           vitte_main_source_has_named_call(source, length, "ends_with") ||
           vitte_main_source_has_borrowed_call(source, length) ||
           vitte_main_source_has_if_block_local(source, length) ||
           vitte_main_source_has_word(source, length, "comptime") ||
           vitte_main_source_has_word(source, length, "unsafe");
}
#endif

bool
vitte_main_driver_lex(
    vitte_driver_t *driver,
    vitte_driver_phase_t phase,
    const vitte_driver_input_t *input,
    void *user_data)
{
    vitte_main_driver_context_t *context;

    (void)driver;
    (void)phase;
    (void)input;

    context = (vitte_main_driver_context_t *)user_data;

    if (context == NULL ||
        context->options == NULL ||
        context->file.data == NULL ||
        context->source_id == VITTE_SOURCE_INVALID_ID) {
        return false;
    }

#ifdef VITTE_NATIVE_FRONTEND_LINKED
    /*
     * Route compile, check, run, and --emit-ir through one Vitte pipeline.
     * Inspection commands and earlier explicit phase stops retain their
     * dedicated CLI behavior. Known unsupported language features are kept
     * on the bootstrap compatibility pipeline before selecting the native
     * frontend; a failed native invocation stays authoritative.
     */
    if ((context->options->command == VITTE_MAIN_COMMAND_COMPILE ||
         context->options->command == VITTE_MAIN_COMMAND_RUN ||
         context->options->command == VITTE_MAIN_COMMAND_CHECK ||
         context->options->emit_ir ||
         context->options->stop_after_sema) &&
        !vitte_main_source_has_import_keyword(
            context->file.data,
            context->file.length) &&
        !vitte_main_source_requires_compatibility_frontend(
            context->file.data,
            context->file.length) &&
        !context->options->stop_after_lexer &&
        !context->options->stop_after_parser &&
        !context->options->emit_tokens &&
        !context->options->emit_ast &&
        !context->options->emit_hir) {
        uint8_t *native_output = NULL;
        size_t native_output_length = 0u;
        vitte_cli_diagnostic_t *native_diagnostics = NULL;
        size_t native_diagnostic_count = 0u;
        const vitte_source_entry_t *registered_source;
        const char *source_name;
        uint32_t native_action = context->options->emit_ir
            ? VITTE_NATIVE_FRONTEND_ACTION_EMIT_IR
            : ((context->options->command == VITTE_MAIN_COMMAND_CHECK ||
                context->options->stop_after_sema)
                ? VITTE_NATIVE_FRONTEND_ACTION_CHECK
                : VITTE_NATIVE_FRONTEND_ACTION_COMPILE);
        uint32_t native_status =
            vitte_native_frontend_execute_with_diagnostics(
                native_action,
                (const char *)context->file.data,
                context->file.length,
                (uint64_t)context->source_id,
                context->options->input_path,
                &context->options->config,
                &native_output,
                &native_output_length,
                &native_diagnostics,
                &native_diagnostic_count);

        registered_source = vitte_source_get(
            &context->sources,
            context->source_id);
        source_name = registered_source != NULL
            ? (registered_source->path != NULL
                ? registered_source->path
                : registered_source->name)
            : context->options->input_path;

        vitte_main_diagnostic_map_native_batch(
            context,
            native_diagnostics,
            native_diagnostic_count);

        if (native_diagnostic_count != 0u) {
            bool rendered = vitte_diagnostic_render_cli_batch(
                stderr,
                &context->sources,
                context->source_id,
                source_name,
                native_diagnostics,
                native_diagnostic_count,
                context->options->color,
                vitte_main_cli_diagnostic_format(
                    context->options->diagnostic_format));
            vitte_native_frontend_release_diagnostics(native_diagnostics);
            native_diagnostics = NULL;
            if (!rendered) {
                if (native_output != NULL) {
                    vitte_native_frontend_release_output(native_output);
                }
                vitte_main_error(
                    "failed to render native frontend diagnostics");
                return false;
            }
        } else if (native_diagnostics != NULL) {
            vitte_native_frontend_release_diagnostics(native_diagnostics);
            native_diagnostics = NULL;
        }

        if (native_status == 1u || native_status == 2u) {
            context->native_output = native_output;
            context->native_output_length = native_output_length;
            context->native_frontend_completed = true;
            vitte_main_verbose(
                context->options,
                "native Vitte frontend completed before the bootstrap lexer");
            return true;
        }
        if (native_output != NULL) {
            vitte_native_frontend_release_output(native_output);
        }
        context->native_frontend_completed = true;
        if (native_diagnostic_count == 0u) {
            if (!vitte_main_render_diagnostic(
                    context,
                    VITTE_CLI_DIAGNOSTIC_ERROR,
                    "VITTE_INFRA_E_PHASE",
                    "native frontend did not complete",
                    "no structured diagnostics or native pipeline output were returned; bootstrap fallback is disabled for this command",
                    NULL,
                    NULL,
                    "native frontend")) {
                vitte_main_error(
                    "failed to render native frontend failure");
            }
        }
        vitte_main_verbose(
            context->options,
            "native frontend failure is authoritative; bootstrap frontend fallback is disabled");
        return false;
    }
#endif

    vitte_main_verbose(context->options, "lexing");

    if (!vitte_lexer_init(
            &context->lexer,
            (const char *)context->file.data,
            context->file.length,
            (uint32_t)context->source_id)) {
        vitte_main_error("failed to initialize lexer");
        return false;
    }

    context->lexer_initialized = true;

    if (!vitte_lexer_run(&context->lexer)) {
        if (!vitte_main_render_lexer_diagnostic(context)) {
            vitte_main_error(
                "failed to render lexical diagnostic");
        }
        return false;
    }

    if (context->options->emit_tokens) {
        vitte_main_print_tokens(&context->lexer);
    }

    return true;
}

static bool
vitte_main_render_diagnostic(
    const vitte_main_driver_context_t *context,
    vitte_cli_diagnostic_severity_t severity,
    const char *code,
    const char *message,
    const char *details,
    const vitte_parser_span_t *source_span,
    const char *procedure,
    const char *diagnostic_context)
{
    const vitte_source_entry_t *source;
    vitte_source_id_t diagnostic_source_id;
    vitte_source_location_t start;
    vitte_source_location_t end;
    size_t diagnostic_begin;
    size_t diagnostic_end;
    size_t start_line;
    size_t start_column;
    size_t end_line;
    size_t end_column;
    const char *source_name;
    bool has_span;

    if (context == NULL ||
        context->options == NULL) {
        return false;
    }

    has_span = source_span != NULL &&
        source_span->valid &&
        source_span->begin <= source_span->end;
    diagnostic_source_id = context->source_id;
    diagnostic_begin = has_span ? source_span->begin : 0u;
    diagnostic_end = has_span ? source_span->end : 0u;
    if (has_span && source_span->end <= context->file.length) {
        (void)vitte_main_origin_span(
            context,
            source_span->begin,
            source_span->end,
            &diagnostic_source_id,
            &diagnostic_begin,
            &diagnostic_end);
    }

    source = vitte_source_get(
        &context->sources,
        diagnostic_source_id);
    source_name = source != NULL
        ? (source->path != NULL ? source->path : source->name)
        : context->options->input_path;

    start_line = source_span != NULL
        ? source_span->line
        : 0u;
    start_column = source_span != NULL
        ? source_span->column
        : 0u;
    end_line = start_line;
    end_column = start_column;

    if (has_span) {
        if (vitte_source_location_from_offset(
                &context->sources,
                diagnostic_source_id,
                diagnostic_begin,
                &start) &&
            start.valid) {
            start_line = start.line - 1u;
            start_column = start.column - 1u;
        }

        if (vitte_source_location_from_offset(
                &context->sources,
                diagnostic_source_id,
                diagnostic_end,
                &end) &&
            end.valid) {
            end_line = end.line - 1u;
            end_column = end.column - 1u;
        } else {
            end_column += diagnostic_end - diagnostic_begin;
        }
    }

    return vitte_diagnostic_render_cli(
        stderr,
        &context->sources,
        diagnostic_source_id,
        source_name,
        severity,
        code,
        message,
        details,
        has_span,
        has_span ? (uint32_t)diagnostic_source_id : 0u,
        has_span ? diagnostic_begin : 0u,
        has_span ? diagnostic_end : 0u,
        start_line,
        start_column,
        end_line,
        end_column,
        procedure,
        diagnostic_context,
        context->options->color,
        vitte_main_cli_diagnostic_format(
            context->options->diagnostic_format));
}

static bool
vitte_main_make_cli_diagnostic(
    const vitte_main_driver_context_t *context,
    vitte_cli_diagnostic_t *diagnostic,
    vitte_cli_diagnostic_severity_t severity,
    const char *code,
    const char *message,
    const char *details,
    const vitte_parser_span_t *source_span,
    const char *procedure,
    const char *diagnostic_context)
{
    const vitte_source_entry_t *source;
    vitte_source_id_t diagnostic_source_id;
    vitte_source_location_t location;
    size_t diagnostic_begin;
    size_t diagnostic_end;

    if (context == NULL ||
        context->options == NULL ||
        diagnostic == NULL ||
        code == NULL) {
        return false;
    }

    vitte_cli_diagnostic_init(diagnostic);
    diagnostic->severity = severity;
    diagnostic->code = code;
    diagnostic->message = message;
    diagnostic->details = details;
    diagnostic->procedure = procedure;
    diagnostic->context = diagnostic_context;
    diagnostic->end_offset = 0u;
    diagnostic->file_id = 0u;
    diagnostic->source_id = VITTE_SOURCE_INVALID_ID;
    diagnostic->source_name = NULL;

    if (source_span == NULL ||
        !source_span->valid ||
        source_span->begin > source_span->end ||
        source_span->end > context->file.length) {
        return true;
    }

    diagnostic_source_id = context->source_id;
    diagnostic_begin = source_span->begin;
    diagnostic_end = source_span->end;
    (void)vitte_main_origin_span(
        context,
        source_span->begin,
        source_span->end,
        &diagnostic_source_id,
        &diagnostic_begin,
        &diagnostic_end);

    source = vitte_source_get(
        &context->sources,
        diagnostic_source_id);
    diagnostic->has_span = true;
    diagnostic->file_id = (uint32_t)diagnostic_source_id;
    diagnostic->source_id = diagnostic_source_id;
    diagnostic->source_name = source != NULL
        ? source->path
        : context->options->input_path;
    diagnostic->start_offset = diagnostic_begin;
    diagnostic->end_offset = diagnostic_end;
    diagnostic->start_line = source_span->line;
    diagnostic->start_column = source_span->column;
    diagnostic->end_line = source_span->line;
    diagnostic->end_column = source_span->column;

    if (vitte_source_location_from_offset(
                &context->sources,
                diagnostic_source_id,
                diagnostic_begin,
                &location) &&
        location.valid) {
        diagnostic->start_line = location.line - 1u;
        diagnostic->start_column = location.column - 1u;
    }

    if (vitte_source_location_from_offset(
                &context->sources,
                diagnostic_source_id,
                diagnostic_end,
                &location) &&
        location.valid) {
        diagnostic->end_line = location.line - 1u;
        diagnostic->end_column = location.column - 1u;
    } else {
        diagnostic->end_column +=
                diagnostic_end - diagnostic_begin;
    }

    return true;
}

static const char *
vitte_main_parser_diagnostic_code(
    vitte_parser_error_t error)
{
    switch (error) {
        case VITTE_PARSER_ERROR_EXPECTED_TOKEN:
        case VITTE_PARSER_ERROR_EXPECTED_IDENTIFIER:
            return "VITTE_PARSER_E_EXPECTED_TOKEN";

        case VITTE_PARSER_ERROR_EXPECTED_EXPRESSION:
            return "VITTE_PARSER_E_EXPECTED_EXPRESSION";

        case VITTE_PARSER_ERROR_EXPECTED_TYPE:
            return "VITTE_PARSER_E_EXPECTED_TYPE";

        case VITTE_PARSER_ERROR_EXPECTED_DECLARATION:
            return "VITTE_PARSER_E_EXPECTED_DECLARATION";

        case VITTE_PARSER_ERROR_UNEXPECTED_TOKEN:
        case VITTE_PARSER_ERROR_EXPECTED_STATEMENT:
            return "VITTE_PARSER_E_UNEXPECTED_TOKEN";

        case VITTE_PARSER_ERROR_UNSUPPORTED:
            return "VITTE_PARSER_E_UNSUPPORTED_SYNTAX";

        case VITTE_PARSER_ERROR_INVALID_MODULE_PATH:
            return "VITTE_PARSER_E_INVALID_MODULE_PATH";
        case VITTE_PARSER_ERROR_DUPLICATE_MODULE_HEADER:
            return "VITTE_PARSER_E_DUPLICATE_MODULE_HEADER";
        case VITTE_PARSER_ERROR_MISPLACED_MODULE_HEADER:
            return "VITTE_PARSER_E_MISPLACED_MODULE_HEADER";
        case VITTE_PARSER_ERROR_INVALID_USE_DECLARATION:
            return "VITTE_PARSER_E_INVALID_USE_DECLARATION";
        case VITTE_PARSER_ERROR_INVALID_USE_ALIAS:
            return "VITTE_PARSER_E_INVALID_USE_ALIAS";
        case VITTE_PARSER_ERROR_INVALID_IMPORT_GROUP:
            return "VITTE_PARSER_E_INVALID_IMPORT_GROUP";
        case VITTE_PARSER_ERROR_INVALID_GLOB_IMPORT:
            return "VITTE_PARSER_E_INVALID_GLOB_IMPORT";
        case VITTE_PARSER_ERROR_INVALID_EXPORT_CLAUSE:
            return "VITTE_PARSER_E_INVALID_EXPORT_CLAUSE";
        case VITTE_PARSER_ERROR_EMPTY_EXPORT_GROUP:
            return "VITTE_PARSER_E_EMPTY_EXPORT_GROUP";
        case VITTE_PARSER_ERROR_INVALID_EXPORT_ALIAS:
            return "VITTE_PARSER_E_INVALID_EXPORT_ALIAS";
        case VITTE_PARSER_ERROR_TRAILING_TOKENS:
            return "VITTE_PARSER_E_TRAILING_TOKENS";
        case VITTE_PARSER_ERROR_MISSING_MODULE_HEADER:
            return "VITTE_PARSER_E_MISSING_MODULE_HEADER";
        case VITTE_PARSER_ERROR_INVALID_VALUE_DECLARATION:
            return "VITTE_PARSER_E_INVALID_VALUE_DECLARATION";
        case VITTE_PARSER_ERROR_MISSING_VALUE_INITIALIZER:
            return "VITTE_PARSER_E_MISSING_VALUE_INITIALIZER";
        case VITTE_PARSER_ERROR_INVALID_TYPE_DECLARATION:
            return "VITTE_PARSER_E_INVALID_TYPE_DECLARATION";
        case VITTE_PARSER_ERROR_INVALID_OPAQUE_DECLARATION:
            return "VITTE_PARSER_E_INVALID_OPAQUE_DECLARATION";
        case VITTE_PARSER_ERROR_INVALID_FORM_DECLARATION:
            return "VITTE_PARSER_E_INVALID_FORM_DECLARATION";
        case VITTE_PARSER_ERROR_DUPLICATE_FORM_FIELD:
            return "VITTE_PARSER_E_DUPLICATE_FORM_FIELD";
        case VITTE_PARSER_ERROR_INVALID_FORM_FIELD_SEPARATOR:
            return "VITTE_PARSER_E_INVALID_FORM_FIELD_SEPARATOR";
        case VITTE_PARSER_ERROR_INVALID_PICK_DECLARATION:
            return "VITTE_PARSER_E_INVALID_PICK_DECLARATION";
        case VITTE_PARSER_ERROR_DUPLICATE_PICK_VARIANT:
            return "VITTE_PARSER_E_DUPLICATE_PICK_VARIANT";
        case VITTE_PARSER_ERROR_INVALID_PICK_VARIANT_PAYLOAD:
            return "VITTE_PARSER_E_INVALID_PICK_VARIANT_PAYLOAD";
        case VITTE_PARSER_ERROR_INVALID_TRAIT_DECLARATION:
            return "VITTE_PARSER_E_INVALID_TRAIT_DECLARATION";
        case VITTE_PARSER_ERROR_INVALID_IMPL_DECLARATION:
            return "VITTE_PARSER_E_INVALID_IMPL_DECLARATION";
        case VITTE_PARSER_ERROR_MISSING_IMPL_TARGET_TYPE:
            return "VITTE_PARSER_E_MISSING_IMPL_TARGET_TYPE";
        case VITTE_PARSER_ERROR_INVALID_IMPL_FOR_CLAUSE:
            return "VITTE_PARSER_E_INVALID_IMPL_FOR_CLAUSE";
        case VITTE_PARSER_ERROR_INVALID_PROCEDURE_DECLARATION:
            return "VITTE_PARSER_E_INVALID_PROCEDURE_DECLARATION";
        case VITTE_PARSER_ERROR_INVALID_PROCEDURE_PARAMETERS:
            return "VITTE_PARSER_E_INVALID_PROCEDURE_PARAMETERS";
        case VITTE_PARSER_ERROR_INVALID_PROCEDURE_RETURN_TYPE:
            return "VITTE_PARSER_E_INVALID_PROCEDURE_RETURN_TYPE";
        case VITTE_PARSER_ERROR_INVALID_PROCEDURE_MODIFIER:
            return "VITTE_PARSER_E_INVALID_PROCEDURE_MODIFIER";
        case VITTE_PARSER_ERROR_INVALID_PROCEDURE_BODY_FORM:
            return "VITTE_PARSER_E_INVALID_PROCEDURE_BODY_FORM";
        case VITTE_PARSER_ERROR_INVALID_EXTERNAL_PROCEDURE:
            return "VITTE_PARSER_E_INVALID_EXTERNAL_PROCEDURE";
        case VITTE_PARSER_ERROR_INVALID_INTRINSIC_DECLARATION:
            return "VITTE_PARSER_E_INVALID_INTRINSIC_DECLARATION";
        case VITTE_PARSER_ERROR_INVALID_ABI_STRING_LITERAL:
            return "VITTE_PARSER_E_INVALID_ABI_STRING_LITERAL";
        case VITTE_PARSER_ERROR_INVALID_GENERIC_PARAMETER_LIST:
            return "VITTE_PARSER_E_INVALID_GENERIC_PARAMETER_LIST";
        case VITTE_PARSER_ERROR_INVALID_CONST_GENERIC_PARAMETER:
            return "VITTE_PARSER_E_INVALID_CONST_GENERIC_PARAMETER";
        case VITTE_PARSER_ERROR_INVALID_GENERIC_BOUND:
            return "VITTE_PARSER_E_INVALID_GENERIC_BOUND";
        case VITTE_PARSER_ERROR_INVALID_WHERE_CLAUSE:
            return "VITTE_PARSER_E_INVALID_WHERE_CLAUSE";
        case VITTE_PARSER_ERROR_INVALID_REQUIRES_CLAUSE:
            return "VITTE_PARSER_E_INVALID_REQUIRES_CLAUSE";
        case VITTE_PARSER_ERROR_INVALID_ENSURES_CLAUSE:
            return "VITTE_PARSER_E_INVALID_ENSURES_CLAUSE";
        case VITTE_PARSER_ERROR_INVALID_MACRO_DECLARATION:
            return "VITTE_PARSER_E_INVALID_MACRO_DECLARATION";
        case VITTE_PARSER_ERROR_INVALID_MACRO_PARAMETERS:
            return "VITTE_PARSER_E_INVALID_MACRO_PARAMETERS";
        case VITTE_PARSER_ERROR_INVALID_TEST_DECLARATION:
            return "VITTE_PARSER_E_INVALID_TEST_DECLARATION";
        case VITTE_PARSER_ERROR_INVALID_DECLARATION_MODIFIER_ORDER:
            return "VITTE_PARSER_E_INVALID_DECLARATION_MODIFIER_ORDER";
        case VITTE_PARSER_ERROR_INVALID_ATTRIBUTE:
            return "VITTE_PARSER_E_INVALID_ATTRIBUTE";
        case VITTE_PARSER_ERROR_INVALID_ATTRIBUTE_NAME:
            return "VITTE_PARSER_E_INVALID_ATTRIBUTE_NAME";
        case VITTE_PARSER_ERROR_INVALID_ATTRIBUTE_ARGUMENTS:
            return "VITTE_PARSER_E_INVALID_ATTRIBUTE_ARGUMENTS";
        case VITTE_PARSER_ERROR_MISSING_ATTRIBUTE_VALUE:
            return "VITTE_PARSER_E_MISSING_ATTRIBUTE_VALUE";
        case VITTE_PARSER_ERROR_INVALID_ATTRIBUTE_LITERAL:
            return "VITTE_PARSER_E_INVALID_ATTRIBUTE_LITERAL";
        case VITTE_PARSER_ERROR_UNEXPECTED_DECLARATION:
            return "VITTE_PARSER_E_UNEXPECTED_DECLARATION";
        case VITTE_PARSER_ERROR_UNEXPECTED_STATEMENT:
            return "VITTE_PARSER_E_UNEXPECTED_STATEMENT";
        case VITTE_PARSER_ERROR_UNEXPECTED_EXPRESSION:
            return "VITTE_PARSER_E_UNEXPECTED_EXPRESSION";
        case VITTE_PARSER_ERROR_INVALID_PROCEDURE_TYPE:
            return "VITTE_PARSER_E_INVALID_PROCEDURE_TYPE";
        case VITTE_PARSER_ERROR_INVALID_REFERENCE_TYPE:
            return "VITTE_PARSER_E_INVALID_REFERENCE_TYPE";
        case VITTE_PARSER_ERROR_INVALID_POINTER_TYPE:
            return "VITTE_PARSER_E_INVALID_POINTER_TYPE";
        case VITTE_PARSER_ERROR_INVALID_ARRAY_TYPE:
            return "VITTE_PARSER_E_INVALID_ARRAY_TYPE";
        case VITTE_PARSER_ERROR_INVALID_SLICE_TYPE:
            return "VITTE_PARSER_E_INVALID_SLICE_TYPE";
        case VITTE_PARSER_ERROR_INVALID_OPTIONAL_TYPE:
            return "VITTE_PARSER_E_INVALID_OPTIONAL_TYPE";
        case VITTE_PARSER_ERROR_INVALID_TYPE_PATH:
            return "VITTE_PARSER_E_INVALID_TYPE_PATH";
        case VITTE_PARSER_ERROR_INVALID_GENERIC_ARGUMENTS:
            return "VITTE_PARSER_E_INVALID_GENERIC_ARGUMENTS";
        case VITTE_PARSER_ERROR_INVALID_LET_STATEMENT:
            return "VITTE_PARSER_E_INVALID_LET_STATEMENT";
        case VITTE_PARSER_ERROR_INVALID_SET_STATEMENT:
            return "VITTE_PARSER_E_INVALID_SET_STATEMENT";
        case VITTE_PARSER_ERROR_INVALID_ASSIGNMENT_OPERATOR:
            return "VITTE_PARSER_E_INVALID_ASSIGNMENT_OPERATOR";
        case VITTE_PARSER_ERROR_INVALID_RETURN_STATEMENT:
            return "VITTE_PARSER_E_INVALID_RETURN_STATEMENT";
        case VITTE_PARSER_ERROR_INVALID_JUMP_STATEMENT:
            return "VITTE_PARSER_E_INVALID_JUMP_STATEMENT";
        case VITTE_PARSER_ERROR_INVALID_DEFER_STATEMENT:
            return "VITTE_PARSER_E_INVALID_DEFER_STATEMENT";
        case VITTE_PARSER_ERROR_INVALID_ASSERT_STATEMENT:
            return "VITTE_PARSER_E_INVALID_ASSERT_STATEMENT";
        case VITTE_PARSER_ERROR_INVALID_UNSAFE_STATEMENT:
            return "VITTE_PARSER_E_INVALID_UNSAFE_STATEMENT";
        case VITTE_PARSER_ERROR_INVALID_ASM_STATEMENT:
            return "VITTE_PARSER_E_INVALID_ASM_STATEMENT";
        case VITTE_PARSER_ERROR_INVALID_CONTROL_FLOW_STATEMENT:
            return "VITTE_PARSER_E_INVALID_CONTROL_FLOW_STATEMENT";
        case VITTE_PARSER_ERROR_INVALID_MATCH_STATEMENT:
            return "VITTE_PARSER_E_INVALID_MATCH_STATEMENT";
        case VITTE_PARSER_ERROR_INVALID_MATCH_ARM:
            return "VITTE_PARSER_E_INVALID_MATCH_ARM";
        case VITTE_PARSER_ERROR_INVALID_PATTERN:
            return "VITTE_PARSER_E_INVALID_PATTERN";
        case VITTE_PARSER_ERROR_INVALID_RANGE_PATTERN:
            return "VITTE_PARSER_E_INVALID_RANGE_PATTERN";
        case VITTE_PARSER_ERROR_INVALID_ASSIGNMENT_EXPRESSION:
            return "VITTE_PARSER_E_INVALID_ASSIGNMENT_EXPRESSION";
        case VITTE_PARSER_ERROR_INVALID_COALESCING_EXPRESSION:
            return "VITTE_PARSER_E_INVALID_COALESCING_EXPRESSION";
        case VITTE_PARSER_ERROR_INVALID_LOGICAL_EXPRESSION:
            return "VITTE_PARSER_E_INVALID_LOGICAL_EXPRESSION";
        case VITTE_PARSER_ERROR_INVALID_BITWISE_EXPRESSION:
            return "VITTE_PARSER_E_INVALID_BITWISE_EXPRESSION";
        case VITTE_PARSER_ERROR_INVALID_COMPARISON_EXPRESSION:
            return "VITTE_PARSER_E_INVALID_COMPARISON_EXPRESSION";
        case VITTE_PARSER_ERROR_INVALID_SHIFT_EXPRESSION:
            return "VITTE_PARSER_E_INVALID_SHIFT_EXPRESSION";
        case VITTE_PARSER_ERROR_INVALID_ARITHMETIC_EXPRESSION:
            return "VITTE_PARSER_E_INVALID_ARITHMETIC_EXPRESSION";
        case VITTE_PARSER_ERROR_INVALID_RANGE_EXPRESSION:
            return "VITTE_PARSER_E_INVALID_RANGE_EXPRESSION";
        case VITTE_PARSER_ERROR_INVALID_UNARY_EXPRESSION:
            return "VITTE_PARSER_E_INVALID_UNARY_EXPRESSION";
        case VITTE_PARSER_ERROR_INVALID_CAST_EXPRESSION:
            return "VITTE_PARSER_E_INVALID_CAST_EXPRESSION";
        case VITTE_PARSER_ERROR_INVALID_POSTFIX_EXPRESSION:
            return "VITTE_PARSER_E_INVALID_POSTFIX_EXPRESSION";
        case VITTE_PARSER_ERROR_INVALID_IF_EXPRESSION:
            return "VITTE_PARSER_E_INVALID_IF_EXPRESSION";
        case VITTE_PARSER_ERROR_INVALID_BLOCK_EXPRESSION:
            return "VITTE_PARSER_E_INVALID_BLOCK_EXPRESSION";
        case VITTE_PARSER_ERROR_INVALID_ARRAY_EXPRESSION:
            return "VITTE_PARSER_E_INVALID_ARRAY_EXPRESSION";
        case VITTE_PARSER_ERROR_INVALID_SIZEOF_EXPRESSION:
            return "VITTE_PARSER_E_INVALID_SIZEOF_EXPRESSION";
        case VITTE_PARSER_ERROR_INVALID_ALIGNOF_EXPRESSION:
            return "VITTE_PARSER_E_INVALID_ALIGNOF_EXPRESSION";
        case VITTE_PARSER_ERROR_INVALID_NULL_CONSTRUCTION:
            return "VITTE_PARSER_E_INVALID_NULL_CONSTRUCTION";
        case VITTE_PARSER_ERROR_INVALID_EXPRESSION_LIST:
            return "VITTE_PARSER_E_INVALID_EXPRESSION_LIST";

        case VITTE_PARSER_ERROR_OUT_OF_MEMORY:
        case VITTE_PARSER_ERROR_OVERFLOW:
        case VITTE_PARSER_ERROR_NODE_LIMIT:
        case VITTE_PARSER_ERROR_DIAGNOSTIC_LIMIT:
        case VITTE_PARSER_ERROR_RECURSION_LIMIT:
            return "VITTE_INFRA_E_RESOURCE";

        case VITTE_PARSER_ERROR_INVALID_TOKEN:
        case VITTE_PARSER_ERROR_INVALID_NODE:
        case VITTE_PARSER_ERROR_VALIDATION:
        case VITTE_PARSER_ERROR_CORRUPTION:
            return "VITTE_INFRA_E_INVARIANT";

        case VITTE_PARSER_ERROR_NONE:
        case VITTE_PARSER_ERROR_INVALID_ARGUMENT:
        case VITTE_PARSER_ERROR_INVALID_CONTEXT:
        case VITTE_PARSER_ERROR_INVALID_STATE:
        case VITTE_PARSER_ERROR_INTERNAL:
        case VITTE_PARSER_ERROR_COUNT:
            return "VITTE_INFRA_E_INTERNAL";
    }

    return "VITTE_INFRA_E_INTERNAL";
}

/*
 * Lexical diagnostics are reported before the parser has a token that can
 * describe the bad source text.  Keep a small, escaped rendering here so
 * control bytes and malformed UTF-8 never leak into terminal diagnostics or
 * machine-readable output.
 */
static bool
vitte_main_lexer_append_escaped_byte(
    char *buffer,
    size_t capacity,
    size_t *length,
    unsigned char byte)
{
    char escaped[5];
    const char *text;
    size_t text_length;

    if (buffer == NULL || length == NULL || *length >= capacity) {
        return false;
    }

    text = NULL;
    text_length = 0u;
    switch (byte) {
        case '\\': text = "\\"; break;
        case '`':  text = "\\`"; break;
        case '\'': text = "'"; break;
        case '"':  text = "\""; break;
        case '\n': text = "\\n"; break;
        case '\r': text = "\\r"; break;
        case '\t': text = "\\t"; break;
        case '\f': text = "\\f"; break;
        case '\v': text = "\\v"; break;
        case '\b': text = "\\b"; break;
        case '\a': text = "\\a"; break;
        default:
            if (byte >= 0x20u && byte <= 0x7eu) {
                escaped[0] = (char)byte;
                escaped[1] = '\0';
                text = escaped;
            } else {
                int count = snprintf(
                    escaped,
                    sizeof(escaped),
                    "\\x%02X",
                    (unsigned)byte);
                if (count < 0 || (size_t)count >= sizeof(escaped)) {
                    return false;
                }
                text = escaped;
            }
            break;
    }

    text_length = strlen(text);
    if (text_length > capacity - *length - 1u) {
        return false;
    }

    memcpy(buffer + *length, text, text_length);
    *length += text_length;
    buffer[*length] = '\0';
    return true;
}

static size_t
vitte_main_lexer_display_end(
    const vitte_main_driver_context_t *context,
    vitte_lexer_error_t error,
    const vitte_lexer_span_t *span)
{
    size_t end;

    if (context == NULL || span == NULL) {
        return 0u;
    }

    end = span->end;
    if (end > context->file.length) {
        end = context->file.length;
    }

    /* scan_escape stops just before the byte that made the escape invalid. */
    if (error == VITTE_LEXER_ERROR_INVALID_ESCAPE &&
        end < context->file.length) {
        unsigned char next = context->file.data[end];
        if (next != (unsigned char)'"' &&
            next != (unsigned char)'\'' &&
            next != (unsigned char)'\n' &&
            next != (unsigned char)'\r') {
            ++end;
        }
    }

    /* Include the second quote in an empty character literal (`''`). */
    if (error == VITTE_LEXER_ERROR_INVALID_CHARACTER &&
        span->begin < context->file.length &&
        context->file.data[span->begin] == (unsigned char)'\'' &&
        end < context->file.length &&
        context->file.data[end] == (unsigned char)'\'') {
        ++end;
    }

    return end;
}

static bool
vitte_main_lexer_format_details(
    const vitte_main_driver_context_t *context,
    vitte_lexer_error_t error,
    const vitte_lexer_span_t *span,
    size_t display_end,
    char *storage,
    size_t capacity)
{
    const char *prefix;
    const char *suffix;
    const char *fixed;
    size_t length;
    size_t index;
    size_t fixed_length;
    int written;

    if (context == NULL || span == NULL || storage == NULL ||
        capacity == 0u || span->begin > display_end ||
        display_end > context->file.length) {
        return false;
    }

    if (error == VITTE_LEXER_ERROR_INVALID_UTF8) {
        vitte_unicode_result_t decoded;
        const char *encoding_context;

        decoded = vitte_unicode_decode_utf8(
            context->file.data + span->begin,
            display_end - span->begin);
        switch (decoded.error) {
            case VITTE_UNICODE_ERROR_TRUNCATED_UTF8:
                encoding_context = "truncated UTF-8 sequence";
                break;
            case VITTE_UNICODE_ERROR_OVERLONG_UTF8:
                encoding_context = "overlong UTF-8 encoding";
                break;
            case VITTE_UNICODE_ERROR_SURROGATE:
                encoding_context = "UTF-8 sequence encodes a surrogate value";
                break;
            case VITTE_UNICODE_ERROR_OUT_OF_RANGE:
                encoding_context = "UTF-8 sequence encodes a value above U+10FFFF";
                break;
            case VITTE_UNICODE_ERROR_INVALID_UTF8:
            default:
                encoding_context = "malformed UTF-8 sequence";
                break;
        }
        written = snprintf(
            storage,
            capacity,
            "%s `",
            encoding_context);
        if (written < 0 || (size_t)written >= capacity) {
            return false;
        }
        length = (size_t)written;
        for (index = span->begin; index < display_end; ++index) {
            if (!vitte_main_lexer_append_escaped_byte(
                    storage,
                    capacity,
                    &length,
                    context->file.data[index])) {
                return false;
            }
        }
        written = snprintf(
            storage + length,
            capacity - length,
            "` at UTF-8 byte range [%zu, %zu); UTF-8 must encode Unicode scalar values only",
            span->begin,
            display_end);
        return written >= 0 && (size_t)written < capacity - length;
    }

    if (error == VITTE_LEXER_ERROR_INVALID_IDENTIFIER) {
        const vitte_unicode_result_t decoded = vitte_unicode_decode_utf8(
            context->file.data + span->begin,
            display_end - span->begin);
        if (decoded.error != VITTE_UNICODE_ERROR_NONE) {
            return false;
        }
        written = snprintf(
            storage,
            capacity,
            "character U+%04X is not allowed at identifier start; UTF-8 byte range [%zu, %zu)",
            (unsigned)decoded.codepoint,
            span->begin,
            display_end);
        return written >= 0 && (size_t)written < capacity;
    }

    fixed = NULL;
    suffix = "";
    if (error == VITTE_LEXER_ERROR_UNTERMINATED_STRING) {
        fixed = "string opened with `\"` here is missing its closing `\"` delimiter";
    } else if (error == VITTE_LEXER_ERROR_UNTERMINATED_COMMENT) {
        fixed = "block comment opened with `/*` here is missing its matching `*/` terminator";
    } else if (error == VITTE_LEXER_ERROR_INVALID_CHARACTER_LITERAL) {
        prefix = "malformed character literal `";
        suffix = "`; expected exactly one Unicode scalar value or one supported escape sequence";
    } else if (error == VITTE_LEXER_ERROR_INVALID_ESCAPE) {
        prefix = "invalid escape sequence `";
        suffix = "`; accepted forms: `\\\\`, `\\'`, `\\\"`, `\\0`, `\\a`, `\\b`, `\\f`, `\\n`, `\\r`, `\\t`, `\\v`, `\\xNN` (2 hex digits), `\\uNNNN` (4), and `\\UNNNNNNNN` (8)";
    } else if (error == VITTE_LEXER_ERROR_INVALID_UTF8) {
        prefix = "invalid Unicode sequence `";
    } else if (error == VITTE_LEXER_ERROR_INVALID_IDENTIFIER) {
        prefix = "character `";
        suffix = " cannot start an identifier";
    } else if (error == VITTE_LEXER_ERROR_INVALID_NUMBER) {
        prefix = "invalid numeric literal `";
    } else if (error == VITTE_LEXER_ERROR_INVALID_INTEGER) {
        prefix = "malformed integer literal `";
    } else if (error == VITTE_LEXER_ERROR_INVALID_FLOAT) {
        prefix = "malformed floating-point literal `";
    } else if (error == VITTE_LEXER_ERROR_INVALID_NUMERIC_SEPARATOR) {
        prefix = "invalid numeric separator in literal `";
        suffix = "`; underscores must occur singly between digits";
    } else if (error == VITTE_LEXER_ERROR_INVALID_NUMERIC_BASE) {
        prefix = "invalid digit or missing digits in based integer literal `";
        suffix = "`; expected digits valid for the literal's base";
        if (span->begin + 1u < display_end) {
            const unsigned char base_prefix = context->file.data[span->begin + 1u];
            if (base_prefix == (unsigned char)'b' || base_prefix == (unsigned char)'B') {
                prefix = "invalid digit or missing digits in base-2 integer literal `";
                suffix = "`; expected binary digits 0-1";
            } else if (base_prefix == (unsigned char)'o' || base_prefix == (unsigned char)'O') {
                prefix = "invalid digit or missing digits in base-8 integer literal `";
                suffix = "`; expected octal digits 0-7";
            } else if (base_prefix == (unsigned char)'x' || base_prefix == (unsigned char)'X') {
                prefix = "invalid digit or missing digits in base-16 integer literal `";
                suffix = "`; expected hexadecimal digits 0-9 or A-F";
            }
        }
    } else if (error == VITTE_LEXER_ERROR_INVALID_NUMERIC_SUFFIX) {
        prefix = "unsupported numeric suffix in literal `";
        suffix = "`; Vitte numeric literals do not define suffixes";
    } else if (error == VITTE_LEXER_ERROR_INVALID_EXPONENT) {
        prefix = "invalid exponent in literal `";
        suffix = "`; decimal exponents use `e` and hexadecimal exponents use `p`, followed by decimal digits";
    } else if (error == VITTE_LEXER_ERROR_INVALID_OPERATOR) {
        prefix = "invalid or incomplete operator/punctuation sequence `";
        suffix = "` is not a supported Vitte operator";
    } else if (error == VITTE_LEXER_ERROR_NUMBER_OVERFLOW) {
        prefix = "integer literal `";
        suffix = "` exceeds the supported unsigned 64-bit range [0, 18446744073709551615] (u64)";
    } else {
        prefix = "invalid character `";
    }

    length = 0u;
    storage[0] = '\0';
    if (fixed != NULL) {
        fixed_length = strlen(fixed);
        if (fixed_length >= capacity) {
            return false;
        }
        memcpy(storage, fixed, fixed_length + 1u);
        return true;
    }
    if (strlen(prefix) >= capacity) {
        return false;
    }
    memcpy(storage, prefix, strlen(prefix));
    length = strlen(prefix);

    if (span->begin == display_end) {
        const char *empty = "<empty>";
        if (strlen(empty) > capacity - length - 2u) {
            return false;
        }
        memcpy(storage + length, empty, strlen(empty));
        length += strlen(empty);
    } else {
        for (index = span->begin; index < display_end; ++index) {
            if (!vitte_main_lexer_append_escaped_byte(
                    storage,
                    capacity,
                    &length,
                    context->file.data[index])) {
                return false;
            }
        }
    }

    if (capacity - length < 2u) {
        return false;
    }
    storage[length++] = '`';
    if (strlen(suffix) >= capacity - length) {
        return false;
    }
    if (suffix[0] != '\0') {
        memcpy(storage + length, suffix, strlen(suffix));
        length += strlen(suffix);
    }
    if (error == VITTE_LEXER_ERROR_INVALID_CHARACTER ||
        error == VITTE_LEXER_ERROR_INVALID_TOKEN ||
        error == VITTE_LEXER_ERROR_INVALID_CHARACTER_LITERAL ||
        error == VITTE_LEXER_ERROR_INVALID_INTEGER ||
        error == VITTE_LEXER_ERROR_INVALID_FLOAT ||
        error == VITTE_LEXER_ERROR_INVALID_NUMERIC_SEPARATOR ||
        error == VITTE_LEXER_ERROR_INVALID_NUMERIC_BASE ||
        error == VITTE_LEXER_ERROR_INVALID_NUMERIC_SUFFIX ||
        error == VITTE_LEXER_ERROR_INVALID_EXPONENT ||
        error == VITTE_LEXER_ERROR_INVALID_OPERATOR ||
        error == VITTE_LEXER_ERROR_NUMBER_OVERFLOW) {
        written = snprintf(
            storage + length,
            capacity - length,
            " at UTF-8 byte range [%zu, %zu)",
            span->begin,
            display_end);
        if (written < 0 || (size_t)written >= capacity - length) {
            return false;
        }
        length += (size_t)written;
    }
    storage[length] = '\0';
    return true;
}

static bool
vitte_main_lexer_literal_boundary(
    unsigned char byte)
{
    if (byte == (unsigned char)'\n' ||
        byte == (unsigned char)'\r' ||
        byte == (unsigned char)'\0' ||
        (byte >= 0x09u && byte <= 0x0du) ||
        byte == (unsigned char)' ' ||
        byte == (unsigned char)'\t') {
        return true;
    }

    switch (byte) {
        case ';': case ',': case ')': case ']': case '}':
        case ':': case '+': case '-': case '*': case '/':
        case '%': case '=': case '&': case '|': case '^':
        case '!': case '<': case '>': case '?': case '.':
            return true;
        default:
            return false;
    }
}

static void
vitte_main_lexer_add_delimiter_suggestion(
    const vitte_main_driver_context_t *context,
    vitte_lexer_error_t error,
    const vitte_lexer_span_t *span,
    size_t display_end,
    const char *source_name,
    vitte_cli_diagnostic_t *diagnostic,
    vitte_cli_diagnostic_suggestion_t *suggestion)
{
    const char *replacement;
    const char *message;
    size_t offset;
    bool machine_applicable;
    vitte_source_location_t location;

    if (context == NULL || span == NULL || source_name == NULL ||
        diagnostic == NULL || suggestion == NULL ||
        span->begin >= context->file.length ||
        display_end > context->file.length) {
        return;
    }

    replacement = NULL;
    message = NULL;
    offset = display_end;
    machine_applicable = false;

    /*
     * A double-quoted string can be closed automatically at a line boundary.
     * For a character literal, the same insertion can change the intended
     * value, so keep it advisory even when the byte boundary looks plausible.
     */
    if (error == VITTE_LEXER_ERROR_UNTERMINATED_STRING &&
        context->file.data[span->begin] == (unsigned char)'"' &&
        (offset == context->file.length ||
         context->file.data[offset] == (unsigned char)'\n' ||
         context->file.data[offset] == (unsigned char)'\r')) {
        replacement = "\"";
        message = "Insert the missing closing `\"` for this string literal.";
        machine_applicable = true;
    } else if ((error == VITTE_LEXER_ERROR_INVALID_CHARACTER ||
                error == VITTE_LEXER_ERROR_INVALID_CHARACTER_LITERAL) &&
               context->file.data[span->begin] == (unsigned char)'\'' &&
               span->begin + 1u < context->file.length &&
               context->file.data[span->begin + 1u] >= 0x20u &&
               context->file.data[span->begin + 1u] < 0x7fu &&
               context->file.data[span->begin + 1u] != (unsigned char)'\\' &&
               context->file.data[span->begin + 1u] != (unsigned char)'\'') {
        size_t candidate;
        size_t boundary;

        candidate = span->begin + 2u;
        boundary = candidate;
        while (boundary < context->file.length &&
               (context->file.data[boundary] == (unsigned char)' ' ||
                context->file.data[boundary] == (unsigned char)'\t')) {
            ++boundary;
        }
        if (candidate <= display_end &&
            (boundary == context->file.length ||
             vitte_main_lexer_literal_boundary(
                 context->file.data[boundary]))) {
            offset = candidate;
            replacement = "'";
            message = "Possible edit: insert a closing `'` after this one ASCII character; review manually because the intended character cannot be inferred.";
        }
    }

    if (replacement == NULL) {
        return;
    }

    memset(suggestion, 0, sizeof(*suggestion));
    suggestion->message = message;
    suggestion->applicability = machine_applicable
        ? VITTE_CLI_DIAGNOSTIC_APPLICABILITY_MACHINE
        : VITTE_CLI_DIAGNOSTIC_APPLICABILITY_MANUAL;
    suggestion->confidence = machine_applicable
        ? VITTE_CLI_DIAGNOSTIC_CONFIDENCE_HIGH
        : VITTE_CLI_DIAGNOSTIC_CONFIDENCE_LOW;
    suggestion->preferred = machine_applicable;
    suggestion->suggestion_id = (uint64_t)offset + UINT64_C(1);
    suggestion->has_span = true;
    suggestion->span.valid = true;
    suggestion->span.file_id = diagnostic->file_id;
    suggestion->span.source_id = context->source_id;
    suggestion->span.source_name = source_name;
    suggestion->span.start_offset = offset;
    suggestion->span.end_offset = offset;
    suggestion->span.start_line = diagnostic->end_line;
    suggestion->span.start_column = diagnostic->end_column;
    suggestion->span.end_line = diagnostic->end_line;
    suggestion->span.end_column = diagnostic->end_column;
    if (vitte_source_location_from_offset(
            &context->sources,
            context->source_id,
            offset,
            &location) &&
        location.valid) {
        suggestion->span.start_line = location.line - 1u;
        suggestion->span.start_column = location.column - 1u;
        suggestion->span.end_line = suggestion->span.start_line;
        suggestion->span.end_column = suggestion->span.start_column;
    }
    suggestion->has_replacement = true;
    suggestion->replacement = replacement;
    diagnostic->suggestions = suggestion;
    diagnostic->suggestion_count = 1u;
    diagnostic->has_machine_fix = machine_applicable;
}

static vitte_cli_diagnostic_severity_t
vitte_main_parser_diagnostic_severity(
    vitte_parser_diagnostic_kind_t kind)
{
    switch (kind) {
        case VITTE_PARSER_DIAGNOSTIC_NOTE:
            return VITTE_CLI_DIAGNOSTIC_NOTE;

        case VITTE_PARSER_DIAGNOSTIC_HELP:
            return VITTE_CLI_DIAGNOSTIC_HELP;

        case VITTE_PARSER_DIAGNOSTIC_WARNING:
            return VITTE_CLI_DIAGNOSTIC_WARNING;

        case VITTE_PARSER_DIAGNOSTIC_ERROR:
        case VITTE_PARSER_DIAGNOSTIC_INVALID:
        case VITTE_PARSER_DIAGNOSTIC_COUNT:
            return VITTE_CLI_DIAGNOSTIC_ERROR;
    }

    return VITTE_CLI_DIAGNOSTIC_ERROR;
}

static bool
vitte_main_render_one_lexer_diagnostic(
    const vitte_main_driver_context_t *context,
    vitte_lexer_error_t error,
    vitte_lexer_span_t lexer_span)
{
    vitte_parser_span_t source_span;
    vitte_cli_diagnostic_t diagnostic;
    vitte_cli_diagnostic_suggestion_t suggestion;
    const vitte_source_entry_t *source;
    const char *source_name;
    size_t display_end;
    const char *code;
    bool has_details;

    if (context == NULL ||
        !context->lexer_initialized) {
        return false;
    }

    switch (error) {
        case VITTE_LEXER_ERROR_INVALID_UTF8:
            code = "VITTE_LEXER_E_INVALID_UNICODE";
            break;

        case VITTE_LEXER_ERROR_INVALID_IDENTIFIER:
            code = "VITTE_LEXER_E_INVALID_IDENTIFIER";
            break;

        case VITTE_LEXER_ERROR_INVALID_NUMBER:
            code = "VITTE_LEXER_E_INVALID_NUMBER";
            break;

        case VITTE_LEXER_ERROR_INVALID_INTEGER:
            code = "VITTE_LEXER_E_INVALID_INTEGER";
            break;

        case VITTE_LEXER_ERROR_INVALID_FLOAT:
            code = "VITTE_LEXER_E_INVALID_FLOAT";
            break;

        case VITTE_LEXER_ERROR_INVALID_CHARACTER_LITERAL:
            code = "VITTE_LEXER_E_INVALID_CHARACTER_LITERAL";
            break;

        case VITTE_LEXER_ERROR_INVALID_NUMERIC_SEPARATOR:
            code = "VITTE_LEXER_E_INVALID_NUMERIC_SEPARATOR";
            break;

        case VITTE_LEXER_ERROR_INVALID_NUMERIC_BASE:
            code = "VITTE_LEXER_E_INVALID_NUMERIC_BASE";
            break;

        case VITTE_LEXER_ERROR_INVALID_NUMERIC_SUFFIX:
            code = "VITTE_LEXER_E_INVALID_NUMERIC_SUFFIX";
            break;

        case VITTE_LEXER_ERROR_INVALID_EXPONENT:
            code = "VITTE_LEXER_E_INVALID_EXPONENT";
            break;

        case VITTE_LEXER_ERROR_INVALID_OPERATOR:
            code = "VITTE_LEXER_E_INVALID_OPERATOR";
            break;

        case VITTE_LEXER_ERROR_INVALID_ESCAPE:
            code = "VITTE_LEXER_E_INVALID_ESCAPE";
            break;

        case VITTE_LEXER_ERROR_UNTERMINATED_STRING:
            code = "VITTE_LEXER_E_UNTERMINATED_STRING";
            break;

        case VITTE_LEXER_ERROR_UNTERMINATED_COMMENT:
            code = "VITTE_LEXER_E_UNTERMINATED_COMMENT";
            break;

        case VITTE_LEXER_ERROR_INVALID_CHARACTER:
        case VITTE_LEXER_ERROR_INVALID_TOKEN:
            code = "VITTE_LEXER_E_INVALID_CHARACTER";
            break;

        case VITTE_LEXER_ERROR_NUMBER_OVERFLOW:
            code = "VITTE_LEXER_E_NUMBER_OVERFLOW";
            break;

        case VITTE_LEXER_ERROR_OVERFLOW:
            code = "VITTE_INFRA_E_LIMIT";
            break;

        case VITTE_LEXER_ERROR_COMMENT_DEPTH:
        case VITTE_LEXER_ERROR_TOKEN_LIMIT:
        case VITTE_LEXER_ERROR_SOURCE_TOO_LARGE:
            code = "VITTE_INFRA_E_LIMIT";
            break;

        case VITTE_LEXER_ERROR_OUT_OF_MEMORY:
            code = "VITTE_INFRA_E_RESOURCE";
            break;

        case VITTE_LEXER_ERROR_NONE:
        case VITTE_LEXER_ERROR_INVALID_ARGUMENT:
        case VITTE_LEXER_ERROR_INVALID_CONTEXT:
        case VITTE_LEXER_ERROR_INVALID_STATE:
        case VITTE_LEXER_ERROR_VALIDATION:
        case VITTE_LEXER_ERROR_INTERNAL:
        case VITTE_LEXER_ERROR_COUNT:
            code = "VITTE_INFRA_E_PHASE";
            break;

        default:
            code = "VITTE_INFRA_E_PHASE";
            break;
    }

    display_end = vitte_main_lexer_display_end(
        context,
        error,
        &lexer_span);
    memset(&source_span, 0, sizeof(source_span));
    if (lexer_span.valid &&
        lexer_span.begin <= display_end &&
        display_end <= context->file.length) {
        source_span.file_id = lexer_span.file_id;
        source_span.begin = lexer_span.begin;
        source_span.end = display_end;
        if (error == VITTE_LEXER_ERROR_UNTERMINATED_STRING &&
            source_span.begin < context->file.length) {
            source_span.end = source_span.begin + 1u;
        } else if (error == VITTE_LEXER_ERROR_UNTERMINATED_COMMENT &&
                   context->file.length - source_span.begin >= 2u) {
            source_span.end = source_span.begin + 2u;
        }
        source_span.valid = true;
    }

    source = vitte_source_get(
        &context->sources,
        context->source_id);
    source_name = source != NULL
        ? source->name
        : context->options->input_path;

    if (!vitte_main_make_cli_diagnostic(
            context,
            &diagnostic,
            VITTE_CLI_DIAGNOSTIC_ERROR,
            code,
            NULL,
            NULL,
            source_span.valid ? &source_span : NULL,
            NULL,
            "lexing")) {
        return false;
    }

    has_details = false;
    if (lexer_span.valid &&
        (error == VITTE_LEXER_ERROR_INVALID_CHARACTER ||
         error == VITTE_LEXER_ERROR_INVALID_TOKEN ||
         error == VITTE_LEXER_ERROR_INVALID_ESCAPE ||
         error == VITTE_LEXER_ERROR_INVALID_UTF8 ||
         error == VITTE_LEXER_ERROR_INVALID_IDENTIFIER ||
         error == VITTE_LEXER_ERROR_INVALID_NUMBER ||
         error == VITTE_LEXER_ERROR_INVALID_INTEGER ||
         error == VITTE_LEXER_ERROR_INVALID_FLOAT ||
         error == VITTE_LEXER_ERROR_INVALID_CHARACTER_LITERAL ||
         error == VITTE_LEXER_ERROR_INVALID_NUMERIC_SEPARATOR ||
         error == VITTE_LEXER_ERROR_INVALID_NUMERIC_BASE ||
         error == VITTE_LEXER_ERROR_INVALID_NUMERIC_SUFFIX ||
         error == VITTE_LEXER_ERROR_INVALID_EXPONENT ||
         error == VITTE_LEXER_ERROR_NUMBER_OVERFLOW ||
         error == VITTE_LEXER_ERROR_UNTERMINATED_STRING ||
         error == VITTE_LEXER_ERROR_UNTERMINATED_COMMENT)) {
        has_details = vitte_main_lexer_format_details(
            context,
            error,
            &lexer_span,
            display_end,
            diagnostic.details_storage,
            sizeof(diagnostic.details_storage));
    }
    diagnostic.details = has_details
        ? diagnostic.details_storage
        : NULL;

    memset(&suggestion, 0, sizeof(suggestion));
    if (lexer_span.valid) {
        vitte_main_lexer_add_delimiter_suggestion(
            context,
            error,
            &lexer_span,
            display_end,
            source_name,
            &diagnostic,
            &suggestion);
    }
    if (suggestion.has_span) {
        vitte_main_diagnostic_map_span(context, &suggestion.span);
    }

    return vitte_diagnostic_render_cli_batch(
        stderr,
        &context->sources,
        context->source_id,
        source_name,
        &diagnostic,
        1u,
        context->options->color,
        vitte_main_cli_diagnostic_format(
            context->options->diagnostic_format));
}

bool
vitte_main_render_lexer_diagnostic(
    const vitte_main_driver_context_t *context)
{
    size_t diagnostic_count;
    size_t index;

    if (context == NULL || !context->lexer_initialized) {
        return false;
    }

    diagnostic_count = vitte_lexer_diagnostic_count(&context->lexer);
    if (diagnostic_count == 0u) {
        return vitte_main_render_one_lexer_diagnostic(
            context,
            vitte_lexer_last_error(&context->lexer),
            vitte_lexer_error_span(&context->lexer));
    }

    for (index = 0u; index < diagnostic_count; ++index) {
        vitte_lexer_diagnostic_t diagnostic;
        if (!vitte_lexer_diagnostic_at(
                &context->lexer,
                index,
                &diagnostic) ||
            !vitte_main_render_one_lexer_diagnostic(
                context,
                diagnostic.error,
                diagnostic.span)) {
            return false;
        }
    }
    return true;
}

static bool
vitte_main_parser_token_can_start_list_item(
    vitte_token_kind_t kind)
{
    return kind == VITTE_TOKEN_IDENTIFIER ||
           kind == VITTE_TOKEN_INTEGER_LITERAL ||
           kind == VITTE_TOKEN_FLOAT_LITERAL ||
           kind == VITTE_TOKEN_STRING_LITERAL ||
           kind == VITTE_TOKEN_CHARACTER_LITERAL ||
           kind == VITTE_TOKEN_KW_TRUE ||
           kind == VITTE_TOKEN_KW_FALSE ||
           kind == VITTE_TOKEN_KW_NULL ||
           kind == VITTE_TOKEN_KW_SELF ||
           kind == VITTE_TOKEN_KW_IF ||
           kind == VITTE_TOKEN_KW_ELIF ||
           kind == VITTE_TOKEN_KW_PRINT ||
           kind == VITTE_TOKEN_KW_SIZEOF ||
           kind == VITTE_TOKEN_KW_ALIGNOF ||
           kind == VITTE_TOKEN_LEFT_PAREN ||
           kind == VITTE_TOKEN_LEFT_BRACKET ||
           kind == VITTE_TOKEN_LEFT_BRACE ||
           kind == VITTE_TOKEN_PLUS ||
           kind == VITTE_TOKEN_MINUS ||
           kind == VITTE_TOKEN_TILDE ||
           kind == VITTE_TOKEN_STAR ||
           kind == VITTE_TOKEN_AMP ||
           kind == VITTE_TOKEN_KW_NOT ||
           kind == VITTE_TOKEN_KW_MOVE ||
           kind == VITTE_TOKEN_KW_REF ||
           kind == VITTE_TOKEN_KW_AWAIT;
}

static const char *
vitte_main_parser_punctuation_replacement(
    vitte_token_kind_t expected,
    vitte_token_kind_t found)
{
    switch (expected) {
        case VITTE_TOKEN_RIGHT_PAREN:
            if (found == VITTE_TOKEN_EOF ||
                found == VITTE_TOKEN_SEMICOLON ||
                found == VITTE_TOKEN_RIGHT_BRACE ||
                found == VITTE_TOKEN_RIGHT_BRACKET) {
                return ")";
            }
            break;

        case VITTE_TOKEN_RIGHT_BRACKET:
            if (found == VITTE_TOKEN_EOF ||
                found == VITTE_TOKEN_SEMICOLON ||
                found == VITTE_TOKEN_RIGHT_BRACE ||
                found == VITTE_TOKEN_RIGHT_PAREN) {
                return "]";
            }
            break;

        case VITTE_TOKEN_RIGHT_BRACE:
            if (found == VITTE_TOKEN_EOF ||
                found == VITTE_TOKEN_SEMICOLON ||
                found == VITTE_TOKEN_KW_ELSE ||
                found == VITTE_TOKEN_KW_ELIF) {
                return "}";
            }
            break;

        case VITTE_TOKEN_SEMICOLON:
            if (found == VITTE_TOKEN_EOF ||
                found == VITTE_TOKEN_RIGHT_BRACE ||
                found == VITTE_TOKEN_KW_LET ||
                found == VITTE_TOKEN_KW_RETURN ||
                found == VITTE_TOKEN_KW_PRINT ||
                found == VITTE_TOKEN_KW_IF ||
                found == VITTE_TOKEN_KW_WHILE ||
                found == VITTE_TOKEN_KW_FOR ||
                found == VITTE_TOKEN_KW_BREAK ||
                found == VITTE_TOKEN_KW_CONTINUE ||
                found == VITTE_TOKEN_KW_DEFER ||
                found == VITTE_TOKEN_KW_SET ||
                found == VITTE_TOKEN_KW_ASSERT) {
                return ";";
            }
            break;

        case VITTE_TOKEN_COMMA:
            if (vitte_main_parser_token_can_start_list_item(found)) {
                return ",";
            }
            break;

        default:
            break;
    }

    return NULL;
}

static bool
vitte_main_parser_is_import_path_dot(
    const vitte_main_driver_context_t *context,
    const vitte_parser_diagnostic_t *parser_diagnostic)
{
    size_t index;

    if (context == NULL ||
        parser_diagnostic == NULL ||
        !context->lexer_initialized ||
        !parser_diagnostic->span.valid) {
        return false;
    }

    for (index = 0u; index < context->lexer.token_count; ++index) {
        const vitte_token_t *token;
        size_t previous;

        token = &context->lexer.tokens[index];
        if (token->kind != VITTE_TOKEN_DOT ||
            token->span.begin != parser_diagnostic->span.begin ||
            token->span.end != parser_diagnostic->span.end) {
            continue;
        }

        previous = index;
        while (previous != 0u) {
            const vitte_token_t *candidate;

            candidate = &context->lexer.tokens[--previous];
            if (candidate->kind == VITTE_TOKEN_SEMICOLON) {
                return false;
            }
            if (candidate->kind == VITTE_TOKEN_KW_USE) {
                return true;
            }
        }
        return false;
    }

    return false;
}

static void
vitte_main_parser_add_grammar_migration_suggestion(
    const vitte_main_driver_context_t *context,
    const vitte_parser_diagnostic_t *parser_diagnostic,
    const char *source_name,
    vitte_cli_diagnostic_t *diagnostic,
    vitte_cli_diagnostic_suggestion_t *suggestion)
{
    const char *replacement = NULL;
    const char *message = NULL;
    vitte_source_location_t start;
    vitte_source_location_t end;

    if (context == NULL ||
        parser_diagnostic == NULL ||
        source_name == NULL ||
        diagnostic == NULL ||
        suggestion == NULL ||
        parser_diagnostic->kind != VITTE_PARSER_DIAGNOSTIC_ERROR ||
        !parser_diagnostic->span.valid ||
        parser_diagnostic->span.begin >= parser_diagnostic->span.end ||
        parser_diagnostic->span.end > context->file.length) {
        return;
    }

    switch (parser_diagnostic->found) {
        case VITTE_TOKEN_AMP_AMP:
            replacement = "and";
            message = "If logical conjunction was intended, use `and`; review the surrounding expression because this edit changes the operator token.";
            break;
        case VITTE_TOKEN_PIPE_PIPE:
            replacement = "or";
            message = "If logical disjunction was intended, use `or`; review the surrounding expression because this edit changes the operator token.";
            break;
        case VITTE_TOKEN_BANG:
            replacement = "not";
            message = "If logical negation was intended, use `not`; review the operand and grouping because this edit changes the operator token.";
            break;
        case VITTE_TOKEN_DOT:
            if (vitte_main_parser_is_import_path_dot(
                    context,
                    parser_diagnostic)) {
                replacement = "::";
                message = "If this dot separates import path segments, use `::`; review the path before changing its qualification.";
            }
            break;
        default:
            break;
    }

    if (replacement == NULL || message == NULL ||
        !vitte_source_location_from_offset(
            &context->sources,
            context->source_id,
            parser_diagnostic->span.begin,
            &start) ||
        !start.valid ||
        !vitte_source_location_from_offset(
            &context->sources,
            context->source_id,
            parser_diagnostic->span.end,
            &end) ||
        !end.valid) {
        return;
    }

    suggestion->message = message;
    suggestion->applicability = VITTE_CLI_DIAGNOSTIC_APPLICABILITY_MAYBE;
    suggestion->confidence = VITTE_CLI_DIAGNOSTIC_CONFIDENCE_MEDIUM;
    suggestion->preferred = false;
    suggestion->suggestion_id =
        (uint64_t)parser_diagnostic->span.begin + UINT64_C(1);
    suggestion->has_span = true;
    suggestion->span.valid = true;
    suggestion->span.file_id = diagnostic->file_id;
    suggestion->span.source_id = context->source_id;
    suggestion->span.source_name = source_name;
    suggestion->span.start_offset = parser_diagnostic->span.begin;
    suggestion->span.end_offset = parser_diagnostic->span.end;
    suggestion->span.start_line = start.line - 1u;
    suggestion->span.start_column = start.column - 1u;
    suggestion->span.end_line = end.line - 1u;
    suggestion->span.end_column = end.column - 1u;
    suggestion->has_replacement = true;
    suggestion->replacement = replacement;

    diagnostic->suggestions = suggestion;
    diagnostic->suggestion_count = 1u;
}

static void
vitte_main_parser_add_punctuation_suggestion(
    const vitte_main_driver_context_t *context,
    const vitte_parser_diagnostic_t *parser_diagnostic,
    const char *source_name,
    vitte_cli_diagnostic_t *diagnostic,
    vitte_cli_diagnostic_suggestion_t *suggestion)
{
    const char *replacement;
    vitte_source_location_t location;
    size_t offset;
    bool machine_applicable;

    if (context == NULL ||
        parser_diagnostic == NULL ||
        source_name == NULL ||
        diagnostic == NULL ||
        suggestion == NULL ||
        parser_diagnostic->kind != VITTE_PARSER_DIAGNOSTIC_ERROR ||
        (parser_diagnostic->error != VITTE_PARSER_ERROR_EXPECTED_TOKEN &&
         !(parser_diagnostic->error ==
               VITTE_PARSER_ERROR_INVALID_EXPRESSION_LIST &&
           parser_diagnostic->expected == VITTE_TOKEN_COMMA)) ||
        !parser_diagnostic->span.valid) {
        return;
    }

    replacement =
        vitte_main_parser_punctuation_replacement(
            parser_diagnostic->expected,
            parser_diagnostic->found);
    if (replacement == NULL ||
        parser_diagnostic->span.begin > context->file.length) {
        return;
    }

    offset = parser_diagnostic->span.begin;
    machine_applicable = strcmp(replacement, ",") != 0;
    suggestion->message = !machine_applicable
        ? "Insert a comma only if the following token starts another list item; if one expression was intended, add the intended operator or grouping instead."
        : strcmp(replacement, ")") == 0
            ? "Insert the missing `)` here."
            : strcmp(replacement, "]") == 0
                ? "Insert the missing `]` here."
                : strcmp(replacement, "}") == 0
                    ? "Insert the missing `}` here."
                    : strcmp(replacement, ";") == 0
                        ? "Insert the missing `;` here."
                        : "Insert the missing `,` here.";
    suggestion->applicability = machine_applicable
        ? VITTE_CLI_DIAGNOSTIC_APPLICABILITY_MACHINE
        : VITTE_CLI_DIAGNOSTIC_APPLICABILITY_MAYBE;
    suggestion->confidence = machine_applicable
        ? VITTE_CLI_DIAGNOSTIC_CONFIDENCE_HIGH
        : VITTE_CLI_DIAGNOSTIC_CONFIDENCE_MEDIUM;
    suggestion->preferred = machine_applicable;
    suggestion->suggestion_id = (uint64_t)offset + UINT64_C(1);
    suggestion->has_span = true;
    suggestion->span.valid = true;
    suggestion->span.file_id = diagnostic->file_id;
    suggestion->span.source_id = context->source_id;
    suggestion->span.source_name = source_name;
    suggestion->span.start_offset = offset;
    suggestion->span.end_offset = offset;
    suggestion->span.start_line = diagnostic->start_line;
    suggestion->span.start_column = diagnostic->start_column;
    suggestion->span.end_line = diagnostic->start_line;
    suggestion->span.end_column = diagnostic->start_column;
    if (vitte_source_location_from_offset(
            &context->sources,
            context->source_id,
            offset,
            &location) &&
        location.valid) {
        suggestion->span.start_line = location.line - 1u;
        suggestion->span.start_column = location.column - 1u;
        suggestion->span.end_line = suggestion->span.start_line;
        suggestion->span.end_column = suggestion->span.start_column;
    }
    suggestion->has_replacement = true;
    suggestion->replacement = replacement;

    diagnostic->suggestions = suggestion;
    diagnostic->suggestion_count = 1u;
    diagnostic->has_machine_fix = machine_applicable;
}

static bool
vitte_main_parser_token_index_at_diagnostic(
    const vitte_main_driver_context_t *context,
    const vitte_parser_diagnostic_t *parser_diagnostic,
    size_t *token_index)
{
    size_t index;

    if (context == NULL ||
        parser_diagnostic == NULL ||
        token_index == NULL ||
        !context->lexer_initialized ||
        !parser_diagnostic->span.valid) {
        return false;
    }

    for (index = 0u; index < context->lexer.token_count; ++index) {
        const vitte_token_t *token;

        token = &context->lexer.tokens[index];
        if (token->span.begin == parser_diagnostic->span.begin &&
            token->span.end == parser_diagnostic->span.end &&
            token->kind == parser_diagnostic->found) {
            *token_index = index;
            return true;
        }
    }

    return false;
}

static bool
vitte_main_parser_declaration_expects_block(
    const vitte_main_driver_context_t *context,
    size_t token_index)
{
    size_t index;

    if (context == NULL || token_index > context->lexer.token_count) {
        return false;
    }

    index = token_index;
    while (index != 0u) {
        const vitte_token_t *token;

        token = &context->lexer.tokens[--index];
        switch (token->kind) {
            case VITTE_TOKEN_KW_SPACE:
            case VITTE_TOKEN_KW_FORM:
            case VITTE_TOKEN_KW_PICK:
            case VITTE_TOKEN_KW_TRAIT:
            case VITTE_TOKEN_KW_IMPL:
            case VITTE_TOKEN_KW_PROC:
            case VITTE_TOKEN_KW_MACRO:
            case VITTE_TOKEN_KW_TEST:
                return true;

            case VITTE_TOKEN_KW_IF:
            case VITTE_TOKEN_KW_ELSE:
            case VITTE_TOKEN_KW_ELIF:
            case VITTE_TOKEN_KW_WHILE:
            case VITTE_TOKEN_KW_LOOP:
            case VITTE_TOKEN_KW_MATCH:
            case VITTE_TOKEN_KW_UNSAFE:
            case VITTE_TOKEN_KW_ASM:
            case VITTE_TOKEN_KW_DEFER:
            case VITTE_TOKEN_KW_USE:
            case VITTE_TOKEN_SEMICOLON:
            case VITTE_TOKEN_LEFT_BRACE:
            case VITTE_TOKEN_RIGHT_BRACE:
                return false;

            case VITTE_TOKEN_KW_FOR:
            {
                size_t search;

                search = index;
                while (search != 0u) {
                    const vitte_token_t *preceding;

                    preceding = &context->lexer.tokens[--search];
                    if (preceding->kind == VITTE_TOKEN_KW_IMPL) {
                        return true;
                    }
                    if (preceding->kind == VITTE_TOKEN_SEMICOLON ||
                        preceding->kind == VITTE_TOKEN_LEFT_BRACE ||
                        preceding->kind == VITTE_TOKEN_RIGHT_BRACE) {
                        return false;
                    }
                }
                return false;
            }

            default:
                break;
        }
    }

    return false;
}

static bool
vitte_main_parser_is_import_alias_as(
    const vitte_main_driver_context_t *context,
    size_t token_index)
{
    size_t index;

    if (context == NULL || token_index > context->lexer.token_count) {
        return false;
    }

    index = token_index;
    while (index != 0u) {
        const vitte_token_t *token;

        token = &context->lexer.tokens[--index];
        if (token->kind == VITTE_TOKEN_SEMICOLON) {
            return false;
        }
        if (token->kind == VITTE_TOKEN_KW_USE ||
            token->kind == VITTE_TOKEN_KW_EXPORT) {
            return true;
        }
    }

    return false;
}

static bool
vitte_main_parser_contextual_expectation(
    const vitte_main_driver_context_t *context,
    const vitte_parser_diagnostic_t *parser_diagnostic,
    const char **message,
    const char **help)
{
    size_t token_index;
    vitte_token_kind_t previous;

    if (context == NULL ||
        parser_diagnostic == NULL ||
        message == NULL ||
        help == NULL ||
        parser_diagnostic->kind != VITTE_PARSER_DIAGNOSTIC_ERROR ||
        !vitte_main_parser_token_index_at_diagnostic(
            context,
            parser_diagnostic,
            &token_index)) {
        return false;
    }

    if (parser_diagnostic->expected == VITTE_TOKEN_IDENTIFIER &&
        vitte_token_kind_is_keyword(parser_diagnostic->found)) {
        *message = "reserved word cannot be used as a name";
        *help = "Rename this item; Vitte reserved words cannot be used as identifiers.";
        return true;
    }

    if (parser_diagnostic->error == VITTE_PARSER_ERROR_UNSUPPORTED) {
        *message = "unsupported syntax in this position";
        *help = "This token is recognized, but it has no supported grammar form here. Replace it with a construct valid in this position, or use a supported declaration such as `proc`, `form`, `pick`, `const`, or `type`.";
        return true;
    }

    switch (parser_diagnostic->error) {
        case VITTE_PARSER_ERROR_INVALID_MODULE_PATH:
            *message = "malformed module path";
            if (token_index + 1u < context->lexer.token_count &&
                context->lexer.tokens[token_index].kind ==
                    VITTE_TOKEN_COLON_COLON &&
                context->lexer.tokens[token_index + 1u].kind ==
                    VITTE_TOKEN_IDENTIFIER) {
                size_t scan = token_index;
                while (scan != 0u) {
                    const vitte_token_t *previous_token =
                        &context->lexer.tokens[--scan];
                    if (previous_token->kind == VITTE_TOKEN_SEMICOLON ||
                        previous_token->kind == VITTE_TOKEN_LEFT_BRACE ||
                        previous_token->kind == VITTE_TOKEN_RIGHT_BRACE) {
                        break;
                    }
                    if (previous_token->kind == VITTE_TOKEN_KW_USE) {
                        *help = "A `use` path names modules. If the final segment is an exported symbol, import it through a group, for example `use sample::math::{add};`; use `as local_name` inside the group to avoid a name conflict.";
                        return true;
                    }
                }
            }
            *help = "Write each path segment as an identifier separated by `::` or `/`; remove leading, repeated or trailing separators.";
            return true;
        case VITTE_PARSER_ERROR_MISSING_MODULE_HEADER:
            *message = "missing module header";
            *help = "Add one top-level `space path;` declaration before the first module import or export clause.";
            return true;
        case VITTE_PARSER_ERROR_DUPLICATE_MODULE_HEADER:
            *message = "duplicate module header";
            *help = "Keep one top-level `space path` header in this file and remove the duplicate declaration.";
            return true;
        case VITTE_PARSER_ERROR_MISPLACED_MODULE_HEADER:
            *message = "misplaced module header";
            *help = "Move `space path;` before the first import or other top-level declaration.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_USE_DECLARATION:
            *message = "malformed use declaration";
            *help = "Use `use path;`, `use path as alias;`, `use path::*;` or `use path::{item, nested::{item}};`, then end it with `;`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_USE_ALIAS:
            *message = "invalid import alias";
            *help = "Add an identifier after `as`, for example `use math::vector as vectors;`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_IMPORT_GROUP:
            *message = "malformed import group";
            *help = "Add comma-separated import paths, close every brace, and place nested groups after a path.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_GLOB_IMPORT:
            *message = "invalid glob import";
            *help = "Place `::*` at the end of the import, for example `use math::*;`; remove aliases or following path segments.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_EXPORT_CLAUSE:
            *message = "malformed export clause";
            *help = "Use `export path [as alias];`, `export {path, other};` or `export *;`.";
            return true;
        case VITTE_PARSER_ERROR_EMPTY_EXPORT_GROUP:
            *message = "empty export group";
            *help = "Add at least one path inside the braces, for example `export {math::add};`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_EXPORT_ALIAS:
            *message = "invalid export alias";
            *help = "Add an identifier after `as`, for example `export math::add as add;`.";
            return true;
        case VITTE_PARSER_ERROR_TRAILING_TOKENS:
            *message = "unexpected trailing tokens";
            *help = "Remove the extra token before the required semicolon or finish the clause in the form shown by the diagnostic.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_VALUE_DECLARATION:
            *message = "malformed const or static declaration";
            *help = "Give the declaration a name, put an optional type after `:`, add `= expression`, and finish with `;`.";
            return true;
        case VITTE_PARSER_ERROR_MISSING_VALUE_INITIALIZER:
            *message = "missing const or static initializer";
            *help = "Add `= expression` after the name and optional type, for example `const LIMIT: int = 10;`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_TYPE_DECLARATION:
            *message = "malformed type declaration";
            *help = "Complete the type name, each bound or aliased type, and the final `;`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_OPAQUE_DECLARATION:
            *message = "malformed opaque declaration";
            *help = "Give the opaque type a name, use `= Type` for an optional alias, and omit bounds.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_FORM_DECLARATION:
            *message = "malformed form declaration";
            *help = "Write a named form with `{}` and fields in the form `name: Type`.";
            return true;
        case VITTE_PARSER_ERROR_DUPLICATE_FORM_FIELD:
            *message = "duplicate form field";
            *help = "Rename this field; every field in one form must have a unique name.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_FORM_FIELD_SEPARATOR:
            *message = "missing form field separator";
            *help = "Separate adjacent fields with `,` or `;`, for example `form Point { x: int, y: int }`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_PICK_DECLARATION:
            *message = "malformed pick declaration";
            *help = "Write a named pick with braces and comma-separated variants, for example `pick Option { None, Some(int) }`.";
            return true;
        case VITTE_PARSER_ERROR_DUPLICATE_PICK_VARIANT:
            *message = "duplicate pick variant";
            *help = "Rename this variant; each variant name must be unique within its pick.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_PICK_VARIANT_PAYLOAD:
            *message = "invalid pick variant payload";
            *help = "Put valid comma-separated types inside the variant parentheses, for example `Some(int, string)`; `()` is also valid.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_TRAIT_DECLARATION:
            *message = "malformed trait declaration";
            *help = "Write `trait Name { ... }`, with optional generic parameters and a `where` clause before the block.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_IMPL_DECLARATION:
            *message = "malformed implementation declaration";
            *help = "Write an inherent implementation as `impl Type { ... }` or a trait implementation as `impl Trait for Type { ... }`.";
            return true;
        case VITTE_PARSER_ERROR_MISSING_IMPL_TARGET_TYPE:
            *message = "missing implementation target type";
            *help = "Add the type after `for`, for example `impl Display for Point { ... }`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_IMPL_FOR_CLAUSE:
            *message = "invalid implementation `for` clause";
            *help = "Use one trait type, `for`, one target type, then an optional `where` clause and the implementation block.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_PROCEDURE_DECLARATION:
            *message = "malformed procedure declaration";
            *help = "Write `proc name(parameters)`, then an optional `-> Type`, optional constraints, and a body or `;`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_PROCEDURE_PARAMETERS:
            *message = "malformed procedure parameter list";
            *help = "Enclose parameters in `(...)`; each parameter needs a pattern, `:`, a type, commas between entries, and a closing `)`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_PROCEDURE_RETURN_TYPE:
            *message = "invalid procedure return type";
            *help = "Put a valid type after `->`, for example `proc size() -> int;`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_PROCEDURE_MODIFIER:
            *message = "invalid procedure modifier";
            *help = "Use `async`, `unsafe` or `comptime` at most once and only before a procedure declaration.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_PROCEDURE_BODY_FORM:
            *message = "invalid procedure body form";
            *help = "A normal procedure needs a block or `;`; an `extern` or `intrinsic` procedure must end with `;` and has no body.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_EXTERNAL_PROCEDURE:
            *message = "malformed external procedure declaration";
            *help = "Write `extern [\"ABI\"] proc name(parameters) [-> Type] [where ...] [requires ...;] [ensures ...;];`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_INTRINSIC_DECLARATION:
            *message = "malformed intrinsic declaration";
            *help = "Write `intrinsic name(parameters) [-> Type] [where ...] [requires ...;] [ensures ...;];` without an ABI literal or body.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_ABI_STRING_LITERAL:
            *message = "invalid ABI string literal";
            *help = "Use a nonempty quoted ABI name after `extern`, for example `extern \"C\" proc write();`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_GENERIC_PARAMETER_LIST:
            *message = "malformed generic parameter list";
            *help = "Write `<T, U>` with comma-separated parameters and a closing `>`; a trailing comma is allowed.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_CONST_GENERIC_PARAMETER:
            *message = "malformed const generic parameter";
            *help = "Write a const parameter as `const NAME`, optionally followed by a type bound, for example `const N: usize`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_GENERIC_BOUND:
            *message = "malformed generic bound";
            *help = "Add a valid type after `:` or `+`, for example `T: Display + Debug`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_PROCEDURE_TYPE:
            *message = "malformed procedure type";
            *help = "Write `proc(Type, Type)`, optionally followed by `-> ReturnType`; separate parameters with commas and close the list with `)`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_REFERENCE_TYPE:
            *message = "malformed reference type";
            *help = "Write `&Type`, `&mut Type`, `ref Type` or `ref mut Type`, with a complete type after the reference marker.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_POINTER_TYPE:
            *message = "malformed pointer type";
            *help = "Write `*Type`, `*mut Type`, `*const Type`, `*volatile Type` or `*const volatile Type`; provide one complete pointee type.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_ARRAY_TYPE:
            *message = "malformed array type";
            *help = "Write `[Type; length]` or `[length] Type`, with a valid length expression, closing `]` and element type.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_SLICE_TYPE:
            *message = "malformed slice type";
            *help = "Write a slice as `[ElementType]` and include a complete element type before the closing `]`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_OPTIONAL_TYPE:
            *message = "malformed optional type";
            *help = "Write `?Type` and put a complete type after `?`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_TYPE_PATH:
            *message = "malformed type path";
            *help = "Use nonempty path segments separated by `::` or `/`; use `::` instead of `:`, add a segment after every separator and remove repeated separators.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_GENERIC_ARGUMENTS:
            *message = "malformed generic arguments";
            *help = "Use comma-separated types in `<Type, Type>` or `[Type, Type]`; close the list with the matching `>` or `]`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_LET_STATEMENT:
            *message = "malformed let binding statement";
            *help = "Write `let [mut] pattern [: Type] [= expression];`. Add a type annotation when there is no initializer and the binding type cannot be inferred.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_SET_STATEMENT:
            *message = "malformed set statement";
            *help = "Write `set target assignment_operator expression;` and provide a complete target, value and terminating semicolon.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_ASSIGNMENT_OPERATOR:
            *message = "invalid assignment operator";
            *help = "Use one of `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=` or `>>=` after the assignment target.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_RETURN_STATEMENT:
            *message = "malformed return statement";
            *help = "Write `return;` or `return expression;`. The final return in a block may omit `;` before `}` or end of file; procedure placement and return type are checked semantically.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_JUMP_STATEMENT:
            *message = "malformed jump statement";
            *help = "Write `break;` or `continue;` with a semicolon. A jump must appear inside a loop within a procedure.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_DEFER_STATEMENT:
            *message = "malformed defer statement";
            *help = "Write `defer { ... }` or `defer expression;`; the expression form requires its terminating semicolon.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_ASSERT_STATEMENT:
            *message = "malformed assert statement";
            *help = "Write `assert(condition);` or `assert condition;` with a complete expression and a final semicolon.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_UNSAFE_STATEMENT:
            *message = "malformed unsafe statement";
            *help = "Write `unsafe { ... }` and close the block with `}`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_ASM_STATEMENT:
            *message = "malformed assembly statement";
            *help = "Write `asm(operand, ...);` or `asm(\"template\");`; close the operand list and terminate the statement with `;`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_CONTROL_FLOW_STATEMENT:
            *message = "malformed control-flow statement";
            *help = "Use `if condition { ... } [elif condition { ... }]* [else { ... }]`, `while condition { ... }`, `loop { ... }`, or `for pattern in values { ... }`; put `else` after an if/elif chain and use it only before a block or chained conditional.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_MATCH_STATEMENT:
            *message = "malformed match statement";
            *help = "Write `match value { pattern => body, ... }`; provide a subject and braces, and use complete comma-separated arms when the arm list is nonempty.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_MATCH_ARM:
            *message = "malformed match arm";
            *help = "Write `[case] pattern [if guard] => body`, separate adjacent arms with commas, and use a complete expression, statement or block as the body.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_PATTERN:
            *message = "malformed pattern";
            *help = "Use a path or binding, literal, `Path::Variant`, `Variant(pattern, ...)`, `Path { field: pattern, .. }`, `(pattern, ...)`, or `[pattern, ...]`; separate pattern items with commas.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_RANGE_PATTERN:
            *message = "malformed range pattern";
            *help = "Write `lower..upper` or `lower..=upper`; both endpoints must be integer, float or character literals of the same kind.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_ASSIGNMENT_EXPRESSION:
            *message = "malformed assignment expression";
            *help = "Write `target = value` or use a supported compound assignment operator; provide both a complete target and right-hand expression.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_COALESCING_EXPRESSION:
            *message = "malformed coalescing expression";
            *help = "Write `value ?? fallback` and provide a complete expression on both sides of `??`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_LOGICAL_EXPRESSION:
            *message = "malformed logical expression";
            *help = "Write both operands around `and`, `or`, `&&` or `||`; parenthesize nested conditions when needed.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_BITWISE_EXPRESSION:
            *message = "malformed bitwise expression";
            *help = "Write both operands around `&`, `|` or `^`; use a complete expression on each side.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_COMPARISON_EXPRESSION:
            *message = "malformed comparison expression";
            *help = "Write a complete left and right expression around `==`, `!=`, `<`, `<=`, `>` or `>=`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_SHIFT_EXPRESSION:
            *message = "malformed shift expression";
            *help = "Write a value and shift count around `<<` or `>>`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_ARITHMETIC_EXPRESSION:
            *message = "malformed arithmetic expression";
            *help = "Write complete operands around `+`, `-`, `*`, `/` or `%`; add parentheses to make grouping clear.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_RANGE_EXPRESSION:
            *message = "malformed range expression";
            *help = "Write `start..end` for an exclusive range or `start..=end` for an inclusive range, with a complete expression at each endpoint.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_UNARY_EXPRESSION:
            *message = "malformed unary expression";
            *help = "Put one complete operand after the unary operator, for example `-value`, `not ready`, `&value` or `*pointer`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_CAST_EXPRESSION:
            *message = "malformed cast expression";
            *help = "Write `value as Type` and provide a complete type after `as`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_POSTFIX_EXPRESSION:
            *message = "malformed postfix expression";
            *help = "Complete the suffix: use `callee(arguments)`, `value[index]`, `value.field`, `value?`, or `Type { field: value }`; generic form and variant suffixes need nonempty, well-formed type arguments.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_IF_EXPRESSION:
            *message = "malformed if expression";
            *help = "Write `if condition { value } else { fallback }`; an if expression needs an else branch, and `elif` may continue the chain.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_BLOCK_EXPRESSION:
            *message = "malformed block expression";
            *help = "Close the block with `}`; place complete statements before the optional final expression.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_ARRAY_EXPRESSION:
            *message = "malformed array expression";
            *help = "Write `[]` or `[expression, ...]`; separate elements with commas and close the array with `]`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_SIZEOF_EXPRESSION:
            *message = "malformed sizeof expression";
            *help = "Write `sizeof(Type)` or `sizeof(expression)` with a complete operand and matching parentheses.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_ALIGNOF_EXPRESSION:
            *message = "malformed alignof expression";
            *help = "Write `alignof(Type)` with a complete type and matching parentheses.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_NULL_CONSTRUCTION:
            *message = "malformed null construction";
            *help = "Write `null()` or `null(expression, ...)` and close the argument list with `)`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_EXPRESSION_LIST:
            *message = "malformed expression list";
            *help = "Provide complete expressions separated by commas. A trailing comma is allowed, but empty items are invalid; a missing-comma insertion is suggested only when the next token clearly starts another item.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_WHERE_CLAUSE:
            *message = "malformed where clause";
            *help = "Write one or more comma-separated predicates after `where`, such as `where T: Display, N > 0`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_REQUIRES_CLAUSE:
            *message = "malformed requires clause";
            *help = "Add a boolean expression and its semicolon, for example `requires count > 0;`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_ENSURES_CLAUSE:
            *message = "malformed ensures clause";
            *help = "Add a boolean expression and its semicolon, for example `ensures result >= 0;`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_MACRO_DECLARATION:
            *message = "malformed macro declaration";
            *help = "Write `macro name` with an optional parameter list and a body, for example `macro log(value: string) { ... }`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_MACRO_PARAMETERS:
            *message = "malformed macro parameter list";
            *help = "Use `(...)` with comma-separated `pattern: Type` parameters and a closing `)`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_TEST_DECLARATION:
            *message = "malformed test declaration";
            *help = "Write `test { ... }`, `test test_name { ... }`, or `test \"description\" { ... }`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_DECLARATION_MODIFIER_ORDER:
            *message = "invalid declaration modifier order or combination";
            *help = "Use at most one visibility modifier (`pub` or prefix `export`) before procedure modifiers; write export clauses without declaration modifiers.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_ATTRIBUTE:
            *message = "malformed attribute";
            *help = "Write an attribute as `#[name]` or `#![name]`; include the opening `[` and closing `]` and close any argument parentheses.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_ATTRIBUTE_NAME:
            *message = "missing or invalid attribute name";
            *help = "Provide an identifier or keyword after `#[` or `#![` and after each `::` in a qualified attribute name.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_ATTRIBUTE_ARGUMENTS:
            *message = "malformed attribute arguments";
            *help = "Use comma-separated names or literals in parentheses; write named arguments as `name = literal`.";
            return true;
        case VITTE_PARSER_ERROR_MISSING_ATTRIBUTE_VALUE:
            *message = "missing attribute value";
            *help = "Add a literal after `=`, for example `#[cfg(enabled = true)]`.";
            return true;
        case VITTE_PARSER_ERROR_INVALID_ATTRIBUTE_LITERAL:
            *message = "invalid attribute literal";
            *help = "Use an integer, float, string, character, boolean, or `null` literal as the attribute value.";
            return true;
        case VITTE_PARSER_ERROR_UNEXPECTED_DECLARATION:
            *message = "unexpected declaration";
            *help = "Move this declaration to a declaration position, such as the top level or a trait/implementation declaration block.";
            return true;
        case VITTE_PARSER_ERROR_UNEXPECTED_STATEMENT:
            *message = "unexpected statement";
            *help = "Move this statement into a block that accepts statements, such as a procedure body.";
            return true;
        case VITTE_PARSER_ERROR_UNEXPECTED_EXPRESSION:
            *message = "unexpected expression";
            *help = "Move this expression into a statement or expression context; declarations require a declaration position.";
            return true;
        default:
            break;
    }

    previous = token_index != 0u
        ? context->lexer.tokens[token_index - 1u].kind
        : VITTE_TOKEN_INVALID;

    if (previous == VITTE_TOKEN_KW_AS &&
        vitte_main_parser_is_import_alias_as(context, token_index) &&
        (parser_diagnostic->expected == VITTE_TOKEN_IDENTIFIER ||
         parser_diagnostic->error == VITTE_PARSER_ERROR_EXPECTED_IDENTIFIER)) {
        *message = "expected a name after `as`";
        *help = "Provide the local name that should refer to the imported item.";
        return true;
    }

    if (previous == VITTE_TOKEN_KW_RETURN &&
        parser_diagnostic->error == VITTE_PARSER_ERROR_EXPECTED_EXPRESSION) {
        *message = "expected an expression after `return`";
        *help = "Write the return value after `return`, before the semicolon.";
        return true;
    }

    if (previous == VITTE_TOKEN_COLON &&
        (parser_diagnostic->error == VITTE_PARSER_ERROR_EXPECTED_TYPE ||
         parser_diagnostic->error == VITTE_PARSER_ERROR_EXPECTED_TOKEN ||
         parser_diagnostic->error == VITTE_PARSER_ERROR_EXPECTED_IDENTIFIER)) {
        *message = "expected a type after `:`";
        *help = "Write a type expression after the colon.";
        return true;
    }

    if (parser_diagnostic->expected == VITTE_TOKEN_LEFT_BRACE &&
        vitte_main_parser_declaration_expects_block(context, token_index)) {
        *message = "expected a block after this declaration";
        *help = "Start the declaration body with `{` and close it with `}`.";
        return true;
    }

    return false;
}

static bool
vitte_main_parser_matching_opener_location(
    const vitte_main_driver_context_t *context,
    const vitte_parser_diagnostic_t *parser_diagnostic,
    const char *source_name,
    vitte_cli_diagnostic_location_t *location)
{
    const vitte_token_t *opener;
    const char *label;
    size_t token_index;

    if (context == NULL ||
        parser_diagnostic == NULL ||
        source_name == NULL ||
        location == NULL ||
        parser_diagnostic->kind != VITTE_PARSER_DIAGNOSTIC_ERROR ||
        !vitte_main_parser_token_index_at_diagnostic(
            context,
            parser_diagnostic,
            &token_index)) {
        return false;
    }

    switch (parser_diagnostic->expected) {
        case VITTE_TOKEN_RIGHT_PAREN:
            opener = NULL;
            label = "opening `(` here; closing `)` expected";
            break;
        case VITTE_TOKEN_RIGHT_BRACKET:
            opener = NULL;
            label = "opening `[` here; closing `]` expected";
            break;
        case VITTE_TOKEN_RIGHT_BRACE:
            opener = NULL;
            label = "opening `{` here; closing `}` expected";
            break;
        case VITTE_TOKEN_GREATER:
            opener = NULL;
            label = "opening `<` here; closing `>` expected";
            break;
        default:
            opener = NULL;
            label = NULL;
            break;
    }

    if (parser_diagnostic->expected == VITTE_TOKEN_RIGHT_PAREN ||
        parser_diagnostic->expected == VITTE_TOKEN_RIGHT_BRACKET ||
        parser_diagnostic->expected == VITTE_TOKEN_RIGHT_BRACE ||
        parser_diagnostic->expected == VITTE_TOKEN_GREATER) {
        vitte_token_kind_t opener_kind;
        size_t nesting;

        opener_kind = parser_diagnostic->expected == VITTE_TOKEN_RIGHT_PAREN
            ? VITTE_TOKEN_LEFT_PAREN
            : parser_diagnostic->expected == VITTE_TOKEN_RIGHT_BRACKET
                ? VITTE_TOKEN_LEFT_BRACKET
                : parser_diagnostic->expected == VITTE_TOKEN_RIGHT_BRACE
                    ? VITTE_TOKEN_LEFT_BRACE
                    : VITTE_TOKEN_LESS;
        nesting = 0u;
        while (token_index != 0u) {
            const vitte_token_t *token;

            token = &context->lexer.tokens[--token_index];
            if (parser_diagnostic->expected == VITTE_TOKEN_GREATER &&
                token->kind == VITTE_TOKEN_SHIFT_RIGHT) {
                nesting += 2u;
            } else if (token->kind == parser_diagnostic->expected) {
                ++nesting;
            } else if (token->kind == opener_kind) {
                if (nesting == 0u) {
                    opener = token;
                    break;
                }
                --nesting;
            }
        }

        if (opener != NULL &&
            (parser_diagnostic->found == VITTE_TOKEN_RIGHT_PAREN ||
             parser_diagnostic->found == VITTE_TOKEN_RIGHT_BRACKET ||
             parser_diagnostic->found == VITTE_TOKEN_RIGHT_BRACE ||
             parser_diagnostic->found == VITTE_TOKEN_GREATER ||
             parser_diagnostic->found == VITTE_TOKEN_SHIFT_RIGHT) &&
            parser_diagnostic->found != parser_diagnostic->expected) {
            if (parser_diagnostic->expected == VITTE_TOKEN_GREATER) {
                label = parser_diagnostic->found == VITTE_TOKEN_RIGHT_PAREN
                    ? "opening `<` does not match closing `)`; expected `>`"
                    : parser_diagnostic->found == VITTE_TOKEN_RIGHT_BRACKET
                        ? "opening `<` does not match closing `]`; expected `>`"
                        : parser_diagnostic->found == VITTE_TOKEN_RIGHT_BRACE
                            ? "opening `<` does not match closing `}`; expected `>`"
                            : "opening `<` does not match closing `>>`; expected `>`";
            } else if (parser_diagnostic->expected == VITTE_TOKEN_RIGHT_PAREN) {
                label = parser_diagnostic->found == VITTE_TOKEN_RIGHT_BRACKET
                    ? "opening `(` does not match closing `]`; expected `)`"
                    : parser_diagnostic->found == VITTE_TOKEN_RIGHT_BRACE
                        ? "opening `(` does not match closing `}`; expected `)`"
                        : parser_diagnostic->found == VITTE_TOKEN_SHIFT_RIGHT
                            ? "opening `(` does not match closing `>>`; expected `)`"
                            : "opening `(` does not match closing `>`; expected `)`";
            } else if (parser_diagnostic->expected ==
                       VITTE_TOKEN_RIGHT_BRACKET) {
                label = parser_diagnostic->found == VITTE_TOKEN_RIGHT_PAREN
                    ? "opening `[` does not match closing `)`; expected `]`"
                    : parser_diagnostic->found == VITTE_TOKEN_RIGHT_BRACE
                        ? "opening `[` does not match closing `}`; expected `]`"
                        : parser_diagnostic->found == VITTE_TOKEN_SHIFT_RIGHT
                            ? "opening `[` does not match closing `>>`; expected `]`"
                            : "opening `[` does not match closing `>`; expected `]`";
            } else if (parser_diagnostic->expected == VITTE_TOKEN_RIGHT_BRACE) {
                label = parser_diagnostic->found == VITTE_TOKEN_RIGHT_PAREN
                    ? "opening `{` does not match closing `)`; expected `}`"
                    : parser_diagnostic->found == VITTE_TOKEN_RIGHT_BRACKET
                        ? "opening `{` does not match closing `]`; expected `}`"
                        : parser_diagnostic->found == VITTE_TOKEN_SHIFT_RIGHT
                            ? "opening `{` does not match closing `>>`; expected `}`"
                            : "opening `{` does not match closing `>`; expected `}`";
            }
        }
    } else if (parser_diagnostic->found == VITTE_TOKEN_RIGHT_PAREN ||
               parser_diagnostic->found == VITTE_TOKEN_RIGHT_BRACKET ||
               parser_diagnostic->found == VITTE_TOKEN_RIGHT_BRACE) {
        const vitte_token_t *candidates[3] = { NULL, NULL, NULL };
        size_t nesting[3] = { 0u, 0u, 0u };
        size_t candidate_index;
        size_t found_delimiter_index;
        size_t selected_index;

        found_delimiter_index =
            parser_diagnostic->found == VITTE_TOKEN_RIGHT_PAREN
                ? 0u
                : parser_diagnostic->found == VITTE_TOKEN_RIGHT_BRACKET
                    ? 1u
                    : 2u;

        while (token_index != 0u) {
            const vitte_token_t *token;
            size_t delimiter_index;

            token = &context->lexer.tokens[--token_index];
            switch (token->kind) {
                case VITTE_TOKEN_LEFT_PAREN:
                    delimiter_index = 0u;
                    if (nesting[delimiter_index] != 0u) {
                        --nesting[delimiter_index];
                    } else if (candidates[delimiter_index] == NULL) {
                        candidates[delimiter_index] = token;
                    }
                    break;
                case VITTE_TOKEN_RIGHT_PAREN:
                    ++nesting[0];
                    break;
                case VITTE_TOKEN_LEFT_BRACKET:
                    delimiter_index = 1u;
                    if (nesting[delimiter_index] != 0u) {
                        --nesting[delimiter_index];
                    } else if (candidates[delimiter_index] == NULL) {
                        candidates[delimiter_index] = token;
                    }
                    break;
                case VITTE_TOKEN_RIGHT_BRACKET:
                    ++nesting[1];
                    break;
                case VITTE_TOKEN_LEFT_BRACE:
                    delimiter_index = 2u;
                    if (nesting[delimiter_index] != 0u) {
                        --nesting[delimiter_index];
                    } else if (candidates[delimiter_index] == NULL) {
                        candidates[delimiter_index] = token;
                    }
                    break;
                case VITTE_TOKEN_RIGHT_BRACE:
                    ++nesting[2];
                    break;
                default:
                    break;
            }
        }

        if (candidates[found_delimiter_index] != NULL) {
            return false;
        }

        selected_index = 3u;
        for (candidate_index = 0u; candidate_index < 3u; ++candidate_index) {
            if (candidates[candidate_index] != NULL &&
                (selected_index == 3u ||
                 candidates[candidate_index]->span.begin >
                    candidates[selected_index]->span.begin)) {
                selected_index = candidate_index;
            }
        }
        if (selected_index != 3u) {
            opener = candidates[selected_index];
            if (selected_index == 0u) {
                label = parser_diagnostic->found == VITTE_TOKEN_RIGHT_BRACKET
                    ? "opening `(` does not match closing `]`"
                    : "opening `(` does not match closing `}`";
            } else if (selected_index == 1u) {
                label = parser_diagnostic->found == VITTE_TOKEN_RIGHT_PAREN
                    ? "opening `[` does not match closing `)`"
                    : "opening `[` does not match closing `}`";
            } else {
                label = parser_diagnostic->found == VITTE_TOKEN_RIGHT_PAREN
                    ? "opening `{` does not match closing `)`"
                    : "opening `{` does not match closing `]`";
            }
        }
    }

    if (opener == NULL || label == NULL || !opener->span.valid) {
        return false;
    }

    {
        vitte_source_location_t start;
        vitte_source_location_t end;

        if (!vitte_source_location_from_offset(
                &context->sources,
                context->source_id,
                opener->span.begin,
                &start) ||
            !start.valid ||
            !vitte_source_location_from_offset(
                &context->sources,
                context->source_id,
                opener->span.end,
                &end) ||
            !end.valid) {
            return false;
        }

        memset(location, 0, sizeof(*location));
        location->kind = VITTE_CLI_DIAGNOSTIC_LOCATION_RELATED;
        location->label = label;
        location->span.valid = true;
        location->span.file_id = opener->span.file_id;
        location->span.source_id = context->source_id;
        location->span.source_name = source_name;
        location->span.start_offset = opener->span.begin;
        location->span.end_offset = opener->span.end;
        location->span.start_line = start.line - 1u;
        location->span.start_column = start.column - 1u;
        location->span.end_line = end.line - 1u;
        location->span.end_column = end.column - 1u;
    }
    return true;
}

static bool
vitte_main_parser_module_header_related_location(
    const vitte_main_driver_context_t *context,
    const vitte_parser_diagnostic_t *parser_diagnostic,
    const char *source_name,
    vitte_cli_diagnostic_location_t *location)
{
    size_t current_child_index;
    size_t index;
    const vitte_ast_node_t *root;
    const vitte_ast_node_t *candidate = NULL;
    const vitte_source_entry_t *source;
    vitte_source_location_t start;
    vitte_source_location_t end;
    const char *label = NULL;

    if (context == NULL || parser_diagnostic == NULL ||
        source_name == NULL || location == NULL ||
        (parser_diagnostic->error !=
             VITTE_PARSER_ERROR_DUPLICATE_MODULE_HEADER &&
         parser_diagnostic->error !=
             VITTE_PARSER_ERROR_MISPLACED_MODULE_HEADER) ||
        !parser_diagnostic->span.valid) {
        return false;
    }

    root = vitte_parser_get_node(
        &context->parser,
        vitte_parser_root(&context->parser));
    source = vitte_source_get(&context->sources, context->source_id);
    if (root == NULL || root->kind != VITTE_AST_NODE_TRANSLATION_UNIT) {
        return false;
    }

    current_child_index = root->child_count;
    for (index = 0u; index < root->child_count; ++index) {
        const vitte_ast_node_t *declaration =
            vitte_parser_get_node(&context->parser, root->children[index]);
        if (declaration != NULL &&
            declaration->kind == VITTE_AST_NODE_SPACE_DECL &&
            declaration->child_count == 1u &&
            declaration->span.valid &&
            declaration->span.begin == parser_diagnostic->span.begin) {
            current_child_index = index;
            break;
        }
    }
    if (current_child_index == root->child_count) {
        return false;
    }

    for (index = 0u; index < current_child_index; ++index) {
        const vitte_ast_node_t *declaration =
            vitte_parser_get_node(&context->parser, root->children[index]);
        if (declaration == NULL) {
            continue;
        }
        if (parser_diagnostic->error ==
                VITTE_PARSER_ERROR_DUPLICATE_MODULE_HEADER &&
            declaration->kind == VITTE_AST_NODE_SPACE_DECL &&
            declaration->child_count == 1u) {
            candidate = declaration;
            label = "first module header declared here";
            break;
        }
        if (parser_diagnostic->error ==
                VITTE_PARSER_ERROR_MISPLACED_MODULE_HEADER &&
            !(declaration->kind == VITTE_AST_NODE_SPACE_DECL &&
              declaration->child_count == 1u)) {
            candidate = declaration;
            label = "preceding top-level declaration is here";
            break;
        }
    }
    if (candidate == NULL || label == NULL || !candidate->span.valid ||
        source == NULL || candidate->span.begin > candidate->span.end ||
        candidate->span.end > source->length ||
        !vitte_source_location_from_offset(
            &context->sources,
            context->source_id,
            candidate->span.begin,
            &start) ||
        !start.valid ||
        !vitte_source_location_from_offset(
            &context->sources,
            context->source_id,
            candidate->span.end,
            &end) ||
        !end.valid) {
        return false;
    }

    memset(location, 0, sizeof(*location));
    location->kind = VITTE_CLI_DIAGNOSTIC_LOCATION_RELATED;
    location->label = label;
    location->span.valid = true;
    location->span.file_id = (uint32_t)context->source_id;
    location->span.source_id = context->source_id;
    location->span.source_name = source->path != NULL
        ? source->path
        : source_name;
    location->span.start_offset = candidate->span.begin;
    location->span.end_offset = candidate->span.end;
    location->span.start_line = start.line - 1u;
    location->span.start_column = start.column - 1u;
    location->span.end_line = end.line - 1u;
    location->span.end_column = end.column - 1u;
    return true;
}

static bool
vitte_main_parser_duplicate_form_field_related_location(
    const vitte_main_driver_context_t *context,
    const vitte_parser_diagnostic_t *parser_diagnostic,
    const char *source_name,
    vitte_cli_diagnostic_location_t *location)
{
    const vitte_ast_node_t *root;
    const vitte_ast_node_t *candidate = NULL;
    const vitte_source_entry_t *source;
    vitte_source_location_t start;
    vitte_source_location_t end;
    size_t declaration_index;

    if (context == NULL || parser_diagnostic == NULL ||
        source_name == NULL || location == NULL ||
        parser_diagnostic->error !=
            VITTE_PARSER_ERROR_DUPLICATE_FORM_FIELD ||
        !parser_diagnostic->span.valid) {
        return false;
    }

    root = vitte_parser_get_node(
        &context->parser,
        vitte_parser_root(&context->parser));
    source = vitte_source_get(&context->sources, context->source_id);
    if (root == NULL || root->kind != VITTE_AST_NODE_TRANSLATION_UNIT ||
        source == NULL) {
        return false;
    }

    for (declaration_index = 0u;
         declaration_index < root->child_count && candidate == NULL;
         ++declaration_index) {
        const vitte_ast_node_t *declaration = vitte_parser_get_node(
            &context->parser, root->children[declaration_index]);
        size_t field_index;

        if (declaration == NULL ||
            declaration->kind != VITTE_AST_NODE_FORM_DECL) {
            continue;
        }
        for (field_index = 1u;
             field_index < declaration->child_count;
             ++field_index) {
            const vitte_ast_node_t *field = vitte_parser_get_node(
                &context->parser, declaration->children[field_index]);
            const vitte_ast_node_t *field_name;
            const vitte_token_t *field_token;
            size_t previous_index;

            if (field == NULL || field->kind != VITTE_AST_NODE_FIELD_DECL ||
                field->child_count == 0u) {
                continue;
            }
            field_name = vitte_parser_get_node(
                &context->parser, field->children[0]);
            if (field_name == NULL || !field_name->span.valid ||
                field_name->span.begin != parser_diagnostic->span.begin ||
                field_name->span.end != parser_diagnostic->span.end) {
                continue;
            }
            field_token = field_name->token_index < context->parser.token_count
                ? &context->parser.tokens[field_name->token_index]
                : NULL;
            if (field_token == NULL) {
                return false;
            }
            for (previous_index = 1u;
                 previous_index < field_index;
                 ++previous_index) {
                const vitte_ast_node_t *previous_field =
                    vitte_parser_get_node(
                        &context->parser,
                        declaration->children[previous_index]);
                const vitte_ast_node_t *previous_name;
                const vitte_token_t *previous_token;

                if (previous_field == NULL ||
                    previous_field->kind != VITTE_AST_NODE_FIELD_DECL ||
                    previous_field->child_count == 0u) {
                    continue;
                }
                previous_name = vitte_parser_get_node(
                    &context->parser, previous_field->children[0]);
                previous_token = previous_name != NULL &&
                    previous_name->token_index < context->parser.token_count
                    ? &context->parser.tokens[previous_name->token_index]
                    : NULL;
                if (previous_token != NULL && previous_name->span.valid &&
                    previous_token->length == field_token->length &&
                    memcmp(previous_token->lexeme, field_token->lexeme,
                           field_token->length) == 0) {
                    candidate = previous_name;
                    break;
                }
            }
            break;
        }
    }

    if (candidate == NULL || !candidate->span.valid ||
        candidate->span.begin > candidate->span.end ||
        candidate->span.end > source->length ||
        !vitte_source_location_from_offset(
            &context->sources, context->source_id,
            candidate->span.begin, &start) || !start.valid ||
        !vitte_source_location_from_offset(
            &context->sources, context->source_id,
            candidate->span.end, &end) || !end.valid) {
        return false;
    }

    memset(location, 0, sizeof(*location));
    location->kind = VITTE_CLI_DIAGNOSTIC_LOCATION_RELATED;
    location->label = "first form field declared here";
    location->span.valid = true;
    location->span.file_id = candidate->span.file_id;
    location->span.source_id = context->source_id;
    location->span.source_name = source->path != NULL
        ? source->path
        : source_name;
    location->span.start_offset = candidate->span.begin;
    location->span.end_offset = candidate->span.end;
    location->span.start_line = start.line - 1u;
    location->span.start_column = start.column - 1u;
    location->span.end_line = end.line - 1u;
    location->span.end_column = end.column - 1u;
    return true;
}

static bool
vitte_main_parser_duplicate_pick_variant_related_location(
    const vitte_main_driver_context_t *context,
    const vitte_parser_diagnostic_t *parser_diagnostic,
    const char *source_name,
    vitte_cli_diagnostic_location_t *location)
{
    const vitte_ast_node_t *root;
    const vitte_ast_node_t *candidate = NULL;
    const vitte_source_entry_t *source;
    vitte_source_location_t start;
    vitte_source_location_t end;
    size_t declaration_index;

    if (context == NULL || parser_diagnostic == NULL || source_name == NULL ||
        location == NULL || parser_diagnostic->error !=
            VITTE_PARSER_ERROR_DUPLICATE_PICK_VARIANT ||
        !parser_diagnostic->span.valid) {
        return false;
    }
    root = vitte_parser_get_node(
        &context->parser, vitte_parser_root(&context->parser));
    source = vitte_source_get(&context->sources, context->source_id);
    if (root == NULL || root->kind != VITTE_AST_NODE_TRANSLATION_UNIT ||
        source == NULL) {
        return false;
    }

    for (declaration_index = 0u;
         declaration_index < root->child_count && candidate == NULL;
         ++declaration_index) {
        const vitte_ast_node_t *declaration = vitte_parser_get_node(
            &context->parser, root->children[declaration_index]);
        size_t variant_index;
        if (declaration == NULL || declaration->kind != VITTE_AST_NODE_PICK_DECL) {
            continue;
        }
        for (variant_index = 1u; variant_index < declaration->child_count;
             ++variant_index) {
            const vitte_ast_node_t *variant = vitte_parser_get_node(
                &context->parser, declaration->children[variant_index]);
            const vitte_ast_node_t *variant_name;
            const vitte_token_t *variant_token;
            size_t previous_index;
            if (variant == NULL || variant->kind != VITTE_AST_NODE_VARIANT_DECL ||
                variant->child_count == 0u) {
                continue;
            }
            variant_name = vitte_parser_get_node(
                &context->parser, variant->children[0]);
            if (variant_name == NULL || !variant_name->span.valid ||
                variant_name->span.begin != parser_diagnostic->span.begin ||
                variant_name->span.end != parser_diagnostic->span.end) {
                continue;
            }
            variant_token = variant_name->token_index < context->parser.token_count
                ? &context->parser.tokens[variant_name->token_index] : NULL;
            if (variant_token == NULL) {
                return false;
            }
            for (previous_index = 1u; previous_index < variant_index;
                 ++previous_index) {
                const vitte_ast_node_t *previous = vitte_parser_get_node(
                    &context->parser, declaration->children[previous_index]);
                const vitte_ast_node_t *previous_name;
                const vitte_token_t *previous_token;
                if (previous == NULL || previous->kind != VITTE_AST_NODE_VARIANT_DECL ||
                    previous->child_count == 0u) {
                    continue;
                }
                previous_name = vitte_parser_get_node(
                    &context->parser, previous->children[0]);
                previous_token = previous_name != NULL &&
                    previous_name->token_index < context->parser.token_count
                    ? &context->parser.tokens[previous_name->token_index] : NULL;
                if (previous_token != NULL && previous_name->span.valid &&
                    previous_token->length == variant_token->length &&
                    memcmp(previous_token->lexeme, variant_token->lexeme,
                           variant_token->length) == 0) {
                    candidate = previous_name;
                    break;
                }
            }
            break;
        }
    }
    if (candidate == NULL || !candidate->span.valid ||
        candidate->span.begin > candidate->span.end ||
        candidate->span.end > source->length ||
        !vitte_source_location_from_offset(
            &context->sources, context->source_id, candidate->span.begin, &start) ||
        !start.valid ||
        !vitte_source_location_from_offset(
            &context->sources, context->source_id, candidate->span.end, &end) ||
        !end.valid) {
        return false;
    }
    memset(location, 0, sizeof(*location));
    location->kind = VITTE_CLI_DIAGNOSTIC_LOCATION_RELATED;
    location->label = "first variant declared here";
    location->span.valid = true;
    location->span.file_id = candidate->span.file_id;
    location->span.source_id = context->source_id;
    location->span.source_name = source->path != NULL ? source->path : source_name;
    location->span.start_offset = candidate->span.begin;
    location->span.end_offset = candidate->span.end;
    location->span.start_line = start.line - 1u;
    location->span.start_column = start.column - 1u;
    location->span.end_line = end.line - 1u;
    location->span.end_column = end.column - 1u;
    return true;
}

bool
vitte_main_render_parser_diagnostics(
    const vitte_main_driver_context_t *context,
    bool *rendered)
{
    vitte_cli_diagnostic_t *diagnostics;
    vitte_cli_diagnostic_suggestion_t *suggestions;
    vitte_cli_diagnostic_location_t *related_locations;
    const vitte_source_entry_t *source;
    const char *source_name;
    size_t diagnostic_count;
    size_t index;

    if (context == NULL ||
        rendered == NULL) {
        return false;
    }

    *rendered = false;
    diagnostic_count = vitte_parser_diagnostic_count(&context->parser);
    if (diagnostic_count == 0u) {
        return true;
    }

    diagnostics = (vitte_cli_diagnostic_t *)calloc(
        diagnostic_count,
        sizeof(*diagnostics));
    suggestions = (vitte_cli_diagnostic_suggestion_t *)calloc(
        diagnostic_count,
        sizeof(*suggestions));
    related_locations = (vitte_cli_diagnostic_location_t *)calloc(
        diagnostic_count,
        sizeof(*related_locations));
    if (diagnostics == NULL || suggestions == NULL ||
        related_locations == NULL) {
        free(diagnostics);
        free(suggestions);
        free(related_locations);
        return false;
    }

    source = vitte_source_get(
        &context->sources,
        context->source_id);
    source_name = source != NULL
        ? source->name
        : context->options->input_path;

    for (index = 0u; index < diagnostic_count; ++index) {
        const vitte_parser_diagnostic_t *parser_diagnostic;
        vitte_cli_diagnostic_t *diagnostic;
        const char *parser_code;
        vitte_cli_diagnostic_severity_t parser_severity;
        const char *details;
        const char *contextual_message;
        const char *contextual_help;
        int details_length;

        parser_diagnostic = vitte_parser_diagnostic_at(
            &context->parser,
            index);
        if (parser_diagnostic == NULL) {
            free(diagnostics);
            free(suggestions);
            free(related_locations);
            return false;
        }

        diagnostic = &diagnostics[index];
        parser_code = vitte_main_parser_diagnostic_code(
            parser_diagnostic->error);
        if (parser_diagnostic->expected == VITTE_TOKEN_IDENTIFIER &&
            vitte_token_kind_is_keyword(parser_diagnostic->found)) {
            parser_code = "VITTE_PARSER_E_RESERVED_WORD_AS_NAME";
        }
        parser_severity =
            strcmp(parser_code, "VITTE_INFRA_E_INTERNAL") == 0 ||
            strcmp(parser_code, "VITTE_INFRA_E_INVARIANT") == 0 ||
            strcmp(parser_code, "VITTE_INFRA_E_RESOURCE") == 0
                ? VITTE_CLI_DIAGNOSTIC_FATAL
                : vitte_main_parser_diagnostic_severity(
                      parser_diagnostic->kind);
        if (!vitte_main_make_cli_diagnostic(
                context,
                diagnostic,
                parser_severity,
                parser_code,
                NULL,
                NULL,
                &parser_diagnostic->span,
                NULL,
                NULL)) {
            free(diagnostics);
            free(suggestions);
            free(related_locations);
            return false;
        }

        details_length = 0;
        if (parser_diagnostic->expected != VITTE_TOKEN_INVALID &&
            parser_diagnostic->found != VITTE_TOKEN_INVALID) {
            details_length = snprintf(
                diagnostic->details_storage,
                sizeof(diagnostic->details_storage),
                "expected %s, found %s",
                vitte_token_kind_name(parser_diagnostic->expected),
                parser_diagnostic->found == VITTE_TOKEN_EOF
                    ? "end of file"
                    : vitte_token_kind_name(parser_diagnostic->found));
        } else if (parser_diagnostic->expected != VITTE_TOKEN_INVALID) {
            details_length = snprintf(
                diagnostic->details_storage,
                sizeof(diagnostic->details_storage),
                "expected %s",
                vitte_token_kind_name(parser_diagnostic->expected));
        } else if (parser_diagnostic->found != VITTE_TOKEN_INVALID) {
            details_length = snprintf(
                diagnostic->details_storage,
                sizeof(diagnostic->details_storage),
                "found %s",
                vitte_token_kind_name(parser_diagnostic->found));
        }

        if (details_length < 0 ||
            (size_t)details_length >=
                sizeof(diagnostic->details_storage)) {
            free(diagnostics);
            free(suggestions);
            free(related_locations);
            return false;
        }

        details = diagnostic->details_storage[0] != '\0'
            ? diagnostic->details_storage
            : NULL;
        diagnostic->details = details;
        if (parser_diagnostic->expected != VITTE_TOKEN_INVALID) {
            diagnostic->expected = vitte_token_kind_name(
                parser_diagnostic->expected);
        }
        if (parser_diagnostic->found != VITTE_TOKEN_INVALID) {
            diagnostic->found = parser_diagnostic->found == VITTE_TOKEN_EOF
                ? "end of file"
                : vitte_token_kind_name(parser_diagnostic->found);
        }
        if (strcmp(parser_code, "VITTE_PARSER_E_RESERVED_WORD_AS_NAME") == 0 &&
            parser_diagnostic->span.valid &&
            parser_diagnostic->span.begin <= parser_diagnostic->span.end &&
            parser_diagnostic->span.end <= context->file.length &&
            parser_diagnostic->span.end - parser_diagnostic->span.begin <=
                (size_t)INT_MAX) {
            details_length = snprintf(
                diagnostic->details_storage,
                sizeof(diagnostic->details_storage),
                "reserved word `%.*s` cannot be used as a name",
                (int)(parser_diagnostic->span.end -
                      parser_diagnostic->span.begin),
                (const char *)context->file.data +
                    parser_diagnostic->span.begin);
            if (details_length < 0 ||
                (size_t)details_length >=
                    sizeof(diagnostic->details_storage)) {
                free(diagnostics);
                free(suggestions);
                free(related_locations);
                return false;
            }
            diagnostic->details = diagnostic->details_storage;
        }
        if (vitte_main_parser_contextual_expectation(
                context,
                parser_diagnostic,
                &contextual_message,
                &contextual_help)) {
            diagnostic->message = contextual_message;
            diagnostic->help = contextual_help;
        }
        vitte_main_parser_add_grammar_migration_suggestion(
            context,
            parser_diagnostic,
            source_name,
            diagnostic,
            &suggestions[index]);
        if (diagnostic->suggestion_count == 0u) {
            vitte_main_parser_add_punctuation_suggestion(
                context,
                parser_diagnostic,
                source_name,
                diagnostic,
                &suggestions[index]);
        }
        if (vitte_main_parser_matching_opener_location(
                context,
                parser_diagnostic,
                source_name,
                &related_locations[index])) {
            diagnostic->related = &related_locations[index];
            diagnostic->related_count = 1u;
        } else if (vitte_main_parser_module_header_related_location(
                       context,
                       parser_diagnostic,
                       source_name,
                       &related_locations[index])) {
            diagnostic->related = &related_locations[index];
            diagnostic->related_count = 1u;
        } else if (vitte_main_parser_duplicate_form_field_related_location(
                       context,
                       parser_diagnostic,
                       source_name,
                       &related_locations[index])) {
            diagnostic->related = &related_locations[index];
            diagnostic->related_count = 1u;
        } else if (vitte_main_parser_duplicate_pick_variant_related_location(
                       context,
                       parser_diagnostic,
                       source_name,
                       &related_locations[index])) {
            diagnostic->related = &related_locations[index];
            diagnostic->related_count = 1u;
        }
        if (diagnostic->suggestion_count != 0u) {
            size_t suggestion_index;

            for (suggestion_index = 0u;
                 suggestion_index < diagnostic->suggestion_count;
                 ++suggestion_index) {
                vitte_main_diagnostic_map_span(
                    context,
                    &suggestions[index].span);
            }
        }
        if (diagnostic->related_count != 0u) {
            size_t related_index;

            for (related_index = 0u;
                 related_index < diagnostic->related_count;
                 ++related_index) {
                vitte_main_diagnostic_map_span(
                    context,
                    &related_locations[index].span);
            }
        }
    }

    *rendered = vitte_diagnostic_render_cli_batch(
        stderr,
        &context->sources,
        context->source_id,
        source_name,
        diagnostics,
        diagnostic_count,
        context->options->color,
        vitte_main_cli_diagnostic_format(
            context->options->diagnostic_format));
    free(diagnostics);
    free(suggestions);
    free(related_locations);
    return *rendered;
}

bool
vitte_main_driver_parse(
    vitte_driver_t *driver,
    vitte_driver_phase_t phase,
    const vitte_driver_input_t *input,
    void *user_data)
{
    vitte_main_driver_context_t *context;

    (void)driver;
    (void)phase;
    (void)input;

    context = (vitte_main_driver_context_t *)user_data;

    if (context == NULL) {
        return false;
    }

    if (context->native_frontend_completed) {
        return true;
    }
    if (!context->lexer_initialized) {
        return false;
    }

    vitte_main_verbose(context->options, "parsing");

    {
        vitte_macro_expansion_result_t expansion;
        if (!vitte_macro_expand_tokens(context->lexer.tokens,
                context->lexer.token_count, &expansion)) {
            const char *code = expansion.error == VITTE_MACRO_EXPANSION_ERROR_RECURSION
                ? "E1802" : "E1801";
            const char *message = expansion.error == VITTE_MACRO_EXPANSION_ERROR_RECURSION
                ? "macro expansion recursion limit reached" : "macro expansion failed";
            const char *details = expansion.error == VITTE_MACRO_EXPANSION_ERROR_RECURSION
                ? "A macro expands itself or exceeds the supported expansion depth."
                : expansion.error == VITTE_MACRO_EXPANSION_ERROR_INVALID_INVOCATION
                    ? "Check that the macro exists and that its invocation supplies the declared number of arguments."
                    : "Macro expansion exceeded an implementation resource limit.";
            vitte_parser_span_t span;
            span.file_id = expansion.error_span.file_id;
            span.begin = expansion.error_span.begin;
            span.end = expansion.error_span.end;
            span.line = 0u;
            span.column = 0u;
            span.valid = expansion.error_span.valid;
            (void)vitte_main_render_diagnostic(context, VITTE_CLI_DIAGNOSTIC_ERROR,
                code, message, details, &span, NULL, "macro expansion");
            vitte_macro_expansion_result_destroy(&expansion);
            return false;
        }
        context->expanded_tokens = expansion.tokens;
        context->expanded_token_count = expansion.token_count;
    }

    if (!vitte_parser_init(
            &context->parser,
            context->expanded_tokens,
            context->expanded_token_count)) {
        vitte_main_error("failed to initialize parser");
        return false;
    }

    context->parser_initialized = true;

    if (!vitte_parser_run(&context->parser)) {
        bool rendered;

        if (!vitte_main_render_parser_diagnostics(
                context,
                &rendered)) {
            vitte_main_error(
                "failed to render parser diagnostics");
            return false;
        }

        if (!rendered) {
            if (!vitte_main_render_diagnostic(
                    context,
                    VITTE_CLI_DIAGNOSTIC_ERROR,
                    "VITTE_INFRA_E_PHASE",
                    "parsing failed",
                    vitte_parser_error_name(
                        vitte_parser_last_error(&context->parser)),
                    NULL,
                    NULL,
                    NULL)) {
                vitte_main_error(
                    "failed to render parser failure");
            }
        }
        return false;
    }

    /* Re-check the completed AST at the driver boundary so later phases
       never consume a tree that changed after parser finalization. */
    if (!vitte_parser_validate(&context->parser)) {
        vitte_main_error("AST validation failed after parsing");
        return false;
    }

    if (context->options->emit_ast) {
        const vitte_ast_node_t *root;

        root = vitte_parser_get_node(
            &context->parser,
            vitte_parser_root(&context->parser));

        if (root == NULL) {
            vitte_main_error("parser produced no translation unit");
            return false;
        }

        (void)printf(
            "AST root kind: %u\n"
            "nodes: %zu\n",
            (unsigned int)root->kind,
            context->parser.node_count);
    }

    return true;
}

static const char *
vitte_main_sema_diagnostic_code(
    vitte_sema_diagnostic_code_t code)
{
    switch (code) {
        case VITTE_SEMA_DIAGNOSTIC_UNRESOLVED_NAME:
            return "VITTE_RESOLVE_E_UNKNOWN_SYMBOL";

        case VITTE_SEMA_DIAGNOSTIC_AMBIGUOUS_NAME:
            return "VITTE_RESOLVE_E_AMBIGUOUS_SYMBOL";

        case VITTE_SEMA_DIAGNOSTIC_DUPLICATE_IMPORT:
            return "VITTE_IMPORT_E_DUPLICATE";

        case VITTE_SEMA_DIAGNOSTIC_PRIVATE_SYMBOL:
            return "VITTE_RESOLVE_E_PRIVATE";

        case VITTE_SEMA_DIAGNOSTIC_DUPLICATE_DECLARATION:
            return "VITTE_RESOLVE_E_REDECLARATION";

        case VITTE_SEMA_DIAGNOSTIC_INVALID_SCOPE:
            return "VITTE_RESOLVE_E_INVALID_SCOPE";

        case VITTE_SEMA_DIAGNOSTIC_DUPLICATE_BINDING:
            return "VITTE_RESOLVE_E_DUPLICATE_BINDING";

        case VITTE_SEMA_DIAGNOSTIC_INVALID_EXPORT:
            return "VITTE_RESOLVE_E_INVALID_EXPORT";

        case VITTE_SEMA_DIAGNOSTIC_SHADOWED_DECLARATION:
            return "VITTE_WARNING_SHADOWING";

        case VITTE_SEMA_DIAGNOSTIC_NOT_CALLABLE:
            return "VITTE_TYPE_E_NOT_CALLABLE";

        case VITTE_SEMA_DIAGNOSTIC_INVALID_ARGUMENT_COUNT:
            return "VITTE_CALL_E_ARITY";

        case VITTE_SEMA_DIAGNOSTIC_INVALID_ARGUMENT_TYPE:
            return "VITTE_CALL_E_ARGUMENT_TYPE";

        case VITTE_SEMA_DIAGNOSTIC_NO_MATCHING_PROCEDURE:
            return "VITTE_CALL_E_NO_CANDIDATE";

        case VITTE_SEMA_DIAGNOSTIC_AMBIGUOUS_CALL:
            return "VITTE_CALL_E_AMBIGUOUS";

        case VITTE_SEMA_DIAGNOSTIC_INVALID_PROCEDURE_MODIFIER:
            return "VITTE_CALL_E_MODIFIER";

        case VITTE_SEMA_DIAGNOSTIC_ASYNC_CONTEXT:
            return "VITTE_ASYNC_E_AWAIT_CONTEXT";

        case VITTE_SEMA_DIAGNOSTIC_INVALID_TASK_TYPE:
            return "VITTE_ASYNC_E_TASK_TYPE";

        case VITTE_SEMA_DIAGNOSTIC_INVALID_PROCEDURE_PARAMETER:
            return "VITTE_CALL_E_PARAMETER";

        case VITTE_SEMA_DIAGNOSTIC_UNKNOWN_TYPE:
            return "VITTE_TYPE_E_UNKNOWN_TYPE";

        case VITTE_SEMA_DIAGNOSTIC_NOT_A_TYPE:
            return "VITTE_TYPE_E_NOT_A_TYPE";
        case VITTE_SEMA_DIAGNOSTIC_INVALID_CAST:
            return "VITTE_TYPE_E_INVALID_CAST";
        case VITTE_SEMA_DIAGNOSTIC_NOT_A_VALUE:
            return "VITTE_TYPE_E_NOT_A_VALUE";

        case VITTE_SEMA_DIAGNOSTIC_INVALID_UNARY_OPERAND:
        case VITTE_SEMA_DIAGNOSTIC_INVALID_BINARY_OPERANDS:
            return "VITTE_TYPE_E_INVALID_OPERAND";

        case VITTE_SEMA_DIAGNOSTIC_NOT_ASSIGNABLE:
            return "VITTE_TYPE_E_NOT_ASSIGNABLE";

        case VITTE_SEMA_DIAGNOSTIC_NOT_INDEXABLE:
            return "VITTE_TYPE_E_NOT_INDEXABLE";

        case VITTE_SEMA_DIAGNOSTIC_INVALID_INDEX_TYPE:
            return "VITTE_TYPE_E_INVALID_INDEX";

        case VITTE_SEMA_DIAGNOSTIC_UNKNOWN_MEMBER:
            return "VITTE_TYPE_E_NO_MEMBER";

        case VITTE_SEMA_DIAGNOSTIC_INCOMPLETE_TYPE:
            return "VITTE_TYPE_E_INCOMPLETE";

        case VITTE_SEMA_DIAGNOSTIC_RECURSIVE_TYPE:
            return "VITTE_TYPE_E_RECURSIVE";

        case VITTE_SEMA_DIAGNOSTIC_GENERIC_BOUND_UNSATISFIED:
            return "VITTE_GENERIC_E_BOUND";

        case VITTE_SEMA_DIAGNOSTIC_GENERIC_ARGUMENT_COUNT:
            return "VITTE_GENERIC_E_ARGUMENT_COUNT";

        case VITTE_SEMA_DIAGNOSTIC_GENERIC_ARGUMENT_KIND:
            return "VITTE_GENERIC_E_ARGUMENT_KIND";

        case VITTE_SEMA_DIAGNOSTIC_GENERIC_INFERENCE_CONFLICT:
            return "VITTE_GENERIC_E_INFERENCE_CONFLICT";

        case VITTE_SEMA_DIAGNOSTIC_TRAIT_NOT_IMPLEMENTED:
            return "VITTE_TRAIT_E_NOT_IMPLEMENTED";

        case VITTE_SEMA_DIAGNOSTIC_AMBIGUOUS_IMPL:
            return "VITTE_TRAIT_E_AMBIGUOUS_IMPL";

        case VITTE_SEMA_DIAGNOSTIC_CONFLICTING_IMPL:
            return "VITTE_TRAIT_E_CONFLICTING_IMPL";

        case VITTE_SEMA_DIAGNOSTIC_MISSING_TRAIT_MEMBER:
            return "VITTE_TRAIT_E_MISSING_MEMBER";

        case VITTE_SEMA_DIAGNOSTIC_INVALID_TRAIT_MEMBER:
            return "VITTE_TRAIT_E_INVALID_MEMBER";

        case VITTE_SEMA_DIAGNOSTIC_INVALID_IMPL_TARGET:
            return "VITTE_TRAIT_E_INVALID_IMPL_TARGET";

        case VITTE_SEMA_DIAGNOSTIC_INVALID_WHERE_PREDICATE:
            return "VITTE_TRAIT_E_INVALID_WHERE_PREDICATE";

        case VITTE_SEMA_DIAGNOSTIC_AMBIGUOUS_METHOD:
            return "VITTE_TRAIT_E_AMBIGUOUS_METHOD";

        case VITTE_SEMA_DIAGNOSTIC_CONTRACT_REQUIRES:
            return "VITTE_CONTRACT_E_REQUIRES";
        case VITTE_SEMA_DIAGNOSTIC_CONTRACT_ENSURES:
            return "VITTE_CONTRACT_E_ENSURES";
        case VITTE_SEMA_DIAGNOSTIC_CONTRACT_RESULT:
            return "VITTE_CONTRACT_E_RESULT";
        case VITTE_SEMA_DIAGNOSTIC_CONTRACT_CONTRADICTION:
            return "VITTE_CONTRACT_E_CONTRADICTION";
        case VITTE_SEMA_DIAGNOSTIC_CONTRACT_POSTCONDITION:
            return "VITTE_CONTRACT_E_POSTCONDITION";
        case VITTE_SEMA_DIAGNOSTIC_MISSING_RETURN:
            return "VITTE_CONTROL_E_MISSING_RETURN";
        case VITTE_SEMA_DIAGNOSTIC_CONTRACT_PRECONDITION:
            return "VITTE_CONTRACT_E_PRECONDITION";
        case VITTE_SEMA_DIAGNOSTIC_CONSTANT_OVERFLOW:
            return "VITTE_CONST_E_OVERFLOW";
        case VITTE_SEMA_DIAGNOSTIC_CONSTANT_DIVIDE_BY_ZERO:
            return "VITTE_CONST_E_DIVIDE_BY_ZERO";
        case VITTE_SEMA_DIAGNOSTIC_INDEX_OUT_OF_BOUNDS:
            return "VITTE_MEMORY_E_INVALID_ACCESS";
        case VITTE_SEMA_DIAGNOSTIC_CONTRACT_EFFECT:
            return "VITTE_CONTRACT_E_EFFECT";
        case VITTE_SEMA_DIAGNOSTIC_CONTRACT_OLD_VALUE:
            return "VITTE_CONTRACT_E_OLD_VALUE";
        case VITTE_SEMA_DIAGNOSTIC_CONSTANT_NOT_CONSTANT:
            return "VITTE_CONST_E_NOT_CONSTANT";
        case VITTE_SEMA_DIAGNOSTIC_CONSTANT_REMAINDER_BY_ZERO:
            return "VITTE_CONST_E_REMAINDER_BY_ZERO";
        case VITTE_SEMA_DIAGNOSTIC_MATCH_NON_EXHAUSTIVE:
            return "VITTE_PATTERN_E_NON_EXHAUSTIVE";
        case VITTE_SEMA_DIAGNOSTIC_MATCH_UNREACHABLE:
            return "VITTE_PATTERN_E_UNREACHABLE";
        case VITTE_SEMA_DIAGNOSTIC_CONSTANT_CYCLE:
            return "VITTE_CONST_E_CYCLE";
        case VITTE_SEMA_DIAGNOSTIC_MATCH_TYPE:
            return "VITTE_PATTERN_E_TYPE";
        case VITTE_SEMA_DIAGNOSTIC_INVALID_PATTERN_BINDINGS:
            return "VITTE_PATTERN_E_BINDING";
        case VITTE_SEMA_DIAGNOSTIC_INVALID_DEFER_CONTROL_FLOW:
            return "VITTE_CONTROL_E_INVALID_DEFER";
        case VITTE_SEMA_DIAGNOSTIC_OVERLAPPING_PATTERN:
            return "VITTE_WARNING_OVERLAPPING_PATTERN";
        case VITTE_SEMA_DIAGNOSTIC_UNSAFE_REQUIRED:
            return "VITTE_UNSAFE_E_REQUIRED";
        case VITTE_SEMA_DIAGNOSTIC_INVALID_ASM:
            return "VITTE_UNSAFE_E_INVALID_ASM";
        case VITTE_SEMA_DIAGNOSTIC_FFI_ABI:
            return "VITTE_FFI_E_ABI";
        case VITTE_SEMA_DIAGNOSTIC_FFI_TYPE:
            return "VITTE_FFI_E_TYPE";
        case VITTE_SEMA_DIAGNOSTIC_REFERENCE_ESCAPE:
            return "VITTE_MEMORY_E_REFERENCE_ESCAPE";
        case VITTE_SEMA_DIAGNOSTIC_BORROW_CONFLICT:
            return "VITTE_MEMORY_E_BORROW_CONFLICT";
        case VITTE_SEMA_DIAGNOSTIC_BORROWED_MUTATION:
            return "VITTE_MEMORY_E_BORROWED_MUTATION";
        case VITTE_SEMA_DIAGNOSTIC_MOVE_BORROWED:
            return "VITTE_MEMORY_E_MOVE_BORROWED";
        case VITTE_SEMA_DIAGNOSTIC_USE_AFTER_MOVE:
            return "VITTE_MEMORY_E_USE_AFTER_MOVE";
        case VITTE_SEMA_DIAGNOSTIC_CANNOT_INFER:
            return "VITTE_TYPE_E_CANNOT_INFER";
        case VITTE_SEMA_DIAGNOSTIC_CONDITION_NOT_BOOL:
            return "VITTE_TYPE_E_CONDITION_NOT_BOOL";

        case VITTE_SEMA_DIAGNOSTIC_RETURN_TYPE_MISMATCH:
            return "VITTE_CALL_E_RETURN_TYPE";

        case VITTE_SEMA_DIAGNOSTIC_RETURN_OUTSIDE_PROC:
            return "VITTE_CALL_E_RETURN_OUTSIDE_PROC";

        case VITTE_SEMA_DIAGNOSTIC_BREAK_OUTSIDE_LOOP:
            return "VITTE_CONTROL_E_INVALID_BREAK";

        case VITTE_SEMA_DIAGNOSTIC_CONTINUE_OUTSIDE_LOOP:
            return "VITTE_CONTROL_E_INVALID_CONTINUE";

        case VITTE_SEMA_DIAGNOSTIC_UNREACHABLE_CODE:
            return "VITTE_WARNING_UNREACHABLE_CODE";

        case VITTE_SEMA_DIAGNOSTIC_RECURSION_LIMIT:
            return "VITTE_INFRA_E_LIMIT";

        case VITTE_SEMA_DIAGNOSTIC_NONE:
        case VITTE_SEMA_DIAGNOSTIC_TYPE_MISMATCH:
        case VITTE_SEMA_DIAGNOSTIC_INVALID_TYPE:
            return "VITTE_TYPE_E_MISMATCH";

        case VITTE_SEMA_DIAGNOSTIC_CODE_COUNT:
            return "VITTE_INFRA_E_FALLBACK";
    }

    return "VITTE_INFRA_E_FALLBACK";
}

static vitte_cli_diagnostic_severity_t
vitte_main_sema_diagnostic_severity(
    vitte_sema_diagnostic_kind_t kind)
{
    switch (kind) {
        case VITTE_SEMA_DIAGNOSTIC_NOTE:
            return VITTE_CLI_DIAGNOSTIC_NOTE;

        case VITTE_SEMA_DIAGNOSTIC_HELP:
            return VITTE_CLI_DIAGNOSTIC_HELP;

        case VITTE_SEMA_DIAGNOSTIC_WARNING:
            return VITTE_CLI_DIAGNOSTIC_WARNING;

        case VITTE_SEMA_DIAGNOSTIC_ERROR:
        case VITTE_SEMA_DIAGNOSTIC_KIND_COUNT:
            return VITTE_CLI_DIAGNOSTIC_ERROR;
    }

    return VITTE_CLI_DIAGNOSTIC_ERROR;
}

#define VITTE_MAIN_SEMA_TYPE_NAME_CAPACITY 512u

static bool
vitte_main_sema_format_type_inner(
    const vitte_sema_t *sema,
    vitte_sema_type_id_t type_id,
    char *buffer,
    size_t capacity,
    size_t depth)
{
    const vitte_sema_type_t *type;
    const vitte_symbol_t *symbol;
    char element[VITTE_MAIN_SEMA_TYPE_NAME_CAPACITY];
    const char *name;
    int length;

    if (sema == NULL || buffer == NULL || capacity == 0u ||
        type_id == VITTE_SEMA_INVALID_TYPE_ID) {
        return false;
    }
    buffer[0] = '\0';
    if (depth >= 16u) {
        length = snprintf(buffer, capacity, "…");
        return length >= 0 && (size_t)length < capacity;
    }

    type = vitte_sema_get_type(sema, type_id);
    if (type == NULL) {
        return false;
    }

    switch (type->kind) {
        case VITTE_SEMA_TYPE_POINTER:
        case VITTE_SEMA_TYPE_REFERENCE:
        case VITTE_SEMA_TYPE_ARRAY:
        case VITTE_SEMA_TYPE_SLICE:
        case VITTE_SEMA_TYPE_RANGE:
            if (!vitte_main_sema_format_type_inner(
                    sema,
                    type->element_type,
                    element,
                    sizeof(element),
                    depth + 1u)) {
                return false;
            }
            if (type->kind == VITTE_SEMA_TYPE_POINTER) {
                name = type->is_mutable ? "*mut " : "*";
                length = snprintf(buffer, capacity, "%s%s", name, element);
            } else if (type->kind == VITTE_SEMA_TYPE_REFERENCE) {
                name = type->is_mutable ? "&mut " : "&";
                length = snprintf(buffer, capacity, "%s%s", name, element);
            } else if (type->kind == VITTE_SEMA_TYPE_SLICE) {
                length = snprintf(buffer, capacity, "[%s]", element);
            } else if (type->kind == VITTE_SEMA_TYPE_ARRAY &&
                       type->array_length != 0u) {
                length = snprintf(
                    buffer,
                    capacity,
                    "[%s; %zu]",
                    element,
                    type->array_length);
            } else if (type->kind == VITTE_SEMA_TYPE_ARRAY) {
                length = snprintf(buffer, capacity, "array<%s>", element);
            } else {
                length = snprintf(buffer, capacity, "range<%s>", element);
            }
            return length >= 0 && (size_t)length < capacity;

        case VITTE_SEMA_TYPE_FUNCTION:
            if (type->return_type == VITTE_SEMA_INVALID_TYPE_ID) {
                length = snprintf(buffer, capacity, "proc()");
            } else if (!vitte_main_sema_format_type_inner(
                           sema,
                           type->return_type,
                           element,
                           sizeof(element),
                           depth + 1u)) {
                return false;
            } else {
                length = snprintf(buffer, capacity, "proc() -> %s", element);
            }
            return length >= 0 && (size_t)length < capacity;

        case VITTE_SEMA_TYPE_NAMED:
        case VITTE_SEMA_TYPE_DYN:
            symbol = vitte_scope_get_symbol(&sema->scopes, type->symbol_id);
            if (symbol != NULL && symbol->name != NULL &&
                symbol->name_length != 0u) {
                length = snprintf(
                    buffer,
                    capacity,
                    type->kind == VITTE_SEMA_TYPE_DYN
                        ? "dyn %.*s"
                        : "%.*s",
                    (int)(symbol->name_length > (size_t)INT32_MAX
                        ? (size_t)INT32_MAX
                        : symbol->name_length),
                    symbol->name);
                if (length >= 0 &&
                    (size_t)length < capacity &&
                    type->kind == VITTE_SEMA_TYPE_NAMED &&
                    type->generic_argument_count != 0u) {
                    size_t offset = (size_t)length;
                    size_t argument_index;
                    for (argument_index = 0u;
                         argument_index < type->generic_argument_count;
                         ++argument_index) {
                        char argument[VITTE_MAIN_SEMA_TYPE_NAME_CAPACITY];
                        int written;
                        if (!vitte_main_sema_format_type_inner(
                                sema,
                                type->generic_arguments[argument_index],
                                argument,
                                sizeof(argument),
                                depth + 1u)) {
                            return false;
                        }
                        written = snprintf(
                            buffer + offset,
                            capacity - offset,
                            argument_index == 0u ? "<%s" : ", %s",
                            argument);
                        if (written < 0 ||
                            (size_t)written >= capacity - offset) {
                            return false;
                        }
                        offset += (size_t)written;
                    }
                    if (offset + 1u >= capacity) {
                        return false;
                    }
                    buffer[offset++] = '>';
                    buffer[offset] = '\0';
                    length = (int)offset;
                }
            } else {
                name = vitte_sema_type_kind_name(type->kind);
                length = snprintf(buffer, capacity, "%s", name);
            }
            return length >= 0 && (size_t)length < capacity;

        default:
            name = vitte_sema_type_kind_name(type->kind);
            length = snprintf(buffer, capacity, "%s", name);
            return length >= 0 && (size_t)length < capacity;
    }
}

static bool
vitte_main_sema_format_type(
    const vitte_sema_t *sema,
    vitte_sema_type_id_t type_id,
    char *buffer,
    size_t capacity)
{
    return vitte_main_sema_format_type_inner(
        sema,
        type_id,
        buffer,
        capacity,
        0u);
}

static bool
vitte_main_origin_offset(
    const vitte_main_driver_context_t *context,
    size_t combined_offset,
    bool end_position,
    const vitte_main_source_origin_t **origin,
    size_t *source_offset)
{
    size_t index;

    if (context == NULL || origin == NULL || source_offset == NULL) {
        return false;
    }

    for (index = 0u; index < context->origin_count; ++index) {
        const vitte_main_source_origin_t *candidate;
        bool matches;

        candidate = &context->origins[index];
        matches = end_position
            ? combined_offset > candidate->combined_begin &&
                combined_offset <= candidate->combined_end
            : combined_offset >= candidate->combined_begin &&
                combined_offset < candidate->combined_end;
        if (matches && candidate->source_id != VITTE_SOURCE_INVALID_ID) {
            *origin = candidate;
            *source_offset = candidate->source_begin +
                (combined_offset - candidate->combined_begin);
            return true;
        }
    }

    return false;
}

static bool
vitte_main_origin_span(
    const vitte_main_driver_context_t *context,
    size_t combined_begin,
    size_t combined_end,
    vitte_source_id_t *source_id,
    size_t *source_begin,
    size_t *source_end)
{
    const vitte_main_source_origin_t *begin_origin;
    const vitte_main_source_origin_t *end_origin;
    size_t mapped_begin;
    size_t mapped_end;

    if (context == NULL || source_id == NULL ||
        source_begin == NULL || source_end == NULL ||
        combined_begin > combined_end) {
        return false;
    }

    begin_origin = NULL;
    end_origin = NULL;
    if (combined_begin == combined_end) {
        if (!vitte_main_origin_offset(
                context,
                combined_begin,
                false,
                &begin_origin,
                &mapped_begin) &&
            !vitte_main_origin_offset(
                context,
                combined_begin,
                true,
                &begin_origin,
                &mapped_begin)) {
            return false;
        }
        end_origin = begin_origin;
        mapped_end = mapped_begin;
    } else if (!vitte_main_origin_offset(
                   context,
                   combined_begin,
                   false,
                   &begin_origin,
                   &mapped_begin) ||
               !vitte_main_origin_offset(
                   context,
                   combined_end,
                   true,
                   &end_origin,
                   &mapped_end)) {
        return false;
    }

    if (begin_origin == NULL || end_origin == NULL ||
        begin_origin->source_id == VITTE_SOURCE_INVALID_ID ||
        begin_origin->source_id != end_origin->source_id ||
        mapped_begin > mapped_end) {
        return false;
    }

    *source_id = begin_origin->source_id;
    *source_begin = mapped_begin;
    *source_end = mapped_end;
    return true;
}

static void
vitte_main_diagnostic_map_span(
    const vitte_main_driver_context_t *context,
    vitte_cli_diagnostic_span_t *span)
{
    const vitte_source_entry_t *source;
    vitte_source_id_t source_id;
    vitte_source_location_t start;
    vitte_source_location_t end;
    size_t source_begin;
    size_t source_end;

    if (context == NULL || span == NULL || !span->valid ||
        (span->source_id != VITTE_SOURCE_INVALID_ID &&
         span->source_id != context->source_id) ||
        !vitte_main_origin_span(
            context,
            span->start_offset,
            span->end_offset,
            &source_id,
            &source_begin,
            &source_end)) {
        return;
    }

    source = vitte_source_get(&context->sources, source_id);
    if (source == NULL ||
        !vitte_source_location_from_offset(
            &context->sources,
            source_id,
            source_begin,
            &start) ||
        !start.valid ||
        !vitte_source_location_from_offset(
            &context->sources,
            source_id,
            source_end,
            &end) ||
        !end.valid) {
        return;
    }

    span->file_id = (uint32_t)source_id;
    span->source_id = source_id;
    span->source_name = source->path != NULL
        ? source->path
        : source->name;
    span->start_offset = source_begin;
    span->end_offset = source_end;
    span->start_line = start.line - 1u;
    span->start_column = start.column - 1u;
    span->end_line = end.line - 1u;
    span->end_column = end.column - 1u;
}

#ifdef VITTE_NATIVE_FRONTEND_LINKED
static void
vitte_main_diagnostic_map_native_batch(
    const vitte_main_driver_context_t *context,
    vitte_cli_diagnostic_t *diagnostics,
    size_t diagnostic_count)
{
    size_t index;

    if (context == NULL || diagnostics == NULL) {
        return;
    }
    for (index = 0u; index < diagnostic_count; ++index) {
        vitte_cli_diagnostic_t *diagnostic;
        vitte_cli_diagnostic_span_t span;

        diagnostic = &diagnostics[index];
        if (diagnostic->expected != NULL && diagnostic->found != NULL &&
            (strcmp(diagnostic->code, "VITTE_CALL_E_ARGUMENT_TYPE") == 0 ||
             strcmp(diagnostic->code, "VITTE_TYPE_E_MISMATCH") == 0 ||
             strcmp(diagnostic->code, "VITTE_CALL_E_RETURN_TYPE") == 0 ||
             strcmp(diagnostic->code, "VITTE_TYPE_E_INVALID_OPERAND") == 0)) {
            diagnostic->expected_type = diagnostic->expected;
            diagnostic->actual_type = diagnostic->found;
        }
        if (!diagnostic->has_span) {
            continue;
        }
        memset(&span, 0, sizeof(span));
        span.valid = true;
        span.file_id = diagnostic->file_id;
        span.source_id = diagnostic->source_id;
        span.source_name = diagnostic->source_name;
        span.start_offset = diagnostic->start_offset;
        span.end_offset = diagnostic->end_offset;
        span.start_line = diagnostic->start_line;
        span.start_column = diagnostic->start_column;
        span.end_line = diagnostic->end_line;
        span.end_column = diagnostic->end_column;
        vitte_main_diagnostic_map_span(context, &span);
        diagnostic->file_id = span.file_id;
        diagnostic->source_id = span.source_id;
        diagnostic->source_name = span.source_name;
        diagnostic->start_offset = span.start_offset;
        diagnostic->end_offset = span.end_offset;
        diagnostic->start_line = span.start_line;
        diagnostic->start_column = span.start_column;
        diagnostic->end_line = span.end_line;
        diagnostic->end_column = span.end_column;
        if (diagnostic->suggestions != NULL &&
            diagnostic->suggestion_count != 0u) {
            size_t suggestion_index;
            vitte_cli_diagnostic_suggestion_t *mutable_suggestions =
                (vitte_cli_diagnostic_suggestion_t *)diagnostic->suggestions;

            for (suggestion_index = 0u;
                 suggestion_index < diagnostic->suggestion_count;
                 ++suggestion_index) {
                if (mutable_suggestions[suggestion_index].has_span) {
                    vitte_main_diagnostic_map_span(
                        context,
                        &mutable_suggestions[suggestion_index].span);
                }
            }
        }
    }
}
#endif

static bool
vitte_main_sema_symbol_origin_location(
    const vitte_main_driver_context_t *context,
    const vitte_sema_diagnostic_t *sema_diagnostic,
    const char *source_name,
    vitte_cli_diagnostic_t *diagnostic,
    vitte_cli_diagnostic_location_t *location)
{
    const vitte_symbol_t *symbol;
    const vitte_source_entry_t *origin_source;
    vitte_source_location_t start;
    vitte_source_location_t end;
    vitte_source_id_t origin_source_id;
    vitte_scope_span_t declaration_span;
    const vitte_ast_node_t *declaration;
    size_t origin_begin;
    size_t origin_end;
    const vitte_main_source_origin_t *begin_origin;
    const vitte_main_source_origin_t *end_origin;
    const char *label;
    vitte_symbol_id_t origin_symbol_id;
    bool type_origin;

    if (context == NULL || sema_diagnostic == NULL ||
        diagnostic == NULL || location == NULL) {
        return false;
    }

    type_origin =
        sema_diagnostic->context_symbol_id !=
            VITTE_SCOPE_INVALID_SYMBOL_ID &&
        (sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_TYPE_MISMATCH ||
         sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_INVALID_ARGUMENT_TYPE);
    if (sema_diagnostic->code != VITTE_SEMA_DIAGNOSTIC_PRIVATE_SYMBOL &&
        sema_diagnostic->code != VITTE_SEMA_DIAGNOSTIC_DUPLICATE_DECLARATION &&
        sema_diagnostic->code != VITTE_SEMA_DIAGNOSTIC_DUPLICATE_BINDING &&
        sema_diagnostic->code != VITTE_SEMA_DIAGNOSTIC_INVALID_EXPORT &&
        sema_diagnostic->code != VITTE_SEMA_DIAGNOSTIC_SHADOWED_DECLARATION &&
        sema_diagnostic->code != VITTE_SEMA_DIAGNOSTIC_AMBIGUOUS_NAME &&
        sema_diagnostic->code != VITTE_SEMA_DIAGNOSTIC_DUPLICATE_IMPORT &&
        sema_diagnostic->code != VITTE_SEMA_DIAGNOSTIC_NOT_A_TYPE &&
        sema_diagnostic->code != VITTE_SEMA_DIAGNOSTIC_NOT_A_VALUE &&
        !type_origin) {
        return false;
    }
    if (!type_origin &&
        sema_diagnostic->symbol_id == VITTE_SCOPE_INVALID_SYMBOL_ID) {
        return false;
    }

    origin_symbol_id = type_origin
        ? sema_diagnostic->context_symbol_id
        : sema_diagnostic->symbol_id;
    if (sema_diagnostic->code ==
        VITTE_SEMA_DIAGNOSTIC_SHADOWED_DECLARATION) {
        const vitte_symbol_t *shadowing_symbol = vitte_scope_get_symbol(
            &context->sema.scopes,
            sema_diagnostic->symbol_id);
        if (shadowing_symbol == NULL ||
            shadowing_symbol->shadowed_symbol ==
                VITTE_SCOPE_INVALID_SYMBOL_ID) {
            return false;
        }
        origin_symbol_id = shadowing_symbol->shadowed_symbol;
    }

    symbol = vitte_scope_get_symbol(
        &context->sema.scopes,
        origin_symbol_id);
    if (symbol == NULL || !symbol->span.valid ||
        symbol->span.begin > symbol->span.end) {
        return false;
    }

    declaration_span = symbol->span;
    declaration = (const vitte_ast_node_t *)symbol->payload;
    if (sema_diagnostic->code != VITTE_SEMA_DIAGNOSTIC_DUPLICATE_IMPORT &&
        !(sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_INVALID_EXPORT &&
          symbol->kind == VITTE_SYMBOL_KIND_SPACE) &&
        declaration != NULL && declaration->span.valid &&
        declaration->span.begin <= declaration->span.end) {
        declaration_span.file_id = declaration->span.file_id;
        declaration_span.begin = declaration->span.begin;
        declaration_span.end = declaration->span.end;
    }

    begin_origin = NULL;
    end_origin = NULL;
    origin_begin = declaration_span.begin;
    origin_end = declaration_span.end;
    origin_source_id = (vitte_source_id_t)declaration_span.file_id;
    if (vitte_main_origin_offset(
            context,
            declaration_span.begin,
            false,
            &begin_origin,
            &origin_begin) &&
        vitte_main_origin_offset(
            context,
            declaration_span.end,
            true,
            &end_origin,
            &origin_end) &&
        begin_origin->source_id == end_origin->source_id) {
        origin_source_id = begin_origin->source_id;
    } else {
        origin_begin = declaration_span.begin;
        origin_end = declaration_span.end;
    }
    origin_source = origin_source_id != VITTE_SOURCE_INVALID_ID
        ? vitte_source_get(&context->sources, origin_source_id)
        : NULL;
    if (origin_source == NULL &&
        declaration_span.file_id == sema_diagnostic->span.file_id) {
        origin_source_id = context->source_id;
        origin_source = vitte_source_get(
            &context->sources,
            origin_source_id);
    }
    if (origin_source == NULL ||
        !vitte_source_location_from_offset(
            &context->sources,
            origin_source_id,
            origin_begin,
            &start) ||
        !start.valid ||
        !vitte_source_location_from_offset(
            &context->sources,
            origin_source_id,
            origin_end,
            &end) ||
        !end.valid) {
        return false;
    }

    label = sema_diagnostic->code ==
            VITTE_SEMA_DIAGNOSTIC_PRIVATE_SYMBOL
        ? "private declaration is here"
        : sema_diagnostic->code ==
              VITTE_SEMA_DIAGNOSTIC_AMBIGUOUS_NAME
            ? "other imported candidate is declared here"
            : sema_diagnostic->code ==
                  VITTE_SEMA_DIAGNOSTIC_DUPLICATE_IMPORT
                ? "earlier identical import is here"
        : sema_diagnostic->code ==
              VITTE_SEMA_DIAGNOSTIC_DUPLICATE_DECLARATION
            ? "conflicting declaration is here"
        : sema_diagnostic->code ==
              VITTE_SEMA_DIAGNOSTIC_DUPLICATE_BINDING
            ? "previous binding is here"
        : sema_diagnostic->code ==
              VITTE_SEMA_DIAGNOSTIC_INVALID_EXPORT
            ? symbol->kind == VITTE_SYMBOL_KIND_SPACE
                ? "imported module is bound here"
                : "imported declaration originates here"
        : sema_diagnostic->code ==
              VITTE_SEMA_DIAGNOSTIC_SHADOWED_DECLARATION
            ? "earlier shadowed declaration is here"
            : sema_diagnostic->type_context ==
                  VITTE_SEMA_TYPE_CONTEXT_ARGUMENT
                ? "parameter declared here"
                : sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_NOT_A_TYPE
                    ? "value declaration is here"
                    : sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_NOT_A_VALUE
                        ? "type declaration is here"
                : sema_diagnostic->type_context ==
                      VITTE_SEMA_TYPE_CONTEXT_ASSIGNMENT
                    ? "assignment target declared here"
                    : "field declared here";
    memset(location, 0, sizeof(*location));
    location->kind = VITTE_CLI_DIAGNOSTIC_LOCATION_RELATED;
    location->label = label;
    location->span.valid = true;
    location->span.file_id = (uint32_t)origin_source_id;
    location->span.source_id = origin_source_id;
    location->span.source_name = origin_source->path != NULL
        ? origin_source->path
        : origin_source->name != NULL
            ? origin_source->name
            : source_name;
    location->span.start_offset = origin_begin;
    location->span.end_offset = origin_end;
    location->span.start_line = start.line - 1u;
    location->span.start_column = start.column - 1u;
    location->span.end_line = end.line - 1u;
    location->span.end_column = end.column - 1u;
    diagnostic->symbol = symbol->name;
    return true;
}

static bool
vitte_main_sema_mutable_binding_location(
    const vitte_main_driver_context_t *context,
    const vitte_sema_diagnostic_t *sema_diagnostic,
    const char *source_name,
    vitte_cli_diagnostic_t *diagnostic,
    vitte_cli_diagnostic_location_t *location)
{
    const vitte_symbol_t *symbol;
    vitte_source_location_t start;
    vitte_source_location_t end;

    if (context == NULL || sema_diagnostic == NULL ||
        source_name == NULL || diagnostic == NULL || location == NULL ||
        sema_diagnostic->code != VITTE_SEMA_DIAGNOSTIC_NOT_ASSIGNABLE ||
        sema_diagnostic->symbol_id == VITTE_SCOPE_INVALID_SYMBOL_ID) {
        return false;
    }

    symbol = vitte_scope_get_symbol(
        &context->sema.scopes,
        sema_diagnostic->symbol_id);
    if (symbol == NULL || !symbol->span.valid ||
        symbol->span.file_id != sema_diagnostic->span.file_id ||
        symbol->span.begin > symbol->span.end ||
        !vitte_source_location_from_offset(
            &context->sources,
            context->source_id,
            symbol->span.begin,
            &start) ||
        !start.valid ||
        !vitte_source_location_from_offset(
            &context->sources,
            context->source_id,
            symbol->span.end,
            &end) ||
        !end.valid) {
        return false;
    }

    memset(location, 0, sizeof(*location));
    location->kind = VITTE_CLI_DIAGNOSTIC_LOCATION_RELATED;
    location->label = "binding declared here; make it mutable";
    location->span.valid = true;
    location->span.file_id = symbol->span.file_id;
    location->span.source_id = context->source_id;
    location->span.source_name = source_name;
    location->span.start_offset = symbol->span.begin;
    location->span.end_offset = symbol->span.end;
    location->span.start_line = start.line - 1u;
    location->span.start_column = start.column - 1u;
    location->span.end_line = end.line - 1u;
    location->span.end_column = end.column - 1u;
    diagnostic->symbol = symbol->name;
    return true;
}

static const char *
vitte_main_sema_type_context(
    const vitte_main_driver_context_t *context,
    const vitte_sema_diagnostic_t *sema_diagnostic,
    char *buffer,
    size_t capacity)
{
    const vitte_symbol_t *symbol;
    const char *name;
    size_t name_length;
    int written = 0;

    if (context == NULL || sema_diagnostic == NULL ||
        buffer == NULL || capacity == 0u) {
        return NULL;
    }
    buffer[0] = '\0';
    symbol = sema_diagnostic->context_symbol_id !=
            VITTE_SCOPE_INVALID_SYMBOL_ID
        ? vitte_scope_get_symbol(
              &context->sema.scopes,
              sema_diagnostic->context_symbol_id)
        : NULL;
    name = symbol != NULL ? symbol->name : NULL;
    name_length = symbol != NULL ? symbol->name_length : 0u;

    switch (sema_diagnostic->type_context) {
        case VITTE_SEMA_TYPE_CONTEXT_ARGUMENT:
            if (sema_diagnostic->context_index != SIZE_MAX &&
                name != NULL && name_length != 0u) {
                written = snprintf(
                    buffer,
                    capacity,
                    "argument %zu / parameter `%.*s`",
                    sema_diagnostic->context_index + 1u,
                    (int)(name_length > 128u ? 128u : name_length),
                    name);
            } else if (sema_diagnostic->context_index != SIZE_MAX) {
                written = snprintf(
                    buffer,
                    capacity,
                    "argument %zu / parameter",
                    sema_diagnostic->context_index + 1u);
            } else {
                written = snprintf(buffer, capacity, "argument / parameter");
            }
            break;

        case VITTE_SEMA_TYPE_CONTEXT_FIELD:
            if (name != NULL && name_length != 0u) {
                written = snprintf(
                    buffer,
                    capacity,
                    "field `%.*s` / value",
                    (int)(name_length > 128u ? 128u : name_length),
                    name);
            } else {
                written = snprintf(buffer, capacity, "field / value");
            }
            break;

        case VITTE_SEMA_TYPE_CONTEXT_RETURN:
            written = snprintf(
                buffer,
                capacity,
                "return value / declared return type");
            break;

        case VITTE_SEMA_TYPE_CONTEXT_ASSIGNMENT:
            written = snprintf(
                buffer,
                capacity,
                "assignment / mutable binding");
            break;

        case VITTE_SEMA_TYPE_CONTEXT_CONDITION:
            written = snprintf(buffer, capacity, "condition");
            break;

        case VITTE_SEMA_TYPE_CONTEXT_EXPRESSION:
            written = snprintf(buffer, capacity, "expression");
            break;

        case VITTE_SEMA_TYPE_CONTEXT_CAST:
            written = snprintf(buffer, capacity,
                "cast expression / source and target types");
            break;

        case VITTE_SEMA_TYPE_CONTEXT_OPERANDS:
            written = snprintf(buffer, capacity, "operator operands");
            break;

        case VITTE_SEMA_TYPE_CONTEXT_CALL:
            written = snprintf(buffer, capacity, "call target / callable type");
            break;

        case VITTE_SEMA_TYPE_CONTEXT_INDEX:
            written = snprintf(buffer, capacity,
                "indexed value / index expression");
            break;

        case VITTE_SEMA_TYPE_CONTEXT_MEMBER:
            written = snprintf(buffer, capacity,
                "member access / receiver type");
            break;

        case VITTE_SEMA_TYPE_CONTEXT_TYPE:
            written = snprintf(buffer, capacity,
                sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_INCOMPLETE_TYPE ||
                sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_RECURSIVE_TYPE
                    ? "type use / originating type declaration"
                    : "type expression / resolved name");
            break;

        case VITTE_SEMA_TYPE_CONTEXT_VALUE:
            written = snprintf(buffer, capacity,
                "value expression / type name");
            break;

        case VITTE_SEMA_TYPE_CONTEXT_INFERENCE:
            written = snprintf(buffer, capacity,
                "type inference / unresolved expression or binding");
            break;

        case VITTE_SEMA_TYPE_CONTEXT_BOUND:
            written = snprintf(buffer, capacity,
                sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_INVALID_WHERE_PREDICATE
                    ? "where predicate / generic declaration"
                    : "concrete type / required trait bound");
            break;

        case VITTE_SEMA_TYPE_CONTEXT_GENERIC_ARGUMENT:
            written = snprintf(buffer, capacity,
                sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_GENERIC_INFERENCE_CONFLICT
                    ? "conflicting arguments / inferred generic parameter"
                    : "generic arguments / declaration parameters");
            break;

        case VITTE_SEMA_TYPE_CONTEXT_TRAIT_MEMBER:
            written = snprintf(buffer, capacity,
                "trait / implementation candidate");
            break;

        case VITTE_SEMA_TYPE_CONTEXT_NONE:
        case VITTE_SEMA_TYPE_CONTEXT_COUNT:
            switch (sema_diagnostic->code) {
                case VITTE_SEMA_DIAGNOSTIC_INVALID_INDEX_TYPE:
                    written = snprintf(buffer, capacity, "index expression");
                    break;
                case VITTE_SEMA_DIAGNOSTIC_INVALID_UNARY_OPERAND:
                case VITTE_SEMA_DIAGNOSTIC_INVALID_BINARY_OPERANDS:
                    written = snprintf(buffer, capacity, "operator operands");
                    break;
                case VITTE_SEMA_DIAGNOSTIC_TYPE_MISMATCH:
                    written = snprintf(buffer, capacity, "expression");
                    break;
                case VITTE_SEMA_DIAGNOSTIC_INVALID_ARGUMENT_TYPE:
                    written = snprintf(buffer, capacity, "argument / parameter");
                    break;
                case VITTE_SEMA_DIAGNOSTIC_RETURN_TYPE_MISMATCH:
                    written = snprintf(
                        buffer,
                        capacity,
                        "return value / declared return type");
                    break;
                case VITTE_SEMA_DIAGNOSTIC_NOT_ASSIGNABLE:
                    written = snprintf(buffer, capacity, "assignment / mutable binding");
                    break;
                case VITTE_SEMA_DIAGNOSTIC_CONDITION_NOT_BOOL:
                    written = snprintf(buffer, capacity, "condition");
                    break;
                default:
                    written = 0;
                    break;
            }
            break;
    }

    return written > 0 && (size_t)written < capacity ? buffer : NULL;
}

static const char *
vitte_main_sema_type_cause(
    const vitte_sema_diagnostic_t *sema_diagnostic)
{
    if (sema_diagnostic == NULL) {
        return NULL;
    }
    switch (sema_diagnostic->type_context) {
        case VITTE_SEMA_TYPE_CONTEXT_ARGUMENT:
            return "the argument does not satisfy the parameter type";
        case VITTE_SEMA_TYPE_CONTEXT_FIELD:
            return "the field value does not satisfy the declared field type";
        case VITTE_SEMA_TYPE_CONTEXT_RETURN:
            return "the returned value does not satisfy the declared return type";
        case VITTE_SEMA_TYPE_CONTEXT_ASSIGNMENT:
            return sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_NOT_ASSIGNABLE
                ? "the assignment target is immutable or not assignable"
                : "the assigned value does not satisfy the target type";
        case VITTE_SEMA_TYPE_CONTEXT_CONDITION:
            return "the condition must be boolean";
        case VITTE_SEMA_TYPE_CONTEXT_EXPRESSION:
            return "the expression's actual type does not satisfy the expected type";
        case VITTE_SEMA_TYPE_CONTEXT_CAST:
            return "the source type cannot be converted to the requested target type";
        case VITTE_SEMA_TYPE_CONTEXT_OPERANDS:
            return "the operator does not support these operand types";
        case VITTE_SEMA_TYPE_CONTEXT_CALL:
            return "the call target does not have a callable type";
        case VITTE_SEMA_TYPE_CONTEXT_INDEX:
            return sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_INVALID_INDEX_TYPE
                ? "the index must have an integer type"
                : "the indexed value does not support indexing";
        case VITTE_SEMA_TYPE_CONTEXT_MEMBER:
            return "the receiver type does not define this member";
        case VITTE_SEMA_TYPE_CONTEXT_TYPE:
            if (sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_INCOMPLETE_TYPE) {
                return "this operation requires a complete type definition";
            }
            if (sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_RECURSIVE_TYPE) {
                return "the type alias expansion contains a recursive cycle";
            }
            return "the name resolves to a value, which cannot be used as a type";
        case VITTE_SEMA_TYPE_CONTEXT_VALUE:
            return "the name resolves to a type, which cannot be evaluated as a value";
        case VITTE_SEMA_TYPE_CONTEXT_INFERENCE:
            return "available information does not determine a unique type";
        case VITTE_SEMA_TYPE_CONTEXT_BOUND:
            return sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_INVALID_WHERE_PREDICATE
                ? "the predicate must refer to a declared type parameter and a valid trait bound"
                : "the concrete type does not implement the required trait bound";
        case VITTE_SEMA_TYPE_CONTEXT_GENERIC_ARGUMENT:
            return sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_GENERIC_INFERENCE_CONFLICT
                ? "two arguments require incompatible substitutions for one generic parameter"
                : "generic arguments must match the declared parameter count, kinds and order";
        case VITTE_SEMA_TYPE_CONTEXT_TRAIT_MEMBER:
            switch (sema_diagnostic->code) {
                case VITTE_SEMA_DIAGNOSTIC_MISSING_TRAIT_MEMBER:
                    return "the trait declares a required member with no default implementation";
                case VITTE_SEMA_DIAGNOSTIC_AMBIGUOUS_IMPL:
                    return "more than one implementation satisfies this trait relation";
                case VITTE_SEMA_DIAGNOSTIC_CONFLICTING_IMPL:
                    return "this implementation overlaps an existing implementation";
                case VITTE_SEMA_DIAGNOSTIC_AMBIGUOUS_METHOD:
                    return "more than one visible implementation supplies this method";
                case VITTE_SEMA_DIAGNOSTIC_TRAIT_NOT_IMPLEMENTED:
                    return "the concrete type has no implementation of the required trait";
                case VITTE_SEMA_DIAGNOSTIC_INVALID_IMPL_TARGET:
                    return "the impl header does not name a trait and target type in a supported form";
                default:
                    return "the implementation member does not satisfy the trait declaration";
            }
        case VITTE_SEMA_TYPE_CONTEXT_NONE:
        case VITTE_SEMA_TYPE_CONTEXT_COUNT:
            switch (sema_diagnostic->code) {
                case VITTE_SEMA_DIAGNOSTIC_INVALID_ARGUMENT_TYPE:
                    return "the argument does not satisfy the parameter type";
                case VITTE_SEMA_DIAGNOSTIC_RETURN_TYPE_MISMATCH:
                    return "the returned value does not satisfy the declared return type";
                case VITTE_SEMA_DIAGNOSTIC_NOT_ASSIGNABLE:
                    return "the assignment target is immutable or not assignable";
                case VITTE_SEMA_DIAGNOSTIC_CONDITION_NOT_BOOL:
                    return "the condition must be boolean";
                case VITTE_SEMA_DIAGNOSTIC_TYPE_MISMATCH:
                    return "the expected and found types are not assignable";
                default:
                    return NULL;
            }
    }
    return NULL;
}

static bool
vitte_main_sema_explicit_related_location(
    const vitte_main_driver_context_t *context,
    const vitte_sema_diagnostic_t *sema_diagnostic,
    const char *source_name,
    vitte_cli_diagnostic_location_t *location)
{
    const vitte_parser_span_t *related_span;

    if (context == NULL || sema_diagnostic == NULL ||
        source_name == NULL || location == NULL) {
        return false;
    }
    related_span = &sema_diagnostic->related_span;
    if (!related_span->valid ||
        related_span->begin > related_span->end ||
        related_span->end > context->file.length) {
        return false;
    }

    memset(location, 0, sizeof(*location));
    location->kind = VITTE_CLI_DIAGNOSTIC_LOCATION_DECLARATION;
    location->span.valid = true;
    location->span.file_id = related_span->file_id;
    location->span.source_id = context->source_id;
    location->span.source_name = source_name;
    location->span.start_offset = related_span->begin;
    location->span.end_offset = related_span->end;
    location->span.start_line = related_span->line != 0u
        ? related_span->line - 1u : 0u;
    location->span.start_column = related_span->column != 0u
        ? related_span->column - 1u : 0u;
    location->span.end_line = location->span.start_line;
    location->span.end_column = location->span.start_column;
    location->label = sema_diagnostic->related_label[0] != '\0'
        ? sema_diagnostic->related_label
        : "related declaration is here";
    vitte_main_diagnostic_map_span(context, &location->span);
    return location->span.valid;
}

static bool
vitte_main_render_sema_diagnostics(
    const vitte_main_driver_context_t *context,
    bool *rendered)
{
    vitte_cli_diagnostic_t *diagnostics;
    char (*type_names)[2][VITTE_MAIN_SEMA_TYPE_NAME_CAPACITY];
    char (*type_contexts)[VITTE_CLI_DIAGNOSTIC_CONTEXT_CAPACITY];
    vitte_cli_diagnostic_location_t *related_locations;
    vitte_cli_diagnostic_cause_t *causes;
    const vitte_source_entry_t *source;
    const char *source_name;
    size_t diagnostic_count;
    size_t index;

    if (context == NULL ||
        rendered == NULL) {
        return false;
    }

    *rendered = false;
    diagnostic_count = vitte_sema_diagnostic_count(&context->sema);
    if (diagnostic_count == 0u) {
        return true;
    }

    diagnostics = (vitte_cli_diagnostic_t *)calloc(
        diagnostic_count,
        sizeof(*diagnostics));
    type_names = (char (*)[2][VITTE_MAIN_SEMA_TYPE_NAME_CAPACITY])calloc(
        diagnostic_count,
        sizeof(*type_names));
    type_contexts = (char (*)[VITTE_CLI_DIAGNOSTIC_CONTEXT_CAPACITY])calloc(
        diagnostic_count,
        sizeof(*type_contexts));
    related_locations = (vitte_cli_diagnostic_location_t *)calloc(
        diagnostic_count,
        sizeof(*related_locations));
    causes = (vitte_cli_diagnostic_cause_t *)calloc(
        diagnostic_count,
        sizeof(*causes));
    if (diagnostics == NULL || type_names == NULL ||
        type_contexts == NULL || related_locations == NULL || causes == NULL) {
        free(diagnostics);
        free(type_names);
        free(type_contexts);
        free(related_locations);
        free(causes);
        return false;
    }

    source = vitte_source_get(
        &context->sources,
        context->source_id);
    source_name = source != NULL
        ? source->name
        : context->options->input_path;

    for (index = 0u; index < diagnostic_count; ++index) {
        const vitte_sema_diagnostic_t *sema_diagnostic;
        vitte_cli_diagnostic_t *diagnostic;
        const char *expected_type;
        const char *found_type;
        int details_length;

        sema_diagnostic = vitte_sema_diagnostic_at(
            &context->sema,
            index);
        if (sema_diagnostic == NULL) {
            free(diagnostics);
            free(type_names);
            free(type_contexts);
            free(related_locations);
            free(causes);
            return false;
        }

        diagnostic = &diagnostics[index];
        if (!vitte_main_make_cli_diagnostic(
                context,
                diagnostic,
                vitte_main_sema_diagnostic_severity(
                    sema_diagnostic->kind),
                vitte_main_sema_diagnostic_code(
                    sema_diagnostic->code),
                NULL,
                NULL,
                &sema_diagnostic->span,
                NULL,
                "semantic analysis")) {
            free(diagnostics);
            free(type_names);
            free(type_contexts);
            free(related_locations);
            free(causes);
            return false;
        }

        expected_type = vitte_main_sema_format_type(
                &context->sema,
                sema_diagnostic->expected_type,
                type_names[index][0],
                sizeof(type_names[index][0]))
            ? type_names[index][0]
            : NULL;
        found_type = vitte_main_sema_format_type(
                &context->sema,
                sema_diagnostic->found_type,
                type_names[index][1],
                sizeof(type_names[index][1]))
            ? type_names[index][1]
            : NULL;
        diagnostic->expected_type = expected_type;
        diagnostic->actual_type = found_type;
        diagnostic->expected = expected_type;
        diagnostic->found = found_type;
        diagnostic->context = vitte_main_sema_type_context(
            context,
            sema_diagnostic,
            type_contexts[index],
            sizeof(type_contexts[index]));
        diagnostic->probable_cause = vitte_main_sema_type_cause(
            sema_diagnostic);
        if (sema_diagnostic->context_symbol_id !=
            VITTE_SCOPE_INVALID_SYMBOL_ID) {
            const vitte_symbol_t *context_symbol;

            context_symbol = vitte_scope_get_symbol(
                &context->sema.scopes,
                sema_diagnostic->context_symbol_id);
            if (context_symbol != NULL) {
                diagnostic->symbol = context_symbol->name;
            }
        } else if ((sema_diagnostic->code ==
                        VITTE_SEMA_DIAGNOSTIC_AMBIGUOUS_NAME ||
                    sema_diagnostic->code ==
                        VITTE_SEMA_DIAGNOSTIC_DUPLICATE_IMPORT) &&
                   sema_diagnostic->symbol_id !=
                        VITTE_SCOPE_INVALID_SYMBOL_ID) {
            const vitte_symbol_t *imported_symbol = vitte_scope_get_symbol(
                &context->sema.scopes,
                sema_diagnostic->symbol_id);
            if (imported_symbol != NULL) {
                diagnostic->symbol = imported_symbol->name;
            }
        }
        if (sema_diagnostic->contract_span.valid &&
            sema_diagnostic->contract_span.begin <= sema_diagnostic->contract_span.end &&
            sema_diagnostic->contract_span.end <= context->file.length) {
            const vitte_symbol_t *procedure_symbol = vitte_scope_get_symbol(
                &context->sema.scopes, sema_diagnostic->contract_symbol_id);
            vitte_cli_diagnostic_t clause;
            size_t expression_length = sema_diagnostic->contract_span.end -
                sema_diagnostic->contract_span.begin;
            bool ensures = (sema_diagnostic->contract_flags &
                VITTE_AST_FLAG_CONTRACT_ENSURES) != 0u;
            if (expression_length >= sizeof(type_contexts[index])) {
                expression_length = sizeof(type_contexts[index]) - 1u;
            }
            memcpy(type_contexts[index],
                context->file.data + sema_diagnostic->contract_span.begin,
                expression_length);
            type_contexts[index][expression_length] = '\0';
            diagnostic->context = ensures ? "ensures clause" : "requires clause";
            diagnostic->procedure = procedure_symbol != NULL ? procedure_symbol->name : NULL;
            diagnostic->has_contract = true;
            diagnostic->contract.kind = ensures
                ? VITTE_CLI_DIAGNOSTIC_CONTRACT_ENSURES
                : VITTE_CLI_DIAGNOSTIC_CONTRACT_REQUIRES;
            diagnostic->contract.expression = type_contexts[index];
            diagnostic->contract.procedure = diagnostic->procedure;
            diagnostic->contract.reason =
                sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_CONTRACT_PRECONDITION
                    ? "the known call arguments do not satisfy the precondition"
                    : sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_CONTRACT_RESULT
                    ? "result is available only in an ensures clause of a procedure returning a value"
                    : sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_CONTRACT_CONTRADICTION ||
                      sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_CONTRACT_POSTCONDITION
                        ? "the contract condition evaluates to false for the known values"
                        : "the contract expression must be valid and have type bool";
            diagnostic->probable_cause = diagnostic->contract.reason;
            if (vitte_main_make_cli_diagnostic(
                    context, &clause, diagnostic->severity, diagnostic->code,
                    NULL, NULL, &sema_diagnostic->contract_span, NULL, NULL) &&
                clause.has_span) {
                diagnostic->contract.has_contract_span = true;
                diagnostic->contract.contract_span.valid = true;
                diagnostic->contract.contract_span.source_name = clause.source_name;
                diagnostic->contract.contract_span.source_id = clause.source_id;
                diagnostic->contract.contract_span.file_id = clause.file_id;
                diagnostic->contract.contract_span.start_offset = clause.start_offset;
                diagnostic->contract.contract_span.end_offset = clause.end_offset;
                diagnostic->contract.contract_span.start_line = clause.start_line;
                diagnostic->contract.contract_span.start_column = clause.start_column;
                diagnostic->contract.contract_span.end_line = clause.end_line;
                diagnostic->contract.contract_span.end_column = clause.end_column;
            }
        }
        if (sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_INDEX_OUT_OF_BOUNDS)
            diagnostic->message = "array index out of bounds";
        diagnostic->help = sema_diagnostic->help;
        details_length = 0;
        if (sema_diagnostic->code ==
                VITTE_SEMA_DIAGNOSTIC_INVALID_UNARY_OPERAND &&
            found_type != NULL) {
            details_length = snprintf(
                diagnostic->details_storage,
                sizeof(diagnostic->details_storage),
                "operator does not support operand type %s",
                found_type);
        } else if (sema_diagnostic->code ==
                       VITTE_SEMA_DIAGNOSTIC_INVALID_BINARY_OPERANDS &&
                   expected_type != NULL &&
                   found_type != NULL) {
            details_length = snprintf(
                diagnostic->details_storage,
                sizeof(diagnostic->details_storage),
                "operator does not support operand types %s and %s",
                expected_type,
                found_type);
        } else if (expected_type != NULL &&
                   found_type != NULL) {
            details_length = snprintf(
                diagnostic->details_storage,
                sizeof(diagnostic->details_storage),
                "expected %s, found %s",
                expected_type,
                found_type);
        } else if (expected_type != NULL) {
            details_length = snprintf(
                diagnostic->details_storage,
                sizeof(diagnostic->details_storage),
                "expected %s",
                expected_type);
        } else if (found_type != NULL) {
            details_length = snprintf(
                diagnostic->details_storage,
                sizeof(diagnostic->details_storage),
                "found %s",
                found_type);
        }
        if (details_length < 0 ||
            (size_t)details_length >=
                sizeof(diagnostic->details_storage)) {
            free(diagnostics);
            free(type_names);
            free(type_contexts);
            free(related_locations);
            free(causes);
            return false;
        }

        diagnostic->details =
            diagnostic->details_storage[0] != '\0'
                ? diagnostic->details_storage
                : NULL;
        if (vitte_main_sema_explicit_related_location(
                context,
                sema_diagnostic,
                source_name,
                &related_locations[index])) {
            diagnostic->related = &related_locations[index];
            diagnostic->related_count = 1u;
        } else if (vitte_main_sema_mutable_binding_location(
                context,
                sema_diagnostic,
                source_name,
                diagnostic,
                &related_locations[index])) {
            diagnostic->related = &related_locations[index];
            diagnostic->related_count = 1u;
        } else if (vitte_main_sema_symbol_origin_location(
                       context,
                       sema_diagnostic,
                       source_name,
                       diagnostic,
                       &related_locations[index])) {
            diagnostic->related = &related_locations[index];
            diagnostic->related_count = 1u;
        }
        if (diagnostic->has_contract && diagnostic->contract.has_contract_span &&
            (sema_diagnostic->span.begin != sema_diagnostic->contract_span.begin ||
             sema_diagnostic->span.end != sema_diagnostic->contract_span.end)) {
            related_locations[index].kind = VITTE_CLI_DIAGNOSTIC_LOCATION_DECLARATION;
            related_locations[index].span = diagnostic->contract.contract_span;
            related_locations[index].label = "contract declared here";
            diagnostic->related = &related_locations[index];
            diagnostic->related_count = 1u;
        }
        if (diagnostic->has_contract) {
            causes[index].kind = VITTE_CLI_DIAGNOSTIC_CAUSE_CONTRACT;
            causes[index].message = diagnostic->contract.reason != NULL
                ? diagnostic->contract.reason
                : "the contract condition is invalid or false";
            causes[index].confidence = VITTE_CLI_DIAGNOSTIC_CONFIDENCE_CERTAIN;
            if (diagnostic->contract.has_contract_span) {
                causes[index].has_span = true;
                causes[index].span = diagnostic->contract.contract_span;
                vitte_main_diagnostic_map_span(context, &causes[index].span);
            } else if (diagnostic->has_span) {
                causes[index].has_span = true;
                causes[index].span.valid = true;
                causes[index].span.file_id = diagnostic->file_id;
                causes[index].span.source_id = diagnostic->source_id;
                causes[index].span.source_name = diagnostic->source_name;
                causes[index].span.start_offset = diagnostic->start_offset;
                causes[index].span.end_offset = diagnostic->end_offset;
                causes[index].span.start_line = diagnostic->start_line;
                causes[index].span.start_column = diagnostic->start_column;
                causes[index].span.end_line = diagnostic->end_line;
                causes[index].span.end_column = diagnostic->end_column;
            }
            diagnostic->causes = &causes[index];
            diagnostic->cause_count = 1u;
        } else if (sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_CONSTANT_NOT_CONSTANT ||
                   sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_CONSTANT_OVERFLOW ||
                   sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_CONSTANT_DIVIDE_BY_ZERO ||
                   sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_CONSTANT_REMAINDER_BY_ZERO ||
                   sema_diagnostic->code == VITTE_SEMA_DIAGNOSTIC_CONSTANT_CYCLE) {
            causes[index].kind = VITTE_CLI_DIAGNOSTIC_CAUSE_CONSTANT_EVAL;
            causes[index].message = sema_diagnostic->help[0] != '\0'
                ? sema_diagnostic->help
                : "the initializer cannot produce a compile-time value";
            causes[index].confidence = VITTE_CLI_DIAGNOSTIC_CONFIDENCE_CERTAIN;
            if (diagnostic->has_span) {
                causes[index].has_span = true;
                causes[index].span.valid = true;
                causes[index].span.file_id = diagnostic->file_id;
                causes[index].span.source_id = diagnostic->source_id;
                causes[index].span.source_name = diagnostic->source_name;
                causes[index].span.start_offset = diagnostic->start_offset;
                causes[index].span.end_offset = diagnostic->end_offset;
                causes[index].span.start_line = diagnostic->start_line;
                causes[index].span.start_column = diagnostic->start_column;
                causes[index].span.end_line = diagnostic->end_line;
                causes[index].span.end_column = diagnostic->end_column;
            }
            diagnostic->causes = &causes[index];
            diagnostic->cause_count = 1u;
        }
        if (diagnostic->related_count != 0u) {
            size_t related_index;

            for (related_index = 0u;
                 related_index < diagnostic->related_count;
                 ++related_index) {
                vitte_main_diagnostic_map_span(
                    context,
                    &related_locations[index].span);
            }
        }
    }

    *rendered = vitte_diagnostic_render_cli_batch(
        stderr,
        &context->sources,
        context->source_id,
        source_name,
        diagnostics,
        diagnostic_count,
        context->options->color,
        vitte_main_cli_diagnostic_format(
            context->options->diagnostic_format));
    free(diagnostics);
    free(type_names);
    free(type_contexts);
    free(related_locations);
    free(causes);
    return *rendered;
}

static const char *
vitte_main_sema_failure_code(vitte_sema_error_t error)
{
    switch (error) {
        case VITTE_SEMA_ERROR_OUT_OF_MEMORY:
        case VITTE_SEMA_ERROR_OVERFLOW:
        case VITTE_SEMA_ERROR_TYPE_LIMIT:
        case VITTE_SEMA_ERROR_DIAGNOSTIC_LIMIT:
        case VITTE_SEMA_ERROR_RECURSION_LIMIT:
            return "VITTE_INFRA_E_RESOURCE";
        case VITTE_SEMA_ERROR_INVALID_AST:
        case VITTE_SEMA_ERROR_INVALID_SCOPE:
        case VITTE_SEMA_ERROR_INVALID_TYPE:
        case VITTE_SEMA_ERROR_VALIDATION:
        case VITTE_SEMA_ERROR_CORRUPTION:
            return "VITTE_INFRA_E_INVARIANT";
        case VITTE_SEMA_ERROR_NONE:
        case VITTE_SEMA_ERROR_INVALID_ARGUMENT:
        case VITTE_SEMA_ERROR_INVALID_CONTEXT:
        case VITTE_SEMA_ERROR_INVALID_STATE:
        case VITTE_SEMA_ERROR_INTERNAL:
        case VITTE_SEMA_ERROR_COUNT:
        default:
            return "VITTE_INFRA_E_INTERNAL";
    }
}

static const char *
vitte_main_sema_failure_message(vitte_sema_error_t error, bool initialization)
{
    const char *code;

    if (initialization && error == VITTE_SEMA_ERROR_NONE) {
        return "semantic analysis could not acquire required resources";
    }
    code = vitte_main_sema_failure_code(error);
    if (strcmp(code, "VITTE_INFRA_E_RESOURCE") == 0) {
        return "compiler resource failure during semantic analysis";
    }
    if (strcmp(code, "VITTE_INFRA_E_INVARIANT") == 0) {
        return "compiler invariant failed during semantic analysis";
    }
    return "internal compiler failure during semantic analysis";
}

bool
vitte_main_driver_analyze(
    vitte_driver_t *driver,
    vitte_driver_phase_t phase,
    void *user_data)
{
    vitte_main_driver_context_t *context;
    bool rendered;

    (void)driver;
    (void)phase;

    context = (vitte_main_driver_context_t *)user_data;

    if (context == NULL) {
        return false;
    }
    if (context->native_frontend_completed) {
        return true;
    }
    if (!context->parser_initialized) {
        vitte_main_error("semantic analysis requires a parsed source file");
        return false;
    }

    vitte_main_verbose(context->options, "analyzing semantics");

    if (!vitte_sema_init(&context->sema, &context->parser)) {
        (void)vitte_main_render_diagnostic(
            context,
            VITTE_CLI_DIAGNOSTIC_FATAL,
            "VITTE_INFRA_E_RESOURCE",
            "semantic analysis could not acquire required resources",
            NULL,
            NULL,
            NULL,
            "semantic analysis");
        return false;
    }

    context->sema_initialized = true;

    if (!vitte_sema_run(&context->sema)) {
        if (!vitte_main_render_sema_diagnostics(
                context,
                &rendered)) {
            vitte_main_error(
                "failed to render semantic diagnostics");
            return false;
        }

        if (!rendered &&
            !vitte_main_render_diagnostic(
                context,
                VITTE_CLI_DIAGNOSTIC_FATAL,
                vitte_main_sema_failure_code(
                    vitte_sema_last_error(&context->sema)),
                vitte_main_sema_failure_message(
                    vitte_sema_last_error(&context->sema),
                    false),
                vitte_sema_error_name(
                    vitte_sema_last_error(&context->sema)),
                NULL,
                NULL,
                "semantic analysis")) {
            vitte_main_error(
                "failed to render semantic analysis failure");
        }
        return false;
    }

    if (!vitte_main_render_sema_diagnostics(
            context,
            &rendered)) {
        vitte_main_error(
            "failed to render semantic diagnostics");
        return false;
    }

    if (vitte_sema_has_errors(&context->sema)) {
        return false;
    }

    return true;
}

bool
vitte_main_driver_compile_ast(
    vitte_driver_t *driver,
    vitte_driver_phase_t phase,
    void *user_data)
{
    vitte_main_driver_context_t *context;
    vitte_c17_compile_result_t compile_result;

    (void)driver;
    (void)phase;

    context = (vitte_main_driver_context_t *)user_data;
    if (context == NULL ||
        context->options == NULL ||
        (!context->native_frontend_completed &&
         !context->parser_initialized)) {
        vitte_main_error(
            "C17 generation requires a successfully parsed input");
        return false;
    }

    /* The CLI currently records --target but the C17 driver only emits for
     * the host toolchain default. Refuse to silently build for a different
     * target, and state the available host values in a stable diagnostic. */
    if (context->options->config_initialized) {
        const vitte_config_resolved_t *resolved =
            vitte_config_resolved(&context->options->config);
        if (resolved != NULL && resolved->target != NULL &&
            resolved->target[0] != '\0') {
            char details[1024];
            const char *architecture = "unknown";
            const char *platform = "unknown";
            const char *compiler = getenv("CC");
            unsigned pointer_bits = (unsigned)(sizeof(void *) * 8u);

#if defined(__x86_64__) || defined(_M_X64)
            architecture = "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
            architecture = "aarch64";
#elif defined(__i386__) || defined(_M_IX86)
            architecture = "x86";
#elif defined(__arm__) || defined(_M_ARM)
            architecture = "arm";
#elif defined(__riscv)
            architecture = "riscv";
#endif

#if defined(_WIN32)
            platform = "windows";
#elif defined(__APPLE__)
            platform = "darwin";
#elif defined(__linux__)
            platform = "linux";
#elif defined(__FreeBSD__)
            platform = "freebsd";
#endif

            if (compiler == NULL || compiler[0] == '\0') {
                compiler = "cc (default from PATH)";
            }
            (void)snprintf(
                details,
                sizeof(details),
                "requested target: `%s`; available built-in host properties: architecture `%s`, platform `%s`, pointer width %u bits; selected compiler: `%s`. The C17 driver does not forward `--target`, and it does not probe the compiler's exact target triple or ABI. Remove `--target` to use host defaults. Cross-target output requires a backend path that selects or forwards the requested triple; inspect the selected compiler's default target before configuring it. `CC` must be one executable path.",
                resolved->target,
                architecture,
                platform,
                pointer_bits,
                compiler);
            (void)vitte_main_render_diagnostic(
                context,
                VITTE_CLI_DIAGNOSTIC_ERROR,
                "VITTE_TARGET_E_UNSUPPORTED",
                "unsupported target",
                details,
                NULL,
                NULL,
                "target validation");
            return false;
        }
    }

    {
        vitte_c17_compile_options_t compile_options;
        vitte_c17_compile_failure_t compile_failure;

        memset(&compile_options, 0, sizeof(compile_options));
        memset(&compile_failure, 0, sizeof(compile_failure));
        compile_options.debug_info = context->options->debug_info;
        compile_options.debug_runtime = context->options->debug_probes;
        compile_options.emit_line_directives =
            context->options->emit_line_directives ||
            context->options->debug_info;
        compile_options.source_path = context->options->input_path;
        compile_options.failure = &compile_failure;

#ifdef VITTE_NATIVE_FRONTEND_LINKED
        if (context->native_frontend_completed) {
            if (!context->options->emit_c) {
                uint32_t failed_stage = 0u;
                uint32_t host_status;
                const char *code;
                const char *message;
                const char *details;
                const char *diagnostic_context;
                char external_details[1024];
                const char *compiler = getenv("CC");

                host_status =
                    compiler_driver_pipeline_vitte_native_driver_run_host_toolchain(
                    context->native_output,
                    context->native_output_length,
                    (uint8_t *)context->options->output_path,
                    context->options->emit_object,
                    context->options->command == VITTE_MAIN_COMMAND_RUN,
                    context->options->debug_info,
                    context->options->debug_probes,
                    context->options->run_argc,
                    (uint8_t **)(void *)context->options->run_argv,
                    &failed_stage);

                if (context->native_output != NULL) {
                    vitte_native_frontend_release_output(
                        context->native_output);
                }
                context->native_output = NULL;
                context->native_output_length = 0u;
                context->native_frontend_completed = false;

                if (host_status == 0u) {
                    return true;
                }

                if (compiler == NULL || compiler[0] == '\0') {
                    compiler = "cc (default from PATH)";
                }

                if (failed_stage == 2u) {
                    code = "VITTE_BACKEND_E_EXTERNAL_COMPILER";
                    message = "generated C source compilation failed";
                    (void)snprintf(
                        external_details,
                        sizeof(external_details),
                        "phase: C compilation; command: `%s -std=c17 [-g ...] -x c -c <temporary-source> -o <temporary-object>`. %s Its stdout/stderr was forwarded above. Configure `CC` as one executable path and check its C17 support, PATH and sysroot. If its output points into generated C, compare with `--emit-c` before treating this as a Vitte source error.",
                        compiler,
                        host_status == 6u
                            ? "The configured compiler could not be started."
                            : "The compiler returned a failure status.");
                    details = external_details;
                    diagnostic_context = "C compilation";
                } else if (failed_stage == 3u) {
                    code = "VITTE_BACKEND_E_LINK";
                    message = "generated C object could not be linked";
                    (void)snprintf(
                        external_details,
                        sizeof(external_details),
                        "phase: linking; command: `%s <temporary-object> -o <output>`. %s Its stdout/stderr was forwarded above. Check the selected compiler driver, linker, SDK/sysroot and required system libraries.",
                        compiler,
                        host_status == 6u
                            ? "The configured linker driver could not be started."
                            : "The linker driver returned a failure status.");
                    details = external_details;
                    diagnostic_context = "linking";
                } else if (failed_stage == 4u) {
                    code = "VITTE_BACKEND_E_EXECUTION";
                    message = "compiled program execution failed";
                    details = host_status == 6u
                        ? "C compilation and linking succeeded, but the driver could not start the generated program"
                        : "C compilation and linking succeeded, but the generated program returned a failure status";
                    diagnostic_context = "program execution";
                } else if (host_status == 4u) {
                    code = "VITTE_BACKEND_E_UNSUPPORTED";
                    message = "C17 host toolchain is unsupported on this platform";
                    details = "the native driver cannot create temporary files or launch host tools on this platform";
                    diagnostic_context = "host toolchain";
                } else if (host_status == 1u) {
                    code = "VITTE_INFRA_E_PHASE";
                    message = "invalid native host toolchain request";
                    details = "the driver received an invalid output path or run configuration";
                    diagnostic_context = "host toolchain";
                } else if (host_status == 5u) {
                    code = "VITTE_INFRA_E_RESOURCE";
                    message = "native host toolchain ran out of memory";
                    details = "the driver could not allocate its temporary compilation state";
                    diagnostic_context = "host toolchain";
                } else {
                    code = "VITTE_BACKEND_E_OUTPUT";
                    message = "native host toolchain could not prepare its files";
                    details = "creating, writing, or closing a temporary C17, object, or executable file failed";
                    diagnostic_context = "host toolchain preparation";
                }

                if (!vitte_main_render_diagnostic(
                        context,
                        VITTE_CLI_DIAGNOSTIC_ERROR,
                        code,
                        message,
                        details,
                        NULL,
                        NULL,
                        diagnostic_context)) {
                    vitte_main_error(
                        "failed to report native host toolchain failure");
                }
                return false;
            }

            /* Source-only output does not invoke host compiler tools. */
            compile_result = vitte_c17_compile_source_result_with_options(
                (const char *)context->native_output,
                context->native_output_length,
                context->options->output_path,
                true,
                false,
                false,
                0,
                NULL,
                &compile_options);
            if (context->native_output != NULL) {
                vitte_native_frontend_release_output(context->native_output);
            }
            context->native_output = NULL;
            context->native_output_length = 0u;
            context->native_frontend_completed = false;
        } else
#endif
        if (context->options->emit_object) {
            compile_result =
                vitte_c17_compile_ast_object_result_with_options(
                    &context->parser,
                    context->options->output_path,
                    &compile_options);
        } else {
            compile_result = vitte_c17_compile_ast_result_with_options(
                &context->parser,
                context->options->output_path,
                context->options->emit_c,
                context->options->command == VITTE_MAIN_COMMAND_RUN,
                context->options->run_argc,
                context->options->run_argv,
                &compile_options);
        }
        if (compile_result != VITTE_C17_COMPILE_SUCCESS) {
            const char *code;
            const char *message;
            const char *details;
            const vitte_parser_span_t *diagnostic_span = NULL;

            switch (compile_result) {
                case VITTE_C17_COMPILE_INVALID_ARGUMENT:
                    code = "VITTE_BACKEND_E_CONFIG";
                    message = "invalid C17 backend configuration";
                    details = "the backend received an invalid or incompatible compilation configuration";
                    break;
                case VITTE_C17_COMPILE_OUTPUT_FAILED:
                    code = "VITTE_BACKEND_E_OUTPUT";
                    message = "C17 output could not be written";
                    details = "creating, writing, or closing a C17 output file failed";
                    break;
                case VITTE_C17_COMPILE_EMIT_FAILED:
                    code = "VITTE_BACKEND_E_CODEGEN";
                    message = "C17 code generation failed";
                    details = "the AST emitter could not produce a complete C17 translation unit";
                    break;
                case VITTE_C17_COMPILE_C_COMPILATION_FAILED:
                    code = "VITTE_BACKEND_E_EXTERNAL_COMPILER";
                    message = "generated C source compilation failed";
                    details = "phase: C compilation; the bootstrap backend invokes `cc -std=c17 -x c ...` from PATH. Its stdout/stderr was forwarded above. Check that `cc` is installed and supports C17; generated-C errors can indicate a backend defect, while missing headers or target configuration belong to the host toolchain.";
                    break;
                case VITTE_C17_COMPILE_LINK_FAILED:
                    code = "VITTE_BACKEND_E_LINK";
                    message = "generated C object could not be linked";
                    details = "phase: linking; the bootstrap backend invokes `cc <object> -o <output>` from PATH. Its stdout/stderr was forwarded above. Check the host linker, SDK/sysroot and required system libraries.";
                    break;
                case VITTE_C17_COMPILE_RUNTIME_FAILED:
                    code = "VITTE_BACKEND_E_EXECUTION";
                    message = "compiled program exited unsuccessfully";
                    details = "C17 compilation and linking succeeded, but program execution returned a failure status";
                    break;
                case VITTE_C17_COMPILE_UNSUPPORTED:
                    code = "VITTE_BACKEND_E_UNSUPPORTED";
                    message = "C17 compilation is unsupported on this host";
                    details = "the bootstrap C17 compile driver does not support this platform";
                    break;
                case VITTE_C17_COMPILE_UNSUPPORTED_FEATURE:
                    code = "VITTE_BACKEND_E_UNSUPPORTED";
                    message = "C17 backend does not support this source construct";
                    details = "the selected construct has no C17 lowering; no output was emitted";
                    break;
                case VITTE_C17_COMPILE_SUCCESS:
                default:
                    code = "VITTE_BACKEND_E_CODEGEN";
                    message = "C17 compilation failed";
                    details = "the backend returned an unclassified failure";
                    break;
            }
            if (compile_result == VITTE_C17_COMPILE_EMIT_FAILED) {
                if (compile_failure.message != NULL) {
                    if (strcmp(compile_failure.message,
                               "macros must be expanded before C17 emission") == 0) {
                        code = "VITTE_BACKEND_E_UNSUPPORTED";
                        message = "macro declarations cannot be emitted by the C17 backend";
                        details = "expand or remove the macro declaration before compiling with the C17 backend";
                    } else if (strstr(compile_failure.message, "unsupported") != NULL ||
                        strstr(compile_failure.message, "not supported") != NULL ||
                        strstr(compile_failure.message, "cannot be emitted") != NULL ||
                        strstr(compile_failure.message, "must be expanded") != NULL ||
                        strstr(compile_failure.message, "must be monomorphized") != NULL ||
                        strstr(compile_failure.message, "not implemented") != NULL ||
                        strstr(compile_failure.message, "cannot be lowered") != NULL) {
                        code = "VITTE_BACKEND_E_UNSUPPORTED";
                        message = "C17 backend does not support this Vitte construct";
                        details = compile_failure.details != NULL
                            ? compile_failure.details
                            : compile_failure.message;
                    } else {
                        message = "C17 code generation failed for this Vitte construct";
                        details = compile_failure.details != NULL
                            ? compile_failure.details
                            : compile_failure.message;
                    }
                } else if (compile_failure.span.valid) {
                    message = "C17 code generation failed for this Vitte construct";
                    details = "the backend failed while lowering the underlined source construct";
                }
                if (compile_failure.span.valid) {
                    diagnostic_span = &compile_failure.span;
                }
            }

            if (!vitte_main_render_diagnostic(
                    context,
                    VITTE_CLI_DIAGNOSTIC_ERROR,
                    code,
                    message,
                    details,
                    diagnostic_span,
                    NULL,
                    "C17 backend")) {
                vitte_main_error(
                    "failed to report the C17 backend failure");
            }
            return false;
        }
    }

    return true;
}

#ifdef VITTE_NATIVE_FRONTEND_LINKED
static bool
vitte_main_emit_native_ir(
    vitte_main_driver_context_t *context)
{
    FILE *output = stdout;
    bool close_output = false;
    bool success;

    if (context == NULL || context->options == NULL ||
        !context->options->emit_ir || context->native_output == NULL) {
        return false;
    }
    if (context->options->output_path != NULL) {
        output = fopen(context->options->output_path, "wb");
        if (output == NULL) {
            vitte_main_report_path_error(
                "VITTE_BACKEND_E_OUTPUT",
                "native IR output could not be opened",
                context->options->output_path,
                "creating the requested IR output file failed");
            return false;
        }
        close_output = true;
    }

    success = fwrite(
        context->native_output,
        1u,
        context->native_output_length,
        output) == context->native_output_length;
    if (success && !close_output) {
        success = fflush(output) == 0;
    }
    if (close_output && fclose(output) != 0) {
        success = false;
    }
    if (!success) {
        vitte_main_report_path_error(
            "VITTE_BACKEND_E_OUTPUT",
            "native IR output could not be written",
            context->options->output_path,
            "writing or closing the IR output failed");
    }
    return success;
}
#endif

static const char *
vitte_main_hir_diagnostic_code(
    vitte_hir_error_t error,
    bool validation)
{
    if (error == VITTE_HIR_ERROR_UNSUPPORTED) {
        return "VITTE_HIR_E_UNSUPPORTED";
    }
    if (error == VITTE_HIR_ERROR_OUT_OF_MEMORY ||
        error == VITTE_HIR_ERROR_OVERFLOW ||
        error == VITTE_HIR_ERROR_TOO_MANY_NODES ||
        error == VITTE_HIR_ERROR_TOO_MANY_SYMBOLS ||
        error == VITTE_HIR_ERROR_TOO_MANY_TYPES ||
        error == VITTE_HIR_ERROR_STRING_LIMIT ||
        error == VITTE_HIR_ERROR_RECURSION_LIMIT) {
        return "VITTE_INFRA_E_RESOURCE";
    }
    if (error == VITTE_HIR_ERROR_INTERNAL ||
        error == VITTE_HIR_ERROR_INVALID_CONTEXT ||
        error == VITTE_HIR_ERROR_INVALID_STATE ||
        error == VITTE_HIR_ERROR_INVALID_ARGUMENT) {
        return "VITTE_INFRA_E_INTERNAL";
    }
    if (validation || error == VITTE_HIR_ERROR_VALIDATION ||
        error == VITTE_HIR_ERROR_CORRUPTION) {
        return "VITTE_HIR_E_INVALID";
    }
    return "VITTE_HIR_E_LOWERING";
}

static const char *
vitte_main_hir_diagnostic_message(
    vitte_hir_error_t error,
    bool validation)
{
    const char *code = vitte_main_hir_diagnostic_code(error, validation);

    if (strcmp(code, "VITTE_HIR_E_UNSUPPORTED") == 0) {
        return "source construct is not supported by HIR lowering";
    }
    if (strcmp(code, "VITTE_INFRA_E_RESOURCE") == 0) {
        return "compiler resource failure during HIR lowering";
    }
    if (strcmp(code, "VITTE_INFRA_E_INTERNAL") == 0) {
        return "internal compiler failure during HIR lowering";
    }
    if (strcmp(code, "VITTE_HIR_E_INVALID") == 0) {
        return "compiler produced invalid HIR";
    }
    return "HIR lowering failed";
}

static vitte_cli_diagnostic_severity_t
vitte_main_hir_diagnostic_severity(
    vitte_hir_error_t error,
    bool validation)
{
    const char *code = vitte_main_hir_diagnostic_code(error, validation);
    return strcmp(code, "VITTE_HIR_E_UNSUPPORTED") == 0 ||
           strcmp(code, "VITTE_HIR_E_LOWERING") == 0
        ? VITTE_CLI_DIAGNOSTIC_ERROR
        : VITTE_CLI_DIAGNOSTIC_FATAL;
}

static const char *
vitte_main_ir_diagnostic_code(
    vitte_ir_error_t error,
    bool verification)
{
    if (error == VITTE_IR_ERROR_UNSUPPORTED) {
        return "VITTE_IR_E_UNSUPPORTED";
    }
    if (error == VITTE_IR_ERROR_OUT_OF_MEMORY ||
        error == VITTE_IR_ERROR_OVERFLOW ||
        error == VITTE_IR_ERROR_LIMIT) {
        return "VITTE_INFRA_E_RESOURCE";
    }
    if (error == VITTE_IR_ERROR_INTERNAL ||
        error == VITTE_IR_ERROR_INVALID_CONTEXT ||
        error == VITTE_IR_ERROR_INVALID_STATE ||
        error == VITTE_IR_ERROR_INVALID_ARGUMENT) {
        return "VITTE_INFRA_E_INTERNAL";
    }
    if (verification || error == VITTE_IR_ERROR_VALIDATION ||
        error == VITTE_IR_ERROR_CORRUPTION ||
        error == VITTE_IR_ERROR_INVALID_CFG ||
        error == VITTE_IR_ERROR_INVALID_PHI ||
        error == VITTE_IR_ERROR_INVALID_INSTRUCTION ||
        error == VITTE_IR_ERROR_MISSING_TERMINATOR ||
        error == VITTE_IR_ERROR_TERMINATED_BLOCK) {
        return "VITTE_IR_E_VERIFY";
    }
    return "VITTE_IR_E_LOWERING";
}

static const char *
vitte_main_ir_diagnostic_message(
    vitte_ir_error_t error,
    bool verification)
{
    const char *code = vitte_main_ir_diagnostic_code(error, verification);

    if (strcmp(code, "VITTE_IR_E_UNSUPPORTED") == 0) {
        return "source construct is not supported by IR lowering";
    }
    if (strcmp(code, "VITTE_INFRA_E_RESOURCE") == 0) {
        return "compiler resource failure during IR lowering";
    }
    if (strcmp(code, "VITTE_INFRA_E_INTERNAL") == 0) {
        return "internal compiler failure during IR lowering";
    }
    if (strcmp(code, "VITTE_IR_E_VERIFY") == 0) {
        return "IR verification failed";
    }
    return "IR lowering failed";
}

static vitte_cli_diagnostic_severity_t
vitte_main_ir_diagnostic_severity(
    vitte_ir_error_t error,
    bool verification)
{
    const char *code = vitte_main_ir_diagnostic_code(error, verification);
    return strcmp(code, "VITTE_IR_E_UNSUPPORTED") == 0 ||
           strcmp(code, "VITTE_IR_E_LOWERING") == 0
        ? VITTE_CLI_DIAGNOSTIC_ERROR
        : VITTE_CLI_DIAGNOSTIC_FATAL;
}

bool
vitte_main_driver_lower_hir(
    vitte_driver_t *driver,
    vitte_driver_phase_t phase,
    void *user_data)
{
    vitte_main_driver_context_t *context =
        (vitte_main_driver_context_t *)user_data;
    (void)driver;
    (void)phase;
    if (context == NULL || context->options == NULL ||
        !context->use_ir_pipeline) {
        return false;
    }
    if (context->native_frontend_completed) {
        return context->options->emit_ir;
    }
    if (!context->parser_initialized) return false;
    if (!vitte_hir_init(&context->hir)) {
        const vitte_hir_error_t error = context->hir.last_error;
        (void)vitte_main_render_diagnostic(
            context,
            vitte_main_hir_diagnostic_severity(error, false),
            vitte_main_hir_diagnostic_code(error, false),
            vitte_main_hir_diagnostic_message(error, false),
            vitte_hir_error_name(error),
            NULL,
            NULL,
            "HIR lowering");
        return false;
    }
    context->hir_initialized = true;
    if (!vitte_hir_lower_ast(&context->hir, &context->parser)) {
        (void)vitte_main_render_diagnostic(
            context,
            vitte_main_hir_diagnostic_severity(context->hir.last_error, false),
            vitte_main_hir_diagnostic_code(context->hir.last_error, false),
            vitte_main_hir_diagnostic_message(context->hir.last_error, false),
            vitte_hir_error_name(context->hir.last_error),
            NULL,
            NULL,
            "HIR lowering");
        return false;
    }
    if (context->hir.state != VITTE_HIR_STATE_VALIDATED) {
        const vitte_hir_error_t error = context->hir.last_error;
        (void)vitte_main_render_diagnostic(
            context,
            vitte_main_hir_diagnostic_severity(error, true),
            vitte_main_hir_diagnostic_code(error, true),
            vitte_main_hir_diagnostic_message(error, true),
            vitte_hir_error_name(error),
            NULL,
            NULL,
            "HIR validation");
        return false;
    }
    return true;
}

bool
vitte_main_driver_lower_ir(
    vitte_driver_t *driver,
    vitte_driver_phase_t phase,
    void *user_data)
{
    vitte_main_driver_context_t *context =
        (vitte_main_driver_context_t *)user_data;
    (void)driver;
    (void)phase;
    if (context == NULL || context->options == NULL ||
        !context->use_ir_pipeline) return false;
#ifdef VITTE_NATIVE_FRONTEND_LINKED
    if (context->native_frontend_completed && context->options->emit_ir) {
        bool written = vitte_main_emit_native_ir(context);
        vitte_native_frontend_release_output(context->native_output);
        context->native_output = NULL;
        context->native_output_length = 0u;
        context->native_frontend_completed = false;
        return written;
    }
#endif
    if (!context->hir_initialized) return false;
    if (!vitte_ir_init(&context->ir)) {
        const vitte_ir_error_t error = context->ir.last_error;
        (void)vitte_main_render_diagnostic(
            context,
            vitte_main_ir_diagnostic_severity(error, false),
            vitte_main_ir_diagnostic_code(error, false),
            vitte_main_ir_diagnostic_message(error, false),
            vitte_ir_error_name(error),
            NULL,
            NULL,
            "IR lowering");
        return false;
    }
    context->ir_initialized = true;
    if (!vitte_ir_lower_hir(&context->ir, &context->hir)) {
        (void)vitte_main_render_diagnostic(
            context,
            vitte_main_ir_diagnostic_severity(context->ir.last_error, false),
            vitte_main_ir_diagnostic_code(context->ir.last_error, false),
            vitte_main_ir_diagnostic_message(context->ir.last_error, false),
            vitte_ir_error_name(context->ir.last_error),
            NULL,
            NULL,
            "IR lowering");
        return false;
    }
    if (context->ir.state != VITTE_IR_STATE_SEALED) {
        const vitte_ir_error_t error = context->ir.last_error;
        (void)vitte_main_render_diagnostic(
            context,
            vitte_main_ir_diagnostic_severity(error, true),
            vitte_main_ir_diagnostic_code(error, true),
            vitte_main_ir_diagnostic_message(error, true),
            vitte_ir_error_name(error),
            NULL,
            NULL,
            "IR verification");
        return false;
    }
    return true;
}

bool
vitte_main_driver_codegen_ir(
    vitte_driver_t *driver,
    vitte_driver_phase_t phase,
    void *user_data)
{
    vitte_main_driver_context_t *context =
        (vitte_main_driver_context_t *)user_data;
    vitte_c17_ir_emit_options_t options;
    vitte_c17_ir_compile_result_t compile_result;
    const char *output_path;
    (void)driver;
    (void)phase;
    if (context == NULL || context->options == NULL ||
        !context->ir_initialized) return false;
    options = vitte_c17_ir_emit_options_default();
    options.emit_debug_comments = context->options->debug_info;
    options.emit_line_directives = context->options->debug_info;
    options.emit_debug_info = context->options->debug_info;
    options.source_path = context->options->input_path;
    output_path = context->options->output_path;
    if (!context->options->emit_c && !context->options->emit_object &&
        output_path == NULL &&
        context->options->command != VITTE_MAIN_COMMAND_RUN) {
        output_path = "vitte.out";
    }
    if (context->options->emit_object) {
        compile_result = vitte_c17_compile_ir_object_result(
            &context->ir,
            output_path,
            &options);
    } else {
        compile_result = vitte_c17_compile_ir_result(
            &context->ir,
            output_path,
            context->options->emit_c,
            context->options->command == VITTE_MAIN_COMMAND_RUN,
            context->options->run_argc,
            context->options->run_argv,
            &options);
    }
    if (compile_result != VITTE_C17_IR_COMPILE_SUCCESS) {
        const char *code;
        const char *message;
        const char *details;

        switch (compile_result) {
            case VITTE_C17_IR_COMPILE_INVALID_ARGUMENT:
                code = "VITTE_INFRA_E_PHASE";
                message = "invalid IR compilation request";
                details = "the backend received an invalid IR compilation configuration";
                break;
            case VITTE_C17_IR_COMPILE_OUTPUT_FAILED:
                code = "VITTE_BACKEND_E_OUTPUT";
                message = "IR C17 output could not be written";
                details = "creating, writing, or closing a temporary or output file failed";
                break;
            case VITTE_C17_IR_COMPILE_EMIT_FAILED:
                code = "VITTE_BACKEND_E_CODEGEN";
                message = "C17 source emission from IR failed";
                details = "the IR emitter could not produce a complete C17 translation unit";
                break;
            case VITTE_C17_IR_COMPILE_C_COMPILATION_FAILED:
                code = "VITTE_BACKEND_E_EXTERNAL_COMPILER";
                message = "generated C source compilation failed";
                details = "phase: C compilation; the bootstrap backend invokes `cc -std=c17 -x c ...` from PATH. Its stdout/stderr was forwarded above. Check that `cc` is installed and supports C17; generated-C errors can indicate a backend defect, while missing headers or target configuration belong to the host toolchain.";
                break;
            case VITTE_C17_IR_COMPILE_LINK_FAILED:
                code = "VITTE_BACKEND_E_LINK";
                message = "generated C object could not be linked";
                details = "phase: linking; the bootstrap backend invokes `cc <object> -o <output>` from PATH. Its stdout/stderr was forwarded above. Check the host linker, SDK/sysroot and required system libraries.";
                break;
            case VITTE_C17_IR_COMPILE_RUNTIME_FAILED:
                code = "VITTE_BACKEND_E_EXECUTION";
                message = "compiled program exited unsuccessfully";
                details = "C17 compilation and linking succeeded, but program execution returned a failure status";
                break;
            case VITTE_C17_IR_COMPILE_UNSUPPORTED:
                code = "VITTE_BACKEND_E_UNSUPPORTED";
                message = "IR C17 compilation is unsupported on this host";
                details = "the IR C17 compile driver does not support this platform";
                break;
            case VITTE_C17_IR_COMPILE_SUCCESS:
            default:
                code = "VITTE_BACKEND_E_CODEGEN";
                message = "IR C17 compilation failed";
                details = "the backend returned an unclassified failure";
                break;
        }
        (void)vitte_main_render_diagnostic(
            context,
            VITTE_CLI_DIAGNOSTIC_ERROR,
            code,
            message,
            details,
            NULL,
            NULL,
            "C17 IR backend");
        return false;
    }
    return true;
}

void
vitte_main_driver_context_destroy(
    vitte_driver_t *driver,
    void *user_data)
{
    vitte_main_driver_context_t *context;

    (void)driver;

    context = (vitte_main_driver_context_t *)user_data;

    if (context == NULL) {
        return;
    }

#ifdef VITTE_NATIVE_FRONTEND_LINKED
    if (context->native_output != NULL) {
        vitte_native_frontend_release_output(context->native_output);
        context->native_output = NULL;
        context->native_output_length = 0u;
    }
#endif

    if (context->sema_initialized) {
        vitte_sema_destroy(&context->sema);
        context->sema_initialized = false;
    }

    if (context->ir_initialized) {
        vitte_ir_destroy(&context->ir);
        context->ir_initialized = false;
    }

    if (context->hir_initialized) {
        vitte_hir_destroy(&context->hir);
        context->hir_initialized = false;
    }

    if (context->parser_initialized) {
        vitte_parser_destroy(&context->parser);
        context->parser_initialized = false;
    }

    free(context->expanded_tokens);
    context->expanded_tokens = NULL;
    context->expanded_token_count = 0u;

    if (context->lexer_initialized) {
        vitte_lexer_destroy(&context->lexer);
        context->lexer_initialized = false;
    }

    if (context->source_initialized) {
        vitte_source_destroy(&context->sources);
        context->source_initialized = false;
    }

    free(context->origins);
    context->origins = NULL;
    context->origin_count = 0u;

    vitte_main_file_destroy(&context->file);
}

/*
 * The driver subsystem is the canonical owner of the complete compilation
 * pipeline.
 *
 * Keeping this bridge isolated means changes to driver.h affect one small
 * section of main.c rather than CLI parsing and process management.
 */
