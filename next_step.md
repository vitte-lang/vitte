# Next Step
## Reprise Mardi 2026-05-19 -> Mercredi 2026-05-20

1. Done: parité structurée stage1/stage2 sur `parse/check/dump-native-ir` (hash + diff).
2. Done: surfaces JSON natives exposées sur `parse/check`:
- `--dump-ast-json`
- `--dump-hir-json`
- `--dump-mir-json`
- `--diagnostics-json`
3. Done: contrat stable `schema_version=1.0.0` appliqué aux surfaces `ast/hir/mir/diagnostics`.
4. Done: gate de parité étendu à un corpus compiler (`driver/frontend/middle/diagnostics`) avec rapport agrégé.
5. Done: test de non-régression ajouté pour flags JSON + contrat de schéma.

## Prochaine étape

1. Remplacer progressivement les métriques structurelles bootstrap par des payloads pipeline réels (nœuds/liaisons/diagnostics détaillés).
2. Versionner le contrat en `1.1.x` uniquement lors d’ajout rétro-compatible; `2.0.0` pour toute rupture.
3. Étendre le corpus de parité à plus de sources `src/vitte/compiler` et publier la baseline dans `target/reports/stage_parity/`.
