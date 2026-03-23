# 43. Anti-patterns frequents en code Vitte

Niveau: Intermediaire

Prérequis: `book/chapters/28-conventions.md`, `book/chapters/42-patterns-procedures-robustes.md`.
Voir aussi: à définir.

## Objectif

Identifier rapidement les formes de code qui augmentent dette technique, bugs et cout de maintenance.

## Anti-patterns frequents

1. Procedures trop longues avec responsabilites mixtes.
2. Validation absente des cas limites.
3. Types implicites la ou une signature explicite est necessaire.
4. Imports excessifs au lieu d'interfaces stables.
5. Messages d'erreur non actionnables.

## Correctif minimal

1. Extraire une responsabilite par procedure.
2. Ajouter gardes en tete.
3. Rendre les signatures explicites.
4. Reduire la surface importee.

## Exemples progressifs (N1 -> N3)

### N1 (base): anti-pattern detecte

```vit
proc process(x: int, y: int, z: int) -> int {
  if x < 0 { give -1 }
  if y < 0 { give -1 }
  if z < 0 { give -1 }
  give x + y + z
}
```

### N2 (intermediaire): extraction

```vit
proc is_valid(x: int) -> bool { give x >= 0 }
proc process3(x: int, y: int, z: int) -> int {
  if not is_valid(x) { give -1 }
  if not is_valid(y) { give -1 }
  if not is_valid(z) { give -1 }
  give x + y + z
}
```

### N3 (avance): responsabilites separees

```vit
proc validate3(x: int, y: int, z: int) -> bool { give x>=0 and y>=0 and z>=0 }
proc sum3(x: int, y: int, z: int) -> int { give x+y+z }
```

### Anti-exemple

```vit
proc mega(a:int,b:int,c:int,d:int,e:int) -> int { give a+b+c+d+e }
```

## Validation rapide

1. Detecter fonction trop large.
2. Extraire responsabilites.
3. Ajouter test de non-regression.

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

Thème: **anti-patterns frequents en code vitte**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc process(x: int, y: int, z: int) -> int {
  if x < 0 { give -1 }
  if y < 0 { give -1 }
  if z < 0 { give -1 }
  give x + y + z
}
```

Lecture ligne par ligne:
1. `proc process(x: int, y: int, z: int) -> int {` -> pose un contrat clair de fonction.
2. `if x < 0 { give -1 }` -> sépare nominal et cas limite.
3. `if y < 0 { give -1 }` -> sépare nominal et cas limite.
4. `if z < 0 { give -1 }` -> sépare nominal et cas limite.
5. `give x + y + z` -> renvoie la sortie vérifiable.
6. `}` -> participe au déroulé du traitement.

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
