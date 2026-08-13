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
candidate artifact. Its report deliberately records runtime readiness
separately; source-sensitive code generation alone is not a self-host proof.
