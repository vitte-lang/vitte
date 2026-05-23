Vitte Compiler Diagnostics

Path: src/vitte/compiler/diagnostics

Purpose

The diagnostics directory owns compiler error reporting.

It defines:
- diagnostic data structures
- severities
- source spans
- labels
- reports
- diagnostic catalog entries
- rendering contracts

It must not own:
- type checking logic
- semantic analysis logic
- borrow checking logic
- parser recovery logic
- backend validation logic

The diagnostics layer describes and renders errors. Other compiler phases produce them.

Invariants

- diagnostics must be deterministic
- every diagnostic must have a stable code when possible
- every diagnostic must carry a severity
- source spans must be explicit
- rendering must not mutate diagnostic state
- diagnostic construction must be independent from terminal output

Canonical Flow

compiler phase
→ Diagnostic
→ DiagnosticReport
→ renderer
→ terminal / JSON / LSP

Reproducible Commands

vittec check examples/error.vit

vittec check examples/error.vit --error-format=json

vittec check examples/error.vit --error-format=human

vittec check examples/error.vit --warnings-as-errors

Contracts

Producer contract

Compiler phases may create diagnostics, but must not render them directly.

Valid producers:
- lexer
- parser
- sema
- typeck
- borrowck
- MIR validation
- backend validation

Renderer contract

Renderers receive immutable diagnostic reports.

A renderer may output:
- human-readable text
- JSON
- LSP diagnostics
- compact terminal summaries

Catalog contract

The catalog owns stable diagnostic codes.

Examples:
- LEX_E_INVALID_CHAR
- PARSE_E_EXPECTED_TOKEN
- SEMA_E_UNKNOWN_SYMBOL
- TYPECK_E_ASSIGN_MISMATCH
- BORROW_E_USE_AFTER_MOVE

Recommended Files

diagnostic.vit

Core diagnostic object.

severity.vit

Severity definitions:
- note
- help
- warning
- error
- fatal

span.vit

Source location and range types.

label.vit

Primary and secondary source labels.

report.vit

Collection and merge logic.

catalog.vit

Stable diagnostic message catalog.

render.vit

Formatting layer.

Testing Strategy

Recommended tests:
- stable diagnostic codes
- deterministic rendering snapshots
- JSON output validity
- span formatting
- label ordering
- warning escalation
- duplicate diagnostic merging

Design Rule

Diagnostics are compiler facts, not terminal strings.

The compiler should first build structured diagnostics, then render them through explicit output backends.
