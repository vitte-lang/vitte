# 45. Performance: allocations et copies

Niveau: Avance

Prérequis: `book/chapters/12-pointeurs.md`, `book/chapters/19-performance.md`.
Voir aussi: à définir.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

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
2. Introduire un cas limite.
3. Vérifier la sortie et documenter l’écart.

## Corrigé minimal

Corrigé: conserver la version la plus simple qui respecte le contrat, puis ajouter un test de non-régression.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **performance: allocations et copies**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc copy_once(x: int) -> int { let y: int = x; give y }
```

Lecture ligne par ligne:
1. `proc copy_once(x: int) -> int { let y: int = x; give y }` -> pose un contrat clair de fonction.

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

## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les gardes sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas limite réel:
- Entree degradee ou incomplete: la garde doit couper le flux tot avec une sortie explicite.

A tester:
- Campagne stable -> sortie 0.
- Paramètre invalide (size=0) -> sortie 41.
- Variance excessive -> sortie 42.


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


## Pourquoi Cette Erreur Arrive En Prod

Cause fréquente: entrée partiellement valide, hypothèse implicite dans une branche, puis projection de sortie trop tardive.
Symptôme: comportement correct en nominal mais instable sous charge ou données incomplètes.
Mesure utile: tracer l'entrée effective, rejouer le cas limite, verrouiller la garde au bon niveau.


## Mini Étude De Cas (Avant / Après)

Avant: logique métier et sortie technique mélangées, diagnostic coûteux.
Après: gardes d'entrée, décision métier, projection finale séparées; comportement plus lisible et testable.
Impact: revue plus rapide, régression plus facile à localiser.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les gardes d'entrée apparaissent avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
