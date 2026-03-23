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
