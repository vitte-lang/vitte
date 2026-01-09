## File: C:\Users\gogin\Documents\GitHub\vitte\lingua\syntax\vitte_expand\messages.ftl
## Vitte — expand subsystem diagnostics (mbe + token-tree expansion)
##
## Conventions:
## - Keys are stable and namespaced: vitte-expand-*
## - Placeholders use Fluent variables: { $name }, { $rule }, { $expected }, { $got }, { $detail }
## - Offsets refer to byte offsets unless stated otherwise.

# -----------------------------------------------------------------------------
# Generic / plumbing
# -----------------------------------------------------------------------------

vitte-expand-kind = expander
vitte-expand-note-trace-enabled = trace: macro expansion tracing enabled
vitte-expand-help-report-bug = This may be a compiler bug. Re-run with `--trace-macros` and report the issue.

# -----------------------------------------------------------------------------
# Rule parsing / compilation
# -----------------------------------------------------------------------------

vitte-expand-e-no-rules =
    no macro rules provided

vitte-expand-e-rules-invalid =
    macro rules are invalid

vitte-expand-e-parse-rules =
    failed to parse macro rules for `{ $name }`

vitte-expand-e-parse-pattern =
    failed to parse macro pattern for `{ $name }`

vitte-expand-e-parse-template =
    failed to parse macro template for `{ $name }`

vitte-expand-e-invalid-metavar =
    invalid metavariable in `{ $name }`

vitte-expand-e-invalid-metavar-name =
    invalid metavariable name in `{ $name }`

vitte-expand-e-invalid-frag =
    invalid fragment specifier `{ $frag }` in `{ $name }`

vitte-expand-e-invalid-repetition =
    invalid repetition in `{ $name }`

vitte-expand-e-invalid-repetition-op =
    invalid repetition operator in `{ $name }`: expected `*`, `+`, or `?`

vitte-expand-e-repetition-no-progress =
    repetition would not make progress in `{ $name }` (empty body)

# -----------------------------------------------------------------------------
# Matching
# -----------------------------------------------------------------------------

vitte-expand-e-no-rules-matched =
    no rules matched macro invocation for `{ $name }`

vitte-expand-e-unexpected-eof =
    unexpected end of input while matching `{ $name }`

vitte-expand-e-unexpected-token =
    unexpected token while matching `{ $name }`: expected { $expected }, got { $got }

vitte-expand-note-expected =
    expected: { $expected }

vitte-expand-note-got =
    got: { $got }

vitte-expand-e-metavar-mismatch =
    metavariable `{ $var }` matched inconsistently in `{ $name }`

vitte-expand-e-repetition-mismatch =
    repetition mismatch in `{ $name }`

vitte-expand-e-repeat-empty =
    repetition `+` requires at least one match in `{ $name }`

vitte-expand-e-repeat-optional-mismatch =
    optional repetition `?` matched more than once in `{ $name }`

# -----------------------------------------------------------------------------
# Transcription
# -----------------------------------------------------------------------------

vitte-expand-e-unknown-metavar =
    unknown metavariable `{ $var }` referenced in template for `{ $name }`

vitte-expand-e-repetition-index-oob =
    repetition index out of bounds while expanding `{ $name }`

vitte-expand-e-transcribe-failed =
    failed to transcribe macro expansion for `{ $name }`

# -----------------------------------------------------------------------------
# Trace (debug)
# -----------------------------------------------------------------------------

vitte-expand-trace-enter =
    trace: expanding `{ $name }` (rules={ $rules })

vitte-expand-trace-try-rule =
    trace: trying rule #{ $rule }

vitte-expand-trace-rule-matched =
    trace: rule #{ $rule } matched

vitte-expand-trace-rule-failed =
    trace: rule #{ $rule } failed

vitte-expand-trace-exit =
    trace: expansion finished for `{ $name }`

vitte-expand-trace-repeat-start =
    trace: entering repetition

vitte-expand-trace-repeat-stop =
    trace: leaving repetition (iters={ $iters })
