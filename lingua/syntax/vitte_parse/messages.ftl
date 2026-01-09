## File: C:\Users\gogin\Documents\GitHub\vitte\lingua\syntax\vitte_parse\messages.ftl
## Vitte — parser diagnostics/messages (MAX)
##
## Key conventions:
## - vitte-parse-<severity>-<topic>[-<detail>]
## - Use variables for dynamic parts: { $expected }, { $got }, { $name }, { $path }, { $detail }
## - Parser is "surface, compiler-friendly" (vitte vit). Blocks end with ".end".

# -----------------------------------------------------------------------------
# Generic
# -----------------------------------------------------------------------------

vitte-parse-kind = parser

vitte-parse-note-recovery = parser recovery: skipping tokens to resume parsing
vitte-parse-help-report-bug = This may be a compiler bug. Re-run with `--trace-parse` and report the issue.

# -----------------------------------------------------------------------------
# Expected / got
# -----------------------------------------------------------------------------

vitte-parse-e-expected =
    expected { $expected }, got { $got }

vitte-parse-e-expected-eof =
    expected end of file, got { $got }

vitte-parse-e-unexpected-token =
    unexpected token: { $got }

vitte-parse-note-expected =
    expected: { $expected }

vitte-parse-note-got =
    got: { $got }

# -----------------------------------------------------------------------------
# File / header
# -----------------------------------------------------------------------------

vitte-parse-e-header =
    invalid file header: expected `vitte` [version]

vitte-parse-e-version =
    invalid version string `{ $got }`

# -----------------------------------------------------------------------------
# Top-level
# -----------------------------------------------------------------------------

vitte-parse-e-toplevel =
    invalid top-level item: { $got }

vitte-parse-e-space-decl =
    invalid `space` declaration

vitte-parse-e-pull-decl =
    invalid `pull` declaration

vitte-parse-e-share-decl =
    invalid `share` declaration

vitte-parse-e-build-decl =
    invalid `build` declaration

vitte-parse-e-global-decl =
    invalid global declaration (expected `const` or `var`)

# -----------------------------------------------------------------------------
# Paths / identifiers
# -----------------------------------------------------------------------------

vitte-parse-e-ident =
    expected identifier, got { $got }

vitte-parse-e-path =
    expected path, got { $got }

vitte-parse-e-ident-list =
    expected identifier list

# -----------------------------------------------------------------------------
# Types
# -----------------------------------------------------------------------------

vitte-parse-e-type-decl =
    invalid type declaration (expected `form`, `pick`, or `bond`)

vitte-parse-e-form-decl =
    invalid `form` declaration

vitte-parse-e-form-field =
    invalid `field` declaration inside `form`

vitte-parse-e-pick-decl =
    invalid `pick` declaration

vitte-parse-e-pick-case =
    invalid `case` declaration inside `pick`

vitte-parse-e-bond-decl =
    invalid `bond` declaration

vitte-parse-e-type-expr =
    invalid type expression

vitte-parse-e-type-args =
    invalid type arguments list

vitte-parse-e-type-pack =
    invalid `Pack of ...` type expression

vitte-parse-e-type-map =
    invalid `Map of ... to ...` type expression

# -----------------------------------------------------------------------------
# Runnables
# -----------------------------------------------------------------------------

vitte-parse-e-proc-decl =
    invalid `proc` declaration

vitte-parse-e-flow-decl =
    invalid `flow` declaration

vitte-parse-e-entry-decl =
    invalid `entry` declaration

vitte-parse-e-entry-kind =
    invalid entry kind (expected app/service/tool/pipeline/driver/kernel)

vitte-parse-e-params =
    invalid parameter list

vitte-parse-e-param =
    invalid parameter

vitte-parse-e-ret-sig =
    invalid return signature (expected `gives <type>`)

vitte-parse-e-mark =
    invalid `mark` attribute

# -----------------------------------------------------------------------------
# Blocks / statements
# -----------------------------------------------------------------------------

vitte-parse-e-block =
    invalid block

vitte-parse-e-block-missing-end =
    missing `.end` terminator for block

vitte-parse-e-stmt =
    invalid statement: { $got }

vitte-parse-e-make =
    invalid `make` statement

vitte-parse-e-keep =
    invalid `keep` statement

vitte-parse-e-set =
    invalid `set` statement

vitte-parse-e-if =
    invalid `if` statement

vitte-parse-e-elif =
    invalid `elif` branch

vitte-parse-e-else =
    invalid `else` branch

vitte-parse-e-loop =
    invalid `loop` statement

vitte-parse-e-each =
    invalid `each` statement

vitte-parse-e-select =
    invalid `select` statement

vitte-parse-e-when =
    invalid `when` arm

vitte-parse-e-otherwise =
    invalid `otherwise` arm

vitte-parse-e-give =
    invalid `give` statement

vitte-parse-e-emit =
    invalid `emit` statement

vitte-parse-e-defer =
    invalid `defer` statement

vitte-parse-e-assert =
    invalid `assert` statement

vitte-parse-e-foreign =
    invalid `foreign` statement

vitte-parse-e-halt =
    invalid `halt` statement

vitte-parse-e-next =
    invalid `next` statement

# -----------------------------------------------------------------------------
# Patterns (select)
# -----------------------------------------------------------------------------

vitte-parse-e-pattern =
    invalid pattern in `when` arm

vitte-parse-e-variant-pattern =
    invalid variant pattern (expected Path.Case(...))

# -----------------------------------------------------------------------------
# Expressions
# -----------------------------------------------------------------------------

vitte-parse-e-expr =
    invalid expression

vitte-parse-e-primary =
    invalid primary expression

vitte-parse-e-call =
    invalid call expression

vitte-parse-e-index =
    invalid index expression

vitte-parse-e-dot =
    invalid member access

vitte-parse-e-args =
    invalid argument list

vitte-parse-e-named-arg =
    invalid named argument (expected name = expr)

# -----------------------------------------------------------------------------
# Literals
# -----------------------------------------------------------------------------

vitte-parse-e-int =
    invalid integer literal `{ $got }`

vitte-parse-e-float =
    invalid float literal `{ $got }`

vitte-parse-e-string =
    invalid string literal

# -----------------------------------------------------------------------------
# Doc zones / doc lines
# -----------------------------------------------------------------------------

vitte-parse-e-doc-zone =
    invalid doc zone (expected `<<<` ... `>>>`)

vitte-parse-e-doc-line =
    invalid doc line

# -----------------------------------------------------------------------------
# Trace (debug)
# -----------------------------------------------------------------------------

vitte-parse-trace-enter =
    trace: enter { $rule }

vitte-parse-trace-exit =
    trace: exit { $rule } (ok={ $ok })

vitte-parse-trace-token =
    trace: token { $got }
