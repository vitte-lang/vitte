# Native compiler configuration

`config.vitte` is the configuration model used by the native compiler
pipeline. Its public records follow the layout and enum values in
`src/bootstrap/config/config.h`, so bootstrap and native compiler code exchange
the same configuration data.

## Responsibilities

The configuration object defines compiler-wide settings for:

- build profile, backend and optimization;
- target and output paths;
- diagnostic format and color;
- job, error and output limits;
- debug information, source maps and comments;
- runtime, bounds, null and overflow checks;
- deterministic and incremental builds;
- dependency locking, offline mode and explicit overrides.

The bootstrap implementation merges defaults, system, user, project,
environment, command-line and override values in increasing precedence.
Equal-precedence conflicts are errors. Entries retain provenance, validation
state and a deterministic semantic fingerprint.

## ABI rules

The Vitte records keep the bootstrap C layout:

- C strings are `*const char`, not Vitte `string` slices;
- `ConfigValue.as_` is the storage for the C tagged union, selected by
  `ConfigValue.type_`;
- `ConfigStats.validation_count` precedes the two boolean flags;
- read-only C arguments and returned records use const pointers;
- the environment callback and command-line vector preserve C pointer shape.

The bootstrap header remains the ABI reference until the native implementation
replaces the corresponding C operations.

## Current migration boundary

`config.vitte` now provides the native resolved defaults, public name helpers
and state-query helpers. The native pipeline maps those resolved values into
`CodegenConfig`, keeping code-generation options derived from the shared
compiler configuration.

Mutation, merge, validation, parsing, file loading, CLI and environment
operations are still declared as bootstrap ABI calls. The standalone
executable continues to use `src/bootstrap/config/config.c`; the native
frontend is built with the repository root `make native-frontend` target.

The bootstrap C module remains the behavior reference while those operations
are ported. It is not a second source of defaults for the native pipeline.
