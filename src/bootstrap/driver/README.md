# src/bootstrap/driver

## Reentrancy and thread safety

Each `vitte_driver_t`, module loader, source manager, configuration, and
diagnostic bag belongs to one compilation session. These objects are not
shared between calls. Multiple sessions can run in parallel when each thread
owns its own context and no object is mutated concurrently.

Constant tables and version strings are immutable; version-formatting caches
are thread-local. The API's last-error state is also thread-local. Objects
returned by a session remain valid only until that session is destroyed.

The driver is the compiler orchestration layer. It owns the ordered
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

The bootstrap CLI runs the compiler frontend and semantic analysis before
selecting a backend:

- parser over the source text or module wrapper
- AST validation
- semantic analysis
- default direct AST-to-C17 emission or native C compilation
- optional HIR and IR lowering when the alternate pipeline is selected

The CLI `check` command stops after semantic analysis. It does not run HIR/IR
lowering or code generation. The driver only runs backend phases configured by
its caller and required by the selected command.

Parser finalization validates the AST before semantic analysis. HIR and IR
contexts validate their cross references when sealed, and the C17 backend
validates its finished state before returning generated output. Backend symbol
names and fingerprints are derived from stable declaration/source identities.

When parsing through `vitte_module_t`, the driver also registers and resolves
imports before semantic analysis. Global imports use configured search paths,
and sibling modules are discoverable from the input file directory.
Imported module graphs are semantically checked before the root module. Backend
flattening only carries exported imported declarations into the lowered module.
That export surface now includes `export *`, explicit local export items, and
local export aliases in addition to inline `export proc/const`.
Flattening assigns stable internal lowered names to imported value and
procedure declarations, so their public Vitte names remain deterministic
through backend lowering. Nominal type declarations still need namespace
mangling in the C AST emitter when separate modules expose the same type name;
the frontend validates those graphs, while collision-free modules can already
be emitted and compiled natively.

## Options

`vitte_driver_options_t` carries input/output paths, module metadata, sysroot,
target triple, C compiler, emit kind, optimization level, diagnostic behavior,
debug toggles, and resource limits. Limits default to the config layer defaults.

Supported emit kinds:

- `VITTE_DRIVER_EMIT_CHECK`
- `VITTE_DRIVER_EMIT_AST`
- `VITTE_DRIVER_EMIT_C`
- `VITTE_DRIVER_EMIT_BINARY`

Object emission is exposed by the C17 backend as
`vitte_c17_compile_ast_object_with_options()`. It compiles the generated C with
`cc -c`, writes the requested relocatable object, and never invokes the linker.
Code generation failures carry the parser root span and source path through the
driver diagnostic bridge.

## Diagnostics

All user-facing failures are recorded in `vitte_diagnostic_bag_t`:

- missing input
- source size limit exceeded
- empty source
- invalid configuration
- import resolver initialization failure
- missing imported module
- AST allocation or validation failure
- semantic failure
- HIR or IR backend lowering failure
- C17 codegen failure
- C compiler/link failure

The driver uses `src/bootstrap/api/error.h` for status and last-error state. It
does not depend on `runtime/*`.

## Output

`check` validates syntax and semantics, then stops before backend phases. For
file inputs it also verifies that declared imports can be resolved on disk.

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
vitte_driver_input_from_buffer(&input, "memory.vit", "return 0", 0u);
vitte_driver_check(&driver, &input, &result);
vitte_driver_input_destroy(&input);
vitte_driver_shutdown(&driver);
```
