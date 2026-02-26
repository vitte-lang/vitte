# Packages Migration Plan

Objectif: passer d'un usage historique `std/*` vers un espace `packages/*` cohérent, avec compatibilité progressive.

## 1) Conventions cibles

- Entrée package obligatoire: `mod.vit`
- Métadonnées package obligatoires: `info.vit`
- Noms publics sans `_` (préférer la hiérarchie avec `/`)
- `std/*` conservé uniquement comme façade legacy (re-export)

## 2) Legacy existant (gelé, pas de nouveaux cas)

Wrappers legacy actifs (import chemin historique):
- `abi/abi.vit`
- `actor/actor.vit`
- `alerts/alerts.vit`
- `alloc/alloc.vit`
- `array/array.vit`
- `ast/ast.vit`
- `async/async.vit`
- `audio/audio.vit`

Noms legacy avec `_`:
- `circuit_breaker`
- `config_derive`
- `const_eval`
- `feature_flags`
- `http_client`
- `mock_http`
- `random_secure`
- `rate_limit`
- `schema_registry`

## 3) Table de migration old -> new

- `http_client` -> `http/client`
- `mock_http` -> `http/mock`
- `random_secure` -> `random/secure`
- `schema_registry` -> `schema/registry`
- `rate_limit` -> `rate/limit`
- `circuit_breaker` -> `resilience/circuit_breaker`
- `feature_flags` -> `config/feature_flags`
- `const_eval` -> `compiler/const_eval`
- `config_derive` -> `config/derive`

Doublons/clarifications:
- `process` vs `subprocess`: garder `process` API publique et déplacer le bas niveau sous `process/internal` (ou alias `subprocess` -> `process/spawn`).
- `runtime` vs `core`: `core` = primitives stables, `runtime` = bootstrap/exécution.
- `serialize` vs `codec`: `codec` = encodage concret (json/yaml/bin), `serialize` = façade stable.

## 4) Familles cibles

- `core/*`: types, collections, alloc, borrow
- `system/*`: fs, env, process, net, path
- `data/*`: json, yaml, sql, db, cache, stream
- `app/*`: http, ws, cli, config, jobs

## 5) Compatibilité et migration

- Phase A: créer les nouveaux packages + re-exports depuis les anciens.
- Phase B: warnings de dépréciation sur imports legacy.
- Phase C: auto-fix (`vitte mod doctor --fix`) old -> new.
- Phase D: suppression progressive des aliases legacy après une fenêtre de compat.

Plan C2 (proposé):
- Ajouter `vitte mod doctor --fix --write <file.vit>` pour appliquer les rewrites directement dans le fichier.
- Garder `--fix` sans `--write` en mode dry-run (suggestions texte uniquement).
- Ajouter un mode `--fix --write --backup` pour créer `<file>.bak` avant écriture.
- Prioriser les rewrites sûrs et syntaxiques (`vitte/x/x` -> `vitte/x`) avant toute transformation plus intrusive.

Fenêtre de dépréciation:
- `v2.x`: compat active + warning `E1020` sur `vitte/x/x`.
- `v3.0`: suppression de la compat `vitte/x/x` (import canonique obligatoire `vitte/x`).

## 6) CI

Règle active: `make package-layout-lint`.

Cette règle:
- refuse tout nouveau package sans `mod.vit`/`info.vit`;
- refuse tout nouveau nom avec `_`;
- conserve une allowlist legacy pour l'existant.

Fichiers d'allowlist:
- `tools/package_entry_legacy_allowlist.txt`
- `tools/package_name_legacy_allowlist.txt`
