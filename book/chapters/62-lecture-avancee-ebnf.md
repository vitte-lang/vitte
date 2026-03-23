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

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **lecture avancee de l'ebnf du langage**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
let_stmt ::= "let" WS1 ident WS? "=" WS? expr ;
```

Lecture ligne par ligne:
1. `let_stmt ::= "let" WS1 ident WS? "=" WS? expr ;` -> participe au déroulé du traitement.

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
