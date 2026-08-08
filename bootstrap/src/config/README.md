# Bootstrap Config

The config layer centralizes bootstrap compiler settings before CLI, driver, API, and codegen are fully wired together.

Invariants:
- Defaults are deterministic.
- No allocation is required.
- Paths are stored as borrowed `const char *` values.
- Errors use `bootstrap/src/api/error.h`.
- No dependency on `runtime/*`.
- Unknown targets, backends, modes, and diagnostic modes are rejected.

Defaults:
- target: `host`
- backend: `c17`
- build mode: `check`
- diagnostics: `color-auto`
- C compiler: `cc`
- emit includes: enabled
- emit debug comments: disabled
- indentation: 4
- keep intermediate C: disabled
- max source bytes: 64 MiB
- max AST depth: 256
- max diagnostics: 100
- max include depth: 64
- max path length: 4096

Paths:
- root path
- sysroot path
- input path
- output path
- cache path
- temp path

Validation:
- Input path is required for check, emit-c, build, and run modes.
- Build and run require a non-empty C compiler.
- All configured paths must be non-empty and within `max_path_length`.
- Limits must be non-zero.
- Indentation width must be 16 or less.

Conversions:
- `vitte_config_to_api_config` maps root/sysroot and deterministic mode into API config.
- `vitte_config_to_codegen_options` maps backend, input path, output path, buffer, indentation, includes, and debug comments into codegen options.

Limitations:
- This layer does not parse config files yet.
- CLI and driver integration should consume this layer in a later pass instead of duplicating defaults.
