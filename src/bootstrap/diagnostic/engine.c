#include "engine.h"

#include "cascade.h"
#include "diagnostic.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define VITTE_DIAGNOSTIC_ENGINE_DEFAULT_ERROR_LIMIT   ((size_t)100u)
#define VITTE_DIAGNOSTIC_ENGINE_DEFAULT_WARNING_LIMIT ((size_t)100u)

#define VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE \
    VITTE_DIAGNOSTIC_INDEX_NONE

typedef struct vitte_diagnostic_engine_sort_entry {
    vitte_diagnostic_t diagnostic;
    size_t old_index;
} vitte_diagnostic_engine_sort_entry_t;

/* ========================================================================= */
/* Validation / text                                                         */
/* ========================================================================= */

static bool
vitte_diagnostic_engine_valid(
    const vitte_diagnostic_engine_t *engine
)
{
    return engine != NULL &&
           engine->initialized &&
           engine->bag != NULL &&
           vitte_diagnostic_bag_is_initialized(engine->bag);
}

static bool
vitte_diagnostic_engine_text_valid(
    const char *text
)
{
    return text != NULL &&
           text[0] != '\0';
}

static bool
vitte_diagnostic_engine_text_fits(
    const char *text,
    size_t capacity
)
{
    if (capacity == 0u) {
        return false;
    }

    if (text == NULL) {
        return true;
    }

    return strlen(text) < capacity;
}

static const char *
vitte_diagnostic_engine_copy_text(
    char *destination,
    size_t capacity,
    const char *source
)
{
    size_t length;

    if (destination == NULL ||
        capacity == 0u) {
        return NULL;
    }

    destination[0] = '\0';

    if (source == NULL) {
        return NULL;
    }

    length = strlen(source);

    if (length >= capacity) {
        return NULL;
    }

    if (length != 0u) {
        memcpy(
            destination,
            source,
            length
        );
    }

    destination[length] = '\0';

    return destination;
}

static bool
vitte_diagnostic_engine_emission_text_fits(
    const char *code,
    const char *message,
    const char *details
)
{
    vitte_diagnostic_t diagnostic;

    return vitte_diagnostic_engine_text_fits(
               code,
               sizeof(diagnostic.code_storage)
           ) &&
           vitte_diagnostic_engine_text_fits(
               message,
               sizeof(diagnostic.message_storage)
           ) &&
           vitte_diagnostic_engine_text_fits(
               details,
               sizeof(diagnostic.details_storage)
           );
}

/* ========================================================================= */
/* Severity                                                                  */
/* ========================================================================= */

static bool
vitte_diagnostic_engine_is_error_severity(
    vitte_diagnostic_severity_t severity
)
{
    return severity == VITTE_DIAGNOSTIC_ERROR ||
           severity == VITTE_DIAGNOSTIC_FATAL;
}

static bool
vitte_diagnostic_engine_is_warning_severity(
    vitte_diagnostic_severity_t severity
)
{
    return severity == VITTE_DIAGNOSTIC_WARNING;
}

/* ========================================================================= */
/* Counts                                                                    */
/* ========================================================================= */

static size_t
vitte_diagnostic_engine_visible_errors(
    const vitte_diagnostic_bag_t *bag
)
{
    size_t index;
    size_t count;

    if (bag == NULL) {
        return 0u;
    }

    count = 0u;

    for (index = 0u;
         index < bag->count;
         index++) {

        const vitte_diagnostic_t *diagnostic;

        diagnostic =
            &bag->storage[index];

        if (diagnostic->is_suppressed) {
            continue;
        }

        if (vitte_diagnostic_engine_is_error_severity(
                diagnostic->severity
            )) {
            count++;
        }
    }

    return count;
}

static size_t
vitte_diagnostic_engine_visible_warnings(
    const vitte_diagnostic_bag_t *bag
)
{
    size_t index;
    size_t count;

    if (bag == NULL) {
        return 0u;
    }

    count = 0u;

    for (index = 0u;
         index < bag->count;
         index++) {

        const vitte_diagnostic_t *diagnostic;

        diagnostic =
            &bag->storage[index];

        if (diagnostic->is_suppressed) {
            continue;
        }

        if (vitte_diagnostic_engine_is_warning_severity(
                diagnostic->severity
            )) {
            count++;
        }
    }

    return count;
}

static void
vitte_diagnostic_engine_refresh_relation_statistics(
    vitte_diagnostic_engine_t *engine
)
{
    if (!vitte_diagnostic_engine_valid(engine)) {
        return;
    }

    engine->cascade_count =
        vitte_diagnostic_cascade_count(
            engine->bag
        );

    engine->suppressed_count =
        engine->bag->counts.suppressed_count;
}

/* ========================================================================= */
/* Policy                                                                    */
/* ========================================================================= */

void
vitte_diagnostic_engine_policy_init(
    vitte_diagnostic_engine_policy_t *policy
)
{
    if (policy == NULL) {
        return;
    }

    memset(
        policy,
        0,
        sizeof(*policy)
    );

    policy->error_limit =
        VITTE_DIAGNOSTIC_ENGINE_DEFAULT_ERROR_LIMIT;

    policy->warning_limit =
        VITTE_DIAGNOSTIC_ENGINE_DEFAULT_WARNING_LIMIT;

    policy->warnings_as_errors = false;

    policy->deduplicate = true;

    policy->classify_cascades = true;
    policy->suppress_cascades = true;

    policy->stop_after_fatal = true;
    policy->stop_after_error_limit = true;

    policy->emit_limit_diagnostic = true;

    policy->sort_before_render = true;

    policy->preserve_suppressed = true;
}

