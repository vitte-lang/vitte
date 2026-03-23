# 38. Refactoring guide par les types

Niveau: Avance

Prérequis: `book/chapters/05-types.md`, `book/chapters/13-generiques.md`.
Voir aussi: `book/chapters/18-tests.md`.

## Objectif

Utiliser le systeme de types comme filet de securite pour modifier du code sans regressions fonctionnelles.

## Methode

1. Ajouter ou clarifier les annotations.
2. Refactoriser une unite a la fois.
3. Compiler apres chaque changement local.
4. Corriger les erreurs de type avant de poursuivre.

## Exemple minimal

```vit
proc parse_count(s: string) -> int {
  # avant: retour implicite ambigu
  give to_int(s)
}
```

Si le contrat change (`int` -> `i64`), laisser le compilateur guider tous les call sites.

## Benefices

- Detection immediate des incoherences.
- Migration plus sure des signatures.
- Reduction des regressions silencieuses.

## Checklist

1. Signature explicite pour chaque `proc` critique.
2. Pas de conversion implicite cachee.
3. Tous les call sites recompilent.


## Exemples progressifs (N1 -> N3)

### N1 (base): type explicite

```vit
proc parse_count(s: string) -> int { give 0 }
```

### N2 (intermediaire): refactor guide par types

```vit
proc parse_count64(s: string) -> i64 { give 0 }
```

### N3 (avance): migration par lots

```vit
proc parse_count(s: string) -> int { give 0 }
proc parse_count64(s: string) -> i64 { give 0 }
```

### Anti-exemple

```vit
proc parse_count(s: string) { give 0 }
```

## Validation rapide

1. Ajouter annotation sur signatures critiques.
2. Corriger call sites via erreurs de type.
3. Garder tests verts entre chaque lot.

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
