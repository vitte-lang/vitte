# Bootstrap Diagnostics

The diagnostic layer stores and formats compiler diagnostics for parser, semantic analysis, driver, and CLI integration.

Invariants:
- No dependency on `runtime/*`.
- Storage is caller-provided and bounded.
- Diagnostic order is stable.
- Severity, code, and message are validated.
- Spans are optional and copied from `vitte_ast_span_t` when valid.
- Counts remain coherent with stored diagnostics.
- `warnings_as_errors` stores warnings as errors.
- When capacity or max diagnostics is reached, new diagnostics are suppressed and reported through `last_error`.

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

Stable format:
```text
error[VITTE_CODE]: message
  --> file.vit:3:12-3:18
  = details
```

Without a span:
```text
warning[VITTE_CODE]: message
  = details
```

Limitations:
- Source-line rendering is not implemented until the source module is real.
- Related diagnostics are not modeled yet.
- Color output is available but disabled by default for stable tests.

Formatting options:
- `show_codes` toggles `[VITTE_CODE]`.
- `show_details` toggles detail lines.
- `color_enabled` wraps diagnostics in ANSI color sequences.
- `show_source_line` is reserved for the future source module.

Summary:
- `vitte_diagnostic_format_summary` reports total diagnostics, errors, warnings, and suppressed diagnostics.
