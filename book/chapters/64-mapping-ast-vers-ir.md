# 64. Mapping AST -> IR (vue pratique)

Niveau: Avancé

Prérequis: `book/chapters/15-pipeline.md`, `book/chapters/27-grammaire.md`.
Voir aussi: à définir.

## Objectif

Comprendre comment les noeuds AST sont transformes en IR exploitable par le backend.

## Etapes

1. Parser produit AST structure.
2. Resolve annote symboles et types.
3. Lowering AST -> IR normalise.
4. Backend consomme IR.

## Checklist

1. Invariants AST explicites.
2. Invariants IR verifiables.
3. Tests de non-regression sur transformations.



## Exemples progressifs (N1 -> N3)

### N1 (base): AST expression

Snippet Vitte:

```vit
entry main at app/demo { return 1 + 2 }
```

Commande:

```bash
make grammar-check
```

### N2 (intermédiaire): AST conditionnel

Snippet Vitte:

```vit
entry main at app/demo {
  if true { return 1 }
  return 0
}
```

Commande:

```bash
make grammar-test
```

### N3 (avancé): mapping stable AST -> IR

Snippet Vitte:

```vit
proc add(a:int,b:int)->int { give a+b }
entry main at app/demo { return add(1,2) }
```

Commandes:

```bash
make grammar-gate
make book-qa
```

### Anti-exemple

```text
Transformer AST->IR sans invariants documentés.
```

## Validation rapide

1. Identifier AST attendu.
2. Vérifier invariants IR.
3. Tester non-régression de transformation.

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
