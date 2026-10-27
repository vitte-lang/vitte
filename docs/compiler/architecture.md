# Compiler Architecture (Current Implementation)

The default Makefile and CMake targets are configured to link the native Vitte
pipeline from `src/compiler/` into the CLI. The pipeline handles ordinary `compile`, `run`,
`check`, `--emit-c`, and `--emit-ir` through native lexer, parser, semantic
analysis, and HIR/IR validation. The native IR C17 backend emits supported
programs by default; declarations and control flow without IR lowering use the
native AST emitter as a compatibility path. Bootstrap C still provides
CLI/source-graph setup, inspection, and the host C compiler/linker boundary.
This is staged integration, not a full self-hosted compiler. The current
`make native-frontend` source build stops on diagnostics in arena, scanner,
HIR, IR, and semantic modules, so a fresh executable has not been built from
these sources yet.

## Default executable

The active path begins at `src/bootstrap/main.c` and runs through the CLI
driver in `src/bootstrap/cli/`:

```text
source/module graph -> bootstrap CLI -> Vitte lexer/parser/sema
                    -> validated HIR -> sealed IR -> Vitte IR-to-C17
                    -> host C compiler/linker
                    \-> Vitte AST-to-C17 for IR-incomplete constructs
```

`check` stops after frontend semantic analysis. The native pipeline is the
default for its supported subset and routes that code through HIR/IR. Arrays
and indexing, picks, contract clauses, export aliases, borrowed call results,
and several expression forms stay on the bootstrap compatibility pipeline.
A selected native invocation remains authoritative for diagnostics;
the driver does not retry a failed native analysis through bootstrap. Forms,
value-producing `if`, `for`, and `match` without the listed unsupported syntax
retain AST emission until their IR lowering is ready.

Key implementation files:

- `src/bootstrap/cli/command_spec.def` — command and option inventory.
- `src/bootstrap/cli/execution.c` and `src/bootstrap/cli/pipeline.c` — command
  routing and compilation phases.
- `src/compiler/driver/pipeline.vitte` — native phases and backend dispatch.
- `src/compiler/ir/ir.vitte` — HIR-to-IR lowering and validation.
- `src/compiler/backend/c17/ir_emitter.vitte` — default C17 backend for the
  supported IR subset.
- `src/compiler/backend/c17/ast_emitter.vitte` — aggregate compatibility path.

## Native Vitte pipeline

The active Vitte frontend includes source management at
`src/compiler/source/source.vitte`, lexing at
`src/compiler/lexer/lexer.vitte`, partial parsing at
`src/compiler/parser/parser.vitte`, semantic analysis at
`src/compiler/sema/sema.vitte`, HIR lowering and validation at
`src/compiler/hir/hir.vitte`, and IR lowering and validation at
`src/compiler/ir/ir.vitte`. Supported programs reach
`src/compiler/backend/c17/ir_emitter.vitte`; AST emission remains available
for constructs whose IR lowering is incomplete.

## Sources of truth

The canonical EBNF grammar is `grammar/vitte.ebnf`; generated grammar copies
are checked by `scripts/check-ebnf-frontend-sync.py`. Standard library source
modules are rooted at `modules/`. The one feature-status matrix is
[`src/docs/language_coverage.md`](../../src/docs/language_coverage.md).

See [the compiler source map](../MODULE_REFERENCE.md) and [native frontend port
notes](../bootstrap/frontend_native_port.md) for file-level status.
