# Documentation Policy

This policy sets the audience and language conventions for Vitte documentation.
The implementation status of language features has one source of truth:
[`language_coverage.md`](language_coverage.md).

## Readers

| Reader | What they need | Recommended documentation path |
| --- | --- | --- |
| Beginner | Install Vitte, write and run a first program, understand common errors | Getting started, first-program tutorial, basic syntax, diagnostics |
| Vitte user | Learn language behavior, CLI commands, modules, and day-to-day workflows | User guide, language reference, CLI reference, examples |
| Library integrator | Find public module APIs, import/export behavior, working examples, and compatibility limits | Standard-library reference, module guide, integration examples, coverage matrix |
| Compiler contributor | Understand the active compiler, source-of-truth files, pipeline, tests, and staged native port | Compiler architecture, source map, grammar, CLI specification, contribution guide |

Every tutorial or reference page should state its intended reader near the start.
Pages that serve several readers should separate their paths instead of assuming
that a beginner and a compiler contributor need the same level of detail.

## Language policy

Documentation is organized by language and is not mixed paragraph by paragraph.

| Documentation area | Canonical language | French version |
| --- | --- | --- |
| Normative grammar, language specification, CLI, diagnostics, API and standard-library references | English | Add a complete French counterpart when there is a clear user need; do not mix translations into the English reference |
| Beginner onboarding, first programs, core concepts, installation and support | English source; publish a complete French counterpart alongside it | Prioritize a complete French path for each beginner-facing guide |
| Compiler architecture, implementation notes, contribution and release engineering | English | Translate the overview when useful; keep code identifiers, commands and file paths unchanged |
| Website navigation and documentation index | English labels with explicit EN/FR links | Label each available translation; do not imply that an untranslated page is bilingual |

Each page must declare its actual language (`lang="en"` or `lang="fr"`). An
English page stays English even when it includes French navigation elsewhere on
the site. Code, CLI output, diagnostic text, identifiers and filenames remain
literal in every translation. Where only one version exists, label that version
clearly rather than presenting a mixed-language page as bilingual.

## Status and examples

- Use [`language_coverage.md`](language_coverage.md) for implementation status.
  Other pages may summarize it but must not maintain competing feature tables.
- Distinguish syntax accepted by the grammar or parser from semantics and
  end-to-end compiler support.
- Label examples with the command and compiler path they require. Keep examples
  executable against the current implementation and identify partial or
  experimental behavior before showing it.
- When a page describes behavior, cite the relevant source file or CLI
  declaration and update the coverage matrix if the supported language surface
  changes.
- Prefer complete examples with input, command, expected output, and important
  limitations over isolated snippets that do not demonstrate a runnable path.
