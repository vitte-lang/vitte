# Vitte

[![CI](https://github.com/vitte-lang/vitte/actions/workflows/ci.yml/badge.svg)](https://github.com/vitte-lang/vitte/actions/workflows/ci.yml)
[![Cirrus CI](https://api.cirrus-ci.com/github/vitte-lang/vitte.svg)](https://cirrus-ci.com/github/vitte-lang/vitte)

**Vitte** is an experimental systems programming language and toolchain focused on **clarity, explicit structure, and deterministic builds**.

It explores an alternative approach to language design, combining low-level control with a readable, structured syntax and a compiler-first architecture.

---

## Features

- Explicit block structure using `.end`
- Strong and structured type system
- Deterministic and reproducible builds
- Multi-stage compiler architecture (frontend → IR → backend)
- Cross-platform toolchain (Linux, macOS, FreeBSD, Windows)
- Custom build system (**Steel**)
- Designed with self-hosting in mind

---

## Project Status

Vitte is **experimental** and under active development.  
Breaking changes are expected.

The repository contains:
- a compiler implementation,
- a runtime and standard library,
- build, test, and packaging tooling,
- extensive automated tests.

---

## Repository Overview

---

## CLI

See `docs/cli.md` for current commands and options.
See `docs/errors.md` for diagnostic codes and fixes.
See `docs/stdlib.md` for standard library module docs and examples.

---

## Goals

- Explore alternative systems-language ergonomics
- Build a clean, maintainable compiler toolchain
- Favor explicitness and long-term design over shortcuts

---

## Non-Goals

- Full compatibility with existing languages
- API or syntax stability at this stage

---

## License

See the `LICENSE` file for details.
