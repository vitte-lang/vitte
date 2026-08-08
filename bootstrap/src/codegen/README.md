# Bootstrap Codegen

The codegen layer is the backend-neutral emission facade for the bootstrap
compiler. It accepts validated AST, HIR, or IR input and dispatches to the C17
backend.

Invariants:
- No dependency on `runtime/*`.
- Errors use `bootstrap/src/api/error.h`.
- AST input must be initialized and have a root module.
- HIR input must be initialized and have a root module.
- IR input must be initialized and have a module.
- Output is deterministic and delegated to the selected backend.
- Codegen does not duplicate C17 generation logic.
- HIR emission lowers through bootstrap IR before backend dispatch.

Lifecycle:
- Initialize `vitte_codegen_options_t` with `vitte_codegen_options_init`.
- Set backend/input/output and output sink.
- Call `vitte_codegen_init`.
- Emit through `vitte_codegen_emit` or the typed helpers for AST/HIR/IR.
- Read `vitte_codegen_result_t` for bytes, lines, units, functions, and errors.
- Call `vitte_codegen_destroy`.

Supported backend:
- `VITTE_CODEGEN_BACKEND_C17`

Supported input:
- `VITTE_CODEGEN_INPUT_AST`
- `VITTE_CODEGEN_INPUT_HIR`
- `VITTE_CODEGEN_INPUT_IR`

Supported outputs:
- fixed caller-provided buffer
- output file path

Options mapped to C17:
- source name
- output path
- indentation width
- include emission
- debug comments

Limitations:
- AST emission remains supported for compatibility.
- HIR support depends on successful lowering to bootstrap IR.
- C17-specific type and instruction support is defined by `bootstrap/src/backend/c17`.
