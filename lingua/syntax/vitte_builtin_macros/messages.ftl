## Vitte — Builtin Macros (diagnostics/messages)
## File: lingua/syntax/vitte_builtin_macros/messages.ftl
##
## Notes:
## - This bundle is intended for compiler diagnostics (errors/warnings/notes/help).
## - Placeholders use Fluent syntax: { $name }, { $expected }, etc.
## - Keep identifiers stable; add new keys instead of renaming when possible.

# -----------------------------------------------------------------------------
# Shared / generic
# -----------------------------------------------------------------------------

vitte-macro-kind-builtin = builtin macro
vitte-macro-kind-user = user macro

vitte-macro-span-here = here
vitte-macro-span-invocation = macro invocation
vitte-macro-span-expansion = macro expansion

vitte-macro-note-expands-to = expands to: { $snippet }
vitte-macro-note-defined-at = defined at: { $path }:{ $line }:{ $col }

vitte-macro-help-see-docs = See documentation: { $topic }

vitte-macro-error-prefix = error
vitte-macro-warning-prefix = warning
vitte-macro-note-prefix = note
vitte-macro-help-prefix = help

# -----------------------------------------------------------------------------
# Registry / resolution
# -----------------------------------------------------------------------------

vitte-macro-e-unknown-builtin =
    unknown builtin macro `{ $name }`

vitte-macro-help-list-builtins =
    Available builtin macros: { $list }

vitte-macro-e-ambiguous-macro =
    ambiguous macro name `{ $name }`

vitte-macro-note-ambiguous-candidates =
    candidates: { $candidates }

# -----------------------------------------------------------------------------
# Invocation parsing / structure
# -----------------------------------------------------------------------------

vitte-macro-e-invalid-invocation =
    invalid macro invocation syntax for `{ $name }`

vitte-macro-help-invocation-shape-paren =
    Expected `{ $name }( ... )`.

vitte-macro-help-invocation-shape-bracket =
    Expected `{ $name }[ ... ]`.

vitte-macro-help-invocation-shape-bang =
    Expected `{ $name }! ...`.

vitte-macro-e-unexpected-token =
    unexpected token `{ $token }` in `{ $name }` invocation

vitte-macro-note-expected-token =
    expected: { $expected }

vitte-macro-e-missing-arg =
    missing argument `{ $arg }` in `{ $name }`

vitte-macro-e-extra-arg =
    unexpected extra argument in `{ $name }`: { $arg }

vitte-macro-e-arg-count =
    wrong number of arguments for `{ $name }`: expected { $expected }, got { $got }

vitte-macro-help-arg-count-range =
    `{ $name }` expects between { $min } and { $max } arguments.

vitte-macro-e-arg-name-unknown =
    unknown named argument `{ $arg }` for `{ $name }`

vitte-macro-note-arg-name-known =
    known named arguments: { $list }

vitte-macro-e-arg-duplicate =
    duplicate argument `{ $arg }` in `{ $name }`

vitte-macro-e-arg-missing-value =
    missing value for argument `{ $arg }` in `{ $name }`

vitte-macro-e-arg-unexpected-value =
    argument `{ $arg }` does not take a value in `{ $name }`

# -----------------------------------------------------------------------------
# Argument typing / coercions
# -----------------------------------------------------------------------------

vitte-macro-e-arg-type =
    invalid type for `{ $arg }` in `{ $name }`: expected { $expected }, got { $got }

vitte-macro-help-arg-type =
    Provide a value of type { $expected }.

vitte-macro-e-arg-literal-required =
    `{ $name }` requires `{ $arg }` to be a literal

vitte-macro-help-arg-literal-required =
    Use a literal value (string/number/bool), not an expression.

vitte-macro-e-arg-const-required =
    `{ $name }` requires `{ $arg }` to be a compile-time constant

vitte-macro-help-arg-const-required =
    Replace the expression with a constant expression.

vitte-macro-e-arg-nonempty-string =
    `{ $name }` requires `{ $arg }` to be a non-empty string

vitte-macro-e-arg-path-invalid =
    invalid path in `{ $name }` for `{ $arg }`: `{ $value }`

vitte-macro-e-arg-ident-invalid =
    invalid identifier in `{ $name }` for `{ $arg }`: `{ $value }`

vitte-macro-e-arg-int-range =
    integer out of range for `{ $name }` `{ $arg }`: expected { $min }..{ $max }, got { $got }

# -----------------------------------------------------------------------------
# Expansion / hygiene / context
# -----------------------------------------------------------------------------

vitte-macro-e-expand-failed =
    failed to expand builtin macro `{ $name }`

vitte-macro-note-expand-reason =
    reason: { $reason }

