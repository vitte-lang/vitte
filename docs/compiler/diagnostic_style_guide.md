# Vitte Diagnostic Style Guide

Every diagnostic must answer four questions:

- What is incorrect?
- Where is the problem?
- Why is it incorrect in Vitte?
- How can the user correct it?

Use one terminology set everywhere:

- `procedure`, never function or routine.
- `give`, never return.
- `form`, `pick`, `let`, `set`, `space`, and `use` for language syntax.
- `lexer`, `parser`, `type checker`, `borrow checker`, `backend`, and `linker` for compiler phases.

Kinds

- `user`: source code does not satisfy the Vitte language rules.
- `configuration`: command line, profile, target selection, or project setup is wrong.
- `environment`: native tools, filesystem, platform, or backend execution failed outside the user's Vitte source.
- `linker`: native link step found missing symbols, libraries, duplicate definitions, incompatible architecture, invalid objects, missing entry point, or permissions.
- `internal_compiler`: compiler invariant was broken. Only this kind may use `ICE`.

ICE Rules

`ICE` is reserved for compiler defects. An ICE must never be rendered as a user error.

An ICE diagnostic must include:

- phase;
- Vitte source file;
- internal operation;
- stable identifier;
- reproduction instruction;
- concise technical cause.

Suggestions

Every suggestion carries a confidence:

- `high`: machine applicable and safe for the exact span.
- `medium`: likely useful, but requires user review.
- `low`: explanatory guidance only.

Never mark a medium or low confidence suggestion as machine applicable. Use `help` for explanation, `suggestion` for possible edits, and `fix` only for edits the compiler can apply safely.

Spans

User-facing columns start at one. Internal offsets are UTF-8 byte offsets and must not be described as visual columns.

Diagnostics may include multiple spans when one source error requires coordinated edits, for example declaration and use, or all arguments that must be reordered together.

Rendering

All renderers consume the same `Diagnostic` object. Do not rebuild diagnostics separately for terminal, JSON, LSP, or SARIF.

Default terminal output must be concise:

- short title;
- precise source span;
- labels, notes, help, and suggestions;
- external tool stderr summarized to the useful first line.

External commands are shown only in verbose mode. Full stderr remains available in structured JSON.

Forbidden Vague Terms

Avoid vague terms unless the same sentence gives a concrete cause:

- invalid;
- failed;
- unknown error;
- unexpected failure;
- something went wrong;
- semantic problem;
- internal issue.

Tests

Each new diagnostic needs a catalog entry and at least one test that checks:

- code;
- title;
- span;
- notes;
- suggestions;
- absence of parasitic diagnostics;
- deterministic ordering;
- recovery behavior when applicable.
