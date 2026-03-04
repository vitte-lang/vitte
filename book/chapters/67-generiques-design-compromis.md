# 67. Generiques: design et compromis

Niveau: Avancé

Prérequis: `book/chapters/13-generiques.md`.

## Objectif

Concevoir des API generiques utiles sans complexite superflue.

## Compromis

1. Flexibilite vs lisibilite.
2. Reutilisation vs erreurs de type plus complexes.
3. Abstraction vs performance.

## Checklist

1. Besoin reel de generique justifie.
2. Contraintes de type explicites.
3. Exemples d'usage clairs pour consommateurs API.



## Exemples progressifs (N1 -> N3)

### N1 (base): générique simple

Snippet Vitte:

```vit
type Box[T] = T
```

Commande:

```bash
make grammar-check
```

### N2 (intermédiaire): API générique

Snippet Vitte:

```vit
proc id[T](x: T) -> T { give x }
```

Commande:

```bash
make grammar-test
```

### N3 (avancé): compromis générique/spécialisé

Snippet Vitte:

```vit
proc sum_i32(a:i32,b:i32)->i32 { give a+b }
proc sum_i64(a:i64,b:i64)->i64 { give a+b }
```

Commandes:

```bash
make grammar-gate
make book-qa
```

### Anti-exemple

```text
Généraliser trop tôt sans besoin ni mesure.
```

## Validation rapide

1. Justifier l'abstraction.
2. Définir les contraintes minimales.
3. Mesurer lisibilité/performance.
