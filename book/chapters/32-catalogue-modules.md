# 32. Catalogue Des Modules Vitte

Prérequis: à définir.
Voir aussi: à définir.
Ce document fournit une vue d'ensemble des modules de `src/vitte/packages`.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Catalogue Des Modules Vitte**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Catalogue Des Modules Vitte**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Catalogue Des Modules Vitte**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Catalogue Des Modules Vitte** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Catalogue Des Modules Vitte**.

## Exercice court

Prenez un exemple du chapitre sur **Catalogue Des Modules Vitte**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Catalogue Des Modules Vitte**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## Lecture rapide

- **Base langage/runtime**: fondations du langage, types, mémoire, exécution.
- **Compiler/tooling**: modules proches du compilateur et des diagnostics.
- **Système et I/O**: accès fichiers, processus, réseau, sécurité.
- **App/backend**: HTTP, données, observabilité, résilience.
- **Interop et plateformes**: ABI, FFI, codegen, cibles spécifiques.

## Familles De Modules

### Base langage et runtime

- `vitte/core`: primitives de base.
- `vitte/std`: façade standard.
- `vitte/runtime`: couches runtime communes.
- `vitte/alloc`, `vitte/borrow`, `vitte/slab`: allocation et gestion mémoire.
- `vitte/array`, `vitte/collections`, `vitte/indexmap`: structures de données.
- `vitte/math`, `vitte/hash`, `vitte/random_secure`: calcul, hash, aléatoire.

### Compiler et outillage

- `vitte/ast`, `vitte/hir`, `vitte/mir`, `vitte/typeck`: représentations intermédiaires et typage.
- `vitte/monomorphize`, `vitte/macros`, `vitte/const_eval`: transformations compile-time.
- `vitte/lsp`, `vitte/cli`, `vitte/lint`, `vitte/doc`: expérience développeur.
- `vitte/bench`, `vitte/test`, `vitte/fixtures`: validation et tests.

### Système, fichiers, shell, sécurité

- `vitte/io`, `vitte/fs`, `vitte/pathlib`, `vitte/process`, `vitte/subprocess`, `vitte/shell`.
- `vitte/crypto`, `vitte/tls`, `vitte/secrets`, `vitte/auth`, `vitte/jwt`.
- `vitte/env`, `vitte/config`, `vitte/config_derive`.

### Réseau, API et backend

- `vitte/net`, `vitte/http`, `vitte/http_client`, `vitte/ws`, `vitte/requests`.
- `vitte/openapi`, `vitte/jsonpath`, `vitte/serialize`, `vitte/codec`, `vitte/yaml`, `vitte/pickle`.
- `vitte/db`, `vitte/sql`, `vitte/orm`, `vitte/registry`.
- `vitte/observability`, `vitte/trace`, `vitte/stats`, `vitte/health`, `vitte/slo`, `vitte/log`, `vitte/alerts`.
- `vitte/queue`, `vitte/channel`, `vitte/stream`, `vitte/scheduler`, `vitte/actor`, `vitte/async`, `vitte/jobs`.
- `vitte/retry`, `vitte/timeout`, `vitte/rate_limit`, `vitte/circuit_breaker`, `vitte/bulkhead`, `vitte/idempotency`, `vitte/saga`.

### Domaines et utilitaires applicatifs

- `vitte/cache`, `vitte/kv`, `vitte/outbox`, `vitte/search`, `vitte/pagination`.
- `vitte/template`, `vitte/plot`, `vitte/image`, `vitte/audio`, `vitte/video`, `vitte/gui`.
- `vitte/pack`, `vitte/plugin`, `vitte/contracts`, `vitte/policy`, `vitte/eval`.
- `vitte/feature_flags`, `vitte/schema_registry`, `vitte/tenant`, `vitte/mock_http`, `vitte/ecs`, `vitte/ml`.

### Interop, ABI et génération de code

- `vitte/abi`, `vitte/ffi`, `vitte/codegen`, `vitte/codegen/llvm`, `vitte/codegen/gcc`, `vitte/codegen/cranelift`.

