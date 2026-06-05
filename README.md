# Vitte

> Structured systems programming language focused on clarity, tooling, scalability, and long-term maintainability.

Designed for scalable systems software, compilers, runtimes, tooling, and long-term maintainable architectures.

![status](https://img.shields.io/badge/status-active-4c1)
![language](https://img.shields.io/badge/language-Vitte-7a3cff)
![build](https://img.shields.io/badge/build-experimental-orange)


Vitte is a modern programming language focused on clarity, structure, performance, and long-term maintainability.


The language is designed to stay readable at every scale, from small utilities to large systems, compilers, runtimes, services, tooling, and low-level software.

## Compiler project

Vitte is a compiler project.

This repository contains the compiler, its supporting toolchain, bootstrap infrastructure, documentation systems, grammar synchronization workflows, and validation tooling.

The goal is long-term stability with deterministic workflows, explicit architecture, reproducible generation, and maintainable infrastructure.

## Repository scope

The repository includes:

- compiler sources,
- bootstrap and verification systems,
- grammar synchronization workflows,
- documentation infrastructure,
- static site generation,
- validation and integrity tooling,
- diagnostics infrastructure,
- deterministic build orchestration.

Main technical areas include:

- `src/vitte/compiler` — compiler pipeline and driver infrastructure,
- `src/vitte/grammar` — grammar source and synchronization artifacts,
- `docs/` — static documentation and generated indexes,
- `tools/` — validation, synchronization, and quality workflows,
- `toolchain/` — bootstrap orchestration and compiler evolution infrastructure.

Grammar source of truth:

```text
src/vitte/grammar/vitte.ebnf
```

## Project philosophy

Vitte is developed with a practical long-term focus:

- keep the compiler understandable,
- evolve incrementally,
- validate before publication,
- maintain deterministic workflows,
- preserve architectural clarity,
- treat documentation as a first-class artifact.

The project emphasizes stable evolution over uncontrolled complexity.

## Architecture overview

### Lexer

Reads source text and transforms it into structured tokens.

### Parser

Transforms tokens into structured syntax representations.

### Intermediate Representation (IR)

Normalizes structures for semantic analysis, transformations, diagnostics, and backend preparation.

### Backend

Prepares and emits final compilation outputs.

### Diagnostics

Reports compiler errors and warnings with contextual information and validation metadata.

## Core principles

- One grammar source of truth
- Deterministic compiler workflows
- Generated artifacts are not manually edited
- Validation before publication
- Documentation synchronized with repository state
- Reproducible build infrastructure
- Long-term maintainability

## Build and validation model

The Vitte documentation and grammar infrastructure follow deterministic workflows.

Typical flows include:

1. build documentation pages,
2. generate grammar artifacts,
3. synchronize EBNF references,
4. validate checksums and manifests,
5. apply static post-processing,
6. verify integrity and consistency.

Validation workflows verify:

- grammar/documentation synchronization,
- manifest integrity,
- generated artifact consistency,
- deterministic infrastructure behavior,
- security policy consistency,
- required generated file presence.

If validation fails, publication is blocked.

## Generated vs maintained artifacts

| Maintained manually | Generated automatically |
|---|---|
| `src/vitte/grammar/vitte.ebnf` | `docs/ebnf.sha256` |
| `docs/*.html` | `docs/book/*.html` |
| `docs/css/*` | `docs/search-index*.json` |
| `tools/build_*.py` | `docs/build-manifest.json` |

## Release validation gate

Before publication:

- documentation build pipelines must pass,
- grammar synchronization checks must pass,
- required generated artifacts must exist,
- integrity and validation checks must succeed,
- deterministic generation workflows must validate,
- status verification pages must remain synchronized.

If a critical validation step fails, there is no publication.

## Documentation policy

Documentation is maintained in `docs/`.

The repository prioritizes:

- synchronized documentation,
- deterministic generated artifacts,
- maintainable static infrastructure,
- stable EN/FR documentation workflows,
- reproducible publication pipelines.

## Security and infrastructure quality

The static documentation infrastructure follows strict validation and quality rules.

This includes:

- content security policies,
- deterministic static generation,
- infrastructure integrity validation,
- generated checksum verification,
- synchronization validation,
- accessibility-aware navigation,
- reproducible publication workflows.

## Non-goals

The project intentionally avoids:

- manual editing of generated artifacts,
- uncontrolled parallel language trees,
- undocumented generation workflows,
- unsynchronized publication states,
- unstable infrastructure shortcuts.

## Glossary

| Term | Meaning |
|---|---|
| Bootstrap | Trusted workflow used to rebuild compiler stages safely |
| EBNF | Grammar format describing language rules |
| Artifact | Generated file produced by scripts or build workflows |
| Checksum | Hash used to detect unintended changes |
| CSP | Browser security policy applied to static pages |
| Drift | Mismatch between maintained and generated infrastructure |

## Intended audience

Vitte is intended for:

- systems programmers,
- compiler developers,
- tooling developers,
- infrastructure maintainers,
- contributors interested in deterministic language ecosystems,
- developers building long-term maintainable systems.

## Current direction

Current work focuses on:

- improving compiler stability,
- evolving semantic infrastructure,
- strengthening diagnostics,
- synchronizing bootstrap and documentation workflows,
- improving deterministic tooling infrastructure,
- expanding editor integration,
- refining reproducible validation systems.

## Example

```vitte
space runtime/scheduler

use core/io

form Task {
    id: u64
    state: string
}

proc schedule(task: Task) {
    if task.state == "ready" {
        emit task.id
    }
}

entry main at runtime/scheduler
```

## Why Vitte

Vitte is designed for developers who want:

- readable large-scale systems code,
- explicit architecture,
- scalable compiler-oriented design,
- modern low-level capabilities,
- structured tooling ecosystems,
- maintainable long-term software projects.

The language emphasizes clarity and structure without sacrificing performance or architectural scalability.

## Code that scales

Vitte is designed to remain readable and maintainable as projects grow.

The language emphasizes:

- explicit architecture,
- deterministic structure,
- scalable compiler-oriented design,
- long-term readability,
- large project maintainability.

From small tools to complex runtimes and compilers, Vitte aims to keep code understandable.

## Features

- modern structured syntax,
- compiler-oriented architecture,
- explicit module system,
- scalable project organization,
- asynchronous programming support,
- diagnostics-oriented compiler pipeline,
- systems programming capabilities,
- low-level interoperability,
- editor and tooling integration,
- deterministic language behavior.

## Tooling-first ecosystem

Vitte is developed with strong tooling integration in mind.

The ecosystem focuses on:

- compiler diagnostics,
- editor integration,
- static analysis,
- formatting,
- language-aware tooling,
- scalable build systems,
- deterministic workflows.

Tooling quality is considered a core part of the language experience.

## Quick installation

```bash
git clone https://github.com/vitte-lang/vitte
cd vitte
make
```

## Syntax overview

```vitte
space app/server

use core/net
use core/io

form Config {
    host: string
    port: u16
}

trait Service {
    proc start()
}

proc launch(config: Config) {
    io.println(config.host)
}

entry main at app/server
```

## Vision

Vitte aims to provide:

- clear and expressive syntax,
- predictable language behavior,
- strong project organization,
- scalable architecture for large codebases,
- modern systems programming capabilities,
- practical tooling and long-term stability.

The project evolves incrementally with emphasis on consistency, readability, and maintainable language design.

## Language philosophy

Vitte favors:

- explicit structure over hidden behavior,
- readable code over unnecessary syntax complexity,
- practical abstractions,
- modular architecture,
- compiler-oriented correctness,
- progressive language evolution.

The language is intended to remain understandable even in large and highly technical projects.

## Design principles

- Explicit over implicit
- Readability over syntax density
- Structure over magic behavior
- Deterministic compiler behavior
- Strong tooling integration
- Scalable architecture
- Progressive language evolution
- Long-term maintainability

## Main language areas

Vitte includes support for:

- modular application development,
- low-level systems programming,
- asynchronous workflows,
- compiler and tooling development,
- runtime and backend infrastructure,
- structured diagnostics,
- scalable project organization.

## Compiler-oriented language

Vitte is designed with compiler architecture as a first-class concern.

The language ecosystem is intended to support:

- parsers,
- AST/HIR/MIR pipelines,
- diagnostics systems,
- semantic analysis,
- optimization passes,
- backend infrastructure,
- language tooling.

The goal is to make large compiler and tooling projects easier to structure and maintain.

## Syntax style

The language uses a structured syntax centered around declarations such as:

- `space`
- `use`
- `form`
- `pick`
- `trait`
- `proc`
- `entry`

The syntax is designed to remain consistent and readable across the entire ecosystem.

## Project goals

The project focuses on:

- language consistency,
- readable compiler architecture,
- reliable tooling,
- deterministic workflows,
- maintainable documentation,
- long-term ecosystem stability.

## Ecosystem direction

The Vitte ecosystem is intended to support:

- compilers,
- developer tooling,
- editors and language integrations,
- build systems,
- runtimes,
- static analysis,
- documentation systems,
- low-level infrastructure projects.

## Design priorities

Important priorities include:

- readability,
- explicitness,
- predictable behavior,
- maintainability,
- scalability,
- performance,
- tooling quality.

## Philosophy compared to other languages

### Compared to C++

Vitte prioritizes readability, compiler structure, and predictable language behavior over legacy complexity.

### Compared to Rust

Vitte focuses on explicit architectural clarity and progressive compiler-oriented evolution while remaining pragmatic and scalable.

### Compared to Zig

Vitte emphasizes structured ecosystem organization, modular tooling, and large-scale maintainability.

## Intended audience

Vitte is intended for:

- systems programmers,
- compiler developers,
- tooling developers,
- advanced application developers,
- contributors interested in structured language design.

## Current direction

Current work focuses on:

- improving language stability,
- refining syntax consistency,
- expanding tooling support,
- strengthening the compiler pipeline,
- improving editor integration,
- evolving the ecosystem progressively.

## Roadmap

Current long-term directions include:

- compiler stabilization,
- parser and MIR improvements,
- diagnostics expansion,
- borrow checking evolution,
- standard library growth,
- editor ecosystem improvements,
- package management infrastructure,
- incremental compilation,
- backend expansion,
- tooling and documentation refinement.

## Current focus

- compiler architecture,
- parser evolution,
- diagnostics infrastructure,
- editor tooling,
- language consistency,
- ecosystem foundations.

## Long-term vision

Vitte aims to evolve progressively toward a complete systems programming ecosystem with:

- stable language foundations,
- modern compiler infrastructure,
- scalable tooling,
- low-level capabilities,
- structured package management,
- robust editor support,
- long-term ecosystem consistency.

## Repository structure

- `src/vitte/compiler` — compiler pipeline
- `src/vitte/stdlib/runtime.vitl` — runtime infrastructure
- `src/vitte/stdlib` — standard library
- `src/vitte/tools` — tooling ecosystem
- `docs/` — language documentation

## Status

The project is active and evolving incrementally.

## Getting started

Explore the repository structure, language grammar, compiler sources, and documentation to understand the language and ecosystem direction.

---

Vitte is an evolving systems programming language and tooling ecosystem focused on clarity, architecture, and long-term maintainability.