vitte-macro-help-report-bug =
    This may be a compiler bug. Re-run with `--trace-macros` and report the issue.

vitte-macro-e-context-not-allowed =
    `{ $name }` is not allowed in this context ({ $context })

vitte-macro-help-context-not-allowed =
    Move `{ $name }` to a supported context: { $allowed }.

vitte-macro-e-requires-unsafe =
    `{ $name }` requires an unsafe context

vitte-macro-help-requires-unsafe =
    Wrap the call in an unsafe block or enable the required capability.

vitte-macro-e-hygiene-capture =
    macro `{ $name }` would capture identifier `{ $ident }`

vitte-macro-help-hygiene-capture =
    Rename the local symbol or use a fully-qualified path.

# -----------------------------------------------------------------------------
# Builtin: emit / print-like
# -----------------------------------------------------------------------------

vitte-macro-e-emit-format-invalid =
    invalid format string in `{ $name }`: { $detail }

vitte-macro-note-emit-format-span =
    format error at byte offset { $offset }

vitte-macro-help-emit-format-escape =
    Escape braces using `{{` and `}}`.

vitte-macro-e-emit-placeholder-mismatch =
    format placeholders mismatch in `{ $name }`: expected { $expected }, got { $got }

vitte-macro-help-emit-placeholder-mismatch =
    Ensure the number of `{}` placeholders matches the provided arguments.

# -----------------------------------------------------------------------------
# Builtin: assert / debug_assert-like
# -----------------------------------------------------------------------------

vitte-macro-e-assert-missing-condition =
    `{ $name }` requires a condition expression

vitte-macro-help-assert-missing-condition =
    Example: `{ $name }(x > 0, "x must be positive")`

vitte-macro-note-assert-disabled =
    `{ $name }` is disabled in this build/profile

# -----------------------------------------------------------------------------
# Builtin: include / embed-like
# -----------------------------------------------------------------------------

vitte-macro-e-include-not-found =
    `{ $name }` could not find file `{ $path }`

vitte-macro-help-include-not-found =
    Check the path, working directory, and build manifest includes.

vitte-macro-e-include-io =
    `{ $name }` failed to read `{ $path }`: { $error }

vitte-macro-e-include-non-utf8 =
    `{ $name }` expected UTF-8 text in `{ $path }`, but the file is not valid UTF-8

vitte-macro-help-include-non-utf8 =
    Use the binary variant or convert the file to UTF-8.

# -----------------------------------------------------------------------------
# Builtin: cfg / feature gates
# -----------------------------------------------------------------------------

vitte-macro-e-cfg-invalid =
    invalid cfg expression in `{ $name }`: { $detail }

vitte-macro-help-cfg-examples =
    Examples: `cfg(os = "linux")`, `cfg(feature = "foo")`, `cfg(any(...))`.

vitte-macro-note-cfg-evaluated =
    cfg evaluated to: { $value }

# -----------------------------------------------------------------------------
# Builtin: env
# -----------------------------------------------------------------------------

vitte-macro-e-env-missing =
    environment variable `{ $var }` is not set (required by `{ $name }`)

vitte-macro-help-env-missing =
    Set `{ $var }` in the build environment or provide a default.

vitte-macro-e-env-nonutf8 =
    environment variable `{ $var }` is not valid UTF-8 (required by `{ $name }`)

# -----------------------------------------------------------------------------
# Builtin: file/line/column/module_path-like
# -----------------------------------------------------------------------------

vitte-macro-note-compile-location =
    compile-time location: { $file }:{ $line }:{ $col }

# -----------------------------------------------------------------------------
# Builtin: foreign / intrinsic-like
# -----------------------------------------------------------------------------

vitte-macro-e-foreign-abi-invalid =
    invalid ABI `{ $abi }` in `{ $name }`

vitte-macro-note-foreign-abi-known =
    known ABIs: { $list }

vitte-macro-e-intrinsic-unknown =
    unknown intrinsic `{ $intrinsic }` in `{ $name }`

vitte-macro-help-intrinsic-unknown =
    Check spelling or target support for `{ $intrinsic }`.

vitte-macro-e-intrinsic-target-unsupported =
    intrinsic `{ $intrinsic }` is not supported for target `{ $target }`

# -----------------------------------------------------------------------------
# Diagnostics plumbing (trace/debug)
# -----------------------------------------------------------------------------

vitte-macro-trace-enter =
    trace: expanding `{ $name }` at { $file }:{ $line }:{ $col }

vitte-macro-trace-args =
    trace: args = { $args }

vitte-macro-trace-exit =
    trace: expanded `{ $name }` successfully

vitte-macro-trace-fail =
    trace: `{ $name }` expansion failed: { $reason }
