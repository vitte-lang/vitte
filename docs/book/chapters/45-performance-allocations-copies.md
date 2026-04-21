# 45. Performance: allocations et copies

Niveau: Avance

Prérequis: `docs/book/chapters/12-pointeurs.md`, `docs/book/chapters/19-performance.md`.
Voir aussi: à définir.

## Problème Concret

Situation réelle: dans ce chapitre sur Performance: allocations et copies, l'échec vient souvent d'une décision mal ordonnée plutôt que d'une faute de syntaxe. On suit donc le flux exact: entrée, test, branche, sortie.
Question directrice: quelle condition est évaluée en premier, et quelle sortie cette décision impose-t-elle ?

## Fil Rouge (Projet Unique)

Fil conducteur: on conserve un même mini-programme pour comparer les effets d'une modification sans changer tout le contexte.
Objectif pédagogique: relire un bloc, prédire la sortie, puis confirmer la prédiction avec une exécution simple et reproductible.

## Objectif

Reduire allocations et copies inutiles dans les chemins chauds.

## Points de vigilance

1. Eviter copies de structures volumineuses en boucle.
2. Favoriser reutilisation de buffers.
3. Limiter conversions intermediaires couteuses.
4. Profiler avant/apres chaque changement.

## Verification

1. Compter allocations sur scenario fixe.
2. Mesurer impact latence/p95.
3. Valider absence de regression fonctionnelle.

## Exemples progressifs (N1 -> N3)

### N1 (base): copie simple

```vit
proc copy_once(x: int) -> int { let y: int = x; give y }
```

### N2 (intermediaire): boucle critique

```vit
proc sum3(a:int,b:int,c:int) -> int { give a+b+c }
```

### N3 (avance): chemin chaud stable

```vit
proc hot_path(v: int) -> int {
  let acc: int = v

  give acc
}
```

### Anti-exemple

```vit
proc noisy(v:int)->int{
  let a:int=v
  let b:int=a
  let c:int=b

  give c
}
```

## Validation rapide

1. Supprimer copies inutiles.
2. Mesurer impact avant/apres.
3. Verifier non-regression fonctionnelle.

## Pourquoi

Cette section explicite la valeur pratique: réduire les erreurs, accélérer le diagnostic et stabiliser les évolutions.

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

Thème: **performance: allocations et copies**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc copy_once(x: int) -> int { let y: int = x; give y }
```

Lecture ligne par ligne:
1. `proc copy_once(x: int) -> int { let y: int = x; give y }` -> pose une règle clair de fonction.

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
// Scenario performance allocations copies: execution complete et verifiable
space demo/performance-allocations-copies

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

Ce gros bloc montre un programme entier, pas un extrait isolé: on suit le flux du début à la fin.

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


## Mini Étude De Cas (Avant / Après)

Avant: logique métier et sortie technique mélangées, diagnostic coûteux.
Après: tests d'entrée, décision métier, projection finale séparées; comportement plus lisible et testable.
Impact: revue plus rapide, régression plus facile à localiser.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les tests d'entrée sont placés avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
