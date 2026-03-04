# 65. Contrats ABI et interop native

Niveau: Avancé

Prérequis: `book/chapters/16-interop.md`, `book/chapters/55-securite-memoire-invariants.md`.

## Objectif

Stabiliser la frontiere Vitte/natif avec des contrats ABI explicites.

## Points critiques

1. Signatures coherentes entre mondes.
2. Tailles/alignements de types verifies.
3. Convention d'appel documentee.
4. Gestion claire de la mémoire partagee.

## Checklist

1. Tests d'interop automatiques.
2. Compatibilité multi-plateforme validee.
3. Strategie de version ABI definie.



## Exemples progressifs (N1 -> N3)

### N1 (base): frontière simple

Snippet Vitte:

```vit
proc host_add(a:int,b:int)->int { give a+b }
```

Commande:

```bash
make grammar-check
```

### N2 (intermédiaire): cohérence de type

Snippet Vitte:

```vit
proc host_add64(a:i64,b:i64)->i64 { give a+b }
```

Commandes:

```bash
make grammar-test
```

### N3 (avancé): compatibilité versionnée + runbook

Snippet Vitte:

```vit
proc host_add_v1(a:int,b:int)->int { give a+b }
proc host_add_v2(a:i64,b:i64)->i64 { give a+b }
```

Runbook:

```bash
# 1. verifier signatures Vitte/natif
# 2. verifier tailles/alignements
# 3. valider CI multi-cible
make grammar-gate
```

### Anti-exemple

```text
Changer ABI natif sans version ni plan de migration.
```

## Validation rapide

1. Aligner signatures.
2. Vérifier layout mémoire.
3. Vérifier tests interop.
