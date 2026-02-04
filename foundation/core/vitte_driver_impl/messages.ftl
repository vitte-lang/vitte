# ============================================================
# vitte_driver_impl — diagnostics messages
# ============================================================

# ------------------------------------------------------------
# General / Internal
# ------------------------------------------------------------

driver-impl-internal-error =
    internal error in driver implementation: { $detail }

driver-impl-unimplemented =
    feature not implemented in driver implementation: { $feature }

driver-impl-invalid-state =
    invalid driver state: { $state }

# ------------------------------------------------------------
# Pipeline
# ------------------------------------------------------------

pipeline-start =
    starting compilation pipeline

pipeline-step-start =
    running pipeline step: { $step }

pipeline-step-success =
    pipeline step succeeded: { $step }

pipeline-step-failed =
    pipeline step failed: { $step }

pipeline-aborted =
    compilation aborted due to previous errors

pipeline-finished =
    compilation finished successfully

# ------------------------------------------------------------
# Cache / Incremental
# ------------------------------------------------------------

cache-init =
    initializing incremental cache at { $path }

cache-load-failed =
    failed to load incremental cache index: { $path }

cache-save-failed =
    failed to save incremental cache index: { $path }

cache-hit =
    incremental cache hit for { $key }

cache-miss =
    incremental cache miss for { $key }

cache-invalidated =
    incremental cache invalidated for { $key }

cache-fingerprint-failed =
    failed to compute fingerprint for { $path }

# ------------------------------------------------------------
# Codegen
# ------------------------------------------------------------

codegen-start =
    starting code generation

codegen-finished =
    code generation finished

codegen-failed =
    code generation failed: { $reason }

codegen-backend-not-found =
    codegen backend not found: { $backend }

# ------------------------------------------------------------
# Linking
# ------------------------------------------------------------

link-start =
    linking objects into final binary

link-no-objects =
    no object files available for linking

linker-not-found =
    linker executable not found: { $linker }

link-unsupported-target =
    linker does not support target: { $target }

link-failed =
    linking failed (exit code { $code })

link-success =
    linking finished successfully

# ------------------------------------------------------------
# LLD specific
# ------------------------------------------------------------

lld-selected =
    using LLVM lld linker ({ $flavor })

lld-command =
    lld command: { $command }

lld-failed =
    lld invocation failed: { $stderr }

# ------------------------------------------------------------
# Runtime / Run
# ------------------------------------------------------------

run-start =
    running program: { $program }

run-dry =
    dry-run enabled, program not executed

run-exit-code =
    program exited with code { $code }

run-failed =
    program execution failed: { $reason }

binary-not-found =
    output binary not found at { $path }

# ------------------------------------------------------------
# Logging
# ------------------------------------------------------------

log-init =
    logger initialized (level = { $level })

log-disabled =
    logging disabled

# ------------------------------------------------------------
# Tests
# ------------------------------------------------------------

test-start =
    running driver implementation smoke tests

test-success =
    all driver implementation tests passed

test-failed =
    driver implementation tests failed

# ------------------------------------------------------------
# Notes
# ------------------------------------------------------------
#
# - Messages in this file are specific to vitte_driver_impl.
# - They are referenced by error / warning / note codes.
# - This file is:
#     * localisable
#     * deterministic
#     * safe for bootstrap
#
# - Naming convention:
#     <subsystem>-<action>-<detail>
#
