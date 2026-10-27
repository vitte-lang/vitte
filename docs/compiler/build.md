# Build and Compiler Paths

## Active compiler path

The standard `make` target builds a stage bootstrap compiler, uses it to compile
the native Vitte pipeline, then links that pipeline into the normal executable.
The compile/run path is:

1. `src/bootstrap/main.c` and `src/bootstrap/cli/` — process entry, CLI
   compatibility, source loading, and module-graph assembly.
2. `src/compiler/driver/pipeline.vitte` — native pipeline coordinator.
3. `src/compiler/lexer/lexer.vitte`, `src/compiler/parser/parser.vitte`, and
   `src/compiler/sema/sema.vitte` — native lexing, parsing, and semantic
   analysis for the ordinary compile/run route.
4. `src/compiler/hir/hir.vitte` and `src/compiler/ir/ir.vitte` — native HIR/IR
   lowering and validation.
5. `src/compiler/backend/c17/ir_emitter.vitte` — default C17 emission for the
   supported IR subset; `ast_emitter.vitte` handles constructs without IR
   lowering.
6. `src/bootstrap/backend/c17/` — host C compilation, object generation,
   linking, execution, and compatibility emission paths.

`compile`, `run`, `check`, `--emit-c`, and `--emit-ir` use the native pipeline
for the implemented subset and route supported programs through HIR/IR to the
native IR C17 backend. Language features without complete native support stay
on the bootstrap compatibility pipeline; selected native frontend failures
remain authoritative. The host C driver handles object generation, linking,
and execution; inspection commands retain bootstrap stages.

## Staged native Vitte path

The Vitte parser and semantic surface remain partial. HIR/IR lowering does
not yet cover the full language, and parser/sema diagnostic locations in the
CLI remain incomplete. At present, `make native-frontend` stops on source
diagnostics in the compiler's arena, scanner, HIR, IR, and semantic modules, so
it does not produce a fresh linked native executable. A full self-hosted
rebuild is not yet established.

The authoritative feature and CLI status is in
[`src/docs/language_coverage.md`](../../src/docs/language_coverage.md). See [the
compiler source map](../MODULE_REFERENCE.html) and [Make targets](../MAKE_TARGETS.html)
for the current implementation overview and build commands.
