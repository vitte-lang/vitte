# Bootstrap Real

This directory owns the durable bootstrap path for the real native Vitte compiler.

The source of truth is `src/vitte/compiler/main.vit`. Working artifacts are written
under `target/bootstrap-real/`, and reports are written under `target/reports/`.
Nothing in this path requires `/private/tmp`.

## Commands

Install the single trusted stage0 compiler:

```sh
python3 tools/bootstrap_real/bootstrap_real.py --install-stage0 <real-vitte-binary>
```

Build from the single trusted stage0 compiler and verify the result:

```sh
python3 tools/bootstrap_real/bootstrap_real.py --stage0 target/bootstrap-real/stage0/vitte
```

That command compiles `src/vitte/compiler/main.vit` to
`target/bootstrap-real/vitte`, then verifies the produced compiler.
Before compiling, stale `target/bootstrap-real/vitte` and
`target/bootstrap-real/vitte.bootstrap-bridge` artifacts are moved to
`target/bootstrap-real/quarantine/` and recorded in the report, so a failed
stage0 build cannot be mistaken for a fresh compiler and stale artifacts remain
auditable.

Expected outputs:

```text
target/bootstrap-real/vitte
target/reports/bootstrap_real_gate.json
target/reports/bootstrap_real_gate.md
```

Verify an already-built candidate:

```sh
python3 tools/bootstrap_real/bootstrap_real.py --candidate target/bootstrap-real/vitte
```

Build Stage1 from the verified bootstrap compiler:

```sh
python3 tools/bootstrap_real/bootstrap_real.py --stage1
```

That command runs:

```sh
target/bootstrap-real/vitte build src/vitte/compiler/main.vit -o target/stage1/vitte
```

Expected Stage1 output:

```text
target/stage1/vitte
```

Build Stage2 from Stage1:

```sh
python3 tools/bootstrap_real/bootstrap_real.py --stage2
```

That command runs:

```sh
target/stage1/vitte build src/vitte/compiler/main.vit -o target/stage2/vitte
```

Expected Stage2 output:

```text
target/stage2/vitte
```

Smoke-test the installed stage0 directly:

```sh
target/bootstrap-real/stage0/vitte --version
target/bootstrap-real/stage0/vitte --help
target/bootstrap-real/stage0/vitte check src/vitte/compiler/main.vit
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
Each required marker is checked independently for stage0 sources, installed
stage0 binaries, and bootstrap candidates.

The only accepted stage0 path is `target/bootstrap-real/stage0/vitte`.
`--install-stage0` validates a source binary as a real native Vitte compiler
before copying it there. The copy is staged under `target/bootstrap-real/stage0/`
and atomically replaces the trusted stage0 only after validation. The installed
stage0 must itself pass the same native Vitte checks before it is allowed to
compile `src/vitte/compiler/main.vit`.
Failed installs leave neither a replacement stage0 nor a `.installing` artifact,
and reports include both the install source and the installed artifact when an
install succeeds.

The gate rejects binaries that still contain self-copy or bootstrap bridge
markers such as `BOOTSTRAP_FULL_COMPILER`,
`VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE`, `vitte-bootstrap-payload`,
`_command_build`, or `_copy_file`.
Each forbidden marker is checked independently for stage0 sources, installed
stage0 binaries, and bootstrap candidates.
Stage0 installation also rejects scripts and sources copied from `/private/tmp`.

This path is intentionally 100% Vitte: the only build mode is a Vitte compiler
building `src/vitte/compiler/main.vit`, followed by strict verification of the
native candidate. C, C++, bridge wrappers, self-copy dispatchers, and temporary
`/private/tmp` artifacts are not accepted.

This tool intentionally fails when no real Vitte candidate exists. It is a
durable bootstrap boundary, not a fallback compiler.
