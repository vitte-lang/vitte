# Alloc Patterns

## Purpose
Guide pratique pour `vitte/alloc` en mode stable avec garde-fous OOM, alignement et integrite.

## Stable Surface
- `stable_alloc`, `stable_free`, `stable_realloc`, `stable_calloc`
- `stable_stats`, `stable_reset`, `stable_ready`, `stable_package_meta`
- `stable_api_version`, `api_version`, `stable_api_surface`

## Pool Allocator
- Definir des quotas par `PoolId` (`limit_bytes + burst_bytes`).
- Appeler `stable_alloc(..., pool_id("tenant:*"))` pour isoler la pression memoire.
- Sur OOM, inspecter `alloc_error_name(err)` et `stable_stats(runtime).alloc_failures`.

## Arena / Bump Reset
- Utiliser `stable_reset(runtime)` pour vider proprement en fin de phase.
- Conserver `peak_bytes` pour comparer les cycles de charge.

## Small-Object Slab
- Aligner les petits objets (`Alignment` puissance de 2).
- En mode `Slab`, preferer des tailles homogenes pour limiter la fragmentation.

## OOM Policy
- `OomPolicy.ReturnError`: echec explicite, pas de latch.
- `OomPolicy.Panic`: runtime latche en etat panic (`panic_latched=true`) apres OOM.
- Pour reprise test, utiliser `clear_panic(runtime)`.

## Poison + Canaries
- Poison alloc/free actif via `with_poison_bytes(runtime, alloc_pattern, free_pattern)`.
- Canaries actives via `with_canaries(runtime, true)`.
- Integrite verifiee sur `stable_free` / `stable_realloc`.
- Injection test possible via `experimental_corrupt_canary(runtime, handle)`.

## Anti-Patterns
- Melanger handles entre pools sans contrat explicite.
- Ignorer `Alignment` (valeur non puissance de 2).
- Traiter `stable_realloc` comme destructif a l'echec: le handle precedent reste valide.
- Desactiver les checks integrite dans les tests de non-regression.
