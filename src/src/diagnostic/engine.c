#include "engine.h"

#include "cascade.h"
#include "diagnostic.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
 * Vitte diagnostic engine.
 *
 * The engine is the policy/orchestration layer above diagnostic.c.
 *
 * diagnostic.c:
 *   owns diagnostic data and primitive mutation.
 *
 * cascade.c:
 *   owns causal/cascade classification.
 *
 * engine.c:
 *   owns emission policy, checkpoints, limits, context propagation,
 *   deterministic ordering and compiler-facing orchestration.
 *
 * Design goals:
 *   - deterministic;
 *   - no hidden global mutable state;
 *   - conservative cascade suppression;
 *   - explicit compiler-phase provenance;
 *   - recoverable frontend operation;
 *   - suitable for terminal, JSON, SARIF and LSP consumers;
 *   - stable behavior under fuzzing and malformed input;
 *   - C17.
 */

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

#define VITTE_DIAGNOSTIC_ENGINE_DEFAULT_ERROR_LIMIT   ((size_t)100u)
#define VITTE_DIAGNOSTIC_ENGINE_DEFAULT_WARNING_LIMIT ((size_t)100u)

#define VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE \
    VITTE_DIAGNOSTIC_INDEX_NONE

/* ========================================================================= */
/* Internal helpers                                                          */
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
        length = capacity - 1u;
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

/* ========================================================================= */
/* Severity helpers                                                          */
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
    return severity ==
           VITTE_DIAGNOSTIC_WARNING;
}

