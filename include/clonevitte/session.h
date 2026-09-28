/*
 * CloneVitte
 * Compilation Session API
 *
 * Copyright (C) 2026 VitteFoundation
 * SPDX-License-Identifier: LicenseRef-VFPL-1.0
 */

#ifndef CLONEVITTE_SESSION_H
#define CLONEVITTE_SESSION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <clonevitte/config.h>
#include <clonevitte/diagnostic.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Opaque types
 * ============================================================ */

typedef struct CloneVitteSession CloneVitteSession;
typedef struct CloneVitteSource CloneVitteSource;
typedef struct CloneVitteModule CloneVitteModule;

typedef struct CloneVitteAst CloneVitteAst;
typedef struct CloneVitteHir CloneVitteHir;
typedef struct CloneVitteMir CloneVitteMir;
typedef struct CloneVitteIr CloneVitteIr;

/* ============================================================
 * Session status
 * ============================================================ */

typedef enum CloneVitteSessionStatus {
    CLONEVITTE_SESSION_OK = 0,

    CLONEVITTE_SESSION_INVALID_ARGUMENT,
    CLONEVITTE_SESSION_OUT_OF_MEMORY,
    CLONEVITTE_SESSION_IO_ERROR,

    CLONEVITTE_SESSION_SOURCE_ERROR,
    CLONEVITTE_SESSION_LEX_ERROR,
    CLONEVITTE_SESSION_PARSE_ERROR,
    CLONEVITTE_SESSION_SEMANTIC_ERROR,

    CLONEVITTE_SESSION_HIR_ERROR,
    CLONEVITTE_SESSION_MIR_ERROR,
    CLONEVITTE_SESSION_IR_ERROR,

    CLONEVITTE_SESSION_CODEGEN_ERROR,
    CLONEVITTE_SESSION_LINK_ERROR,

    CLONEVITTE_SESSION_ABORTED,
    CLONEVITTE_SESSION_INTERNAL_ERROR
} CloneVitteSessionStatus;

/* ============================================================
 * Compilation phase
 * ============================================================ */

typedef enum CloneVittePhase {
    CLONEVITTE_PHASE_NONE = 0,

    CLONEVITTE_PHASE_INITIALIZATION,
    CLONEVITTE_PHASE_SOURCE,
    CLONEVITTE_PHASE_LEX,
    CLONEVITTE_PHASE_PARSE,
    CLONEVITTE_PHASE_RESOLVE,
    CLONEVITTE_PHASE_TYPECHECK,
    CLONEVITTE_PHASE_SEMA,

    CLONEVITTE_PHASE_HIR,
    CLONEVITTE_PHASE_MIR,
    CLONEVITTE_PHASE_IR,

    CLONEVITTE_PHASE_OPTIMIZE,
    CLONEVITTE_PHASE_CODEGEN,
    CLONEVITTE_PHASE_LINK,

    CLONEVITTE_PHASE_FINISHED
} CloneVittePhase;

/* ============================================================
 * Session state
 * ============================================================ */

typedef enum CloneVitteSessionState {
    CLONEVITTE_SESSION_STATE_CREATED = 0,
    CLONEVITTE_SESSION_STATE_READY,
    CLONEVITTE_SESSION_STATE_RUNNING,
    CLONEVITTE_SESSION_STATE_FAILED,
    CLONEVITTE_SESSION_STATE_FINISHED,
    CLONEVITTE_SESSION_STATE_DESTROYED
} CloneVitteSessionState;

/* ============================================================
 * Source identifier
 * ============================================================ */

typedef uint32_t CloneVitteSourceId;

#define CLONEVITTE_SOURCE_ID_INVALID \
    ((CloneVitteSourceId)UINT32_MAX)

/* ============================================================
 * Module identifier
 * ============================================================ */

typedef uint32_t CloneVitteModuleId;

#define CLONEVITTE_MODULE_ID_INVALID \
    ((CloneVitteModuleId)UINT32_MAX)

