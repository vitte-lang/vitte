# Benchmarks

Crate : `benchmarks/` (Criterion).

```bash
cargo bench -p vitte-benchmarks
```

Recommandations :
- inputs stables et déterministes
- comparer *avant/après* feature/pull
- éviter de faire tourner en CI systématique (bruit), préférer des jobs dédiés