## Index Complet Des Modules Disponibles

- `vitte/abi`
- `vitte/actor`
- `vitte/alerts`
- `vitte/alloc`
- `vitte/array`
- `vitte/ast`
- `vitte/async`
- `vitte/audio`
- `vitte/audit`
- `vitte/auth`
- `vitte/bench`
- `vitte/borrow`
- `vitte/bulkhead`
- `vitte/cache`
- `vitte/channel`
- `vitte/circuit_breaker`
- `vitte/cli`
- `vitte/codec`
- `vitte/codegen`
- `vitte/codegen/cranelift`
- `vitte/codegen/gcc`
- `vitte/codegen/llvm`
- `vitte/collections`
- `vitte/config`
- `vitte/config_derive`
- `vitte/const_eval`
- `vitte/contracts`
- `vitte/core`
- `vitte/crypto`
- `vitte/datetime`
- `vitte/db`
- `vitte/doc`
- `vitte/ecs`
- `vitte/env`
- `vitte/eval`
- `vitte/feature_flags`
- `vitte/ffi`
- `vitte/fixtures`
- `vitte/fs`
- `vitte/graph`
- `vitte/gui`
- `vitte/hash`
- `vitte/health`
- `vitte/hir`
- `vitte/http`
- `vitte/http_client`
- `vitte/idempotency`
- `vitte/image`
- `vitte/indexmap`
- `vitte/io`
- `vitte/jobs`
- `vitte/jsonpath`
- `vitte/jwt`
- `vitte/kv`
- `vitte/lint`
- `vitte/log`
- `vitte/lru`
- `vitte/lsp`
- `vitte/macros`
- `vitte/math`
- `vitte/migrate`
- `vitte/mir`
- `vitte/ml`
- `vitte/mock_http`
- `vitte/monomorphize`
- `vitte/net`
- `vitte/observability`
- `vitte/openapi`
- `vitte/orm`
- `vitte/outbox`
- `vitte/pack`
- `vitte/pagination`
- `vitte/pathlib`
- `vitte/pickle`
- `vitte/plot`
- `vitte/plugin`
- `vitte/policy`
- `vitte/process`
- `vitte/queue`
- `vitte/random_secure`
- `vitte/rate_limit`
- `vitte/reflect`
- `vitte/registry`
- `vitte/requests`
- `vitte/retry`
- `vitte/rope`
- `vitte/runtime`
- `vitte/saga`
- `vitte/scheduler`
- `vitte/schema_registry`
- `vitte/search`
- `vitte/secrets`
- `vitte/serialize`
- `vitte/shell`
- `vitte/slab`
- `vitte/slo`
- `vitte/sql`
- `vitte/stats`
- `vitte/std`
- `vitte/stream`
- `vitte/subprocess`
- `vitte/template`
- `vitte/tenant`
- `vitte/test`
- `vitte/test/internal`
- `vitte/timeout`
- `vitte/tls`
- `vitte/trace`
- `vitte/typeck`
- `vitte/video`
- `vitte/ws`
- `vitte/yaml`

## Objectif

Rendre ce chapitre immédiatement exploitable avec un objectif concret, mesurable et réutilisable.

## Test mental

Question de contrôle: si vous modifiez une hypothèse clé, quel résultat doit changer et pourquoi?

## À faire

1. Exécuter l’exemple nominal.
2. Introduire un cas limite.
3. Vérifier la sortie et documenter l’écart.

## Corrigé minimal

Corrigé: conserver la version la plus simple qui respecte le contrat, puis ajouter un test de non-régression.

<!-- AUTO_EXPANSION_V1 START -->

## Approfondissement concret (sans répétition)

### 1. Snippet de référence

```vit
entry main at app/demo {
  return 0
}
```

### 2. Lecture du code ligne par ligne

1. `entry main at app/demo {` -> fixe le point d'entrée et le contexte d'exécution.
2. `return 0` -> rend la sortie observable sans ambiguïté.
3. `}` -> participe au flux principal du traitement.

### 3. Exécution réelle (entrée -> traitement -> sortie)

