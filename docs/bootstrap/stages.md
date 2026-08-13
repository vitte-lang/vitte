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
source trees. `make compiler-source-sensitivity-gate` proves that all 971 Vitte
compiler modules are compiled and that a controlled driver mutation changes the
candidate artifact at `target/compiler-source-sensitivity/stage1-candidate`.
Its report deliberately records runtime readiness
separately; source-sensitive code generation alone is not a self-host proof.

Byte parity has three independent scopes. The verified stage0 installation must
match the signed trust root, stage1 must match stage2 at the self-host fixed
point, and stage2 must match the release and installed binaries. Stage0 is not
required to match a compiler rebuilt from the current sources.

Copying is allowed only at the two installation boundaries: signed stage0 to
the verified stage0 location, and verified release to installed command paths.
The bootstrap compiler, stage1, stage2 and release must each be emitted by a
compiler `build` invocation.
