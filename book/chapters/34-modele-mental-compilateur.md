# 34. Modele mental du compilateur

Niveau: Intermediaire

Prérequis: `book/chapters/15-pipeline.md`, `book/chapters/27-grammaire.md`.
Voir aussi: `book/chapters/10-diagnostics.md`, `book/chapters/31-erreurs-build.md`.

## Objectif

Comprendre le flux complet `source -> parse -> resolve/type -> IR -> backend -> binaire` pour localiser vite les erreurs.

## Carte rapide du pipeline

1. Lex/parse: verifier que la forme du code respecte l'EBNF.
2. Resolve/type: verifier symboles, imports, types et contrats.
3. IR: representer le programme dans une forme exploitable par les passes.
4. Backend: generer C++ (ou autre cible) puis compiler/lier.
5. Runtime: verifier ABI, symboles natifs et comportement final.

## Exemple minimal

```vit
entry main at app/demo {
  let x: int = 2
  // Sortie programme: code de retour observable
  return x + 1
}
```

Lecture pipeline:
- Parse: structure `entry` + `block` valide.
- Resolve/type: `x` est defini et reste `int`.
- Backend: emission d'un binaire executable.

## Triage par couche

- Erreur de forme: corriger d'abord la grammaire.
- Erreur de type/symbole: corriger declaration/import/annotation.
- Erreur de link/runtime: corriger ABI, symboles ou config outillage.

## Checklist

1. Identifier la premiere couche qui casse.
2. Produire un reproducer minimal.
3. Corriger localement une seule cause.
4. Relancer build/tests.
5. Documenter la cause racine.

## Exemples progressifs (N1 -> N3)

### N1 (base): parse valide

```vit
entry main at app/demo {
  // Sortie programme: code de retour observable
  return 0
}
```

### N2 (intermediaire): parse invalide top-level

```vit
emit 1
```

### N3 (avance): parse + resolve

```vit
entry main at app/demo {
  // Sortie programme: code de retour observable
  return missing_symbol
}
```

### Anti-exemple

```vit
entry main at app/demo {
  return
}
```

## Validation rapide

1. N1 compile.
2. N2 echoue au parse.
3. N3 parse puis echoue en resolve.

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

Thème: **modele mental du compilateur**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
entry main at app/demo {
  let x: int = 2
  // Sortie programme: code de retour observable
  return x + 1
}
```

Lecture ligne par ligne:
1. `entry main at app/demo {` -> définit le point d'entrée du scénario.
2. `let x: int = 2` -> nomme une valeur intermédiaire utile.
3. `return x + 1` -> renvoie la sortie vérifiable.
4. `}` -> participe au déroulé du traitement.

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

Exemple approfondi pour **modele mental compilateur**: pipeline validation -> transformation -> décision -> projection.

```vit
// Exemple long: flux complet et vérifiable
space demo/modele-mental-compilateur

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

Scénarios recommandés (modele mental compilateur):
- Cas nominal -> sortie 0.
- Cas quota strict -> comportement déterministe.
- Cas invalide id<=0 -> sortie 21.
