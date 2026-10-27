#include "import/import.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMPORT_GRAPH_MODULE_COUNT ((size_t)1024u)

static int
fail(
    const char *message)
{
    (void)fprintf(stderr, "[IMPORT] %s\n", message);
    return 0;
}

static vitte_import_module_id_t
add_module(
    vitte_import_context_t *context,
    const char *name)
{
    vitte_import_module_desc_t description;
    vitte_import_string_id_t name_id;
    size_t name_length;

    name_length = strlen(name);
    name_id = vitte_import_intern_string(
        context,
        name,
        name_length);

    if (name_id == VITTE_IMPORT_INVALID_ID) {
        return VITTE_IMPORT_INVALID_ID;
    }

    memset(
        &description,
        0,
        sizeof(description));

    description.name = name_id;
    description.path = VITTE_IMPORT_INVALID_ID;
    description.visibility = VITTE_IMPORT_VISIBILITY_PRIVATE;

    return vitte_import_add_module(
        context,
        &description);
}

static vitte_import_edge_id_t
add_import(
    vitte_import_context_t *context,
    vitte_import_module_id_t source_module,
    const char *target_name)
{
    vitte_import_desc_t description;
    vitte_import_string_id_t target_name_id;

    target_name_id = vitte_import_intern_string(
        context,
        target_name,
        strlen(target_name));

    if (target_name_id == VITTE_IMPORT_INVALID_ID) {
        return VITTE_IMPORT_INVALID_ID;
    }

    memset(
        &description,
        0,
        sizeof(description));

    description.source_module = source_module;
    description.module_name = target_name_id;
    description.alias = VITTE_IMPORT_INVALID_ID;
    description.visibility = VITTE_IMPORT_VISIBILITY_PRIVATE;

    return vitte_import_add(
        context,
        &description);
}

static int
check_large_graph(void)
{
    vitte_import_context_t context;
    vitte_import_module_id_t *modules;
    char module_name[32];
    uint64_t first_fingerprint;
    uint64_t second_fingerprint;
    size_t index;
    int success;

    memset(
        &context,
        0,
        sizeof(context));
    modules = NULL;
    success = 0;

    if (!vitte_import_init(&context)) {
        return fail("failed to initialize import context");
    }

    modules = (vitte_import_module_id_t *)calloc(
        IMPORT_GRAPH_MODULE_COUNT,
        sizeof(*modules));

    if (modules == NULL) {
        (void)fail("failed to allocate test module IDs");
        goto cleanup;
    }

    if (!vitte_import_set_limits(
            &context,
            IMPORT_GRAPH_MODULE_COUNT,
            IMPORT_GRAPH_MODULE_COUNT,
            (size_t)(4u * 1024u * 1024u))) {
        (void)fail("failed to configure large graph limits");
        goto cleanup;
    }

    for (index = 0u; index < IMPORT_GRAPH_MODULE_COUNT; ++index) {
        int written;

        written = snprintf(
            module_name,
            sizeof(module_name),
            "scale.module_%04zu",
            index);

        if (written < 0 ||
            (size_t)written >= sizeof(module_name)) {
            (void)fail("failed to format module name");
            goto cleanup;
        }

        modules[index] = add_module(
            &context,
            module_name);

        if (modules[index] == VITTE_IMPORT_INVALID_ID) {
            (void)fail("failed to register all large-graph modules");
            goto cleanup;
        }
    }

    for (index = 0u; index + 1u < IMPORT_GRAPH_MODULE_COUNT; ++index) {
        int written;
        vitte_import_edge_id_t edge;

        written = snprintf(
            module_name,
            sizeof(module_name),
            "scale.module_%04zu",
            index + 1u);

        if (written < 0 ||
            (size_t)written >= sizeof(module_name)) {
            (void)fail("failed to format dependency name");
            goto cleanup;
        }

        edge = add_import(
            &context,
            modules[index],
            module_name);

        if (edge == VITTE_IMPORT_INVALID_ID) {
            (void)fail("failed to register all large-graph imports");
            goto cleanup;
        }
    }

    if (!vitte_import_resolve(&context)) {
        (void)fail("failed to resolve large import graph");
        goto cleanup;
    }

    if (!vitte_import_validate(&context)) {
        (void)fail("resolved large import graph failed validation");
        goto cleanup;
    }

    if (vitte_import_topological_count(&context) !=
            IMPORT_GRAPH_MODULE_COUNT ||
        vitte_import_topological_at(&context, 0u) !=
            modules[IMPORT_GRAPH_MODULE_COUNT - 1u] ||
        vitte_import_topological_at(
            &context,
            IMPORT_GRAPH_MODULE_COUNT - 1u) != modules[0]) {
        (void)fail("large graph order is not dependency-first");
        goto cleanup;
    }

    first_fingerprint = vitte_import_fingerprint(&context);
    second_fingerprint = vitte_import_fingerprint(&context);

    if (first_fingerprint == UINT64_C(0) ||
        first_fingerprint != second_fingerprint) {
        (void)fail("large graph fingerprint is not deterministic");
        goto cleanup;
    }

    success = 1;

cleanup:
    free(modules);
    vitte_import_destroy(&context);
    return success;
}

