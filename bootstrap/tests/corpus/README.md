# Bootstrap Corpus

`make -C bootstrap corpus` is the required bootstrap corpus gate.

It covers:

- `check` for mono-file and multi-file inputs
- `emit-c` for mono-file and multi-file inputs
- `build` for mono-file and multi-file inputs
- import chains, rich imports, module aliases, explicit exports, and export-star
- negative diagnostics for glob collisions, unsupported `use module::symbol`, and imported call type mismatches
