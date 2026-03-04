# 59. Revue de code orientee risques

Niveau: Avancé

Prérequis: `book/chapters/18-tests.md`, `book/chapters/31-erreurs-build.md`.

## Objectif

Concentrer la revue sur bugs, régressions comportementales et dette technique critique.

## Axes de revue

1. Correctness et invariants.
2. Compatibilité API et migrations.
3. Performance sur chemins critiques.
4. Qualité des tests ajoutes.

## Checklist

1. Risques severes identifies en premier.
2. Preuves de validation presentes.
3. Plan de suivi pour risques restants.



## Exemples progressifs (N1 -> N3)

### N1 (base): finding correctness

Snippet Vitte:

```vit
proc parse_port(x: int) -> int {
  if x < 0 { give -1 }
  give x
}
```

Commande revue:

```bash
rg -n "parse_port|give" book/chapters src tests
```

### N2 (intermediaire): finding regression

Snippet Vitte:

```vit
proc parse_port(x: int) -> int {
  if x < 0 { give -1 }
}
```

Commande vérification:

```bash
make grammar-test
```

### N3 (avance): finding + preuve

Snippet Vitte (correctif):

```vit
proc parse_port(x: int) -> int {
  if x < 0 { give -1 }
  if x > 65535 { give -1 }
  give x
}
```

Commande preuve:

```bash
make grammar-gate
```

### Anti-exemple

```text
Commentaire de revue sur style uniquement sans risque comportemental.
```

## Validation rapide

1. Classer finding (critique/majeur/mineur).
2. Joindre preuve (test/commande).
3. Verifier correction sans regression.

