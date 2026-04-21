# 19. Performance

Niveau: Avancé

Prérequis: chapitre précédent `docs/book/chapters/18-tests.md` et `docs/book/glossaire.md`.
Voir aussi: `docs/book/chapters/18-tests.md`, `docs/book/chapters/20-repro.md`, `docs/book/glossaire.md`.

## Problème Concret

Situation réelle: Performance devient clair quand on trace chaque étape du calcul. L'objectif est de relier ligne de code et effet concret sur la sortie.
Question directrice: quelle condition est évaluée en premier, et quelle sortie cette décision impose-t-elle ?

## Fil Rouge (Projet Unique)

Fil conducteur: on conserve un même mini-programme pour comparer les effets d'une modification sans changer tout le contexte.
Objectif pédagogique: passer de la lecture passive à la preuve: même entrée, même branche, même sortie attendue.

## Objectif

Vous devez pouvoir relire un extrait, prédire son résultat, puis vérifier cette prédiction avec une exécution simple.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Performance**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez réellement faire

Vous allez lire les extraits dans l'ordre d'exécution réel, puis valider les sorties attendues sur un cas nominal et un cas d'erreur.

## Exemple minimal

Premier réflexe recommandé: lisez d'abord les entrées et les conditions, ensuite seulement la forme syntaxique.

## Méthode de lecture

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou le test principal.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Performance** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas d'erreur dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Performance**.

## Exercice court

Prenez un exemple du chapitre sur **Performance**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au résultat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Performance**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas d'erreur.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 19.1 Établir une version de référence

```vit
proc sum_loop(n: int) -> int {
  let i: int = 0
  let acc: int = 0

  // Boucle: progression controlee jusqu'a la borne
  loop {
    // Borne d'arret: stoppe la boucle de maniere explicite
    if i >= n { break }
    set acc = acc + i
    set i = i + 1
  }

  give acc
}
```

Lecture algorithmique guidée:
1. Entrée lue: identifiez d'abord les paramètres et leur type, ce sont les données de départ du calcul.
2. Condition évaluée en premier: `i >= n`. Si elle est vraie, le chemin de test est exécuté immédiatement.
3. Boucle: vérifiez la condition d'arrêt et la progression de l'état à chaque itération.
4. Sortie produite: le chemin courant renvoie `acc`.
5. Notion clé: on sépare correction fonctionnelle et coût algorithmique pour optimiser sans casser le comportement.
6. Notion clé: comparer deux variantes exige une même entrée et une sortie strictement équivalente.
Vérification rapide: gardez la même entrée avant/après modification pour prouver que le comportement attendu est conservé.


L'intention de cette étape est directe: poser une baseline lisible qui servira de point de comparaison.

Cette version définit à la fois le résultat attendu et la structure de coût initiale (une boucle, une addition, un incrément à chaque tour).

À l'exécution, `sum_loop(4)` accumule `0+1+2+3` et retourne `6`.

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 19.2 Variante avec branche de filtrage

```vit
proc sum_even(n: int) -> int {
  let i: int = 0
  let acc: int = 0

  // Boucle: progression controlee jusqu'a la borne
  loop {
    // Borne d'arret: stoppe la boucle de maniere explicite
    if i >= n { break }
    if (i % 2) != 0 {
      set i = i + 1
      continue
    }
    set acc = acc + i
    set i = i + 1
  }

  give acc
}
```

Lecture algorithmique guidée:
1. Entrée lue: identifiez d'abord les paramètres et leur type, ce sont les données de départ du calcul.
2. Condition évaluée en premier: `i >= n`. Si elle est vraie, le chemin de test est exécuté immédiatement.
3. Boucle: vérifiez la condition d'arrêt et la progression de l'état à chaque itération.
4. Sortie produite: le chemin courant renvoie `acc`.
5. Notion clé: on sépare correction fonctionnelle et coût algorithmique pour optimiser sans casser le comportement.
6. Notion clé: comparer deux variantes exige une même entrée et une sortie strictement équivalente.
Vérification rapide: gardez la même entrée avant/après modification pour prouver que le comportement attendu est conservé.