/* ========================================================================= */
/* Visible counts                                                            */
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
/* Engine initialization                                                     */
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
        !vitte_diagnostic_bag_is_initialized(
            bag
        )) {
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

    engine->initialized = true;

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
    engine->cascade_count = 0u;
    engine->suppressed_count = 0u;
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
/* Explicit causal context                                                   */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_engine_set_parent(
    vitte_diagnostic_engine_t *engine,
    size_t parent_index
)
{
    if (!vitte_diagnostic_engine_valid(
            engine
        )) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (parent_index >=
        engine->bag->count) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    engine->current_parent =
        parent_index;

    engine->current_root =
        vitte_diagnostic_root_index(
            engine->bag,
            parent_index
        );

    if (engine->current_root ==
        VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE) {

        engine->current_parent =
            VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE;

        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

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

static void
vitte_diagnostic_engine_report_limit(
    vitte_diagnostic_engine_t *engine
)
{
    vitte_status_t status;
    vitte_diagnostic_t diagnostic;

    if (!vitte_diagnostic_engine_valid(
            engine
        ) ||
        engine->limit_reported ||
        !engine->policy.emit_limit_diagnostic) {
        return;
    }

    engine->limit_reported = true;

    vitte_diagnostic_init(&diagnostic);
    diagnostic.severity = VITTE_DIAGNOSTIC_FATAL;
    diagnostic.origin = VITTE_DIAGNOSTIC_ORIGIN_DRIVER;
    diagnostic.code =
        vitte_diagnostic_engine_copy_text(
            diagnostic.code_storage,
            sizeof(diagnostic.code_storage),
            VITTE_INFRA_E_LIMIT);
    diagnostic.message =
        vitte_diagnostic_engine_copy_text(
            diagnostic.message_storage,
            sizeof(diagnostic.message_storage),
            "diagnostic limit reached");
    diagnostic.details =
        vitte_diagnostic_engine_copy_text(
            diagnostic.details_storage,
            sizeof(diagnostic.details_storage),
            "compilation stopped after reaching the configured error limit");
    status = vitte_diagnostic_add(engine->bag, &diagnostic);

    (void)status;
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
    size_t count_before;
    vitte_diagnostic_t *diagnostic;
    vitte_diagnostic_t pending_diagnostic;

    if (!vitte_diagnostic_engine_valid(
            engine
        )) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (!vitte_diagnostic_severity_is_valid(
            severity
        ) ||
        !vitte_diagnostic_engine_text_valid(code) ||
        !vitte_diagnostic_engine_text_valid(message)) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (engine->stopped) {
        engine->rejected_count++;

        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    /*
     * Apply engine warning policy before diagnostic.c applies bag policy.
     */
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

        vitte_diagnostic_engine_report_limit(
            engine
        );

        engine->stopped = true;
        engine->rejected_count++;

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

    count_before = engine->bag->count;

    vitte_diagnostic_init(&pending_diagnostic);
    pending_diagnostic.severity = severity;
    pending_diagnostic.origin = engine->current_origin;
    pending_diagnostic.code =
        vitte_diagnostic_engine_copy_text(
            pending_diagnostic.code_storage,
            sizeof(pending_diagnostic.code_storage),
            code);
    pending_diagnostic.message =
        vitte_diagnostic_engine_copy_text(
            pending_diagnostic.message_storage,
            sizeof(pending_diagnostic.message_storage),
            message);
    pending_diagnostic.details =
        vitte_diagnostic_engine_copy_text(
            pending_diagnostic.details_storage,
            sizeof(pending_diagnostic.details_storage),
            details);

    if (span != NULL) {
        status = vitte_diagnostic_set_span(&pending_diagnostic, span);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }

    vitte_diagnostic_set_context(
        &pending_diagnostic,
        engine->package,
        engine->module,
        engine->procedure,
        NULL);

    status = vitte_diagnostic_add(engine->bag, &pending_diagnostic);

    if (status != VITTE_STATUS_OK) {
        return status;
    }

    /*
     * diagnostic.c may deduplicate an emission. If the bag did not grow,
     * there is no new diagnostic to decorate or classify.
     */
    if (engine->bag->count == count_before) {
        engine->suppressed_count++;

        return VITTE_STATUS_OK;
    }

    diagnostic =
        vitte_diagnostic_engine_last(
            engine
        );

    if (diagnostic == NULL) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    /* --------------------------------------------------------------------- */
    /* Compilation context                                                   */
    /* --------------------------------------------------------------------- */

    /* --------------------------------------------------------------------- */
    /* Explicit causal relationship                                          */
    /* --------------------------------------------------------------------- */

    if (engine->current_parent !=
        VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE) {

        status =
            vitte_diagnostic_set_parent(
                engine->bag,
                count_before,
                engine->current_parent
            );

        if (status != VITTE_STATUS_OK) {
            return status;
        }

        engine->cascade_count++;

        if (engine->policy.suppress_cascades) {
            status =
                vitte_diagnostic_mark_last_suppressed(
                    engine->bag
                );

            if (status != VITTE_STATUS_OK) {
                return status;
            }

            engine->suppressed_count++;
        }

    } else if (engine->policy.classify_cascades &&
               vitte_diagnostic_engine_is_error_severity(
                   diagnostic->severity
               )) {

        bool was_cascade;
        bool was_suppressed;

        was_cascade =
            diagnostic->is_cascade;

        was_suppressed =
            diagnostic->is_suppressed;

        status =
            vitte_diagnostic_classify_last_cascade(
                engine->bag,
                engine->policy.suppress_cascades
            );

        if (status != VITTE_STATUS_OK) {
            return status;
        }

        if (!was_cascade &&
            diagnostic->is_cascade) {
            engine->cascade_count++;
        }

        if (!was_suppressed &&
            diagnostic->is_suppressed) {
            engine->suppressed_count++;
        }
    }

    engine->emitted_count++;

    /* --------------------------------------------------------------------- */
    /* Fatal policy                                                          */
    /* --------------------------------------------------------------------- */

    if (diagnostic->severity ==
            VITTE_DIAGNOSTIC_FATAL &&
        engine->policy.stop_after_fatal) {

        engine->stopped = true;
    }

    if (engine->policy.stop_after_error_limit &&
        vitte_diagnostic_engine_visible_errors(
            engine->bag
        ) >= engine->policy.error_limit) {

        vitte_diagnostic_engine_report_limit(
            engine
        );

        engine->stopped = true;
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
/* Decoration convenience                                                    */
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

    diagnostic = vitte_diagnostic_engine_last(engine);
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

    diagnostic = vitte_diagnostic_engine_last(engine);
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

    diagnostic = vitte_diagnostic_engine_last(engine);
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

    diagnostic = vitte_diagnostic_engine_last(engine);
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

    diagnostic = vitte_diagnostic_engine_last(engine);
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

    diagnostic = vitte_diagnostic_engine_last(engine);
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

    diagnostic = vitte_diagnostic_engine_last(engine);
    if (diagnostic == NULL) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    vitte_diagnostic_set_subject(
        diagnostic,
        symbol,
        expected_type,
        actual_type);
    vitte_diagnostic_set_context(
        diagnostic,
        engine->package,
        engine->module,
        engine->procedure,
        context);
    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Contract diagnostics                                                      */
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

    diagnostic = vitte_diagnostic_engine_last(engine);
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
/* Checkpoints / transactions                                                */
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

    /*
     * Clear rolled-back diagnostics so stale pointers/data cannot leak into
     * debugging or future reuse.
     */
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
/* Deterministic ordering                                                    */
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

    return strcmp(left, right);
}

static int
vitte_diagnostic_engine_compare_diagnostics(
    const void *left_pointer,
    const void *right_pointer
)
{
    const vitte_diagnostic_t *left;
    const vitte_diagnostic_t *right;

    unsigned left_rank;
    unsigned right_rank;

    int comparison;

    left =
        (const vitte_diagnostic_t *)left_pointer;

    right =
        (const vitte_diagnostic_t *)right_pointer;

    /* Source identity. */
    if (left->source_id !=
            VITTE_SOURCE_ID_INVALID &&
        right->source_id !=
            VITTE_SOURCE_ID_INVALID &&
        left->source_id !=
            right->source_id) {

        return left->source_id <
               right->source_id
            ? -1
            : 1;
    }

    comparison =
        vitte_diagnostic_engine_compare_nullable_text(
            left->source_name,
            right->source_name
        );

    if (comparison != 0) {
        return comparison;
    }

    /* Source position. */
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

    /* Severity. */
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

    /* Compiler phase. */
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

    /* Stable public code. */
    comparison =
        vitte_diagnostic_engine_compare_nullable_text(
            left->short_code,
            right->short_code
        );

    if (comparison != 0) {
        return comparison;
    }

    /* Stable internal code. */
    comparison =
        vitte_diagnostic_engine_compare_nullable_text(
            left->code,
            right->code
        );

    if (comparison != 0) {
        return comparison;
    }

    /* Final deterministic tie breaker. */
    if (left->fingerprint !=
        right->fingerprint) {

        return left->fingerprint <
               right->fingerprint
            ? -1
            : 1;
    }

    return vitte_diagnostic_engine_compare_nullable_text(
        left->message,
        right->message
    );
}

/* ========================================================================= */
/* Pointer rebinding after sort                                              */
/* ========================================================================= */

/*
 * vitte_diagnostic_t owns strings in inline buffers. A struct move performed
 * by qsort copies pointer values verbatim, so all pointers into those inline
 * buffers must be rebound after sorting.
 */

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

        suggestion->replacement =
            suggestion->replacement_storage[0] != '\0'
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
/* Relation remapping after sort                                             */
/* ========================================================================= */

typedef struct vitte_diagnostic_relation_identity {
    uint64_t fingerprint;
    size_t old_index;
    size_t old_parent;
    size_t old_root;
} vitte_diagnostic_relation_identity_t;

static size_t
vitte_diagnostic_engine_find_fingerprint(
    const vitte_diagnostic_bag_t *bag,
    uint64_t fingerprint
)
{
    size_t index;

    if (bag == NULL) {
        return VITTE_DIAGNOSTIC_INDEX_NONE;
    }

    for (index = 0u;
         index < bag->count;
         index++) {

        if (bag->storage[index].fingerprint ==
            fingerprint) {
            return index;
        }
    }

    return VITTE_DIAGNOSTIC_INDEX_NONE;
}

vitte_status_t
vitte_diagnostic_engine_sort(
    vitte_diagnostic_engine_t *engine
)
{
    vitte_diagnostic_relation_identity_t *relations;
    size_t count;
    size_t index;

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

    relations =
        (vitte_diagnostic_relation_identity_t *)
        calloc(
            count,
            sizeof(*relations)
        );

    if (relations == NULL) {
        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }

    /*
     * Preserve causal relationships by fingerprint before changing physical
     * diagnostic indices.
     */
    for (index = 0u;
         index < count;
         index++) {

        const vitte_diagnostic_t *diagnostic;

        diagnostic =
            &engine->bag->storage[index];

        relations[index].fingerprint =
            diagnostic->fingerprint;

        relations[index].old_index =
            index;

        relations[index].old_parent =
            diagnostic->parent_diagnostic;

        relations[index].old_root =
            diagnostic->root_diagnostic;
    }

    qsort(
        engine->bag->storage,
        count,
        sizeof(engine->bag->storage[0]),
        vitte_diagnostic_engine_compare_diagnostics
    );

    /*
     * qsort moves inline buffers. Repair all self-referential pointers.
     */
    for (index = 0u;
         index < count;
         index++) {

        vitte_diagnostic_engine_rebind(
            &engine->bag->storage[index]
        );
    }

    /*
     * Reconstruct causal indices.
     *
     * Fingerprints are used as stable internal identities here. diagnostic.c
     * still performs full field comparison for deduplication.
     */
    for (index = 0u;
         index < count;
         index++) {

        vitte_diagnostic_t *diagnostic;
        size_t relation_index;

        diagnostic =
            &engine->bag->storage[index];

        relation_index =
            VITTE_DIAGNOSTIC_INDEX_NONE;

        {
            size_t search;

            for (search = 0u;
                 search < count;
                 search++) {

                if (relations[search].fingerprint ==
                    diagnostic->fingerprint) {

                    relation_index = search;
                    break;
                }
            }
        }

        if (relation_index ==
            VITTE_DIAGNOSTIC_INDEX_NONE) {

            diagnostic->parent_diagnostic =
                VITTE_DIAGNOSTIC_INDEX_NONE;

            diagnostic->root_diagnostic =
                index;

            diagnostic->is_primary = true;
            diagnostic->is_cascade = false;

            continue;
        }

        if (relations[relation_index].old_parent !=
                VITTE_DIAGNOSTIC_INDEX_NONE &&
            relations[relation_index].old_parent <
                count) {

            uint64_t parent_fingerprint;

            parent_fingerprint =
                relations[
                    relations[relation_index].old_parent
                ].fingerprint;

            diagnostic->parent_diagnostic =
                vitte_diagnostic_engine_find_fingerprint(
                    engine->bag,
                    parent_fingerprint
                );
        } else {
            diagnostic->parent_diagnostic =
                VITTE_DIAGNOSTIC_INDEX_NONE;
        }

        if (relations[relation_index].old_root !=
                VITTE_DIAGNOSTIC_INDEX_NONE &&
            relations[relation_index].old_root <
                count) {

            uint64_t root_fingerprint;

            root_fingerprint =
                relations[
                    relations[relation_index].old_root
                ].fingerprint;

            diagnostic->root_diagnostic =
                vitte_diagnostic_engine_find_fingerprint(
                    engine->bag,
                    root_fingerprint
                );
        } else {
            diagnostic->root_diagnostic =
                index;
        }

        if (diagnostic->root_diagnostic ==
            VITTE_DIAGNOSTIC_INDEX_NONE) {

            diagnostic->root_diagnostic =
                index;

            diagnostic->parent_diagnostic =
                VITTE_DIAGNOSTIC_INDEX_NONE;

            diagnostic->is_primary = true;
            diagnostic->is_cascade = false;
        }
    }

    free(relations);

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
