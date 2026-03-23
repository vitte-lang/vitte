# 37. Conventions de modules a grande echelle

Niveau: Avance

Prérequis: `book/chapters/09-modules.md`.
Voir aussi: `book/chapters/32-catalogue-modules.md`.

## Objectif

Structurer une base de code importante avec `space`, `pull`, `use`, `share` sans couplage excessif.

## Conventions recommandees

1. `space` aligne sur le domaine (ex: `core/auth`, `core/http`).
2. `pull` reserve aux dependances internes claires.
3. `use` pour l'API consommee localement.
4. `share` limite aux symboles stables.

## Exemple minimal

```vit
space core/auth
pull core/crypto as crypto
use core/user.{UserRepo}
share login, logout
```

## Regles d'echelle

- Interdire les dependances circulaires entre espaces.
- Limiter la profondeur de chemin module.
- Stabiliser les points d'entree publics.

## Checklist

1. Le module a-t-il une responsabilite unique?
2. Les imports sont-ils minimaux?
3. Les symboles `share` sont-ils versionnes?

## Exemples progressifs (N1 -> N3)

### N1 (base): module minimal

```vit
space core/auth
proc login() -> int { give 0 }
share login
```

### N2 (intermediaire): imports explicites

```vit
space core/auth
pull core/crypto as crypto
use core/user.{UserRepo}
proc login() -> int { give 0 }
share login
```

### N3 (avance): surface publique stable

```vit
space core/auth
proc login() -> int { give 0 }
proc logout() -> int { give 0 }
share login, logout
```

### Anti-exemple

```vit
space core/auth
proc internal_only() -> int { give 0 }
share all
```

## Validation rapide

1. Verifier frontiere `share`.
2. Eviter cycles modules.
3. Limiter imports au necessaire.

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

Thème: **conventions de modules a grande echelle**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
space core/auth
pull core/crypto as crypto
use core/user.{UserRepo}
share login, logout
```

Lecture ligne par ligne:
1. `space core/auth` -> participe au déroulé du traitement.
2. `pull core/crypto as crypto` -> participe au déroulé du traitement.
3. `use core/user.{UserRepo}` -> participe au déroulé du traitement.
4. `share login, logout` -> participe au déroulé du traitement.

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