1. Entrée: préciser les valeurs acceptées et refusées.
2. Traitement: suivre le chemin nominal, puis la première garde.
3. Sortie: vérifier la valeur retournée ou l'erreur attendue.

### 4. Cas limite et erreur volontaire

- Cas limite: forcer la garde et confirmer la sortie de secours.
- Cas erreur: injecter un type inattendu et lire le diagnostic exact.
- Correction: modifier une seule ligne, recompiler, valider.

### 5. Refactor concret à faible risque

Méthode: garder la signature, simplifier une branche, et prouver que le comportement reste identique avec un test nominal + un test limite.

### 6. Série de scénarios représentatifs

Cas 1: pour **catalogue des modules vitte**, inspecter l'axe 'contrat d'entrée' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la trace de correction. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 2: pour **catalogue des modules vitte**, inspecter l'axe 'branche nominale' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider l'absence d'effet de bord. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 3: pour **catalogue des modules vitte**, inspecter l'axe 'garde limite' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la sortie exacte. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 4: pour **catalogue des modules vitte**, inspecter l'axe 'sortie de secours' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compréhension en relecture. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 5: pour **catalogue des modules vitte**, inspecter l'axe 'signature publique' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compatibilité des appels. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 6: pour **catalogue des modules vitte**, inspecter l'axe 'cohérence des types' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la lisibilité du message d'erreur. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 7: pour **catalogue des modules vitte**, inspecter l'axe 'ordre d'exécution' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le scénario de non-régression. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 8: pour **catalogue des modules vitte**, inspecter l'axe 'gestion d'erreur' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le comportement du cas limite. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 9: pour **catalogue des modules vitte**, inspecter l'axe 'lisibilité du flux' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la stabilité du contrat. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 10: pour **catalogue des modules vitte**, inspecter l'axe 'coût de maintenance' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la cohérence avant/après. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 11: pour **catalogue des modules vitte**, inspecter l'axe 'stabilité des appels' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la trace de correction. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 12: pour **catalogue des modules vitte**, inspecter l'axe 'lisibilité du module' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider l'absence d'effet de bord. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 13: pour **catalogue des modules vitte**, inspecter l'axe 'robustesse en refactor' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la sortie exacte. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 14: pour **catalogue des modules vitte**, inspecter l'axe 'stabilité du comportement' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compréhension en relecture. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 15: pour **catalogue des modules vitte**, inspecter l'axe 'qualité du diagnostic' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compatibilité des appels. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 16: pour **catalogue des modules vitte**, inspecter l'axe 'contrat d'entrée' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la lisibilité du message d'erreur. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 17: pour **catalogue des modules vitte**, inspecter l'axe 'branche nominale' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le scénario de non-régression. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 18: pour **catalogue des modules vitte**, inspecter l'axe 'garde limite' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le comportement du cas limite. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 19: pour **catalogue des modules vitte**, inspecter l'axe 'sortie de secours' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la stabilité du contrat. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 20: pour **catalogue des modules vitte**, inspecter l'axe 'signature publique' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la cohérence avant/après. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 21: pour **catalogue des modules vitte**, inspecter l'axe 'cohérence des types' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la trace de correction. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 22: pour **catalogue des modules vitte**, inspecter l'axe 'ordre d'exécution' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider l'absence d'effet de bord. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 23: pour **catalogue des modules vitte**, inspecter l'axe 'gestion d'erreur' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la sortie exacte. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 24: pour **catalogue des modules vitte**, inspecter l'axe 'lisibilité du flux' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compréhension en relecture. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 25: pour **catalogue des modules vitte**, inspecter l'axe 'coût de maintenance' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compatibilité des appels. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 26: pour **catalogue des modules vitte**, inspecter l'axe 'stabilité des appels' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la lisibilité du message d'erreur. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 27: pour **catalogue des modules vitte**, inspecter l'axe 'lisibilité du module' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le scénario de non-régression. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 28: pour **catalogue des modules vitte**, inspecter l'axe 'robustesse en refactor' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le comportement du cas limite. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 29: pour **catalogue des modules vitte**, inspecter l'axe 'stabilité du comportement' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la stabilité du contrat. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 30: pour **catalogue des modules vitte**, inspecter l'axe 'qualité du diagnostic' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la cohérence avant/après. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 31: pour **catalogue des modules vitte**, inspecter l'axe 'contrat d'entrée' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la trace de correction. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 32: pour **catalogue des modules vitte**, inspecter l'axe 'branche nominale' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider l'absence d'effet de bord. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 33: pour **catalogue des modules vitte**, inspecter l'axe 'garde limite' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la sortie exacte. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 34: pour **catalogue des modules vitte**, inspecter l'axe 'sortie de secours' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compréhension en relecture. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 35: pour **catalogue des modules vitte**, inspecter l'axe 'signature publique' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compatibilité des appels. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 36: pour **catalogue des modules vitte**, inspecter l'axe 'cohérence des types' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la lisibilité du message d'erreur. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 37: pour **catalogue des modules vitte**, inspecter l'axe 'ordre d'exécution' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le scénario de non-régression. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 38: pour **catalogue des modules vitte**, inspecter l'axe 'gestion d'erreur' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le comportement du cas limite. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 39: pour **catalogue des modules vitte**, inspecter l'axe 'lisibilité du flux' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la stabilité du contrat. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 40: pour **catalogue des modules vitte**, inspecter l'axe 'coût de maintenance' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la cohérence avant/après. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 41: pour **catalogue des modules vitte**, inspecter l'axe 'stabilité des appels' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la trace de correction. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 42: pour **catalogue des modules vitte**, inspecter l'axe 'lisibilité du module' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider l'absence d'effet de bord. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 43: pour **catalogue des modules vitte**, inspecter l'axe 'robustesse en refactor' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la sortie exacte. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 44: pour **catalogue des modules vitte**, inspecter l'axe 'stabilité du comportement' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compréhension en relecture. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 45: pour **catalogue des modules vitte**, inspecter l'axe 'qualité du diagnostic' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compatibilité des appels. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 46: pour **catalogue des modules vitte**, inspecter l'axe 'contrat d'entrée' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la lisibilité du message d'erreur. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 47: pour **catalogue des modules vitte**, inspecter l'axe 'branche nominale' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le scénario de non-régression. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 48: pour **catalogue des modules vitte**, inspecter l'axe 'garde limite' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le comportement du cas limite. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 49: pour **catalogue des modules vitte**, inspecter l'axe 'sortie de secours' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la stabilité du contrat. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 50: pour **catalogue des modules vitte**, inspecter l'axe 'signature publique' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la cohérence avant/après. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.

