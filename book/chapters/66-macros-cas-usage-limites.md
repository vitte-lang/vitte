# 66. Macros: cas d'usage et limites

Niveau: Avancé

Prérequis: `book/chapters/14-macros.md`.

## Objectif

Utiliser les macros quand elles simplifient vraiment, sans detruire lisibilite et debuggabilite.

## Cas d'usage legitimes

1. Eliminer boilerplate repetitif.
2. Encapsuler patron syntaxique stable.

## Limites

1. Expansion opaque difficile a diagnostiquer.
2. Couplage fort a des details de syntaxe.
3. Risque de dette si usage excessif.



## Exemples progressifs (N1 -> N3)

### N1 (base): macro utilitaire locale

Snippet Vitte:

```vit
macro twice(x) { give x + x }
```

Commande:

```bash
make grammar-check
```

### N2 (intermédiaire): usage circonscrit

Snippet Vitte:

```vit
proc f(v:int)->int { give twice(v) }
```

Commande:

```bash
make grammar-test
```

### N3 (avancé): macro gouvernée

Snippet Vitte:

```vit
macro guard_nonneg(x) {
  if x < 0 { give -1 }
  give x
}
```

Commandes:

```bash
make grammar-gate
make book-qa
```

### Anti-exemple

```text
Mettre de la logique métier complexe dans une macro opaque.
```

## Validation rapide

1. Limiter macros au boilerplate.
2. Garder expansion lisible.
3. Tester comportements critiques.
