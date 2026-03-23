# 35. Anatomie d'un message d'erreur

Niveau: Intermediaire

Prérequis: `book/chapters/10-diagnostics.md`, `book/chapters/31-erreurs-build.md`.
Voir aussi: `book/grammar/diagnostics/expected`.

## Objectif

Lire un diagnostic en moins de 30 secondes: categorie, position, cause probable, correction.

## Anatomie standard

1. Code erreur (`E....`).
2. Position (fichier, ligne, colonne).
3. Message principal (ce qui est attendu vs trouve).
4. Contexte (ligne source, zone marquee).
5. Suggestion (action de correction).

## Exemple de lecture

Erreur: `expected top-level declaration` sur `emit 1` en ligne 1.

Interpretation:
- Couche: parse.
- Cause probable: instruction au top-level.
- Fix minimal: encapsuler dans `entry` ou `proc`.

## Strategie de correction

1. Ne corriger que la premiere erreur.
2. Relancer le compilateur.
3. Re-evaluer les erreurs restantes.
4. Eviter les modifications en cascade sans preuve.

## Checklist

1. Copier le message exact.
2. Identifier la couche (`parse`, `type`, `link`).
3. Valider la correction sur un cas minimal.
4. Ajouter un test de non-regression.


## Exemples progressifs (N1 -> N3)

### N1 (base): diagnostic syntaxique lisible

```vit
entry main at app/demo {
  return 0
}
```

### N2 (intermediaire): diagnostic token inattendu

```vit
entry main at app/demo {
  return
}
```

### N3 (avance): diagnostic semantique

```vit
entry main at app/demo {
  return unknown_value
}
```

### Anti-exemple

```vit
entry main at app/demo {
  return 0
  return 1
}
```

## Validation rapide

1. Identifier code + position du premier diagnostic.
2. Corriger une erreur a la fois.
3. Relancer jusqu'a zero erreur.

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
