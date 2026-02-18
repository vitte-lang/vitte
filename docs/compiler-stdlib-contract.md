# Compiler <-> Stdlib Contract

This document defines the stable contract between the Vitte compiler and the standard library.

Terminology used in docs/CLI:
- "Surface de liaison Vitte" replaces "libc surface".
- "liaison native" replaces "interop C".

## 1. Source Of Truth

- ABI surface file (versioned): `docs/stdlib_abi_surface_v1.txt`
- ABI next version: `docs/stdlib_abi_surface_v2.txt`
- Stdlib source root: `src/vitte/packages/std`
- Runtime ABI header: `src/compiler/backends/runtime/vitte_runtime.hpp`

## 2. Stable Stdlib Profiles

`--runtime-profile` controls which std modules are allowed.
`--stdlib-profile` is kept as a legacy alias.

- `core` (legacy `minimal`): only `std/core/**`
- `desktop` (legacy `full`): all std modules
- `system` (legacy `kernel`): `std/core/**` + `std/kernel/**`
- `arduino`: `std/core/**` + `std/arduino/**`

If an import is not allowed, the compiler emits `E1010`.
If a std module is missing, the compiler emits `E1014`.

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
- Lint ABI surface vs std source:
  - `tools/lint_stdlib_api.py`
- ABI compatibility diff:
  - `tools/check_stdlib_abi_compat.py`
- Profile snapshots:
  - `tools/stdlib_profile_snapshots.sh`

## 6. Liaison Surface Naming

For system/native liaison docs and examples, prefer Vitte-facing names:
- `print`
- `read`
- `path`
- `env`
- `bytes`
- `alloc`
- `process`

Bridge namespace plan in docs:
- `std/bridge/io`
- `std/bridge/memory`
- `std/bridge/system`
- `std/bridge/time`
- `std/bridge/net`
