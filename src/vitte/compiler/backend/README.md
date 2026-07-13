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

The canonical C boundary is versioned as `vitte-c-abi-v1`; see
`backend/c/README.md` and `backend/c/abi.vit`.

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

`backend/adapters.vit` is the canonical reachability registry for these
parallel families. The driver imports only `backend/*`; the registry keeps
adapter modules in the compiler source graph without making them production
entrypoints or transferring ownership to the driver.

## Notes

- Keep target contracts explicit (ABI, endianness, object format).
- Avoid silent backend fallback in critical phases.
- Validate runtime payload/link artifacts in tests.
- Keep canonical backend and adapter surfaces from drifting silently.

## Failure diagnostics

`backend/diagnostics.vit` is the canonical constructor surface for native backend and linker failures:

| Task | Public code | Contract |
| --- | --- | --- |
| DIAG-0077 | `BACKEND_E_UNSUPPORTED_FEATURE` | Names the unsupported backend feature and selected target. |
| DIAG-0078 | `BACKEND_E_UNSUPPORTED_TARGET` | Names the backend and rejected target architecture. |
| DIAG-0079 | `BACKEND_E_NATIVE_TOOL_MISSING` | Names the missing C compiler and attempted command. |
| DIAG-0080 | `BACKEND_E_CODEGEN_FAILED` | Includes native command, exit code, stderr and expected object. |
| DIAG-0081 | `LINK_E_SYSTEM_LINKER_FAILED` | Includes linker command, exit code, stderr and output path. |
| DIAG-0082 | `LINK_E_UNDEFINED_SYMBOL` | Identifies the unresolved native symbol. |
| DIAG-0083 | `BACKEND_E_OBJECT_WRITE_FAILED` | Identifies the unwritable output and operation. |
| DIAG-0084 | `DRIVER_E_OUTPUT_WRITE_FAILED` | Refuses an output path that equals the source path. |

The native bridge returns these codes with structured command metadata. The production driver converts them into the canonical diagnostic pipeline without replacing them with driver-local error codes.
