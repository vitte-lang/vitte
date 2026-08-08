# bootstrap/src/module

The module layer owns bootstrap compiler state for one resolved source module.
It sits above `import`, `filesystem`, and `lexer`, and below future parser,
HIR, and IR stages.

## Contract

- No dependency on `runtime/*`.
- Errors use `bootstrap/src/api/error.h`.
- Module names use import-layer validation rules.
- Paths are bounded by `VITTE_FS_MAX_PATH`.
- Source reads are bounded by `max_source_bytes`.
- Import storage is fixed-size and bounded by `max_imports`.
- The module layer never mutates source files.

## Lifecycle

1. `vitte_module_init`
2. `vitte_module_set_name` and `vitte_module_set_source_path`, or
   `vitte_module_resolve`
3. `vitte_module_load_source` or `vitte_module_attach_source`
4. `vitte_module_lex`
5. `vitte_module_add_import` and `vitte_module_resolve_imports`
6. parser / HIR / IR attachment later
7. `vitte_module_destroy`

`vitte_module_resolve` is the normal bootstrap entry point. It resolves a
module name through the import resolver, stores the canonical source path, and
optionally reads the file immediately.

## State Machine

The public state is monotonic except on failure:

- `UNINITIALIZED`
- `CREATED`
- `RESOLVED`
- `LOADED`
- `LEXED`
- `PARSED`
- `LOWERED_HIR`
- `LOWERED_IR`
- `FAILED`

The current implementation drives the lifecycle through resolve, load, lex, and
import resolution. Parser and lowering states are reserved for the next stages.

## Ownership

`vitte_module_t` may either borrow or own its main source buffer:

- `source_owned == false`: caller owns the buffer passed to
  `vitte_module_attach_source`
- `source_owned == true`: the module releases the buffer during
  `vitte_module_destroy`

Each `vitte_module_import_t` follows the same rule for imported source. Repeated
import resolution releases old owned buffers before replacing them.

## Imports

Imports are declared explicitly through `vitte_module_add_import`. Resolution is
separate so the parser can register imports first, then the driver can resolve
them in a bounded pass.

For each import entry the module layer stores:

- validated module name
- relative/global resolution flag
- resolved path
- optional loaded source
- per-import last error

Duplicate imports with the same name and relative flag are ignored.

## Lexing

`vitte_module_lex` uses the bootstrap lexer over the loaded source buffer. The
module stores:

- last token returned by the lexer
- total token count for the pass
- cumulative stats for load, lex, bytes, imports, and failures

Lexing stops on EOF or on the first lexer error token.

## Limitations

- No parser attachment yet.
- No transitive import graph yet.
- No HIR or IR lowering yet.
- `vitte_module_result_t` is initialized and reserved for higher-level driver
  surfaces.