/* ========================================================================= */
/* Initialization                                                            */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_engine_init(
    vitte_diagnostic_engine_t *engine,
    vitte_diagnostic_bag_t *bag,
    const vitte_diagnostic_engine_policy_t *policy
)
{
    vitte_diagnostic_engine_policy_t defaults;

    if (engine == NULL ||
        bag == NULL ||
        !vitte_diagnostic_bag_is_initialized(bag)) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    memset(
        engine,
        0,
        sizeof(*engine)
    );

    if (policy != NULL) {
        engine->policy = *policy;
    } else {
        vitte_diagnostic_engine_policy_init(
            &defaults
        );

        engine->policy = defaults;
    }

    if (engine->policy.error_limit == 0u) {
        engine->policy.error_limit =
            VITTE_DIAGNOSTIC_ENGINE_DEFAULT_ERROR_LIMIT;
    }

    if (engine->policy.warning_limit == 0u) {
        engine->policy.warning_limit =
            VITTE_DIAGNOSTIC_ENGINE_DEFAULT_WARNING_LIMIT;
    }

    engine->bag = bag;

    engine->current_origin =
        VITTE_DIAGNOSTIC_ORIGIN_UNKNOWN;

    engine->current_parent =
        VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE;

    engine->current_root =
        VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE;

    engine->package = NULL;
    engine->module = NULL;
    engine->procedure = NULL;

    engine->stopped = false;
    engine->limit_reported = false;

    engine->emitted_count = 0u;
    engine->rejected_count = 0u;
    engine->cascade_count = 0u;
    engine->suppressed_count = 0u;

    engine->initialized = true;

    vitte_diagnostic_engine_refresh_relation_statistics(
        engine
    );

    return VITTE_STATUS_OK;
}

void
vitte_diagnostic_engine_reset(
    vitte_diagnostic_engine_t *engine
)
{
    if (engine == NULL) {
        return;
    }

    engine->current_origin =
        VITTE_DIAGNOSTIC_ORIGIN_UNKNOWN;

    engine->current_parent =
        VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE;

    engine->current_root =
        VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE;

    engine->package = NULL;
    engine->module = NULL;
    engine->procedure = NULL;

    engine->package_storage[0] = '\0';
    engine->module_storage[0] = '\0';
    engine->procedure_storage[0] = '\0';

    engine->stopped = false;
    engine->limit_reported = false;

    engine->emitted_count = 0u;
    engine->rejected_count = 0u;

    if (engine->initialized &&
        engine->bag != NULL &&
        vitte_diagnostic_bag_is_initialized(
            engine->bag
        )) {

        engine->cascade_count =
            vitte_diagnostic_cascade_count(
                engine->bag
            );

        engine->suppressed_count =
            engine->bag->counts.suppressed_count;

    } else {
        engine->cascade_count = 0u;
        engine->suppressed_count = 0u;
    }
}

bool
vitte_diagnostic_engine_is_initialized(
    const vitte_diagnostic_engine_t *engine
)
{
    return vitte_diagnostic_engine_valid(
        engine
    );
}

/* ========================================================================= */
/* Context                                                                   */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_engine_set_origin(
    vitte_diagnostic_engine_t *engine,
    vitte_diagnostic_origin_t origin
)
{
    if (!vitte_diagnostic_engine_valid(
            engine
        )) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (!vitte_diagnostic_origin_is_valid(
            origin
        )) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    engine->current_origin = origin;

    return VITTE_STATUS_OK;
}

vitte_diagnostic_origin_t
vitte_diagnostic_engine_origin(
    const vitte_diagnostic_engine_t *engine
)
{
    if (!vitte_diagnostic_engine_valid(
            engine
        )) {
        return VITTE_DIAGNOSTIC_ORIGIN_UNKNOWN;
    }

    return engine->current_origin;
}

vitte_status_t
vitte_diagnostic_engine_set_context(
    vitte_diagnostic_engine_t *engine,
    const char *package,
    const char *module,
    const char *procedure
)
{
    if (!vitte_diagnostic_engine_valid(
            engine
        )) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (!vitte_diagnostic_engine_text_fits(
            package,
            sizeof(engine->package_storage)
        ) ||
        !vitte_diagnostic_engine_text_fits(
            module,
            sizeof(engine->module_storage)
        ) ||
        !vitte_diagnostic_engine_text_fits(
            procedure,
            sizeof(engine->procedure_storage)
        )) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    engine->package =
        vitte_diagnostic_engine_copy_text(
            engine->package_storage,
            sizeof(engine->package_storage),
            package
        );

    engine->module =
        vitte_diagnostic_engine_copy_text(
            engine->module_storage,
            sizeof(engine->module_storage),
            module
        );

    engine->procedure =
        vitte_diagnostic_engine_copy_text(
            engine->procedure_storage,
            sizeof(engine->procedure_storage),
            procedure
        );

    return VITTE_STATUS_OK;
}

void
vitte_diagnostic_engine_clear_context(
    vitte_diagnostic_engine_t *engine
)
{
    if (engine == NULL) {
        return;
    }

    engine->package = NULL;
    engine->module = NULL;
    engine->procedure = NULL;

    engine->package_storage[0] = '\0';
    engine->module_storage[0] = '\0';
    engine->procedure_storage[0] = '\0';
}

