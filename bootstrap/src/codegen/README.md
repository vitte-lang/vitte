# Bootstrap Codegen

The codegen layer is the backend-neutral emission facade for the bootstrap compiler. It currently accepts validated AST input and dispatches to the C17 backend.

Invariants:
- No dependency on `runtime/*`.
- Errors use `bootstrap/src/api/error.h`.
- AST input must be initialized and have a root module.
- HIR and IR inputs are explicit `UNSUPPORTED` until those layers are implemented.
- Output is deterministic and delegated to the selected backend.
- Codegen does not duplicate C17 generation logic.

Lifecycle:
- Initialize `vitte_codegen_options_t` with `vitte_codegen_options_init`.
- Set backend/input/output and output sink.
- Call `vitte_codegen_init`.
- Emit through `vitte_codegen_emit`, `vitte_codegen_emit_ast_to_buffer`, or `vitte_codegen_emit_ast_to_file`.
- Read `vitte_codegen_result_t` for bytes, lines, units, functions, and errors.
- Call `vitte_codegen_destroy`.

Supported backend:
- `VITTE_CODEGEN_BACKEND_C17`

Supported input:
- `VITTE_CODEGEN_INPUT_AST`

Reserved inputs:
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
- HIR and IR are placeholders elsewhere in the bootstrap tree and are rejected here.
- C17-specific type and node support is defined by `bootstrap/src/backend/c17`.
- This facade is intentionally thin until parser, HIR, IR, and semantic lowering are implemented.
