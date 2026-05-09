# Vitte

Vitte is a compiler project.

This repository contains the compiler, its supporting toolchain, and the
project documentation. The goal is long-term stability with clear structure.

## Overview and scope

Vitte is developed with a practical focus: keep the compiler understandable,
evolve in small validated steps, and maintain documentation as a first-class artifact.

The repository includes:

- compiler sources,
- bootstrap and verification scripts,
- static documentation site,
- grammar artifacts and synchronization checks.

Main technical areas:

- `src/vitte/compiler`: compiler pipeline and driver surface.
- `src/vitte/grammar`: language grammar inputs.
- `docs/`: static site pages and generated indexes.
- `tools/`: build, sync, and quality scripts.
- Grammar source of truth: `src/vitte/grammar/vitte.ebnf`.

## Who owns what

- Compiler: language pipeline and core behavior.
- Docs: public pages, grammar references, and status visibility.
- Build: generation scripts, checksums, and release checks.

## If you only read one thing

- [Guide docs](/Users/vincent/Documents/Github/vitte/docs/README.md)
- [Architecture docs](/Users/vincent/Documents/Github/vitte/docs/docs-architecture.html)
- [Status docs](/Users/vincent/Documents/Github/vitte/docs/status.html)

## Architecture at a glance

- Lexer: reads text and splits it into tokens.
- Parser: turns tokens into syntax structures.
- Intermediate Representation (IR): normalizes structures for later compiler stages.
- Backend: prepares and emits final outputs.
- Diagnostics: reports errors and warnings with context.

## Principles

- One source of truth for grammar.
- Generated artifacts are not hand-edited.
- Changes are validated before publication.
- Documentation must match repository state.

In practice, this means grammar changes are synchronized into docs artifacts,
then verified by automated checks before release.

## Build and validation model

The docs and grammar flow is deterministic:

1. Build docs pages.
2. Generate grammar extras.
3. Sync EBNF memory pages and checksums.
4. Apply static post-processing (SEO/CSP/indexes/manifests).

Validation then checks:

- grammar/doc sync,
- JSON and manifest integrity,
- critical generated files presence,
- security policy consistency on key pages.

## What is generated vs hand-edited

| Hand-edited | Generated |
| --- | --- |
| `src/vitte/grammar/vitte.ebnf` | `docs/ebnf.sha256` |
| `docs/*.html` (EN source pages) | `docs/fr/*.html` |
| `docs/css/*`, `docs/js/*` | `docs/search-index*.json` |
| `tools/build_*.py` | `docs/build-manifest.json`, `docs/checksums.txt` |

## Release gate

Before publication, all of this must be green:

- docs build pipeline completed,
- grammar sync checks passed,
- required generated files present and valid,
- CSP and integrity checks passed on key pages.
- quick status check visible on [docs/status.html](/Users/vincent/Documents/Github/vitte/docs/status.html).

If one check fails, there is no publication.

## Non-goals

- No manual editing of generated artifacts.
- No parallel internal language trees beyond EN/FR.
- No manual translation workflow outside EN/FR.
- No release when docs and source are out of sync.

## Glossary

- Bootstrap: trusted path used to rebuild compiler stages safely.
- EBNF: grammar format used to describe language rules.
- Artifact: generated file produced by scripts/build.
- Checksum: hash used to detect unintended file changes.
- CSP: browser security policy applied to static pages.
- Drift: mismatch between source files and generated outputs.

## Intended audience

Vitte is for users, contributors, and maintainers responsible for release quality.

## Documentation language policy

Documentation is maintained in English at `docs/` (root) and French at `docs/fr/`.
Other languages are provided through browser auto-translation.

## Security and accessibility

The static documentation site follows strict security and quality rules:

- strict content security policy,
- no inline dynamic scripting policy in published pages,
- regular automated checks,
- keyboard navigation and visible focus support.

The project also tracks generated checksums and status metadata to detect drift
between source files and published artifacts.

## Status

The project is active.

Current direction:

- improve compiler stability,
- keep bootstrap and docs synchronized,
- raise quality incrementally without losing clarity.

## Getting started

Start with the 3 links in "If you only read one thing", then review status.

Questions? Open an issue.
