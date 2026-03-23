# 44. Performance: mesurer avant d'optimiser

Niveau: Intermediaire

Prérequis: `book/chapters/19-performance.md`.
Voir aussi: à définir.

## Objectif

Eviter les optimisations speculatives en partant de mesures reproductibles.

## Demarche

1. Definir une metrique cible (latence, debit, memoire).
2. Capturer une baseline.
3. Modifier une variable a la fois.
4. Comparer avec le meme protocole.

## Checklist

1. Cas nominal et cas limite mesures.
2. Environnement de benchmark stable.
3. Resultats traces et historises.

## Exemples progressifs (N1 -> N3)

### N1 (base): baseline fonctionnelle

```vit
proc compute(x: int) -> int { give x + 1 }
```

### N2 (intermediaire): variation unique

```vit
proc compute_fast(x: int) -> int { give x + 1 }
```

### N3 (avance): protocole stable

```vit
proc benchmark_case(x: int) -> int { give compute_fast(x) }
```

### Anti-exemple

```vit
proc tweak1(x:int)->int{give x}
proc tweak2(x:int)->int{give x}
# aucune mesure comparative
```

## Validation rapide

1. Capturer baseline.
2. Changer une variable.
3. Comparer sur protocole fixe.

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

Thème: **performance: mesurer avant d'optimiser**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc compute(x: int) -> int { give x + 1 }
```

Lecture ligne par ligne:
1. `proc compute(x: int) -> int { give x + 1 }` -> pose un contrat clair de fonction.

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

Exemple approfondi pour **performance mesurer avant optimiser**: mesure reproductible (warmup, séries, garde anti-bruit, décision de stabilité).

```vit
// Exemple long: flux complet et vérifiable
space demo/performance-mesurer-avant-optimiser

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
  // Sortie locale: valeur retournee par la procedure
  give acc
}

proc sample(iter: int, size: int) -> int {
  let base: int = size * 10
  let jitter: int = iter % 9
  // Sortie locale: valeur retournee par la procedure
  give base + jitter
}

// Benchmark: warmup + mesures + décision de stabilité
proc benchmark(size: int) -> Bench {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
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
  // Garde: bloque un cas invalide avant de continuer
  if p95 > (avg * 2) { give Bench.Unstable(42) }
  // Sortie locale: valeur retournee par la procedure
  give Bench.Stable(avg, p95)
}

// Projection finale: convertit l'état métier en code de sortie
proc to_exit(b: Bench) -> int {
  // Bloc logique: decision par branches explicites
  // Match: decision explicite selon l'etat
  match b {
    case Stable(_, _) { give 0 }
    case Unstable(c) { give c }
    otherwise { give 70 }
  }
}

// Orchestration: enchaîne les étapes sans logique cachée
entry main at core/app {
  let b: Bench = benchmark(600)
  // Sortie programme: code de retour observable
  return to_exit(b)
}
```

Scénarios recommandés (performance mesurer avant optimiser):
- Campagne stable -> sortie 0.
- Paramètre invalide (size=0) -> sortie 41.
- Variance excessive -> sortie 42.
