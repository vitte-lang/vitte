# Reproducibility

Use deterministic checks:
- rebuild same source multiple times
- compare binary hashes
- compare diagnostics and dumps

Commands:
- `vittec self-host-check --repeat 3 --compare all`
- `vittec ci-gate --strict`
