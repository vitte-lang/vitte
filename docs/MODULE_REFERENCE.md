# Compiler Source Map

The executable is still launched and linked by the stage0 C bootstrap under
`src/bootstrap/`, but ordinary `compile`, `run`, and `--emit-c` inputs now use
the linked Vitte lexer, parser, semantic analyzer, codegen orchestrator, and
AST-to-C17 emitter. Bootstrap C still handles CLI/source-graph setup,
fallbacks, inspection and `--emit-ir` routes, and host C compilation/linking.
The compiler is not yet fully self-hosted.

## Default executable

These files provide the executable shell, compatibility routes, and host
toolchain boundary:

- `src/bootstrap/main.c`: process entry point.
- `src/bootstrap/cli/`: command setup, module graph assembly, fallback
  diagnostics, and routing.
- `src/bootstrap/parser/parser.c` and `src/bootstrap/sema/sema.c`: compatibility
  frontend used by fallback and `check` paths.
- `src/bootstrap/hir/`, `src/bootstrap/ir/`, and
  `src/bootstrap/backend/c17/ir_emitter.c`: bootstrap HIR/IR route, including
  `--emit-ir`.
- `src/bootstrap/backend/c17/`: host C17 compilation, object, link, and run
  integration.

## Staged Vitte implementation

These Vitte modules are linked into the ordinary compile/run frontend path:

- `src/compiler/source/source.vitte`: source management.
- `src/compiler/lexer/lexer.vitte`: lexer.
- `src/compiler/parser/parser.vitte`: partial native parser.
- `src/compiler/sema/sema.vitte`: native semantic analysis, with incomplete
  language coverage.
- `src/compiler/driver/pipeline.vitte`: native lexer/parser/sema/codegen
  orchestration and direct AST-to-C17 output.
- `src/compiler/backend/c17/ast_emitter.vitte`: active native frontend C17
  emitter.
- `src/compiler/driver/native_frontend_bridge.c`: C ABI wrappers for the
  Vitte-compiled pipeline.
- `src/compiler/backend/c17/ir_emitter.vitte`: separate IR backend not yet
  connected to this route.

## Other source roots

- `grammar/vitte.ebnf`: canonical EBNF grammar.
- `docs/grammar/vitte.pest`: Pest grammar source.
- `modules/`: standard library source tree.

For the current migration status, see [the native frontend port notes](bootstrap/frontend_native_port.md).
