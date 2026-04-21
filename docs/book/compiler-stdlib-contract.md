# Compiler <-> Stdlib Contract

This document defines the stable contract between the Vitte compiler and the stdlib runtime surface.

Terminology used in docs/CLI:
- "Surface de liaison Vitte" replaces "libc surface".
- "liaison native" replaces "interop C".

## 1. Source Of Truth

- ABI surface file (versioned): `docs/book/stdlib_abi_surface_v1.txt` (legacy filename)
- ABI next version: `docs/book/stdlib_abi_surface_v2.txt` (legacy filename)
- Stdlib source root: arborescence source dédiée
- Runtime ABI header: `src/compiler/backends/runtime/vitte_runtime.hpp`

## 2. Stable Stdlib Profiles

`--runtime-profile` controls which stdlib modules are allowed.
`--stdlib-profile` is kept as a legacy alias.

- `core` (legacy `minimal`): only `core/**`
- `desktop` (legacy `full`): all stdlib modules
- `system` (legacy `kernel`): `core/**` + `kernel/**`
- `arduino`: `core/**` + `arduino/**`

If an import is not allowed, the compiler emits `E1010`.
If a stdlib module is missing, the compiler emits `E1014`.

Module levels:
- `public`: stable import surface.
- `internal`: private namespace (`*/internal/*`), blocked outside owner (`E1016`).
- `experimental`: opt-in only with `--allow-experimental` (`E1015`).

## 3. Import Strictness

`--strict-imports` (alias `--strict-bridge`) enforces import hygiene:

- explicit alias required on `use`/`pull` (`E1011`)
- explicit aliases must be used (`E1012`)
- non-canonical relative import paths are rejected (`E1013`)

## 4. Runtime/Native Liaison Error Boundary

- Parse layer: `E000x`
- Resolve layer: `E100x` (includes stdlib/import constraints)
- IR layer: `E200x`
- Backend/toolchain layer: `E300x`

## 5. Tooling

- Dump stdlib exports map:
  - `vitte check --dump-stdlib-map <file.vit>`
- Dump full module index:
  - `vitte check --dump-module-index <file.vit>`
- Graph and doctor:
  - `vitte mod graph <file.vit>`
  - `vitte mod doctor <file.vit>`
- Lint ABI surface vs stdlib source:
  - `tools/lint_stdlib_api.py`
- ABI compatibility diff:
  - `tools/check_stdlib_abi_compat.py`
- Profile snapshots:
  - `tools/stdlib_profile_snapshots.sh`

## 6. Liaison Surface Naming

For system/native liaison docs and examples, prefer stdlib-facing names:
- `print`
- `read`
- `path`
- `env`
- `bytes`
- `alloc`
- `process`

Bridge module plan in docs:
- `bridge/io`
- `bridge/memory`
- `bridge/system`
- `bridge/time`
- `bridge/net`
