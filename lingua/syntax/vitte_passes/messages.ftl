## File: C:\Users\gogin\Documents\GitHub\vitte\lingua\syntax\vitte_passes\messages.ftl
## Vitte — passes pipeline diagnostics/messages (MAX)
##
## Conventions:
## - vitte-passes-<severity>-<topic>[-<detail>]
## - Variables: { $name }, { $path }, { $detail }, { $lhs }, { $rhs }
## - Passes occur after parse and before lowering/codegen.

# -----------------------------------------------------------------------------
# Generic / pipeline
# -----------------------------------------------------------------------------

vitte-passes-kind = passes

vitte-passes-note-stage =
    stage: { $detail }

vitte-passes-trace =
    trace: { $detail }

vitte-passes-e-internal =
    internal passes error: { $detail }

vitte-passes-help-report-bug =
    This may be a compiler bug. Re-run with `--trace-passes` and report the issue.

# -----------------------------------------------------------------------------
# Doc attachment
# -----------------------------------------------------------------------------

vitte-passes-w-doc-orphan =
    doc comment was not attached to any item

vitte-passes-w-doc-duplicate =
    duplicate doc comment; later one overrides earlier

# -----------------------------------------------------------------------------
# Macro expansion
# -----------------------------------------------------------------------------

vitte-passes-e-macros-failed =
    macro expansion failed

vitte-passes-e-macros-recursion-limit =
    macro recursion limit exceeded

vitte-passes-e-macros-step-limit =
    macro expansion step limit exceeded

vitte-passes-e-macros-cycle =
    macro expansion cycle detected

vitte-passes-e-macros-unknown =
    unknown macro `{ $name }`

vitte-passes-e-macros-invalid-output =
    macro expansion produced invalid output

vitte-passes-w-macros-large-output =
    macro expansion output is large ({ $detail })

vitte-passes-note-macros-trace =
    macro trace: { $detail }

# -----------------------------------------------------------------------------
# Name resolution / imports
# -----------------------------------------------------------------------------

vitte-passes-e-resolve-unresolved =
    unresolved name `{ $name }`

vitte-passes-e-resolve-unresolved-path =
    unresolved path `{ $path }`

vitte-passes-e-resolve-duplicate =
    duplicate definition `{ $name }`

vitte-passes-e-resolve-shadowing =
    name `{ $name }` shadows an existing definition

vitte-passes-w-resolve-unused-import =
    unused import `{ $path }`

vitte-passes-w-resolve-unused-share =
    unused shared item `{ $name }`

vitte-passes-e-resolve-invalid-space =
    invalid `space` for this module

# -----------------------------------------------------------------------------
# Desugaring
# -----------------------------------------------------------------------------

vitte-passes-e-desugar =
    desugaring failed: { $detail }

vitte-passes-e-desugar-unsupported =
    unsupported surface construct: { $detail }

# -----------------------------------------------------------------------------
# Type collection
# -----------------------------------------------------------------------------

vitte-passes-e-types-duplicate-form =
    duplicate `form` name `{ $name }`

vitte-passes-e-types-duplicate-pick =
    duplicate `pick` name `{ $name }`

vitte-passes-e-types-duplicate-bond =
    duplicate `bond` name `{ $name }`

vitte-passes-e-types-unknown-type =
    unknown type `{ $name }`

vitte-passes-e-types-cycle =
    type cycle detected: { $detail }

vitte-passes-w-types-unused =
    type `{ $name }` is declared but never used

# -----------------------------------------------------------------------------
# Validation / invariants
# -----------------------------------------------------------------------------

vitte-passes-e-validate =
    validation failed: { $detail }

vitte-passes-e-validate-missing-end =
    missing `.end` terminator (invariant violated)

vitte-passes-e-validate-duplicate-item =
    duplicate item `{ $name }`

vitte-passes-e-validate-invalid-foreign =
    invalid `foreign` usage in this context

vitte-passes-w-validate-unreachable =
    unreachable code detected

vitte-passes-w-validate-unused-var =
    unused variable `{ $name }`

vitte-passes-w-validate-unused-const =
    unused constant `{ $name }`

# -----------------------------------------------------------------------------
# Lints (optional)
# -----------------------------------------------------------------------------

vitte-passes-w-lint =
    lint: { $detail }

vitte-passes-w-lint-style =
    style: { $detail }

vitte-passes-w-lint-complexity =
    complexity: { $detail }