static int
check_cycle_diagnostic(void)
{
    vitte_import_context_t context;
    vitte_import_module_id_t first;
    vitte_import_module_id_t second;
    int success;

    memset(
        &context,
        0,
        sizeof(context));
    success = 0;

    if (!vitte_import_init(&context)) {
        return fail("failed to initialize cycle-test context");
    }

    first = add_module(
        &context,
        "cycle.first");
    second = add_module(
        &context,
        "cycle.second");

    if (first == VITTE_IMPORT_INVALID_ID ||
        second == VITTE_IMPORT_INVALID_ID ||
        add_import(&context, first, "cycle.second") ==
            VITTE_IMPORT_INVALID_ID ||
        add_import(&context, second, "cycle.first") ==
            VITTE_IMPORT_INVALID_ID) {
        (void)fail("failed to construct cycle test graph");
        goto cleanup;
    }

    if (vitte_import_resolve(&context) ||
        vitte_import_last_error(&context) != VITTE_IMPORT_ERROR_CYCLE) {
        (void)fail("dependency cycle did not produce the cycle error");
        goto cleanup;
    }

    success = 1;

cleanup:
    vitte_import_destroy(&context);
    return success;
}

static int
check_alias_conflict(void)
{
    vitte_import_context_t context;
    vitte_import_module_id_t source;
    vitte_import_module_id_t first_target;
    vitte_import_module_id_t second_target;
    vitte_import_string_id_t first_name;
    vitte_import_string_id_t second_name;
    vitte_import_string_id_t alias;
    vitte_import_desc_t description;
    vitte_import_edge_id_t first_edge;
    const vitte_import_edge_t *stored_edge;
    int success;

    memset(
        &context,
        0,
        sizeof(context));
    success = 0;

    if (!vitte_import_init(&context)) {
        return fail("failed to initialize alias-conflict context");
    }

    source = add_module(&context, "alias.source");
    first_target = add_module(&context, "alias.first");
    second_target = add_module(&context, "alias.second");
    first_name = vitte_import_intern_string(
        &context,
        "alias.first",
        strlen("alias.first"));
    second_name = vitte_import_intern_string(
        &context,
        "alias.second",
        strlen("alias.second"));
    alias = vitte_import_intern_string(
        &context,
        "shared",
        strlen("shared"));

    if (source == VITTE_IMPORT_INVALID_ID ||
        first_target == VITTE_IMPORT_INVALID_ID ||
        second_target == VITTE_IMPORT_INVALID_ID ||
        first_name == VITTE_IMPORT_INVALID_ID ||
        second_name == VITTE_IMPORT_INVALID_ID ||
        alias == VITTE_IMPORT_INVALID_ID) {
        (void)fail("failed to construct alias-conflict graph");
        goto cleanup;
    }

    memset(
        &description,
        0,
        sizeof(description));
    description.source_module = source;
    description.module_name = first_name;
    description.alias = alias;
    description.span = vitte_import_span_make(7u, 11u, 29u);
    description.span.line = 3u;
    description.span.column = 5u;
    description.span.end_line = 3u;
    description.span.end_column = 23u;
    description.visibility = VITTE_IMPORT_VISIBILITY_PRIVATE;

    first_edge = vitte_import_add(&context, &description);
    stored_edge = vitte_import_get(&context, first_edge);
    if (first_edge == VITTE_IMPORT_INVALID_ID ||
        stored_edge == NULL ||
        !stored_edge->span.valid ||
        stored_edge->span.file_id != 7u ||
        stored_edge->span.begin != 11u ||
        stored_edge->span.end != 29u ||
        stored_edge->span.line != 3u ||
        stored_edge->span.column != 5u ||
        stored_edge->span.end_line != 3u ||
        stored_edge->span.end_column != 23u) {
        (void)fail("failed to register the first aliased import");
        goto cleanup;
    }

    description.module_name = second_name;
    if (vitte_import_add(&context, &description) !=
            VITTE_IMPORT_INVALID_ID ||
        vitte_import_last_error(&context) !=
            VITTE_IMPORT_ERROR_ALIAS_CONFLICT ||
        strcmp(
            vitte_import_error_name(vitte_import_last_error(&context)),
            "alias_conflict") != 0 ||
        context.edge_count != 1u) {
        (void)fail("different targets incorrectly share one import alias");
        goto cleanup;
    }

    success = 1;

cleanup:
    vitte_import_destroy(&context);
    return success;
}

int
main(void)
{
    if (!check_large_graph() ||
        !check_cycle_diagnostic() ||
        !check_alias_conflict()) {
        return EXIT_FAILURE;
    }

    (void)puts("[IMPORT] graph, cycle, and alias checks passed");
    return EXIT_SUCCESS;
}
