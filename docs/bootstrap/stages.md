# Compiler Generations

Bootstrap configuration contains one seed trust root. Self-hosting validation
creates two temporary compiler generations from the canonical source entry and
compares their artifacts.

```text
vittec0.seed -> generation 1 -> generation 2
```

These generations are audit outputs, not bootstrap fallbacks or repository
source trees. Run `python3 tools/selfhost_completion_audit.py` to inspect the
current transition state.
