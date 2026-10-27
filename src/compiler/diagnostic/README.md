# Vitte Diagnostics

The diagnostic layer stores and formats compiler diagnostics for parser, semantic analysis, driver, and CLI integration.

The diagnostic bag, labels, registry, source-map operations, suggestions and
fix plans are implemented in Vitte. The registry contains the 149 stable
bootstrap entries in a static Vitte table. The source map owns its entries in
a native Vitte vector. Suggestions own their `AstSpan` and replacement
`string` values, so callers may reuse their input buffers after adding one.
The LSP source-position converter reads the native `SourceContext` and maps
byte offsets to UTF-8, UTF-16 or UTF-32 protocol positions. The two label
`dump` entrypoints still cross the host stream boundary. Renderer metadata and
defaults, CLI transport validation and statistics are native. Of the 28
remaining external diagnostic procedures, 25 write to host streams or
implement stream-facing CLI explain/render adapters; three expose the scalar
CLI transport host boundary. Their renderer entrypoints still
consume bootstrap C layouts; do not pass a Vitte-owned `Diagnostic` or
`DiagnosticBag` directly across that boundary. The ordinary native CLI route
uses scalar transport callbacks to create C-owned records before rendering.
Native format serializers remain open. The bootstrap
registry and source map remain parallel implementations until the bootstrap
migration removes them.

The native frontend CLI path sends code, phase, severity, message, details and
source offsets as scalar arguments to an explicit host adapter. The adapter
allocates and fills the bootstrap C diagnostic layout itself, including copies
of transient detail strings. No Vitte diagnostic structure or Vitte-owned
layout crosses into the C renderers. The host boundary currently handles
terminal, JSON, SARIF and LSP output; replacing those C renderers with native
serializers remains open.

The native LSP position converter expects a pointer to the Vitte
`SourceContext`; the current CLI host adapter uses its own source context and
position conversion. Suggestion and fix-plan accessors return pointers into
Vitte vectors; those pointers remain valid only until the corresponding
suggestions or plan edits are mutated.

Invariants:
- No dependency on `runtime/*`.
- Storage is caller-provided and bounded.
- Diagnostic order is stable.
- Severity and origin enums are validated before storage; emission requires a
  non-empty message.
- Spans are optional and copied by value when valid.
- With a source manager in `vitte_diagnostic_options_t`, diagnostics render the
  source line and underline the primary half-open byte range.
- `vitte_diagnostic_add_location` attaches declaration, origin, or other
  related locations to the most recently added diagnostic.
- Every diagnostic records a structured compiler phase. Pass an explicit origin
  to `vitte_diagnostic_emit`, or set it on the object before
  `vitte_diagnostic_add`. `VITTE_DIAGNOSTIC_ORIGIN_UNKNOWN` uses the registered
  code's origin; an explicit origin is preserved.
- Causes form an ordered chain and may point at source spans. Notes and help
  are distinct annotations. Suggestions own an optional replacement range and
  text, and carry `machine-applicable`, `maybe-incorrect`,
  `has-placeholders`, or `manual`
  applicability. Insertions use zero-width spans; deletions use an empty
  replacement for a non-empty span.
- Counts remain coherent with stored diagnostics.
- `warnings_as_errors` stores warnings as errors.
- When capacity or max diagnostics is reached, new diagnostics are suppressed and reported through `last_error`.
- A failed compiler phase with no error diagnostic receives `E0012`; if the bag
  is full of non-errors, `vitte_diagnostic_ensure_failure` replaces the last one.

Infrastructure codes (`E0000`–`E0099` are reserved):
- `E0000` fallback, `E0001` internal error, `E0002` invariant, `E0003` impossible state.
- `E0004` resource allocation, `E0005` unreadable source, `E0006` invalid UTF-8,
  `E0007` source map, `E0008` diagnostic construction, `E0009` internal limit.
- `E0010` incompatible file format, `E0011` configuration, `E0012` phase failure,
  `E0013` component failure. Each code has a fixed symbolic name in `diagnostic.h`.
- Registered public IDs are explicit entries in `registry.vitte` on the native
  path; the bootstrap copy remains in `registry.c` until that route is removed.
  Both symbolic identifiers and public codes (for example `E0302` or `W0003`)
  are accepted by the emission API. Unregistered identifiers retain their
  symbolic code and receive `E????`, `W????`, `N????`, or `H????` as a display
  fallback; register a code to give it a stable public identity.
- Set `VITTE_BACKTRACE=1` to append a best-effort native backtrace to `E0001`,
  `E0002`, `E0003`, and `E0013` on macOS/Linux.

Severities:
- note
- help
- warning
- error
- fatal

Code, severity and origin are independent: promoting `W0003` to an error keeps
its public code and `import` origin. Both `error` and `fatal` count as failures.
Origins include `lexer`, `parser`, `import`, `name-resolution`, `type-check`,
`constant-eval`, `linker`, and the remaining backend/infrastructure phases.

Cascade classification is implemented natively in `cascade.vitte` and used by
the diagnostic engine. Explicit parent relationships take priority; automatic
classification scores phase dependencies together with source spans, symbols,
types, context and diagnostic codes. Suppression retains the diagnostic in
the bag and updates its suppressed count. The bootstrap counterpart is
`src/bootstrap/diagnostic/cascade.c`; both phase graphs include scanner and
code-generation dependencies.

