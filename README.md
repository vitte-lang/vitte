# Vitte



![status](https://img.shields.io/badge/status-experimental-F97316)
![focus](https://img.shields.io/badge/focus-compiler%20%26%20bootstrap-0F766E)
![surfaces](https://img.shields.io/badge/output-AST%20%7C%20HIR%20%7C%20MIR%20%7C%20IR-1D4ED8)
![docs](https://img.shields.io/badge/docs-generated%20site%20included-7C3AED)


Vitte is an experimental systems programming language and compiler project.

Short version:

- a language project
- a compiler project
- a bootstrap and reproducibility project
- a repository built to make failures explicit and tooling-friendly

The goal is simple to explain:

- keep code readable when projects become large
- build a compiler with explicit stages and reproducible workflows
- make diagnostics, tooling, and bootstrap quality first-class parts of the language

## What this repository is

This repository is the main Vitte codebase. It contains:

- the compiler
- the bootstrap toolchain
- the language grammar
- tests and validation gates
- generated documentation and supporting docs

If you want the shortest mental model, think:

`language design + compiler engineering + bootstrap/reproducibility`

## Start here

If you are visiting the repository for the first time:

1. read this README
2. run one `check` command
3. open the compiler pipeline docs
4. run the main validation gates

Minimal first commands:

```bash
bin/vitte check src/vitte/compiler/main.vit
bin/vitte check src/vitte/compiler/main.vit --diagnostics-json
./tools/compiler_test_suite_check_gate.sh
```

## Current status

Vitte is active, but still experimental.

What already exists in the repository:

- a real compiler pipeline
- native JSON surfaces for AST, HIR, MIR, diagnostics, reports
- bootstrap and validation gates
- backend and linker infrastructure
- a growing test suite around frontend, middle-end, and backend behavior

What this means in practice:

- the project is serious enough to inspect and build
- the language and compiler are still evolving
- some parts are already strongly gated, others are still being hardened

## Why people look at Vitte

Vitte is aimed at people who care about:

- compilers
- systems software
- deterministic builds
- long-term maintainability
- diagnostics quality
- self-hosting and bootstrap discipline

The design bias is not "tiny language first". It is "clear language and clear compiler architecture that can scale".

## Repository map

The most useful directories to know first are:

- `src/vitte/compiler` - compiler frontend, middle-end, backend, driver
- `src/vitte/grammar` - source-of-truth grammar
- `toolchain/` - bootstrap stages, seeds, and supporting workflows
- `tests/` - regression and validation coverage
- `tools/` - gates, checks, synchronization scripts
- `docs/` - documentation, reports, generated site material

Grammar source of truth:

```text
src/vitte/grammar/vitte.ebnf
```

## How the compiler is organized

At a high level, the pipeline is:

1. lexer
2. parser
3. AST validation
4. semantic analysis
5. type checking
6. borrow checking
7. MIR lowering and validation
8. IR lowering and validation
9. backend code generation
10. linking

The repository puts a lot of emphasis on making failures explicit and machine-readable.

## Quick start

If you just want to poke the project quickly:

```bash
vitte check src/vitte/compiler/main.vit
```

Dump machine-readable diagnostics:

```bash
vitte check src/vitte/compiler/main.vit --diagnostics-json
```

Build a test binary:

```bash
vitte build src/vitte/compiler/tests/pipeline_tests.vit -o /tmp/vitte-pipeline-tests
```

Run the main compiler test gates:

```bash
./tools/compiler_test_suite_check_gate.sh
./tools/compiler_test_suite_bridge_gate.sh
```

If you prefer to browse before building:

- language and spec: `docs/spec/`
- compiler docs: `docs/compiler/`
- bootstrap docs: `docs/bootstrap/`
- generated site entry point: `docs/index.html`

## Example

```vitte
space hello/app

proc main() -> int {
  give 0;
}
```

## Project principles

Vitte keeps repeating the same rules on purpose:

- one grammar source of truth
- deterministic workflows over hidden magic
- generated artifacts should not be edited manually
- compiler stages should fail clearly
- diagnostics should be useful to both humans and tools
- bootstrap quality matters as much as language features

## Documentation

Useful entry points:

- `docs/index.html`
- `docs/start-here.html`
- `docs/compiler/architecture.md`
- `docs/compiler/pipeline.md`
- `docs/compiler/backend.md`
- `docs/bootstrap/overview.md`
- `docs/spec/language.md`

Useful depending on what you want:

- understand the language: `docs/spec/language.md`
- understand the compiler: `docs/compiler/architecture.md`
- understand the bootstrap path: `docs/bootstrap/overview.md`
- inspect generated public-facing docs: `docs/index.html`

## Visual assets

Repository visuals available in `docs/svg/`:

- `readme-hero.svg` - main GitHub README hero
- `readme-hero-compact.svg` - compact mobile-friendly variant
- `readme-social-card.svg` - social sharing / Open Graph style card

## Who this README is for

If you are:

- a curious developer
- a compiler engineer
- a contributor evaluating the repository
- someone checking whether the project is real or just an idea

this README is the short version.

The deeper technical material lives in `docs/` and under `src/vitte/compiler/`.

If you need a very simple framing:

- users look for the language direction
- contributors look for the compiler structure
- compiler engineers look for the validation and bootstrap model

## Contributing

The repository is large and heavily validated, so the practical contribution style is:

- make focused changes
- keep behavior explicit
- add tests with the change
- prefer canonical paths over duplicate wrappers
- do not hand-edit generated outputs unless the workflow explicitly requires it

## In one sentence

Vitte is an experimental language and compiler project trying to make systems programming, compiler construction, and bootstrap engineering more explicit, more testable, and easier to maintain over time.
