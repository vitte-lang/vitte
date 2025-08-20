# Vitte — Benchmarks Ultra

Suite **macro + micro** prête à l'emploi, avec scripts, baselines, CI et profils.
- **Micro** (Criterion) : tokenizer, parser, VM stack, bytecode ops, alloc, collections, channels.
- **Macro** (CLI) : compile & run de programmes Vitte (fib, primes, JSON, echo).
- **CI** : workflow GitHub pour exécuter et publier les résultats en artefacts.
- **Baselines** : JSON/CSV comparables avec `scripts/compare.py`.

⚙️ **Deux modes d’intégration**
1) *Direct*: les benches importent `vitte_core`, `vitte_vm`, etc.
2) *Shims*: si les APIs diffèrent, ajoute `--features shims` aux benches micro
   et adapte `benches/shims.rs` (tout est scaffoldé).

## Démarrage rapide
```bash
# Micro (Criterion)
cd benchmarks/micro
cargo bench --features shims      # si besoin de shims
# Macro
cd ../macro
./tools/run-macro.sh
```

## Comparer avec une baseline
```bash
cd benchmarks/scripts
python3 compare.py ../baselines/micro/2025-08-17.json ../micro/target/criterion/report/newest.json
```

## Perf & Profilage
- Linux: `perf`, `flamegraph` (script dans `benchmarks/perf/`)
- macOS: DTrace / Instruments (instructions inclues)
