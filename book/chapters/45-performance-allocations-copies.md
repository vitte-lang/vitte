# 45. Performance: allocations et copies

Niveau: Avance

Prérequis: `book/chapters/12-pointeurs.md`, `book/chapters/19-performance.md`.
Voir aussi: à définir.

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
  // Sortie locale: valeur retournee par la procedure
  give acc
}
```

### Anti-exemple

```vit
proc noisy(v:int)->int{
  let a:int=v
  let b:int=a
  let c:int=b
  // Sortie locale: valeur retournee par la procedure
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

Exemple approfondi pour **performance allocations copies**: mesure reproductible (warmup, séries, garde anti-bruit, décision de stabilité).

```vit
// Exemple long: flux complet et vérifiable
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

Scénarios recommandés (performance allocations copies):
- Campagne stable -> sortie 0.
- Paramètre invalide (size=0) -> sortie 41.
- Variance excessive -> sortie 42.