L'intention de cette étape est directe: introduire un filtrage conditionnel pour ne sommer que les valeurs paires.

La correction reste intacte, mais le profil de coût change: une branche supplémentaire est évaluée à chaque itération.

Lecture pas à pas pour `sum_even(6)`:
- `i=0` est pair, `acc=0`.
- `i=1` est impair, `continue`.
- `i=2` est pair, `acc=2`.
- `i=3` est impair, `continue`.
- `i=4` est pair, `acc=6`.
- `i=5` est impair, `continue`.
- arrêt sur `i=6`, retour `6`.

Erreurs classiques à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## 19.3 Variante sans branche de parité

```vit
proc sum_even_step(n: int) -> int {
  let i: int = 0
  let acc: int = 0

  // Boucle: progression controlee jusqu'a la borne
  loop {
    // Borne d'arret: stoppe la boucle de maniere explicite
    if i >= n { break }
    set acc = acc + i
    set i = i + 2
  }

  give acc
}
```

Lecture algorithmique guidée:
1. Entrée lue: identifiez d'abord les paramètres et leur type, ce sont les données de départ du calcul.
2. Condition évaluée en premier: `i >= n`. Si elle est vraie, le chemin de test est exécuté immédiatement.
3. Boucle: vérifiez la condition d'arrêt et la progression de l'état à chaque itération.
4. Sortie produite: le chemin courant renvoie `acc`.
5. Notion clé: on sépare correction fonctionnelle et coût algorithmique pour optimiser sans casser le comportement.
6. Notion clé: comparer deux variantes exige une même entrée et une sortie strictement équivalente.
Vérification rapide: testez une entrée nominale puis une entrée limite, et comparez les deux sorties obtenues.


L'intention de cette étape est directe: supprimer la branche de filtrage en avançant directement de deux en deux.

Ici, le flux est plus régulier: chaque tour exécute les mêmes opérations, sans `if` de parité.

À l'exécution, `sum_even_step(6)` traite aussi `0,2,4` et retourne `6`.

Erreurs classiques à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## À retenir

Chaque optimisation préserve le résultat, change une variable de coût à la fois et reste mesurable. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, règle explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez prouver que deux variantes calculent le même résultat.
- vous savez identifier ce qui change dans le coût d'exécution.
- vous savez comparer des versions sans mélanger plusieurs optimisations en même temps.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Repère: un test explicite ou un chemin de secours stable doit s'appliquer.
## À faire

1. Reprenez un exemple du chapitre et modifiez une condition de test pour observer un comportement différent.
2. Écrivez un mini test mental sur une entrée invalide du chapitre, puis prédisez la branche exécutée.

## Corrigé minimal

- identifiez la ligne modifiée et expliquez en une phrase la nouvelle sortie attendue.
- nommez le test ou la branche de secours réellement utilisée.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `docs/book/chapters/keywords/break.md`.
- `docs/book/chapters/keywords/const.md`.
- `docs/book/chapters/keywords/continue.md`.
- `docs/book/chapters/keywords/give.md`.
- `docs/book/chapters/keywords/if.md`.


## Exemple Étendu


```vit
// Scenario performance: execution complete et verifiable
space demo/performance

pick Bench { case Stable(avg: int, p95: int) case Unstable(code: int) }

proc workload(n: int) -> int {
  let i: int = 0
  let acc: int = 0

  // Boucle: progression controlee jusqu'a la borne
  loop {
    // Borne d'arret: stoppe la boucle de maniere explicite
    if i >= n { break }
    set acc = acc + (i * 5)
    set i = i + 1
  }

  give acc
}

proc sample(iter: int, size: int) -> int {
  let base: int = size * 10
  let jitter: int = iter % 9

  give base + jitter
}

// Benchmark: warmup + mesures + décision de stabilité
proc benchmark(size: int) -> Bench {

  if size <= 0 { give Bench.Unstable(41) }
  let w: int = workload(120)
  let _w: int = w
  let s1: int = sample(1, size)
  let s2: int = sample(2, size)
  let s3: int = sample(3, size)
  let s4: int = sample(4, size)
  let s5: int = sample(5, size)
  let avg: int = (s1 + s2 + s3 + s4 + s5) / 5
  let p95: int = s5

  if p95 > (avg * 2) { give Bench.Unstable(42) }

  give Bench.Stable(avg, p95)
}

// Conversion finale vers un code de sortie
proc to_exit(b: Bench) -> int {

  match b {
    case Stable(_, _) { give 0 }
    case Unstable(c) { give c }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let b: Bench = benchmark(600)

  return to_exit(b)
}
```

