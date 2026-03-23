# Public Surface

This document defines the minimum public perimeter that Vitte currently presents to users.

Everything not clearly inside this perimeter should be treated as `experimental` or `internal`.

## Purpose

The project already uses the words `stable`, `experimental`, and `internal`.
This file makes that boundary more concrete at the repository level.

## Command Surface

The minimum user-facing command surface is:

- `vitte parse`
- `vitte check`
- `vitte build`

These commands are the primary documented entrypoint in `README.md` and `docs/GETTING_STARTED.md`.

Contributor-facing commands that are publicly documented, but not part of the end-user language contract:

- `make build`
- `make test`
- `make parse`
- `make hir-validate`
- `make core-language-gate`
- `make core-release-gate`

All other commands should be treated as narrower workflow tools unless separately documented.

## Language Surface

The minimum protected language promise lives in:

- `docs/LANGUAGE_CORE_GUARANTEES.md`
- `docs/LANGUAGE_CORE_SURFACE.md`
- `docs/LANGUAGE_CORE_COMPATIBILITY.md`

Current practical meaning:

- the language core is the only bounded public language promise
- documented non-core features default to `experimental` unless promoted
- undocumented or `internal` language behavior is not a public compatibility promise

## Package Surface

The current documented package families are the package families with dedicated repository docs or API indexes.

Today that includes:

- `std`
- `fs`
- `db`
- `http`
- `http_client`
- `process`
- `log`
- `json`
- `yaml`
- `lint`
- `test`

Important rule:

- documented package families are visible public surface
- they are not automatically all `stable`
- if a package family is documented but not covered by an explicit compatibility policy, treat it as documented public surface with `experimental` default unless the docs say otherwise

## Internal Surface

The following should not be treated as public API contracts:

- `src/vitte/packages/**/internal/*`
- undocumented package internals
- compiler implementation files under `src/compiler/**`
- generated artifacts unless explicitly documented as contract files
- ad hoc tools in `tools/` without public-facing docs

These areas matter for contributors, but they are not the external promise of the project.

## Promotion Rule

A surface should be promoted more publicly only when all of these are true:

1. it has a clear owner and documentation entrypoint
2. it has a bounded test or snapshot gate
3. it has a stated compatibility expectation
4. it has examples or a clear user story

If those conditions are missing, keep the surface `experimental` or `internal`.
