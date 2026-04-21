# 32. Catalogue de la bibliothèque stdlib Vitte

Prérequis: à définir.
Voir aussi: à définir.
Ce document fournit une vue d'ensemble de la bibliothèque stdlib dans l'arborescence source dédiée.

## Problème Concret

Situation réelle: pour la bibliothèque stdlib Vitte, la question n'est pas 'quella règle écrire' mais 'quel chemin le code prend vraiment'. Cette lecture par exécution évite les interprétations vagues.
Question directrice: quelle condition est évaluée en premier, et quelle sortie cette décision impose-t-elle ?

## Fil Rouge (Projet Unique)

Fil conducteur: on conserve un même mini-programme pour comparer les effets d'une modification sans changer tout le contexte.
Objectif pédagogique: comprendre pourquoi une ligne existe et ce qu'elle change dans la trajectoire du programme.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **la bibliothèque stdlib Vitte**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez lire les extraits dans l'ordre d'exécution réel, puis valider les sorties attendues sur un cas nominal et un cas d'erreur.

## Exemple minimal

Premier réflexe recommandé: lisez d'abord les entrées et les conditions, ensuite seulement la forme syntaxique.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou le test principal.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **la bibliothèque stdlib Vitte** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas d'erreur dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **la bibliothèque stdlib Vitte**.

## Exercice court

Prenez un exemple du chapitre sur **la bibliothèque stdlib Vitte**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au résultat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **la bibliothèque stdlib Vitte**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas d'erreur.
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
- `vitte/std`: façade de la bibliothèque stdlib.
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
2. Introduire un cas d'erreur.
3. Vérifier la sortie et documenter l’écart.

## Corrigé minimal

Corrigé: conserver la version la plus simple qui respecte la règle, puis ajouter un test de non-régression.

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

### Exemple B: variante cas d'erreur (même intention, comportement sécurisé)

Objectif: conserver la logique métier tout en ajoutant un test explicite.

Étapes:
1. Identifier la ligne qui décide la sortie.
2. Ajouter un test avant cette ligne.
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

## Explication détaillée du gros bloc

Ce gros bloc montre un programme entier, pas un extrait isolé: on suit le flux du début à la fin.

### 1. Rôle de chaque partie
- Point de départ: `entry main at core/app`.
- `normalize_len`: lit `n: int` et renvoie `int`.
- `resolve`: lit `m: ModuleSpec` et renvoie `Resolve`.
- `to_exit`: lit `r: Resolve` et renvoie `int`.

### 2. Ordre réel d'exécution
1. Le programme entre dans `main`.
2. `resolve` est appelé pour traiter l'étape suivante.
3. `to_exit` est appelé pour traiter l'étape suivante.
4. La valeur finale est convertie en sortie process (`return ...`).

### 3. Tests qui changent le chemin
- Test évalué: `n <= 0`.
- Test évalué: `n == 0`.
- Test évalué: `m.major <= 0`.
- Test évalué: `m.minor < 0`.
- Sélection par `match r`: le chemin dépend de l'état reçu.

### 4. Trace rapide avec valeurs
- Exemple nominal: `entrée valide -> resolve -> to_exit -> sortie 0`.
- Exemple erreur: `entrée invalide -> resolve renvoie un code d'erreur -> sortie non nulle`.

### 5. Pourquoi ce découpage est utile
- Vous testez chaque fonction seule, puis le flux complet.
- Vous savez où modifier une règle sans casser tout le programme.
- Vous pouvez expliquer la sortie en suivant simplement les appels.

### 6. Vérification rapide
1. Relancer avec une entrée normale et noter la sortie.
2. Relancer avec une entrée invalide et vérifier le code d'erreur.
3. Confirmer que la même entrée donne toujours la même sortie.


## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les tests sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas d'erreur réel:
- Entree degradee ou incomplete: le test doit couper le flux tot avec une sortie explicite.

A tester:
- Spécification valide -> sortie 0.
- Nom invalide -> sortie 71.
- Version invalide -> sortie 72 ou 73.


### 7. Ligne par ligne (variables + valeurs)

Lecture pratique: suivez les variables dans l'ordre réel d'exécution, puis vérifiez la sortie observée.

- Point d'entrée:
- `entry main at core/app` lance le scénario complet.

- Fonctions du bloc:
- `normalize_len` lit `n: int` puis renvoie `int`.
- `resolve` lit `m: ModuleSpec` puis renvoie `Resolve`.
- `to_exit` lit `r: Resolve` puis renvoie `int`.

- Variables créées (valeur initiale):
- `n: int` démarre avec `normalize_len(m.name_len)`.
- `code: int` démarre avec `100 + (m.major * 10) + m.minor`.
- `m: ModuleSpec` démarre avec `ModuleSpec(8, 1, 2)`.
- `r: Resolve` démarre avec `resolve(m)`.

- Conditions qui changent le chemin:
- si `n <= 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `n == 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `m.major <= 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `m.minor < 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.

- Trace nominale (valeurs exemple):
- initialisation: n=normalize_len(m.name_len) -> code=100 + (m.major * 10) + m.minor -> m=ModuleSpec(8, 1, 2) -> r=resolve(m)
- enchaînement: resolve -> to_exit
- sortie finale sur ce chemin: `to_exit(r)`.

- Trace d'erreur (valeurs exemple):
- si `n <= 0` devient vrai, la fonction renvoie immédiatement `0`.

- Vérification rapide:
- relancer avec une entrée normale et noter la sortie,
- relancer avec une entrée invalide et noter le code d'erreur,
- confirmer qu'une même entrée produit toujours la même sortie.

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
| Sortie inattendue | Test absente ou mal ordonnée | Rejouer avec cas d'erreur | Remonter le test avant la zone sensible |
| Branche non prise | Condition trop large/trop stricte | Tracer l'entrée effective | Rendre la condition explicite et testée |
| Régression silencieuse | Règle implicite | Comparer nominal vs limite | Formaliser la règle dans le code |


## Checkpoint

À ce stade, vous devez savoir:
- expliquer le flux entrée -> décision -> sortie sans ambiguïté,
- isoler un cas d'erreur réel et prévoir sa sortie,
- identifier où ajouter un test sans casser le nominal.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les tests d'entrée sont placés avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
