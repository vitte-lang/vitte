# Compiler Generations

Bootstrap configuration contains one signed native stage0 selected by the host
OS/architecture tuple. Self-hosting validation constructs stage1, stage2 and the
release compiler from the canonical source entry and repeats the chain to prove
reproducible hashes. Source sensitivity is a separate invariant: changing the
driver must change the rebuilt stage before fixed-point parity is evaluated.

```text
signed stage0 -> stage1 -> stage2 -> release -> bin/vitte
```

These generations are audit outputs, not bootstrap fallbacks or repository
source trees. `make compiler-source-sensitivity-gate` proves that the canonical
Vitte compiler module closure is compiled and that a controlled driver mutation
changes the candidate artifact at
`target/compiler-source-sensitivity/stage1-candidate`.
Its report deliberately records runtime readiness
separately; source-sensitive code generation alone is not a self-host proof.

The stage-specific artifact manifests are
`toolchain/stage0/stage0-manifest.json`,
`toolchain/stage1/stage1-manifest.json`,
`toolchain/stage2/stage2-manifest.json`, and
`toolchain/release/release-manifest.json`. Run `make toolchain-manifests-gate`
to validate their schemas, predecessor policies, hashes, sizes, native
executability, forbidden markers, signed stage0 binding, and byte-parity rules.

Byte parity has three independent scopes. The verified stage0 installation must
match the signed trust root, stage1 must match stage2 at the self-host fixed
point, and stage2 must match the release and installed binaries. Stage0 is not
required to match a compiler rebuilt from the current sources.

Copying is allowed only at the two installation boundaries: signed stage0 to
the verified stage0 location, and verified release to installed command paths.
The bootstrap compiler, stage1, stage2 and release must each be emitted by a
compiler `build` invocation.

## Resource-bounded builds

The stage1 gate and the Python bootstrap builders use `resource_guard.py`.
Builds are sequential and stop when the sampled RSS of their process group
(compiler plus native-toolchain children) exceeds **1,400,000 KiB**, after
600 seconds, or after 64 MiB of diagnostic output. These are sampled safeguards,
not an OS-enforced hard memory reservation: brief peaks between samples and
unrelated applications' memory are not covered. Process inspection must work;
if `ps` is unavailable or denied, no build is started. Reports include the
observed peak RSS and the stop reason. Captured diagnostic tails are bounded.

To resume only stage2 without rebuilding the whole chain:

```sh
python3 tools/bootstrap_real/bootstrap_real.py --stage2
```

For a targeted command with the same safeguards:

```sh
python3 tools/bootstrap_real/resource_guard.py \
  --report target/reports/stage2_build_resources.json -- \
  target/stage1/vitte build src/vitte/compiler/main.vit -o target/stage2/vitte
```

An interrupted or failed build is not a validated stage: do not refresh its
manifest or promote it to release. Inspect the build report and diagnostics
before resuming; do not automatically increase the memory threshold.

Focused regressions (all guarded, no full self-host rebuild):

```sh
python3 tools/bootstrap_real/test_resource_guard.py
python3 tools/bootstrap_real/test_streaming_codegen.py
```
