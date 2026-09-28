# Vitte Diagnostic Style Guide

Every diagnostic must answer four questions:

- What is incorrect?
- Where is the problem?
- Why is it incorrect in Vitte?
- How can the user correct it?

Code review must reject any new or changed diagnostic that does not answer all
four questions in the central catalog and in the rendered diagnostic surface.
The title may stay short, but the label, note, help, and suggestion content must
make the four answers visible without relying on a free-form fallback message.
The review rule is explicit: every diagnostic must state the cause, the
location, the reason it violates Vitte, and the correction. A diagnostic that
omits one of those four fields is not reviewable.

Use the official Vitte names everywhere. The same source concept must keep the
same name in lexer, parser, resolver, type checker, borrow checker, backend,
JSON, LSP, snapshots, and terminal output.

Official terms:

- `procedure` for the user-facing concept, never function or routine.
- `proc` only when referring to the literal Vitte keyword in source.
- `give`, never return.
- `form`, `pick`, `let`, `set`, `space`, and `use` for language syntax.
- `identifier` for a source name token.
- `symbol` for the compiler entity created after name resolution.
- `token` for lexer output.
- `declaration` for a source construct that introduces a name.
- `module` for importable source units.
- `lexer`, `parser`, `type checker`, `borrow checker`, `backend`, and `linker` for compiler phases.

Do not rename a concept between phases. If the lexer reports an `identifier`,
the parser, resolver, and type checker must not call the same source text a
symbol until name resolution has actually produced a symbol.

Four-question rule:

- What is incorrect? Put the short answer in the title and the precise answer in a label.
- Where is the problem? Put it in the primary span and, when useful, a secondary label.
- Why is it incorrect in Vitte? Put it in a note.
- How can the user correct it? Put it in help or a suggestion.

The central catalog must also answer those questions in structured
documentation fields:

- `incorrect`;
- `location`;
- `reason`;
- `correction`.

Kinds

- `user`: source code does not satisfy the Vitte language rules.
- `configuration`: command line, profile, target selection, or project setup is wrong.
- `environment`: native tools, filesystem, platform, or backend execution failed outside the user's Vitte source.
- `linker`: native link step found missing symbols, libraries, duplicate definitions, incompatible architecture, invalid objects, missing entry point, or permissions.
- `internal_compiler`: compiler invariant was broken. Only this kind may use `ICE`.

Stable Codes

`schemas/diagnostics/codes.json` is the central registry for public diagnostic
codes. A public code is stable once it appears there.

Public codes use the stable phase prefix from the registry followed by four
decimal digits:

- `LEX0000` for lexer diagnostics.
- `PAR0000` for parser diagnostics.
- `RES0000` for resolver diagnostics.
- `SEM0000` for semantic-analysis diagnostics.
- `TYP0000` for type-checker diagnostics.
- `BOR0000` for borrow-checker diagnostics.
- `MIR0000` for MIR diagnostics.
- `IR0000` for IR diagnostics.
- `GEN0000` for backend/codegen diagnostics.
- `LNK0000` for linker diagnostics.
- `ICE0000` for internal compiler errors.

Do not reuse a removed code for a different diagnostic. Keep old message keys in
`aliases` when migrating from legacy names, and mark obsolete entries as
`deprecated` instead of deleting or renumbering them.

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

User-facing columns start at one and are visual Unicode columns. Tabs are rendered with the compiler diagnostic tab width, currently four columns. Internal offsets are UTF-8 byte offsets and must not be described as visual columns.

Diagnostics may include multiple spans when one source error requires coordinated edits, for example declaration and use, or all arguments that must be reordered together.

Rendering

All renderers consume the same `Diagnostic` object. Do not rebuild diagnostics separately for terminal, JSON, LSP, or SARIF.

Do not concatenate user-facing error strings at emission sites. Use a catalogued
diagnostic variant with named parameters, then let the renderer format the final
message. ICE rendering is the only exception.

Use the dedicated render profiles for the surface:

- terminal: human renderer, color allowed, command details hidden unless verbose;
- monochrome: same diagnostic content without ANSI color;
- IDE: canonical diagnostic object converted to LSP/JSON/SARIF without reparsing messages.

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