/* ============================================================
 * Session statistics
 * ============================================================ */

typedef struct CloneVitteSessionStats {
    size_t source_count;
    size_t module_count;

    size_t source_bytes;

    size_t token_count;
    size_t ast_node_count;

    size_t hir_node_count;
    size_t mir_node_count;
    size_t ir_node_count;

    uint32_t note_count;
    uint32_t help_count;
    uint32_t warning_count;
    uint32_t error_count;
    uint32_t fatal_count;

    uint64_t source_time_ns;
    uint64_t lex_time_ns;
    uint64_t parse_time_ns;
    uint64_t resolve_time_ns;
    uint64_t typecheck_time_ns;
    uint64_t sema_time_ns;

    uint64_t hir_time_ns;
    uint64_t mir_time_ns;
    uint64_t ir_time_ns;

    uint64_t optimize_time_ns;
    uint64_t codegen_time_ns;
    uint64_t link_time_ns;

    uint64_t total_time_ns;

    size_t peak_memory_bytes;
} CloneVitteSessionStats;

/* ============================================================
 * Compilation result
 * ============================================================ */

typedef struct CloneVitteSessionResult {
    CloneVitteSessionStatus status;
    CloneVittePhase failed_phase;

    uint32_t warning_count;
    uint32_t error_count;

    const char *output_path;

    bool emitted_output;
} CloneVitteSessionResult;

/* ============================================================
 * Phase callback
 * ============================================================ */

typedef void (*CloneVittePhaseCallback)(
    CloneVitteSession *session,
    CloneVittePhase phase,
    void *userdata
);

/* ============================================================
 * Session options
 * ============================================================ */

typedef struct CloneVitteSessionOptions {
    CloneVittePhase stop_after;

    bool collect_statistics;
    bool keep_ast;
    bool keep_hir;
    bool keep_mir;
    bool keep_ir;

    CloneVittePhaseCallback phase_callback;
    void *phase_userdata;
} CloneVitteSessionOptions;

/* ============================================================
 * Default options
 * ============================================================ */

CloneVitteSessionOptions
clonevitte_session_options_default(void);

/* ============================================================
 * Session lifecycle
 * ============================================================ */

CloneVitteSession *
clonevitte_session_create(
    const CloneVitteConfig *config
);

CloneVitteSession *
clonevitte_session_create_with_options(
    const CloneVitteConfig *config,
    const CloneVitteSessionOptions *options
);

void
clonevitte_session_destroy(
    CloneVitteSession *session
);

/* ============================================================
 * Session reset
 * ============================================================ */

bool
clonevitte_session_reset(
    CloneVitteSession *session
);

/* ============================================================
 * Configuration
 * ============================================================ */

const CloneVitteConfig *
clonevitte_session_config(
    const CloneVitteSession *session
);

bool
clonevitte_session_set_config(
    CloneVitteSession *session,
    const CloneVitteConfig *config
);

/* ============================================================
 * Session state
 * ============================================================ */

CloneVitteSessionState
clonevitte_session_state(
    const CloneVitteSession *session
);

CloneVittePhase
clonevitte_session_phase(
    const CloneVitteSession *session
);

bool
clonevitte_session_failed(
    const CloneVitteSession *session
);

bool
clonevitte_session_finished(
    const CloneVitteSession *session
);

/* ============================================================
 * Source registration
 * ============================================================ */

CloneVitteSessionStatus
clonevitte_session_add_file(
    CloneVitteSession *session,
    const char *path,
    CloneVitteSourceId *out_source
);

CloneVitteSessionStatus
clonevitte_session_add_source(
    CloneVitteSession *session,
    const char *name,
    const char *data,
    size_t size,
    CloneVitteSourceId *out_source
);

/* ============================================================
 * Source queries
 * ============================================================ */

size_t
clonevitte_session_source_count(
    const CloneVitteSession *session
);

const CloneVitteSource *
clonevitte_session_source(
    const CloneVitteSession *session,
    CloneVitteSourceId source
);

