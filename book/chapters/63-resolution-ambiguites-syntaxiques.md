# 63. Resolution d'ambiguites syntaxiques

Niveau: Avancé

Prérequis: `book/chapters/62-lecture-avancee-ebnf.md`.
Voir aussi: à définir.

## Objectif

Traiter les ambiguites de grammaire sans casser la surface utilisateur.

## Cas classiques

1. Association `else`.
2. Conflits de precedence operatoire.
3. Formes proches entre statement et expression.

## Strategie

1. Ajouter tests valid/invalid cibles.
2. Modifier regle la plus locale possible.
3. Verifier diagnostics et AST.



## Exemples progressifs (N1 -> N3)

### N1 (base): ambiguïté `else`

Snippet:

```vit
if a {
  if b { give 1 }
  else { give 2 }
}
```

Commande:

```bash
make grammar-test
```

### N2 (intermédiaire): désambiguïsation explicite

Snippet:

```vit
if a {
  if b { give 1 }
} else {
  give 2
}
```

Commandes:

```bash
make grammar-test
make grammar-docs
```

### N3 (avancé): correction + runbook de validation

Snippet règle:

```vit
if_stmt ::= "if" WS1 expr WS? block [ WS? ("else"|"otherwise") WS? (block|if_stmt) ] ;
```

Runbook:

```bash
# 1. reproduire le conflit
# 2. corriger localement la règle
# 3. relancer grammar-gate
make grammar-gate
```

### Anti-exemple

```text
Corriger l'ambiguïté sans mettre à jour le corpus.
```

## Validation rapide

1. Reproduire avant correction.
2. Vérifier après correction.
3. Confirmer absence de régression globale.

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
