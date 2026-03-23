# 42. Patterns de procedures robustes

Niveau: Intermediaire

Prérequis: `book/chapters/06-procedures.md`, `book/chapters/07-controle.md`.
Voir aussi: `book/chapters/18-tests.md`.

## Objectif

Ecrire des `proc` lisibles, testables et robustes face aux cas limites.

## Patterns recommandes

1. Gardes en tete (fail fast).
2. Contrat explicite en signature.
3. Unites de comportement courtes.
4. Retour deterministe pour chaque branche.

## Exemple minimal

```vit
proc parse_port(x: int) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if x < 0 { give -1 }
  // Garde: bloque un cas invalide avant de continuer
  if x > 65535 { give -1 }
  // Sortie locale: valeur retournee par la procedure
  give x
}
```

## Anti-patterns

- Branches implicites sans `give` clair.
- Procedure trop longue avec plusieurs responsabilites.
- Melange validation, transformation et I/O.

## Checklist

1. Chaque branche a une sortie observable.
2. Les cas limites sont traites avant le nominal.
3. Un test couvre chaque garde.

## Exemples progressifs (N1 -> N3)

### N1 (base): garde minimale

```vit
proc parse_port(x: int) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if x < 0 { give -1 }
  // Sortie locale: valeur retournee par la procedure
  give x
}
```

### N2 (intermediaire): bornes completes

```vit
proc parse_port(x: int) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if x < 0 { give -1 }
  // Garde: bloque un cas invalide avant de continuer
  if x > 65535 { give -1 }
  // Sortie locale: valeur retournee par la procedure
  give x
}
```

### N3 (avance): comportement testable

```vit
proc parse_port_or_default(x: int, d: int) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if x < 0 { give d }
  // Garde: bloque un cas invalide avant de continuer
  if x > 65535 { give d }
  // Sortie locale: valeur retournee par la procedure
  give x
}
```

### Anti-exemple

```vit
proc parse_port(x: int) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if x < 0 { give -1 }
}
```

## Validation rapide

1. Une sortie explicite par branche.
2. Cas limites testes.
3. Proc courte et lisible.

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

Thème: **patterns de procedures robustes**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc parse_port(x: int) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if x < 0 { give -1 }
  // Garde: bloque un cas invalide avant de continuer
  if x > 65535 { give -1 }
  // Sortie locale: valeur retournee par la procedure
  give x
}
```

Lecture ligne par ligne:
1. `proc parse_port(x: int) -> int {` -> pose un contrat clair de fonction.
2. `if x < 0 { give -1 }` -> sépare nominal et cas limite.
3. `if x > 65535 { give -1 }` -> sépare nominal et cas limite.
4. `give x` -> renvoie la sortie vérifiable.
5. `}` -> participe au déroulé du traitement.

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

Exemple approfondi pour **patterns procedures robustes**: pipeline validation -> transformation -> décision -> projection.

```vit
// Exemple long: flux complet et vérifiable
space demo/patterns-procedures-robustes

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

Scénarios recommandés (patterns procedures robustes):
- Cas nominal -> sortie 0.
- Cas quota strict -> comportement déterministe.
- Cas invalide id<=0 -> sortie 21.