const char *
clonevitte_session_source_name(
    const CloneVitteSession *session,
    CloneVitteSourceId source
);

/* ============================================================
 * Module queries
 * ============================================================ */

size_t
clonevitte_session_module_count(
    const CloneVitteSession *session
);

const CloneVitteModule *
clonevitte_session_module(
    const CloneVitteSession *session,
    CloneVitteModuleId module
);

/* ============================================================
 * Frontend phases
 * ============================================================ */

CloneVitteSessionStatus
clonevitte_session_lex(
    CloneVitteSession *session
);

CloneVitteSessionStatus
clonevitte_session_parse(
    CloneVitteSession *session
);

CloneVitteSessionStatus
clonevitte_session_resolve(
    CloneVitteSession *session
);

CloneVitteSessionStatus
clonevitte_session_typecheck(
    CloneVitteSession *session
);

CloneVitteSessionStatus
clonevitte_session_analyze(
    CloneVitteSession *session
);

/* ============================================================
 * Lowering
 * ============================================================ */

CloneVitteSessionStatus
clonevitte_session_build_hir(
    CloneVitteSession *session
);

CloneVitteSessionStatus
clonevitte_session_build_mir(
    CloneVitteSession *session
);

CloneVitteSessionStatus
clonevitte_session_build_ir(
    CloneVitteSession *session
);

/* ============================================================
 * Optimization
 * ============================================================ */

CloneVitteSessionStatus
clonevitte_session_optimize(
    CloneVitteSession *session
);

/* ============================================================
 * Backend
 * ============================================================ */

CloneVitteSessionStatus
clonevitte_session_codegen(
    CloneVitteSession *session
);

CloneVitteSessionStatus
clonevitte_session_link(
    CloneVitteSession *session
);

/* ============================================================
 * Full compilation
 * ============================================================ */

CloneVitteSessionResult
clonevitte_session_compile(
    CloneVitteSession *session
);

CloneVitteSessionResult
clonevitte_session_compile_file(
    CloneVitteSession *session,
    const char *path
);

CloneVitteSessionResult
clonevitte_session_compile_source(
    CloneVitteSession *session,
    const char *name,
    const char *source,
    size_t source_size
);

/* ============================================================
 * Intermediate representation access
 * ============================================================ */

const CloneVitteAst *
clonevitte_session_ast(
    const CloneVitteSession *session
);

const CloneVitteHir *
clonevitte_session_hir(
    const CloneVitteSession *session
);

const CloneVitteMir *
clonevitte_session_mir(
    const CloneVitteSession *session
);

const CloneVitteIr *
clonevitte_session_ir(
    const CloneVitteSession *session
);

/* ============================================================
 * Diagnostics
 * ============================================================ */

size_t
clonevitte_session_diagnostic_count(
    const CloneVitteSession *session
);

const CloneVitteDiagnostic *
clonevitte_session_diagnostic(
    const CloneVitteSession *session,
    size_t index
);

uint32_t
clonevitte_session_error_count(
    const CloneVitteSession *session
);

uint32_t
clonevitte_session_warning_count(
    const CloneVitteSession *session
);

bool
clonevitte_session_has_errors(
    const CloneVitteSession *session
);

/* ============================================================
 * Statistics
 * ============================================================ */

const CloneVitteSessionStats *
clonevitte_session_stats(
    const CloneVitteSession *session
);

/* ============================================================
 * Cancellation
 * ============================================================ */

void
clonevitte_session_cancel(
    CloneVitteSession *session
);

bool
clonevitte_session_cancelled(
    const CloneVitteSession *session
);

/* ============================================================
 * Status helpers
 * ============================================================ */

const char *
clonevitte_session_status_name(
    CloneVitteSessionStatus status
);

const char *
clonevitte_phase_name(
    CloneVittePhase phase
);

const char *
clonevitte_session_state_name(
    CloneVitteSessionState state
);

#ifdef __cplusplus
}
#endif

#endif /* CLONEVITTE_SESSION_H */