## Explication détaillée du gros bloc

Ici, l'objectif est de comprendre le chemin réel du programme, ligne par ligne, jusqu'au code de sortie.

### 1. Rôle de chaque partie
- Point de départ: `entry main at core/app`.
- `workload`: lit `n: int` et renvoie `int`.
- `sample`: lit `iter: int, size: int` et renvoie `int`.
- `benchmark`: lit `size: int` et renvoie `Bench`.
- `to_exit`: lit `b: Bench` et renvoie `int`.

### 2. Ordre réel d'exécution
1. Le programme entre dans `main`.
2. `benchmark` est appelé pour traiter l'étape suivante.
3. `to_exit` est appelé pour traiter l'étape suivante.
4. La valeur finale est convertie en sortie process (`return ...`).

### 3. Tests qui changent le chemin
- Test évalué: `i >= n`.
- Test évalué: `size <= 0`.
- Test évalué: `p95 > (avg * 2)`.
- Sélection par `match b`: le chemin dépend de l'état reçu.

### 4. Trace rapide avec valeurs
- Exemple nominal: `entrée valide -> benchmark -> to_exit -> sortie 0`.
- Exemple erreur: `entrée invalide -> benchmark renvoie un code d'erreur -> sortie non nulle`.

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
- Campagne stable -> sortie 0.
- Paramètre invalide (size=0) -> sortie 41.
- Variance excessive -> sortie 42.


### 7. Ligne par ligne (variables + valeurs)

Lecture pratique: suivez les variables dans l'ordre réel d'exécution, puis vérifiez la sortie observée.

- Point d'entrée:
- `entry main at core/app` lance le scénario complet.

- Fonctions du bloc:
- `workload` lit `n: int` puis renvoie `int`.
- `sample` lit `iter: int, size: int` puis renvoie `int`.
- `benchmark` lit `size: int` puis renvoie `Bench`.
- `to_exit` lit `b: Bench` puis renvoie `int`.

- Variables créées (valeur initiale):
- `i: int` démarre avec `0`.
- `acc: int` démarre avec `0`.
- `base: int` démarre avec `size * 10`.
- `jitter: int` démarre avec `iter % 9`.
- `w: int` démarre avec `workload(120)`.
- `_w: int` démarre avec `w`.
- `s1: int` démarre avec `sample(1, size)`.
- `s2: int` démarre avec `sample(2, size)`.
- `s3: int` démarre avec `sample(3, size)`.
- `s4: int` démarre avec `sample(4, size)`.
- `s5: int` démarre avec `sample(5, size)`.
- `avg: int` démarre avec `(s1 + s2 + s3 + s4 + s5) / 5`.

- Variables modifiées pendant le traitement:
- `acc` est mis à jour avec `acc + (i * 5)`.
- `i` est mis à jour avec `i + 1`.

- Conditions qui changent le chemin:
- si `i >= n` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `size <= 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `p95 > (avg * 2)` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.

- Trace nominale (valeurs exemple):
- initialisation: i=0 -> acc=0 -> base=size * 10 -> jitter=iter % 9
- enchaînement: benchmark -> to_exit
- sortie finale sur ce chemin: `to_exit(b)`.

- Trace d'erreur (valeurs exemple):
- si `size <= 0` devient vrai, la fonction renvoie immédiatement `Bench.Unstable(41)`.

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


## Pourquoi Cette Erreur Arrive En Prod

Cause fréquente: entrée partiellement valide, hypothèse implicite dans une branche, puis projection de sortie trop tardive.
Symptôme: comportement correct en nominal mais instable sous charge ou données incomplètes.
Mesure utile: tracer l'entrée effective, rejouer le cas d'erreur, verrouiller le test au bon niveau.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les tests d'entrée sont placés avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
