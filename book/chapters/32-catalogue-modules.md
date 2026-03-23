# 32. Catalogue Des Modules Vitte

Prérequis: à définir.
Voir aussi: à définir.
Ce document fournit une vue d'ensemble des modules de `src/vitte/packages`.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

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



## Exemple Étendu


```vit
// Scenario catalogue modules: execution complete et verifiable
space demo/catalogue-modules

form ModuleSpec { name_len: int major: int minor: int }
pick Resolve { case Loaded(path_code: int) case Missing(code: int) }

proc normalize_len(n: int) -> int {

  if n <= 0 { give 0 }

  give n
}

// Résolution: valide la référence et produit une décision de chargement
proc resolve(m: ModuleSpec) -> Resolve {
  let n: int = normalize_len(m.name_len)

  if n == 0 { give Resolve.Missing(71) }

  if m.major <= 0 { give Resolve.Missing(72) }

  if m.minor < 0 { give Resolve.Missing(73) }
  let code: int = 100 + (m.major * 10) + m.minor

  give Resolve.Loaded(code)
}

// Conversion finale vers un code de sortie
proc to_exit(r: Resolve) -> int {

  match r {
    case Loaded(_) { give 0 }
    case Missing(c) { give c }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let m: ModuleSpec = ModuleSpec(8, 1, 2)
  let r: Resolve = resolve(m)

  return to_exit(r)
}
```

## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les gardes sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas limite réel:
- Entree degradee ou incomplete: la garde doit couper le flux tot avec une sortie explicite.

A tester:
- Spécification valide -> sortie 0.
- Nom invalide -> sortie 71.
- Version invalide -> sortie 72 ou 73.


## Trade-offs

| Contrainte | Option A | Option B | Décision recommandée |
| --- | --- | --- | --- |
| Lisibilité prioritaire | Branches explicites | Code compact | A si l'équipe maintient le code longtemps |
| Perf critique | Spécialisation ciblée | Généralisation | A si profiling confirme le gain |
| Évolution rapide | Contrats stricts | Conventions implicites | A pour réduire les régressions |


## Décision Selon Contrainte

- Si la contrainte dominante est la sûreté: valider tôt, échouer explicitement.
- Si la contrainte dominante est la latence: mesurer d'abord, optimiser ensuite.
- Si la contrainte dominante est l'évolutivité: isoler orchestration, décisions et conversion de sortie.


## Diagnostic Rapide

| Symptôme | Cause probable | Vérification | Correction |
| --- | --- | --- | --- |
| Sortie inattendue | Garde absente ou mal ordonnée | Rejouer avec cas limite | Remonter la garde avant la zone sensible |
| Branche non prise | Condition trop large/trop stricte | Tracer l'entrée effective | Rendre la condition explicite et testée |
| Régression silencieuse | Contrat implicite | Comparer nominal vs limite | Formaliser le contrat dans le code |


## Checkpoint

À ce stade, vous devez savoir:
- expliquer le flux entrée -> décision -> sortie sans ambiguïté,
- isoler un cas limite réel et prévoir sa sortie,
- identifier où ajouter une garde sans casser le nominal.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les gardes d'entrée apparaissent avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
