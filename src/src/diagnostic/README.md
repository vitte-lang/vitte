# Vitte Diagnostics

The diagnostic layer stores and formats compiler diagnostics for parser, semantic analysis, driver, and CLI integration.

Invariants:
- No dependency on `runtime/*`.
- Storage is caller-provided and bounded.
- Diagnostic order is stable.
- Severity, code, and message are validated.
- Spans are optional and copied from `vitte_ast_span_t` when valid.
- With a source manager in `vitte_diagnostic_options_t`, diagnostics render the
  source line and underline the primary half-open byte range.
- `vitte_diagnostic_add_location` attaches declaration, origin, or other
  related locations to the most recently added diagnostic.
- Every diagnostic records a structured compiler phase. Call
  `vitte_diagnostic_add_with_origin` for an explicit phase; the compatibility
  `vitte_diagnostic_add` API derives one from the stable diagnostic code.
- Causes form an ordered chain and may point at source spans. Notes and help
  are distinct annotations. Suggestions include a replacement range, text,
  and `machine-applicable`, `maybe-applicable`, or `manual` applicability.
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
- Other diagnostic IDs are deterministic hashes in `E0100`–`E9999` (or the
  equivalent warning/note/help prefix). A symbolic code remains the durable key.
- Set `VITTE_BACKTRACE=1` to append a best-effort native backtrace to `E0001`,
  `E0002`, `E0003`, and `E0013` on macOS/Linux.

Severities:
- note
- help
- warning
- error
- fatal

Lifecycle:
- Initialize options with `vitte_diagnostic_options_init`.
- Provide a storage array to `vitte_diagnostic_bag_init`.
- Add diagnostics with `vitte_diagnostic_add`.
- Inspect counts with `vitte_diagnostic_bag_counts`.
- Format one diagnostic or write all diagnostics to `FILE *`.
- Format with default behavior or explicit options using `vitte_diagnostic_format_one_ex`.
- Merge one bag into another with `vitte_diagnostic_merge`.
- Use `vitte_diagnostic_status` to convert stored errors into a compiler status.
- Reset with `vitte_diagnostic_bag_reset`.

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
registry-code-aware help when no explicit help annotation is attached. Explicit
contract or subsystem guidance takes precedence, and the fallback includes the
`vitte explain <code>` command for more detail.

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
