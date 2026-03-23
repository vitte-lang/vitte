# 62. Lecture avancee de l'EBNF du langage

Niveau: Avancé

Prérequis: `book/chapters/27-grammaire.md`, `book/grammar/precedence.md`.
Voir aussi: à définir.

## Objectif

Lire rapidement une regle EBNF et deduire contraintes de parse et implications AST.

## Methode

1. Partir des non-terminaux racine (`program`, `toplevel`, `stmt`, `expr`).
2. Identifier options et repetitions.
3. Repeter avec exemples valid/invalid.
4. Relier regle -> diagnostic utilisateur.

## Checklist

1. Ambiguites potentielles explicitees.
2. Priorites operatoires verifiees.
3. Exemples minimaux associes a chaque regle critique.



## Exemples progressifs (N1 -> N3)

### N1 (base): lire une règle simple

Snippet EBNF:

```vit
let_stmt ::= "let" WS1 ident WS? "=" WS? expr ;
```

Commande:

```bash
make grammar-check
```

### N2 (intermédiaire): alternatives

Snippet EBNF:

```vit
stmt ::= let_stmt | if_stmt | return_stmt ;
```

Commandes:

```bash
make grammar-test
```

### N3 (avancé): priorité opératoire

Snippet EBNF:

```vit
add_expr ::= mul_expr { ("+"|"-") mul_expr } ;
mul_expr ::= unary_expr { ("*"|"/") unary_expr } ;
```

Commandes:

```bash
make grammar-gate
```

### Anti-exemple

```text
Interpréter une règle sans test valid/invalid associé.
```

## Validation rapide

1. Relier la règle à un cas valid.
2. Relier la règle à un cas invalid.
3. Vérifier le diagnostic attendu.

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
