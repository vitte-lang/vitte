# 36. Strategies de nommage en Vitte

Niveau: Intermediaire

Prérequis: `book/chapters/28-conventions.md`.
Voir aussi: `book/chapters/09-modules.md`, `book/chapters/06-procedures.md`.

## Objectif

Choisir des noms predictibles pour reduire les erreurs de lecture et de maintenance.

## Regles pratiques

1. Noms de procedures: verbe + objet (`parse_port`, `load_user`).
2. Predicats booleens: prefixes `is_`, `has_`, `can_`.
3. Types: noms metier (`User`, `OrderStatus`), pas noms techniques opaques.
4. Modules: noms courts, semantiques et stables.

## Exemple minimal

```vit
form User {
  id: int
  name: string
}

proc is_admin(role: int) -> bool {
  give role == 9
}
```

## Anti-patterns

- Prefixes techniques partout (`tmp_`, `data_`, `obj_`).
- Abreviations ambiguës (`cfg2`, `usrx`).
- Renommages massifs sans migration.

## Checklist

1. Le nom exprime-t-il l'intention metier?
2. Le nom est-il coherent avec le module?
3. Le nom reste-t-il stable dans le temps?

## Exemples progressifs (N1 -> N3)

### N1 (base): nommage clair

```vit
proc parse_port(x: int) -> int { give x }
```

### N2 (intermediaire): predicat booleen explicite

```vit
proc is_admin(role: int) -> bool { give role == 9 }
```

### N3 (avance): cohesion de noms

```vit
proc load_user(id: int) -> int { give id }
proc validate_user(id: int) -> bool { give id > 0 }
proc save_user(id: int) -> int { give id }
```

### Anti-exemple

```vit
proc do_it(x: int) -> int { give x }
```

## Validation rapide

1. Verbe + objet.
2. Predicats en `is_`/`has_`.
3. Vocabulaire coherent sur tout le module.

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

Thème: **strategies de nommage en vitte**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
form User {
  id: int
  name: string
}

proc is_admin(role: int) -> bool {
  give role == 9
}
```

Lecture ligne par ligne:
1. `form User {` -> participe au déroulé du traitement.
2. `id: int` -> participe au déroulé du traitement.
3. `name: string` -> participe au déroulé du traitement.
4. `}` -> participe au déroulé du traitement.
5. `proc is_admin(role: int) -> bool {` -> pose un contrat clair de fonction.
6. `give role == 9` -> renvoie la sortie vérifiable.
7. `}` -> participe au déroulé du traitement.

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
