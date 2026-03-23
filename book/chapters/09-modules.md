# 9. Modules et organisation

Niveau: Intermédiaire

Prérequis: chapitre précédent `book/chapters/08-structures.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/08-structures.md`, `book/chapters/10-diagnostics.md`, `book/glossaire.md`.

## Problème Concret

Situation réelle: pour Modules et organisation, la question n'est pas 'quella règle écrire' mais 'quel chemin le code prend vraiment'. Cette lecture par exécution évite les interprétations vagues.
Question directrice: quelle condition est évaluée en premier, et quelle sortie cette décision impose-t-elle ?

## Fil Rouge (Projet Unique)

Fil conducteur: vous retrouvez le même pipeline pour observer ce qui change réellement quand on modifie une branche.
Objectif pédagogique: comprendre pourquoi une ligne existe et ce qu'elle change dans la trajectoire du programme.

## Objectif

Vous devez pouvoir relire un extrait, prédire son résultat, puis vérifier cette prédiction avec une exécution simple.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Modules et organisation**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez réellement faire

Vous allez lire les extraits dans l'ordre d'exécution réel, puis valider les sorties attendues sur un cas nominal et un cas d'erreur.

## Exemple minimal

Premier réflexe recommandé: lisez d'abord les entrées et les conditions, ensuite seulement la forme syntaxique.

## Méthode de lecture

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou le test principal.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Modules et organisation** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas d'erreur dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Modules et organisation**.

## Exercice court

Prenez un exemple du chapitre sur **Modules et organisation**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au résultat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Modules et organisation**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas d'erreur.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 9.1 Module source

```vit
space app/core
proc add(a: int, b: int) -> int {
  give a + b
}
```

Lecture algorithmique guidée:
1. Entrée lue: identifiez d'abord les paramètres et leur type, ce sont les données de départ du calcul.
2. Pas de branchement critique ici: le flux est séquentiel, ligne après ligne.
3. Traitement: appliquez les opérations dans l'ordre écrit, sans sauter d'étape implicite.
4. Sortie produite: le chemin courant renvoie `a + b`.
5. Notion intermédiaire: une fonction est une transformation `Entrée -> Sortie`; sa règle sert à limiter les ambiguïtés.
6. Notion intermédiaire: un invariant est une propriété qui reste vraie pendant la boucle ou pendant un pipeline d'appels.
Vérification rapide: gardez la même entrée avant/après modification pour prouver que le comportement attendu est conservé.


L'intention de cette étape est directe: poser un module source minimal, centré sur une responsabilité unique.

Ce noyau est simple à tester et à réutiliser, car il ne dépend pas d'une couche externe.

À l'exécution:
- `add(10,32)` retourne `42`.
- la fonction reste locale au module tant qu'elle n'est pas exportée.

Erreurs classiques à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## 9.2 Module consommateur avec alias

```vit
space app/math
pull app/core as core
share add_pair
proc add_pair(x: int, y: int) -> int {
  give core.add(x, y)
}
```

Lecture algorithmique guidée:
1. Entrée lue: identifiez d'abord les paramètres et leur type, ce sont les données de départ du calcul.
2. Pas de branchement critique ici: le flux est séquentiel, ligne après ligne.
3. Traitement: appliquez les opérations dans l'ordre écrit, sans sauter d'étape implicite.
4. Sortie produite: le chemin courant renvoie `core.add(x, y)`.
5. Notion intermédiaire: une fonction est une transformation `Entrée -> Sortie`; sa règle sert à limiter les ambiguïtés.
6. Notion intermédiaire: un invariant est une propriété qui reste vraie pendant la boucle ou pendant un pipeline d'appels.
Vérification rapide: tracez une exécution avec des valeurs concrètes (`x=...`, `i=...`) pour confirmer la branche réellement prise.


L'intention de cette étape est directe: consommer un module externe de manière explicite et exposer une surface publique contrôlée.

Le `pull.. as core` rend la dépendance visible, et `share add_pair` limite précisément ce que le module expose.

À l'exécution, `add_pair(1,2)` délègue à `core.add(1,2)` puis retourne `3`.

Erreurs classiques à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## 9.3 Module domaine pur

```vit
space app/domain
form Ticket { id: int, priority: int }
proc is_critical(t: Ticket) -> bool { give t.priority >= 9 }
```

Lecture algorithmique guidée:
1. Entrée lue: identifiez d'abord les paramètres et leur type, ce sont les données de départ du calcul.
2. Pas de branchement critique ici: le flux est séquentiel, ligne après ligne.
3. Traitement: appliquez les opérations dans l'ordre écrit, sans sauter d'étape implicite.
4. Sortie produite: le chemin courant renvoie `t.priority >= 9`.
5. Notion intermédiaire: une fonction est une transformation `Entrée -> Sortie`; sa règle sert à limiter les ambiguïtés.
6. Notion intermédiaire: un invariant est une propriété qui reste vraie pendant la boucle ou pendant un pipeline d'appels.
Vérification rapide: gardez la même entrée avant/après modification pour prouver que le comportement attendu est conservé.


L'intention de cette étape est directe: garder le domaine indépendant des couches d'orchestration et d'infrastructure.

Cette séparation rend les règles métier stables: elles ne changent pas quand l'entrypoint, l'IO ou le transport évoluent.

À l'exécution:
- `Ticket(priority=9)` retourne `true`.
- `Ticket(priority=8)` retourne `false`.

Erreurs classiques à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## À retenir

Dépendances orientées, exports limités, domaine pur. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, règle explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez dire qui dépend de qui en lisant les `pull`.
- vous savez limiter ce qui est public avec `share`.
- vous pouvez faire évoluer un module sans propager des changements partout.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Repère: un test explicite ou un chemin de secours stable doit s'appliquer.
## À faire

1. Reprenez un exemple du chapitre et modifiez une condition de test pour observer un comportement différent.
2. Écrivez un mini test mental sur une entrée invalide du chapitre, puis prédisez la branche exécutée.

## Corrigé minimal

- identifiez la ligne modifiée et expliquez en une phrase la nouvelle sortie attendue.
- nommez le test ou la branche de secours réellement utilisée.

## Mini défi transverse

Défi: combinez au moins deux notions des trois derniers chapitres dans une fonction courte (test + transformation + sortie).
Vérification minimale: montrez un cas nominal et un cas invalide, puis expliquez quelle branche est prise.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `book/keywords/as.md`.
- `book/keywords/bool.md`.
- `book/keywords/false.md`.
- `book/keywords/field.md`.
- `book/keywords/form.md`.

## Checkpoint synthèse

Mini quiz:
1. Quelle est l'invariant central de ce chapitre ?
2. Quelle test évite l'état invalide le plus fréquent ?
3. Quel test simple prouve le comportement nominal ?


## Exemple Étendu


```vit
// Scenario modules: execution complete et verifiable
space demo/modules

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

Vous lisez ce gros bloc comme un scénario complet: préparation des données, traitement, puis sortie finale.

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


## Mini Étude De Cas (Avant / Après)

Avant: logique métier et sortie technique mélangées, diagnostic coûteux.
Après: tests d'entrée, décision métier, projection finale séparées; comportement plus lisible et testable.
Impact: revue plus rapide, régression plus facile à localiser.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les tests d'entrée sont placés avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
