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
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if x < 0 { give -1 }
  // Garde: bloque un cas invalide avant de continuer
  if y < 0 { give -1 }
  // Garde: bloque un cas invalide avant de continuer
  if z < 0 { give -1 }
  // Sortie locale: valeur retournee par la procedure
  give x + y + z
}
```

### N2 (intermediaire): extraction

```vit
proc is_valid(x: int) -> bool { give x >= 0 }
proc process3(x: int, y: int, z: int) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if not is_valid(x) { give -1 }
  // Garde: bloque un cas invalide avant de continuer
  if not is_valid(y) { give -1 }
  // Garde: bloque un cas invalide avant de continuer
  if not is_valid(z) { give -1 }
  // Sortie locale: valeur retournee par la procedure
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
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if x < 0 { give -1 }
  // Garde: bloque un cas invalide avant de continuer
  if y < 0 { give -1 }
  // Garde: bloque un cas invalide avant de continuer
  if z < 0 { give -1 }
  // Sortie locale: valeur retournee par la procedure
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



## Exemple Étendu

Exemple approfondi pour **anti patterns code vitte**: pipeline validation -> transformation -> décision -> projection.

```vit
// Exemple long: flux complet et vérifiable
space demo/anti-patterns-code-vitte

form Input { id: int value: int quota: int }
pick Eval { case Accepted(score: int) case Rejected(code: int) }

proc validate(x: Input) -> Eval {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if x.id <= 0 { give Eval.Rejected(21) }
  // Garde: bloque un cas invalide avant de continuer
  if x.quota < 0 { give Eval.Rejected(22) }
  // Garde: bloque un cas invalide avant de continuer
  if x.value < 0 { give Eval.Rejected(23) }
  // Sortie locale: valeur retournee par la procedure
  give Eval.Accepted(x.value)
}

proc transform(score: int, quota: int) -> int {
  let capped: int = score
  if capped > quota { set capped = quota }
  // Garde: bloque un cas invalide avant de continuer
  if capped < 0 { give 0 }
  // Sortie locale: valeur retournee par la procedure
  give capped * 2
}

proc decide(r: Eval, quota: int) -> Eval {
  // Bloc logique: decision par branches explicites
  // Match: decision explicite selon l'etat
  match r {
    case Accepted(s) {
      let out: int = transform(s, quota)
      // Garde: bloque un cas invalide avant de continuer
  if out >= 10 { give Eval.Accepted(out) }
      // Sortie locale: valeur retournee par la procedure
  give Eval.Rejected(31)
    }
    case Rejected(c) { give Eval.Rejected(c) }
    otherwise { give Eval.Rejected(70) }
  }
}

// Projection finale: convertit l'état métier en code de sortie
proc to_exit(r: Eval) -> int {
  // Bloc logique: decision par branches explicites
  // Match: decision explicite selon l'etat
  match r {
    case Accepted(_) { give 0 }
    case Rejected(code) { give code }
    otherwise { give 70 }
  }
}

// Orchestration: enchaîne les étapes sans logique cachée
entry main at core/app {
  let x: Input = Input(1, 8, 9)
  let v: Eval = validate(x)
  let d: Eval = decide(v, x.quota)
  // Sortie programme: code de retour observable
  return to_exit(d)
}
```

Scénarios recommandés (anti patterns code vitte):
- Cas nominal -> sortie 0.
- Cas quota strict -> comportement déterministe.
- Cas invalide id<=0 -> sortie 21.
