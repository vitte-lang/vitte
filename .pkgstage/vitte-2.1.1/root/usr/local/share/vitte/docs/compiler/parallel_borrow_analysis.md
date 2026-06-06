# Parallel Borrow Analysis (174)

Objectif: rendre `borrowck` compatible avec le parallélisme.

## Contrats

- `Send` equivalent:
  - une valeur déplacée vers un contexte thread doit être transférable sans alias mutable concurrent.
- `Sync` equivalent:
  - tout accès partagé mutable cross-thread doit être explicitement synchronisé.
- Shared mutation:
  - mutation concurrente de ressource partagée sans lock/atomic est rejetée.
- Thread ownership:
  - une référence mutable ne doit pas s’échapper hors de son propriétaire logique vers un thread concurrent.

## Diagnostics de fondation

- `BORROWCK_E_PARALLEL_SEND_VIOLATION`
- `BORROWCK_E_PARALLEL_SYNC_VIOLATION`
- `BORROWCK_E_SHARED_MUTATION_ACROSS_THREADS`
- `BORROWCK_E_THREAD_OWNERSHIP_VIOLATION`
- `BORROWCK_E_CONCURRENCY_RACE_RISK`

## Implémentation actuelle

- Analyse heuristique de patterns thread/send/sync/race à partir des statements HIR.
- Fusion des diagnostics avec le borrowck existant (ownership/moves/loans/lifetimes).
