# Compiler Generations

Bootstrap configuration contains one signed native stage0 selected by the host
OS/architecture tuple. Self-hosting validation constructs stage1, stage2 and the
release compiler from the canonical source entry, requires stage1/stage2 byte
parity, and repeats the chain to prove reproducible hashes.

```text
vittec0.seed -> generation 1 -> generation 2
```

These generations are audit outputs, not bootstrap fallbacks or repository
source trees. Run `python3 tools/selfhost_completion_audit.py` to inspect the
current transition state.