### 7. Checklist finale de compréhension

1. Le contrat d'entrée est explicite.
2. Le cas nominal est testable sans ambiguïté.
3. Le cas limite est traité explicitement.
4. Le diagnostic d'erreur est actionnable.
5. Le corrigé suit une modification locale et vérifiable.

<!-- AUTO_EXPANSION_V1 END -->

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **catalogue des modules vitte**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
entry main at app/demo {
  return 0
}
```

Lecture ligne par ligne:
1. `entry main at app/demo {` -> définit le point d'entrée du scénario.
2. `return 0` -> renvoie la sortie vérifiable.
3. `}` -> participe au déroulé du traitement.

### Exemple B: variante cas limite (même intention, comportement sécurisé)

Objectif: conserver la logique métier tout en ajoutant une garde explicite.

Étapes:
1. Identifier la ligne qui décide la sortie.
2. Ajouter une garde avant cette ligne.
3. Vérifier la nouvelle sortie sur une entrée limite.

### Exemple C: bug reproductible puis correction locale

Procédure:
1. Introduire une incompatibilité de type sur un appel.
2. Compiler et lire le premier diagnostic.
3. Corriger une seule ligne (pas de refactor global).
4. Recompiler et vérifier le retour nominal.

### Résultat attendu

- Le lecteur comprend ce que fait le code sans abstraction inutile.
- Chaque exemple est relié à une action concrète.
- La correction est reproductible et testable.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 END -->
