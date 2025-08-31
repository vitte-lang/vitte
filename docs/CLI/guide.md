# CLI — Guide d’usage

Binaire principal `vitte` (features `cli`) et sous-bins dédiés (`vitte-fmt`, `vitte-check`, ...).

## Sous-commandes typiques
```bash
vitte fmt   path/            # formatter
vitte check path/            # analyse rapide
vitte pack  src.vit -o out.vitbc   # compile en VITBC
vitte dump  out.vitbc        # inspecte VITBC
vitte run   out.vitbc        # exécute
vitte graph src.vit --dot    # export DOT (CFG/DFG)
```

## Complétions
Voir [COMPLETIONS.md](../COMPLETIONS.md).
