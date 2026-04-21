# Carte de migration Stdlib (auto)

Généré à partir des listes d'autorisation historiques dans `tools/`.

| Emplacement | Ancien import | Nouvel import |
|---|---|---|

## Enveloppes historiques (`<pkg>.vit` -> `mod.vit`)

| Module stdlib | Ancienne entrée | Nouvelle entrée |
|---|---|---|
| `abi` | `abi/abi.vit` | `abi/mod.vit` |
| `actor` | `actor/actor.vit` | `actor/mod.vit` |
| `alerts` | `alerts/alerts.vit` | `alerts/mod.vit` |
| `alloc` | `alloc/alloc.vit` | `alloc/mod.vit` |
| `array` | `array/array.vit` | `array/mod.vit` |
| `ast` | `ast/ast.vit` | `ast/mod.vit` |
| `async` | `async/async.vit` | `async/mod.vit` |
| `audio` | `audio/audio.vit` | `audio/mod.vit` |

## Noms historiques (`_` -> `/`)

| Nom historique | Hiérarchie cible |
|---|---|
| `circuit_breaker` | `circuit/breaker` |
| `config_derive` | `config/derive` |
| `const_eval` | `const/eval` |
| `contracts_registry` | `contracts/registry` |
| `docsgen_modules` | `docsgen/modules` |
| `event_bus` | `event/bus` |
| `feature_flags` | `feature/flags` |
| `http_client` | `http/client` |
| `migration_playbook` | `migration/playbook` |
| `mock_http` | `mock/http` |
| `module_index` | `module/index` |
| `random_secure` | `random/secure` |
| `rate_limit` | `rate/limit` |
| `release_guard` | `release/guard` |
| `schema_registry` | `schema/registry` |
| `testkit_modules` | `testkit/modules` |
