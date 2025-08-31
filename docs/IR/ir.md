# IR Vitte (intermédiaire)

- **Nœuds principaux** : `Func`, `Block`, `Instr`, `Value`, `Phi`.
- **Graphes** : CFG (contrôle) & DFG (données). Index rapides via `indexmap`/`petgraph`.
- **Passes** : validation SSA, DCE, const-fold, vérif de dominance.
- **Export** : DOT via `vitte-ir/dot` (petgraph::dot).

## Invariants
- Les `Id<T>` sont non-null (0 réservé).
- Un `Block` a une terminaison explicite (`ret`, `br`, `br_if`).
- Le DFG ne référence que des valeurs dominantes au point d’usage.
