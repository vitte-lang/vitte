# Vitte CLI

The CLI layer owns argument parsing and command dispatch for the C17 compiler binary. `compiler/src/main.c` is intentionally only a wrapper around `vitte_cli_main`.

Commands:
- `check <input.vit>` validates a minimal Vitte source file.
- `emit-c <input.vit> [-o output.c]` emits C17 source.
- `build <input.vit> [-o output] [--cc cc] [--keep-c]` builds a native executable.
- `run <input.vit> [-o output] [--cc cc]` builds then runs the executable.
- `--help`/`-h` prints usage.
- `--version`/`-V` prints the compiler version.

Exit codes:
- `0`: success
- `1`: usage or argument error
- `2`: check/build/run failure
- `3`: internal error such as allocation failure

Rules:
- `CC` is used as the default C compiler when `--cc` is not provided.
- `-o`/`--output` requires a value.
- Unknown options and duplicate input paths are rejected.
- `--` stops option parsing and treats the next argument as the input path.
- Sidecar `.compiler.c` files are removed after build/run unless `--keep-c` is set.

Limitations:
- The compiler parser is intentionally minimal and currently recognizes a `proc main` body with an optional integer `give`.
- The full parser, semantic analysis, and backend layers will replace this narrow path as those compiler directories are completed.
