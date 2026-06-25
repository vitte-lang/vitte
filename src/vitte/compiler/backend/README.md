# backend

Path: `src/vitte/compiler/backend`

## Purpose

Canonical production backend pipeline for the real compiler path.

## Current Role

This tree is the backend surface used by the production compiler pipeline today.
The runtime driver and backend pipeline are expected to route through modules in
`src/vitte/compiler/backend/*`, including:

- `backend/pipeline.vit`
- `backend/codegen/*`
- `backend/link/*`
- `backend/target/*`
- `backend/c/*`

## Relationship To `backends/*`

The repository also contains `src/vitte/compiler/backends/*`.

That sibling tree is not the canonical production backend path. It currently
hosts:

- adapter surfaces
- experimental backend families
- backend-specific support modules such as LLVM bindings
- dedicated backend docs and tests that are not yet the single source of truth

The real compiler path must not silently drift from `backend/*` to
`backends/*` without an explicit migration and matching gate updates.

## Notes

- Keep target contracts explicit (ABI, endianness, object format).
- Avoid silent backend fallback in critical phases.
- Validate runtime payload/link artifacts in tests.
- Keep canonical backend and adapter surfaces from drifting silently.