The native module exports `cascade_set_last_parent`, `cascade_mark_last`,
`cascade_mark_last_suppressed`, `cascade_should_suppress`,
`cascade_classify_last`, `cascade_classify_all`, and root/count queries. These
operate on the Vitte-owned `DiagnosticBag`; the C ABI remains declared in
`src/bootstrap/diagnostic/cascade.h`.

The origin is exposed as `phase` in output for compatibility with existing
consumers:

| Format | Public code | Exact Vitte severity | Origin |
| --- | --- | --- | --- |
| Terminal | `[E0302]` | `error` before the code | `phase: import` |
| JSON | `code` | `severity` | `phase` |
| LSP | `code` | `data.severity` | `data.phase` |
| SARIF | `ruleId` | `properties.severity` | `properties.phase` |

LSP's numeric `severity` and SARIF's `level` keep their protocol mappings.
The additional metadata preserves distinctions such as `fatal` versus `error`,
and SARIF's `help` versus `note`.

Lifecycle:
- Initialize options with `vitte_diagnostic_options_init`.
- Provide a storage array to `vitte_diagnostic_bag_init`.
- Add diagnostics with `vitte_diagnostic_add`.
- Inspect counts with `vitte_diagnostic_bag_counts`.
- Format one diagnostic with the Vitte model. `FILE *` render and write
  functions remain host-side adapters.
- Format with default behavior or explicit options using `vitte_diagnostic_format_one_ex`.
- Merge one bag into another with `vitte_diagnostic_merge`.
- Use `vitte_diagnostic_status` to convert stored errors into a compiler status.
- Reset with `vitte_diagnostic_bag_reset`.

Suggestion construction accepts an optional replacement: `null` creates an
explanation-only suggestion, an empty string deletes the selected span, and a
non-empty string inserts or replaces text. The suggestion and fix-plan
accessors expose pointers into their backing vectors; any mutation of the
corresponding vector invalidates those pointers.

Terminal format:
```text
error[E0702]: postcondition failed
  phase: contract | category: contract
  procedure: clamp
  --> src/math.vit:31:9
   31 | ensures result >= minimum;
      |         ^^^^^^^^^^^^^^^^^ condition evaluated to false
  contract: ensures `result >= minimum`
  note: `minimum`: 0
  note: `maximum`: 100
  note: `result`: -1
  caused by: returned value produced the violation
  help: every successful return from `clamp` must satisfy this condition
```

The CLI uses the structured diagnostic model for lexer, parser, semantic-analysis,
driver, and source-loading failures. `--diagnostic=terminal` renders annotated
source excerpts, `--diagnostic=json` emits one versioned diagnostics document,
and `--diagnostic=sarif` emits a SARIF 2.1.0 log. Diagnostics collected during
the same compiler phase are emitted together, so machine-readable output remains
one valid document rather than concatenated per-error fragments.
When a diagnostic code exists in the registry, the renderer supplies its stable
code, title, explanation, and compiler phase; source spans are rendered through
the registered source manager. Error and warning diagnostics also receive
contextual help from `help.c` when no explicit help annotation is attached.
Every registered error and warning has a dedicated remediation policy, enforced
by a coverage test. Actual types, source excerpts, expected parser tokens,
contract conditions and available failure details specialize that policy.
Name resolution suggests a name only when it is a unique visible candidate one
edit away (including a transposition); ambiguous, short and non-ASCII names do
not trigger a spelling guess. Call diagnostics report the exact expected and
supplied argument counts. These are advisory hints, not automatic edits.

The CLI and direct renderers share the same help formatter. JSON carries help
annotations, SARIF carries `result.properties.help`, and LSP carries `data.help`.
Explicit subsystem guidance takes precedence. Unknown codes retain the actual
failure and cause and explicitly say that no verified correction is registered.
No model, network service or external inference dependency is required.

`vitte explain E0501` and `vitte explain E0501 --diagnostic=json` use the same
per-code remediation policies. The bootstrap C
`vitte_diagnostic_write_all_format` entrypoint dispatches to the current
terminal, JSON and SARIF renderers. For LSP, call the dedicated renderer with
a position converter: the generic entry point cannot invent UTF-16 positions
and returns `VITTE_STATUS_ERROR_INVALID_STATE`.

Run `make test` for coverage, real CLI examples, format consistency and
bounded rendering checks; `make test-diagnostic-help` runs the focused CLI suite.

Without a span:
```text
error[E0501]: type incompatible
  phase: type-check | category: type
  type: expected `int`, found `string`
  context: argument 1 of procedure call
```

Limitations:
- Color output is available in terminal format; use `--no-color` for stable
  snapshots and non-interactive text capture.
- Recovery from a process crash and portable native backtraces are not yet available.

Formatting options:
- `show_codes` toggles `[VITTE_CODE]`.
- `show_details` toggles detail lines.
- `color_enabled` wraps diagnostics in ANSI color sequences.
- `show_source_line` enables source excerpts and underlines when source text is registered.

Summary:
- `vitte_diagnostic_format_summary` reports total diagnostics, errors, warnings, and suppressed diagnostics.
