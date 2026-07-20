# Vitte Compiler Diagnostics

The compiler diagnostic system is a stable API shared by the CLI, LSP,
editor integrations, and tests. Every user-facing diagnostic is represented by
one `Diagnostic` value and one catalog entry.

## Stable Surface

- `Diagnostic` is the canonical object passed between compiler phases.
- `Span`, `PrimarySpan`, and `SecondarySpan` identify source locations.
- Diagnostic codes are stable public identifiers and are never assembled from
  dynamic string fragments.
- The public API status is `migration-freeze-pending` until the migration debt
  reaches zero; after that point the status must be changed to `frozen` with a
  stable schema version.
- The catalog owns the code, title, phase, default severity, required
  parameters, documentation, and associated tests.
- ICE reports are the only diagnostics allowed to describe an internal compiler
  defect directly.

## Rendering

All output formats consume the same `DiagnosticReport`:

- terminal text for `vitte check` and `vitte build`;
- JSON for scripts and snapshot tests;
- LSP diagnostics and code actions for editors;
- SARIF for external analysis tools.

Different renderers may format the same fields differently, but they must not
change the code, title, span, notes, helps, suggestions, or final summary.

## CLI Contracts

`vitte check` is the reference command for diagnostics without final
generation. `vitte build` must report the same diagnostics before generation
continues. The shared options are:

- `--error-format`;
- `--diagnostic-width`;
- `--color`;
- `--max-errors`;
- `--deny-warnings`;
- `--allow CODE`, `--warn CODE`, and `--deny CODE`.

Structural errors cannot be disabled. Warnings are reserved for valid programs
that deserve attention; errors are reserved for programs that cannot compile.

## Test Requirements

Every diagnostic code needs at least one snapshot test. Snapshot expectations
cover the code, title, primary span, secondary spans, notes, helps,
suggestions, ordering, and absence of unrelated diagnostics.

Machine-applicable suggestions must include a concrete replacement, a high
confidence level, and a recompilation check after the fix is applied.
