# bootstrap/src/driver

The driver is the bootstrap compiler orchestration layer. It owns the ordered
pipeline for one compilation session and connects configuration, diagnostics,
AST construction, semantic analysis, HIR/IR backend lowering, C17 code
generation, and optional native C compilation.

## Lifecycle

1. Initialize options with `vitte_driver_options_init`.
2. Create a `vitte_driver_t` with `vitte_driver_init`.
3. Create an input with `vitte_driver_input_from_buffer` or
   `vitte_driver_input_from_file`.
4. Run `vitte_driver_check`, `vitte_driver_emit_c`, `vitte_driver_build`, or
   `vitte_driver_run`.
5. Inspect `vitte_driver_result_t` and `vitte_driver_diagnostics`.
6. Destroy owned input with `vitte_driver_input_destroy`.
7. Shut down the session with `vitte_driver_shutdown`.

## Pipeline

Stages are explicit and stable:

- `init`
- `load-config`
- `load-source`
- `lex`
- `parse`
- `build-ast`
- `validate-ast`
- `constants`
- `semantic`
- `backend`
- `codegen-c`
- `compile-link`
- `diagnostics`
- `cleanup`

The current bootstrap parser is intentionally minimal: it validates source
presence, builds a deterministic AST module containing `main -> int { give N }`,
and extracts the first integer literal from the source as `N`. This gives the
driver a real end-to-end path while the lexer/parser/sema folders are completed.

## Options

`vitte_driver_options_t` carries input/output paths, module metadata, sysroot,
target triple, C compiler, emit kind, optimization level, diagnostic behavior,
debug toggles, and resource limits. Limits default to the config layer defaults.

Supported emit kinds:

- `VITTE_DRIVER_EMIT_CHECK`
- `VITTE_DRIVER_EMIT_AST`
- `VITTE_DRIVER_EMIT_C`
- `VITTE_DRIVER_EMIT_BINARY`

`VITTE_DRIVER_EMIT_OBJECT` is reserved by the public enum and currently follows
the binary compile path until object-specific backend plumbing exists.

## Diagnostics

All user-facing failures are recorded in `vitte_diagnostic_bag_t`:

- missing input
- source size limit exceeded
- empty source
- invalid configuration
- AST allocation or validation failure
- semantic failure
- HIR or IR backend lowering failure
- C17 codegen failure
- C compiler/link failure

The driver uses `bootstrap/src/api/error.h` for status and last-error state. It
does not depend on `runtime/*`.

## Output

`check` validates the pipeline through semantic analysis and backend HIR/IR
validation.

`emit-c` writes to the requested file path or to the result output buffer when
the path is `NULL`.

`build` emits a sibling generated C file named `<output>.c` and invokes the
configured C compiler with strict C17 flags.

## Minimal Example

```c
vitte_driver_t driver;
vitte_driver_options_t options;
vitte_driver_input_t input;
vitte_driver_result_t result;

vitte_driver_options_init(&options);
vitte_driver_init(&driver, NULL, &options);
vitte_driver_input_from_buffer(&input, "memory.vit", "give 0", 0u);
vitte_driver_check(&driver, &input, &result);
vitte_driver_input_destroy(&input);
vitte_driver_shutdown(&driver);
```
