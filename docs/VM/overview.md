# VM Vitte — Interpréteur

- **Piles/regs** : pile de valeurs + registres temporaires.
- **Appels** : frames, trampolines, ABI interne stable.
- **Mémoire** : `Arena`/bump (option `arena`), `SmallVec` (`small`) lorsque pertinent.
- **Parallélisme** : pur interprète (pas de JIT), `rayon` optionnel pour tâches parallèles.
- **Snapshots** : (de)sérialisation d’état si `serde` activée.

## Modes
- `std` : I/O optionnels, horloge, FS, REPL.
- `alloc-only` : exécution embarquée.

## Debugging
- Traces via `tracing`, `RUST_LOG=vitte_vm=trace`.
