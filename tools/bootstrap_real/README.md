# Bootstrap Real

This directory owns the durable bootstrap path for the real native Vitte compiler.

The source of truth is `src/vitte/compiler/main.vit`. Working artifacts are written
under `target/bootstrap-real/`, and reports are written under `target/reports/`.
Nothing in this path requires `/private/tmp`.

## Commands

Build from the single trusted stage0 compiler and verify the result:

```sh
python3 tools/bootstrap_real/bootstrap_real.py --stage0 target/bootstrap-real/stage0/vitte
```

Verify an already-built candidate:

```sh
python3 tools/bootstrap_real/bootstrap_real.py --candidate target/bootstrap-real/vitte
```

Run the local tests:

```sh
python3 tools/bootstrap_real/test_bootstrap_real.py
```

## Gate Contract

The gate accepts only a native candidate aligned with
`src/vitte/compiler/main.vit`. It requires the compiler entry marker
`run_cli_main_with_ice_boundary` and the canonical entrypoint marker
`COMPILER_ENTRY_POINT=src/vitte/compiler/main.vit`.

The only accepted stage0 path is `target/bootstrap-real/stage0/vitte`. That
binary must itself pass the same native Vitte checks before it is allowed to
compile `src/vitte/compiler/main.vit`.

The gate rejects binaries that still contain self-copy or bootstrap bridge
markers such as `BOOTSTRAP_FULL_COMPILER`,
`VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE`, `vitte-bootstrap-payload`,
`_command_build`, or `_copy_file`.

This path is intentionally 100% Vitte: the only build mode is a Vitte compiler
building `src/vitte/compiler/main.vit`, followed by strict verification of the
native candidate. C, C++, bridge wrappers, self-copy dispatchers, and temporary
`/private/tmp` artifacts are not accepted.

This tool intentionally fails when no real Vitte candidate exists. It is a
durable bootstrap boundary, not a fallback compiler.
