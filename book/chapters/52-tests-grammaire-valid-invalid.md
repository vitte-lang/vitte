# 52. Tests de grammaire (valid/invalid corpus)

Niveau: Avance

Prérequis: `book/chapters/27-grammaire.md`, `book/chapters/31-erreurs-build.md`.
Voir aussi: `book/grammar/diagnostics/expected`.

## Objectif

Verifier que la grammaire accepte les formes valides et rejette proprement les invalides.

## Corpus

1. `valid`: exemples representatifs de syntaxe autorisee.
2. `invalid`: erreurs ciblees avec diagnostics attendus.

## Checklist

1. Un test par regle critique.
2. Message d'erreur attendu versionne.
3. Couverture des ambiguities connues.


## Exemples progressifs (N1 -> N3)

### N1 (base): valid

```vit
entry main at app/demo {
  return 0
}
```

### N2 (intermediaire): invalid

```vit
entry main at app/demo {
  return
}
```

### N3 (avance): regle ciblee

```vit
match 1 {
  case 1 { give 1 }
  otherwise { give 0 }
}
```

### Anti-exemple

```vit
# corpus contient seulement des cas valides
```

## Validation rapide

1. 1 valid + 1 invalid par regle critique.
2. Diagnostics attendus versionnes.
3. Couverture ambiguities mise a jour.

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