/* ========================================================================= */
/* Causal context                                                            */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_engine_set_parent(
    vitte_diagnostic_engine_t *engine,
    size_t parent_index
)
{
    size_t root;

    if (!vitte_diagnostic_engine_valid(
            engine
        )) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (parent_index >=
        engine->bag->count) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    root =
        vitte_diagnostic_root_index(
            engine->bag,
            parent_index
        );

    if (root ==
        VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    engine->current_parent =
        parent_index;

    engine->current_root =
        root;

    return VITTE_STATUS_OK;
}

void
vitte_diagnostic_engine_clear_parent(
    vitte_diagnostic_engine_t *engine
)
{
    if (engine == NULL) {
        return;
    }

    engine->current_parent =
        VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE;

    engine->current_root =
        VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE;
}

/* ========================================================================= */
/* Stop policy                                                               */
/* ========================================================================= */

bool
vitte_diagnostic_engine_should_stop(
    const vitte_diagnostic_engine_t *engine
)
{
    if (!vitte_diagnostic_engine_valid(
            engine
        )) {
        return true;
    }

    if (engine->stopped) {
        return true;
    }

    if (engine->policy.stop_after_error_limit &&
        vitte_diagnostic_engine_visible_errors(
            engine->bag
        ) >= engine->policy.error_limit) {

        return true;
    }

    return false;
}

/* ========================================================================= */
/* Limit diagnostic                                                          */
/* ========================================================================= */

static vitte_status_t
vitte_diagnostic_engine_report_limit(
    vitte_diagnostic_engine_t *engine
)
{
    vitte_status_t status;
    vitte_diagnostic_t diagnostic;

    if (!vitte_diagnostic_engine_valid(
            engine
        )) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (engine->limit_reported ||
        !engine->policy.emit_limit_diagnostic) {

        return VITTE_STATUS_OK;
    }

    vitte_diagnostic_init(
        &diagnostic
    );

    diagnostic.severity =
        VITTE_DIAGNOSTIC_FATAL;

    diagnostic.origin =
        VITTE_DIAGNOSTIC_ORIGIN_DRIVER;

    diagnostic.code =
        vitte_diagnostic_engine_copy_text(
            diagnostic.code_storage,
            sizeof(diagnostic.code_storage),
            VITTE_INFRA_E_LIMIT
        );

    diagnostic.message =
        vitte_diagnostic_engine_copy_text(
            diagnostic.message_storage,
            sizeof(diagnostic.message_storage),
            "diagnostic limit reached"
        );

    diagnostic.details =
        vitte_diagnostic_engine_copy_text(
            diagnostic.details_storage,
            sizeof(diagnostic.details_storage),
            "compilation stopped after reaching the configured error limit"
        );

    if (diagnostic.code == NULL ||
        diagnostic.message == NULL ||
        diagnostic.details == NULL) {

        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    status =
        vitte_diagnostic_add(
            engine->bag,
            &diagnostic
        );

    if (status != VITTE_STATUS_OK) {
        return status;
    }

    engine->limit_reported = true;

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Last diagnostic                                                           */
/* ========================================================================= */

vitte_diagnostic_t *
vitte_diagnostic_engine_last(
    vitte_diagnostic_engine_t *engine
)
{
    if (!vitte_diagnostic_engine_valid(
            engine
        ) ||
        engine->bag->count == 0u) {

        return NULL;
    }

    return &engine->bag->storage[
        engine->bag->count - 1u
    ];
}

const vitte_diagnostic_t *
vitte_diagnostic_engine_last_const(
    const vitte_diagnostic_engine_t *engine
)
{
    if (!vitte_diagnostic_engine_valid(
            engine
        ) ||
        engine->bag->count == 0u) {

        return NULL;
    }

    return &engine->bag->storage[
        engine->bag->count - 1u
    ];
}

/* ========================================================================= */
/* Emission                                                                  */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_engine_emit(
    vitte_diagnostic_engine_t *engine,
    vitte_diagnostic_severity_t severity,
    const char *code,
    const char *message,
    const char *details,
    const vitte_ast_span_t *span
)
{
    vitte_status_t status;
    vitte_diagnostic_t pending;
    vitte_diagnostic_t *diagnostic;
    size_t count_before;

    if (!vitte_diagnostic_engine_valid(
            engine
        )) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (!vitte_diagnostic_severity_is_valid(
            severity
        ) ||
        !vitte_diagnostic_engine_text_valid(
            code
        ) ||
        !vitte_diagnostic_engine_text_valid(
            message
        ) ||
        !vitte_diagnostic_engine_emission_text_fits(
            code,
            message,
            details
        )) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (engine->stopped) {
        engine->rejected_count++;

        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (severity ==
            VITTE_DIAGNOSTIC_WARNING &&
        engine->policy.warnings_as_errors) {

        severity =
            VITTE_DIAGNOSTIC_ERROR;
    }

    if (vitte_diagnostic_engine_is_error_severity(
            severity
        ) &&
        engine->policy.stop_after_error_limit &&
        vitte_diagnostic_engine_visible_errors(
            engine->bag
        ) >= engine->policy.error_limit) {

        status =
            vitte_diagnostic_engine_report_limit(
                engine
            );

        engine->stopped = true;
        engine->rejected_count++;

        if (status != VITTE_STATUS_OK) {
            return status;
        }

        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (severity ==
            VITTE_DIAGNOSTIC_WARNING &&
        vitte_diagnostic_engine_visible_warnings(
            engine->bag
        ) >= engine->policy.warning_limit) {

        engine->rejected_count++;

        return VITTE_STATUS_OK;
    }

    vitte_diagnostic_init(
        &pending
    );

    pending.severity = severity;
    pending.origin = engine->current_origin;

    pending.code =
        vitte_diagnostic_engine_copy_text(
            pending.code_storage,
            sizeof(pending.code_storage),
            code
        );

    pending.message =
        vitte_diagnostic_engine_copy_text(
            pending.message_storage,
            sizeof(pending.message_storage),
            message
        );

    pending.details =
        vitte_diagnostic_engine_copy_text(
            pending.details_storage,
            sizeof(pending.details_storage),
            details
        );

    if (pending.code == NULL ||
        pending.message == NULL ||
        (details != NULL &&
         pending.details == NULL)) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (span != NULL) {
        status =
            vitte_diagnostic_set_span(
                &pending,
                span
            );

        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }

    vitte_diagnostic_set_context(
        &pending,
        engine->package,
        engine->module,
        engine->procedure,
        NULL
    );

    count_before =
        engine->bag->count;

    status =
        vitte_diagnostic_add(
            engine->bag,
            &pending
        );

    if (status != VITTE_STATUS_OK) {
        return status;
    }

    /*
     * A non-growing bag means the lower diagnostic layer rejected or
     * deduplicated the candidate. It is not a cascade suppression.
     */
    if (engine->bag->count ==
        count_before) {

        engine->rejected_count++;

        return VITTE_STATUS_OK;
    }

    /*
     * The diagnostic was appended at count_before.
     */
    diagnostic =
        &engine->bag->storage[count_before];

    if (engine->current_parent !=
        VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE) {

        if (engine->current_parent >=
            count_before) {

            return VITTE_STATUS_ERROR_INVALID_STATE;
        }

        status =
            vitte_diagnostic_set_parent(
                engine->bag,
                count_before,
                engine->current_parent
            );

        if (status != VITTE_STATUS_OK) {
            return status;
        }

        if (engine->policy.suppress_cascades) {
            status =
                vitte_diagnostic_mark_last_suppressed(
                    engine->bag
                );

            if (status != VITTE_STATUS_OK) {
                return status;
            }
        }

    } else if (
        engine->policy.classify_cascades &&
        vitte_diagnostic_engine_is_error_severity(
            diagnostic->severity
        )) {

        status =
            vitte_diagnostic_classify_last_cascade(
                engine->bag,
                engine->policy.suppress_cascades
            );

        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }

    engine->emitted_count++;

    vitte_diagnostic_engine_refresh_relation_statistics(
        engine
    );

    diagnostic =
        &engine->bag->storage[count_before];

    if (diagnostic->severity ==
            VITTE_DIAGNOSTIC_FATAL &&
        engine->policy.stop_after_fatal) {

        engine->stopped = true;
    }

    if (engine->policy.stop_after_error_limit &&
        vitte_diagnostic_engine_visible_errors(
            engine->bag
        ) >= engine->policy.error_limit) {

        status =
            vitte_diagnostic_engine_report_limit(
                engine
            );

        engine->stopped = true;

        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Convenience emission                                                      */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_engine_note(
    vitte_diagnostic_engine_t *engine,
    const char *code,
    const char *message,
    const vitte_ast_span_t *span
)
{
    return vitte_diagnostic_engine_emit(
        engine,
        VITTE_DIAGNOSTIC_NOTE,
        code,
        message,
        NULL,
        span
    );
}

vitte_status_t
vitte_diagnostic_engine_help(
    vitte_diagnostic_engine_t *engine,
    const char *code,
    const char *message,
    const vitte_ast_span_t *span
)
{
    return vitte_diagnostic_engine_emit(
        engine,
        VITTE_DIAGNOSTIC_HELP,
        code,
        message,
        NULL,
        span
    );
}

vitte_status_t
vitte_diagnostic_engine_warning(
    vitte_diagnostic_engine_t *engine,
    const char *code,
    const char *message,
    const char *details,
    const vitte_ast_span_t *span
)
{
    return vitte_diagnostic_engine_emit(
        engine,
        VITTE_DIAGNOSTIC_WARNING,
        code,
        message,
        details,
        span
    );
}

vitte_status_t
vitte_diagnostic_engine_error(
    vitte_diagnostic_engine_t *engine,
    const char *code,
    const char *message,
    const char *details,
    const vitte_ast_span_t *span
)
{
    return vitte_diagnostic_engine_emit(
        engine,
        VITTE_DIAGNOSTIC_ERROR,
        code,
        message,
        details,
        span
    );
}

vitte_status_t
vitte_diagnostic_engine_fatal(
    vitte_diagnostic_engine_t *engine,
    const char *code,
    const char *message,
    const char *details,
    const vitte_ast_span_t *span
)
{
    return vitte_diagnostic_engine_emit(
        engine,
        VITTE_DIAGNOSTIC_FATAL,
        code,
        message,
        details,
        span
    );
}

/* ========================================================================= */
/* Decoration                                                               */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_engine_primary_label(
    vitte_diagnostic_engine_t *engine,
    const char *message,
    const vitte_ast_span_t *span
)
{
    vitte_diagnostic_t *diagnostic;

    if (!vitte_diagnostic_engine_valid(
            engine
        )) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    diagnostic =
        vitte_diagnostic_engine_last(
            engine
        );

    if (diagnostic == NULL) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return vitte_diagnostic_add_label(
        diagnostic,
        VITTE_DIAGNOSTIC_LABEL_PRIMARY,
        span,
        message
    );
}

vitte_status_t
vitte_diagnostic_engine_secondary_label(
    vitte_diagnostic_engine_t *engine,
    const char *message,
    const vitte_ast_span_t *span
)
{
    vitte_diagnostic_t *diagnostic;

    if (!vitte_diagnostic_engine_valid(
            engine
        )) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    diagnostic =
        vitte_diagnostic_engine_last(
            engine
        );

    if (diagnostic == NULL) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return vitte_diagnostic_add_label(
        diagnostic,
        VITTE_DIAGNOSTIC_LABEL_SECONDARY,
        span,
        message
    );
}

vitte_status_t
vitte_diagnostic_engine_note_last(
    vitte_diagnostic_engine_t *engine,
    const char *message,
    const vitte_ast_span_t *span
)
{
    vitte_diagnostic_t *diagnostic;

    if (!vitte_diagnostic_engine_valid(
            engine
        )) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    diagnostic =
        vitte_diagnostic_engine_last(
            engine
        );

    if (diagnostic == NULL) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return vitte_diagnostic_add_note(
        diagnostic,
        message,
        span
    );
}

vitte_status_t
vitte_diagnostic_engine_help_last(
    vitte_diagnostic_engine_t *engine,
    const char *message,
    const vitte_ast_span_t *span
)
{
    vitte_diagnostic_t *diagnostic;

    if (!vitte_diagnostic_engine_valid(
            engine
        )) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    diagnostic =
        vitte_diagnostic_engine_last(
            engine
        );

    if (diagnostic == NULL) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return vitte_diagnostic_add_help(
        diagnostic,
        message,
        span
    );
}

vitte_status_t
vitte_diagnostic_engine_cause(
    vitte_diagnostic_engine_t *engine,
    const char *message,
    const vitte_ast_span_t *span
)
{
    vitte_diagnostic_t *diagnostic;

    if (!vitte_diagnostic_engine_valid(
            engine
        )) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    diagnostic =
        vitte_diagnostic_engine_last(
            engine
        );

    if (diagnostic == NULL) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return vitte_diagnostic_add_cause(
        diagnostic,
        message,
        span
    );
}

vitte_status_t
vitte_diagnostic_engine_suggestion(
    vitte_diagnostic_engine_t *engine,
    const char *message,
    const char *replacement,
    vitte_diagnostic_applicability_t applicability,
    const vitte_ast_span_t *span
)
{
    vitte_diagnostic_t *diagnostic;

    if (!vitte_diagnostic_engine_valid(
            engine
        )) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    diagnostic =
        vitte_diagnostic_engine_last(
            engine
        );

    if (diagnostic == NULL) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return vitte_diagnostic_add_suggestion(
        diagnostic,
        message,
        span,
        replacement,
        applicability
    );
}

vitte_status_t
vitte_diagnostic_engine_subject(
    vitte_diagnostic_engine_t *engine,
    const char *symbol,
    const char *expected_type,
    const char *actual_type,
    const char *context
)
{
    vitte_diagnostic_t *diagnostic;

    if (!vitte_diagnostic_engine_valid(
            engine
        )) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    diagnostic =
        vitte_diagnostic_engine_last(
            engine
        );

    if (diagnostic == NULL) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    vitte_diagnostic_set_subject(
        diagnostic,
        symbol,
        expected_type,
        actual_type
    );

    vitte_diagnostic_set_context(
        diagnostic,
        engine->package,
        engine->module,
        engine->procedure,
        context
    );

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Contract                                                                  */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_engine_contract(
    vitte_diagnostic_engine_t *engine,
    vitte_diagnostic_contract_kind_t kind,
    const char *expression,
    const char *procedure,
    const char *reason,
    const vitte_ast_span_t *contract_span,
    const vitte_ast_span_t *call_span,
    const vitte_ast_span_t *declaration_span
)
{
    vitte_diagnostic_t *diagnostic;

    if (!vitte_diagnostic_engine_valid(
            engine
        )) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    diagnostic =
        vitte_diagnostic_engine_last(
            engine
        );

    if (diagnostic == NULL) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return vitte_diagnostic_set_contract(
        diagnostic,
        kind,
        expression,
        procedure,
        reason,
        contract_span,
        call_span,
        declaration_span
    );
}

/* ========================================================================= */
/* Checkpoints                                                               */
/* ========================================================================= */

vitte_diagnostic_checkpoint_t
vitte_diagnostic_engine_checkpoint(
    const vitte_diagnostic_engine_t *engine
)
{
    vitte_diagnostic_checkpoint_t checkpoint;

    memset(
        &checkpoint,
        0,
        sizeof(checkpoint)
    );

    checkpoint.valid = false;

    if (!vitte_diagnostic_engine_valid(
            engine
        )) {
        return checkpoint;
    }

    checkpoint.count =
        engine->bag->count;

    checkpoint.counts =
        engine->bag->counts;

    checkpoint.emitted_count =
        engine->emitted_count;

    checkpoint.rejected_count =
        engine->rejected_count;

    checkpoint.cascade_count =
        engine->cascade_count;

    checkpoint.suppressed_count =
        engine->suppressed_count;

    checkpoint.stopped =
        engine->stopped;

    checkpoint.limit_reported =
        engine->limit_reported;

    checkpoint.valid = true;

    return checkpoint;
}

vitte_status_t
vitte_diagnostic_engine_rollback(
    vitte_diagnostic_engine_t *engine,
    const vitte_diagnostic_checkpoint_t *checkpoint
)
{
    size_t index;

    if (!vitte_diagnostic_engine_valid(
            engine
        ) ||
        checkpoint == NULL ||
        !checkpoint->valid) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (checkpoint->count >
        engine->bag->count) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    for (index = checkpoint->count;
         index < engine->bag->count;
         index++) {

        vitte_diagnostic_init(
            &engine->bag->storage[index]
        );
    }

    engine->bag->count =
        checkpoint->count;

    engine->bag->counts =
        checkpoint->counts;

    engine->emitted_count =
        checkpoint->emitted_count;

    engine->rejected_count =
        checkpoint->rejected_count;

    engine->cascade_count =
        checkpoint->cascade_count;

    engine->suppressed_count =
        checkpoint->suppressed_count;

    engine->stopped =
        checkpoint->stopped;

    engine->limit_reported =
        checkpoint->limit_reported;

    /*
     * Causal context cannot point into discarded diagnostics.
     */
    if (engine->current_parent !=
            VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE &&
        engine->current_parent >=
            engine->bag->count) {

        engine->current_parent =
            VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE;
    }

    if (engine->current_root !=
            VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE &&
        engine->current_root >=
            engine->bag->count) {

        engine->current_root =
            VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE;
    }

    if (engine->current_parent ==
            VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE) {

        engine->current_root =
            VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE;
    }

    return VITTE_STATUS_OK;
}

bool
vitte_diagnostic_engine_changed_since(
    const vitte_diagnostic_engine_t *engine,
    const vitte_diagnostic_checkpoint_t *checkpoint
)
{
    if (!vitte_diagnostic_engine_valid(
            engine
        ) ||
        checkpoint == NULL ||
        !checkpoint->valid) {

        return false;
    }

    return engine->bag->count !=
           checkpoint->count;
}

bool
vitte_diagnostic_engine_has_errors_since(
    const vitte_diagnostic_engine_t *engine,
    const vitte_diagnostic_checkpoint_t *checkpoint
)
{
    size_t index;

    if (!vitte_diagnostic_engine_valid(
            engine
        ) ||
        checkpoint == NULL ||
        !checkpoint->valid ||
        checkpoint->count >
            engine->bag->count) {

        return false;
    }

    for (index = checkpoint->count;
         index < engine->bag->count;
         index++) {

        const vitte_diagnostic_t *diagnostic;

        diagnostic =
            &engine->bag->storage[index];

        if (diagnostic->is_suppressed) {
            continue;
        }

        if (vitte_diagnostic_engine_is_error_severity(
                diagnostic->severity
            )) {

            return true;
        }
    }

    return false;
}

/* ========================================================================= */
/* Sorting ranks                                                             */
/* ========================================================================= */

static unsigned
vitte_diagnostic_engine_severity_rank(
    vitte_diagnostic_severity_t severity
)
{
    switch (severity) {
        case VITTE_DIAGNOSTIC_FATAL:
            return 0u;

        case VITTE_DIAGNOSTIC_ERROR:
            return 1u;

        case VITTE_DIAGNOSTIC_WARNING:
            return 2u;

        case VITTE_DIAGNOSTIC_NOTE:
            return 3u;

        case VITTE_DIAGNOSTIC_HELP:
            return 4u;

        default:
            return 5u;
    }
}

static unsigned
vitte_diagnostic_engine_origin_rank(
    vitte_diagnostic_origin_t origin
)
{
    switch (origin) {
        case VITTE_DIAGNOSTIC_ORIGIN_IO:
            return 10u;

        case VITTE_DIAGNOSTIC_ORIGIN_LEXER:
            return 20u;

        case VITTE_DIAGNOSTIC_ORIGIN_PARSER:
            return 30u;

        case VITTE_DIAGNOSTIC_ORIGIN_IMPORT:
            return 40u;

        case VITTE_DIAGNOSTIC_ORIGIN_NAME_RESOLUTION:
            return 50u;

        case VITTE_DIAGNOSTIC_ORIGIN_TYPE_CHECK:
            return 60u;

        case VITTE_DIAGNOSTIC_ORIGIN_CONTRACT:
            return 63u;

        case VITTE_DIAGNOSTIC_ORIGIN_CONSTANT_EVAL:
            return 65u;

        case VITTE_DIAGNOSTIC_ORIGIN_HIR:
            return 70u;

        case VITTE_DIAGNOSTIC_ORIGIN_IR:
            return 80u;

        case VITTE_DIAGNOSTIC_ORIGIN_C17_BACKEND:
            return 90u;

        case VITTE_DIAGNOSTIC_ORIGIN_C_COMPILER:
            return 100u;

        case VITTE_DIAGNOSTIC_ORIGIN_LINKER:
            return 110u;

        case VITTE_DIAGNOSTIC_ORIGIN_RUNTIME:
            return 120u;

        case VITTE_DIAGNOSTIC_ORIGIN_DRIVER:
            return 130u;

        case VITTE_DIAGNOSTIC_ORIGIN_INTERNAL:
            return 140u;

        case VITTE_DIAGNOSTIC_ORIGIN_UNKNOWN:
        default:
            return 150u;
    }
}

static int
vitte_diagnostic_engine_compare_nullable_text(
    const char *left,
    const char *right
)
{
    if (left == NULL &&
        right == NULL) {
        return 0;
    }

    if (left == NULL) {
        return 1;
    }

    if (right == NULL) {
        return -1;
    }

    return strcmp(
        left,
        right
    );
}

static int
vitte_diagnostic_engine_compare_core(
    const vitte_diagnostic_t *left,
    const vitte_diagnostic_t *right
)
{
    unsigned left_rank;
    unsigned right_rank;
    int comparison;

    /*
     * Prefer canonical source names as the primary deterministic source key.
     * Source IDs may depend on load order.
     */
    comparison =
        vitte_diagnostic_engine_compare_nullable_text(
            left->source_name,
            right->source_name
        );

    if (comparison != 0) {
        return comparison;
    }

    if (left->source_id !=
            right->source_id) {

        if (left->source_id ==
            VITTE_SOURCE_ID_INVALID) {
            return 1;
        }

        if (right->source_id ==
            VITTE_SOURCE_ID_INVALID) {
            return -1;
        }

        return left->source_id <
               right->source_id
            ? -1
            : 1;
    }

    if (left->start_offset !=
        right->start_offset) {

        return left->start_offset <
               right->start_offset
            ? -1
            : 1;
    }

    if (left->end_offset !=
        right->end_offset) {

        return left->end_offset <
               right->end_offset
            ? -1
            : 1;
    }

    left_rank =
        vitte_diagnostic_engine_severity_rank(
            left->severity
        );

    right_rank =
        vitte_diagnostic_engine_severity_rank(
            right->severity
        );

    if (left_rank != right_rank) {
        return left_rank <
               right_rank
            ? -1
            : 1;
    }

    left_rank =
        vitte_diagnostic_engine_origin_rank(
            left->origin
        );

    right_rank =
        vitte_diagnostic_engine_origin_rank(
            right->origin
        );

    if (left_rank != right_rank) {
        return left_rank <
               right_rank
            ? -1
            : 1;
    }

    comparison =
        vitte_diagnostic_engine_compare_nullable_text(
            left->short_code,
            right->short_code
        );

    if (comparison != 0) {
        return comparison;
    }

    comparison =
        vitte_diagnostic_engine_compare_nullable_text(
            left->code,
            right->code
        );

    if (comparison != 0) {
        return comparison;
    }

    comparison =
        vitte_diagnostic_engine_compare_nullable_text(
            left->symbol,
            right->symbol
        );

    if (comparison != 0) {
        return comparison;
    }

    comparison =
        vitte_diagnostic_engine_compare_nullable_text(
            left->expected_type,
            right->expected_type
        );

    if (comparison != 0) {
        return comparison;
    }

    comparison =
        vitte_diagnostic_engine_compare_nullable_text(
            left->actual_type,
            right->actual_type
        );

    if (comparison != 0) {
        return comparison;
    }

    comparison =
        vitte_diagnostic_engine_compare_nullable_text(
            left->message,
            right->message
        );

    if (comparison != 0) {
        return comparison;
    }

    if (left->fingerprint !=
        right->fingerprint) {

        return left->fingerprint <
               right->fingerprint
            ? -1
            : 1;
    }

    return 0;
}

static int
vitte_diagnostic_engine_compare_sort_entries(
    const void *left_pointer,
    const void *right_pointer
)
{
    const vitte_diagnostic_engine_sort_entry_t *left;
    const vitte_diagnostic_engine_sort_entry_t *right;
    int comparison;

    left =
        (const vitte_diagnostic_engine_sort_entry_t *)
        left_pointer;

    right =
        (const vitte_diagnostic_engine_sort_entry_t *)
        right_pointer;

    comparison =
        vitte_diagnostic_engine_compare_core(
            &left->diagnostic,
            &right->diagnostic
        );

    if (comparison != 0) {
        return comparison;
    }

    /*
     * Absolute deterministic tie-breaker that does not assume fingerprints
     * are unique.
     */
    if (left->old_index !=
        right->old_index) {

        return left->old_index <
               right->old_index
            ? -1
            : 1;
    }

    return 0;
}

/* ========================================================================= */
/* Pointer rebinding                                                         */
/* ========================================================================= */

static void
vitte_diagnostic_engine_rebind(
    vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    if (diagnostic == NULL) {
        return;
    }

    diagnostic->code =
        diagnostic->code_storage[0] != '\0'
        ? diagnostic->code_storage
        : NULL;

    diagnostic->short_code =
        diagnostic->short_code_storage[0] != '\0'
        ? diagnostic->short_code_storage
        : NULL;

    diagnostic->category =
        diagnostic->category_storage[0] != '\0'
        ? diagnostic->category_storage
        : NULL;

    diagnostic->message =
        diagnostic->message_storage[0] != '\0'
        ? diagnostic->message_storage
        : NULL;

    diagnostic->details =
        diagnostic->details_storage[0] != '\0'
        ? diagnostic->details_storage
        : NULL;

    diagnostic->source_name =
        diagnostic->source_name_storage[0] != '\0'
        ? diagnostic->source_name_storage
        : NULL;

    diagnostic->symbol =
        diagnostic->symbol_storage[0] != '\0'
        ? diagnostic->symbol_storage
        : NULL;

    diagnostic->expected_type =
        diagnostic->expected_type_storage[0] != '\0'
        ? diagnostic->expected_type_storage
        : NULL;

    diagnostic->actual_type =
        diagnostic->actual_type_storage[0] != '\0'
        ? diagnostic->actual_type_storage
        : NULL;

    diagnostic->context =
        diagnostic->context_storage[0] != '\0'
        ? diagnostic->context_storage
        : NULL;

    diagnostic->package =
        diagnostic->package_storage[0] != '\0'
        ? diagnostic->package_storage
        : NULL;

    diagnostic->module =
        diagnostic->module_storage[0] != '\0'
        ? diagnostic->module_storage
        : NULL;

    diagnostic->procedure =
        diagnostic->procedure_storage[0] != '\0'
        ? diagnostic->procedure_storage
        : NULL;

    for (index = 0u;
         index < diagnostic->label_count;
         index++) {

        vitte_diagnostic_label_t *label;

        label =
            &diagnostic->labels[index];

        label->message =
            label->message_storage[0] != '\0'
            ? label->message_storage
            : NULL;
    }

    for (index = 0u;
         index < diagnostic->related_count;
         index++) {

        vitte_diagnostic_location_t *location;

        location =
            &diagnostic->related[index];

        location->label =
            location->label_storage[0] != '\0'
            ? location->label_storage
            : NULL;
    }

    for (index = 0u;
         index < diagnostic->cause_count;
         index++) {

        vitte_diagnostic_cause_t *cause;

        cause =
            &diagnostic->causes[index];

        cause->message =
            cause->message_storage[0] != '\0'
            ? cause->message_storage
            : NULL;
    }

    for (index = 0u;
         index < diagnostic->annotation_count;
         index++) {

        vitte_diagnostic_annotation_t *annotation;

        annotation =
            &diagnostic->annotations[index];

        annotation->message =
            annotation->message_storage[0] != '\0'
            ? annotation->message_storage
            : NULL;
    }

    for (index = 0u;
         index < diagnostic->suggestion_count;
         index++) {

        vitte_diagnostic_suggestion_t *suggestion;

        suggestion =
            &diagnostic->suggestions[index];

        suggestion->message =
            suggestion->message_storage[0] != '\0'
            ? suggestion->message_storage
            : NULL;

        /*
         * Empty replacement is a valid deletion fix-it.
         *
         * Never use replacement_storage[0] to decide presence.
         */
        suggestion->replacement =
            suggestion->has_replacement
            ? suggestion->replacement_storage
            : NULL;
    }

    if (diagnostic->has_contract) {
        vitte_diagnostic_contract_t *contract;

        contract =
            &diagnostic->contract;

        contract->expression =
            contract->expression_storage[0] != '\0'
            ? contract->expression_storage
            : NULL;

        contract->procedure =
            contract->procedure_storage[0] != '\0'
            ? contract->procedure_storage
            : NULL;

        contract->reason =
            contract->reason_storage[0] != '\0'
            ? contract->reason_storage
            : NULL;
    }
}

/* ========================================================================= */
/* Causal graph                                                              */
/* ========================================================================= */

static bool
vitte_diagnostic_engine_index_valid(
    size_t index,
    size_t count
)
{
    return index !=
               VITTE_DIAGNOSTIC_INDEX_NONE &&
           index < count;
}

static bool
vitte_diagnostic_engine_relations_valid(
    const vitte_diagnostic_bag_t *bag
)
{
    size_t index;

    if (bag == NULL) {
        return false;
    }

    for (index = 0u;
         index < bag->count;
         index++) {

        const vitte_diagnostic_t *diagnostic;
        size_t current;
        size_t steps;

        diagnostic =
            &bag->storage[index];

        if (diagnostic->is_cascade) {
            if (!vitte_diagnostic_engine_index_valid(
                    diagnostic->parent_diagnostic,
                    bag->count
                ) ||
                !vitte_diagnostic_engine_index_valid(
                    diagnostic->root_diagnostic,
                    bag->count
                )) {

                return false;
            }

            if (diagnostic->parent_diagnostic ==
                    index ||
                diagnostic->root_diagnostic ==
                    index) {

                return false;
            }
        }

        if (diagnostic->is_primary &&
            diagnostic->is_cascade) {

            return false;
        }

        if (diagnostic->is_primary &&
            diagnostic->root_diagnostic !=
                VITTE_DIAGNOSTIC_INDEX_NONE &&
            diagnostic->root_diagnostic !=
                index) {

            return false;
        }

        if (diagnostic->parent_diagnostic ==
            VITTE_DIAGNOSTIC_INDEX_NONE) {

            continue;
        }

        current = index;

        for (steps = 0u;
             steps <= bag->count;
             steps++) {

            const vitte_diagnostic_t *node;

            if (!vitte_diagnostic_engine_index_valid(
                    current,
                    bag->count
                )) {

                return false;
            }

            node =
                &bag->storage[current];

            if (node->parent_diagnostic ==
                VITTE_DIAGNOSTIC_INDEX_NONE) {

                break;
            }

            if (!vitte_diagnostic_engine_index_valid(
                    node->parent_diagnostic,
                    bag->count
                )) {

                return false;
            }

            if (node->parent_diagnostic ==
                current) {

                return false;
            }

            current =
                node->parent_diagnostic;

            if (current == index) {
                return false;
            }
        }

        if (steps > bag->count) {
            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* Deterministic sorting                                                     */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_engine_sort(
    vitte_diagnostic_engine_t *engine
)
{
    vitte_diagnostic_engine_sort_entry_t *entries;
    size_t *old_to_new;
    size_t count;
    size_t index;
    size_t old_current_parent;
    size_t old_current_root;

    if (!vitte_diagnostic_engine_valid(
            engine
        )) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    count =
        engine->bag->count;

    if (count < 2u) {
        return VITTE_STATUS_OK;
    }

    if (!vitte_diagnostic_engine_relations_valid(
            engine->bag
        )) {

        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    entries =
        (vitte_diagnostic_engine_sort_entry_t *)
        calloc(
            count,
            sizeof(*entries)
        );

    if (entries == NULL) {
        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }

    old_to_new =
        (size_t *)
        malloc(
            count *
            sizeof(*old_to_new)
        );

    if (old_to_new == NULL) {
        free(entries);

        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }

    old_current_parent =
        engine->current_parent;

    old_current_root =
        engine->current_root;

    for (index = 0u;
         index < count;
         index++) {

        entries[index].diagnostic =
            engine->bag->storage[index];

        entries[index].old_index =
            index;

        old_to_new[index] =
            VITTE_DIAGNOSTIC_INDEX_NONE;
    }

    qsort(
        entries,
        count,
        sizeof(entries[0]),
        vitte_diagnostic_engine_compare_sort_entries
    );

    /*
     * Construct an exact permutation.
     *
     * Unlike fingerprints, old_index is guaranteed unique.
     */
    for (index = 0u;
         index < count;
         index++) {

        size_t old_index;

        old_index =
            entries[index].old_index;

        if (old_index >= count ||
            old_to_new[old_index] !=
                VITTE_DIAGNOSTIC_INDEX_NONE) {

            free(old_to_new);
            free(entries);

            return VITTE_STATUS_ERROR_INVALID_STATE;
        }

        old_to_new[old_index] =
            index;
    }

    /*
     * Install sorted diagnostics.
     */
    for (index = 0u;
         index < count;
         index++) {

        engine->bag->storage[index] =
            entries[index].diagnostic;

        vitte_diagnostic_engine_rebind(
            &engine->bag->storage[index]
        );
    }

    /*
     * Remap all causal indices through the exact old->new permutation.
     */
    for (index = 0u;
         index < count;
         index++) {

        vitte_diagnostic_t *diagnostic;
        size_t old_index;
        size_t old_parent;
        size_t old_root;

        diagnostic =
            &engine->bag->storage[index];

        old_index =
            entries[index].old_index;

        old_parent =
            entries[index].
                diagnostic.
                parent_diagnostic;

        old_root =
            entries[index].
                diagnostic.
                root_diagnostic;

        if (old_parent !=
            VITTE_DIAGNOSTIC_INDEX_NONE) {

            if (old_parent >= count ||
                old_to_new[old_parent] ==
                    VITTE_DIAGNOSTIC_INDEX_NONE) {

                free(old_to_new);
                free(entries);

                return VITTE_STATUS_ERROR_INVALID_STATE;
            }

            diagnostic->parent_diagnostic =
                old_to_new[old_parent];

        } else {
            diagnostic->parent_diagnostic =
                VITTE_DIAGNOSTIC_INDEX_NONE;
        }

        if (old_root !=
            VITTE_DIAGNOSTIC_INDEX_NONE) {

            if (old_root >= count ||
                old_to_new[old_root] ==
                    VITTE_DIAGNOSTIC_INDEX_NONE) {

                free(old_to_new);
                free(entries);

                return VITTE_STATUS_ERROR_INVALID_STATE;
            }

            diagnostic->root_diagnostic =
                old_to_new[old_root];

        } else if (diagnostic->is_primary) {

            diagnostic->root_diagnostic =
                index;

        } else {
            diagnostic->root_diagnostic =
                VITTE_DIAGNOSTIC_INDEX_NONE;
        }

        if (diagnostic->is_primary) {
            diagnostic->parent_diagnostic =
                VITTE_DIAGNOSTIC_INDEX_NONE;

            diagnostic->root_diagnostic =
                index;

            diagnostic->is_cascade =
                false;
        }

        if (diagnostic->is_cascade) {
            diagnostic->is_primary =
                false;
        }

        (void)old_index;
    }

    /*
     * Remap engine causal context as well.
     */
    if (old_current_parent !=
        VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE) {

        if (old_current_parent >= count ||
            old_to_new[old_current_parent] ==
                VITTE_DIAGNOSTIC_INDEX_NONE) {

            engine->current_parent =
                VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE;

            engine->current_root =
                VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE;

        } else {
            engine->current_parent =
                old_to_new[old_current_parent];
        }

    } else {
        engine->current_parent =
            VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE;
    }

    if (old_current_root !=
        VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE) {

        if (old_current_root >= count ||
            old_to_new[old_current_root] ==
                VITTE_DIAGNOSTIC_INDEX_NONE) {

            engine->current_root =
                VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE;

        } else {
            engine->current_root =
                old_to_new[old_current_root];
        }

    } else {
        engine->current_root =
            VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE;
    }

    free(old_to_new);
    free(entries);

    if (!vitte_diagnostic_engine_relations_valid(
            engine->bag
        )) {

        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    vitte_diagnostic_engine_refresh_relation_statistics(
        engine
    );

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Finalization                                                              */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_engine_finalize(
    vitte_diagnostic_engine_t *engine
)
{
    vitte_status_t status;

    if (!vitte_diagnostic_engine_valid(
            engine
        )) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (engine->policy.classify_cascades) {
        status =
            vitte_diagnostic_classify_cascades(
                engine->bag,
                engine->policy.suppress_cascades
            );

        if (status != VITTE_STATUS_OK) {
            return status;
        }

        vitte_diagnostic_engine_refresh_relation_statistics(
            engine
        );
    }

    /*
     * Validate causal topology before any physical reorder.
     */
    if (!vitte_diagnostic_engine_relations_valid(
            engine->bag
        )) {

        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (engine->policy.sort_before_render) {
        status =
            vitte_diagnostic_engine_sort(
                engine
            );

        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }

    vitte_diagnostic_engine_refresh_relation_statistics(
        engine
    );

    return vitte_diagnostic_compilation_status(
        engine->bag
    );
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

void
vitte_diagnostic_engine_get_statistics(
    const vitte_diagnostic_engine_t *engine,
    vitte_diagnostic_engine_statistics_t *statistics
)
{
    if (statistics == NULL) {
        return;
    }

    memset(
        statistics,
        0,
        sizeof(*statistics)
    );

    if (!vitte_diagnostic_engine_valid(
            engine
        )) {
        return;
    }

    statistics->stored =
        engine->bag->count;

    statistics->emitted =
        engine->emitted_count;

    statistics->rejected =
        engine->rejected_count;

    statistics->cascade =
        vitte_diagnostic_cascade_count(
            engine->bag
        );

    statistics->primary =
        vitte_diagnostic_primary_count(
            engine->bag
        );

    statistics->suppressed =
        engine->bag->counts.suppressed_count;

    statistics->visible_errors =
        vitte_diagnostic_engine_visible_errors(
            engine->bag
        );

    statistics->visible_warnings =
        vitte_diagnostic_engine_visible_warnings(
            engine->bag
        );

    statistics->stopped =
        engine->stopped;
}

/* ========================================================================= */
/* Status                                                                    */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_engine_status(
    const vitte_diagnostic_engine_t *engine
)
{
    if (!vitte_diagnostic_engine_valid(
            engine
        )) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return vitte_diagnostic_compilation_status(
        engine->bag
    );
}