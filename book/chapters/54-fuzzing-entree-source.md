# 54. Fuzzing de l'entree source

Niveau: Avance

Prérequis: `book/chapters/53-property-based-testing-parser.md`.
Voir aussi: à définir.

## Objectif

Renforcer robustesse parseur/diagnostics face a des entrees arbitraires.

## Cibles

1. Lexer.
2. Parser.
3. Generation de diagnostics.

## Garde-fous

1. Timeout par execution.
2. Isolation des crashes.
3. Archivage des corpus qui declenchent un bug.

## Exemples progressifs (N1 -> N3)

### N1 (base): fuzz token simple

```vit
entry main at app/demo { return 0 }
```

### N2 (intermediaire): fuzz structure bloc

```vit
entry main at app/demo {
  if true { return 0 }
  // Sortie programme: code de retour observable
  return 1
}
```

### N3 (avance): fuzz corpus continu

```vit
// Match: decision explicite selon l'etat
  match 1 {
  case 1 { give 1 }
  otherwise { give 0 }
}
```

### Anti-exemple

```vit
# crash detecte mais non archive dans corpus
```

## Validation rapide

1. Timeout par cas actif.
2. Crashes archives.
3. Cas crashants convertis en tests.

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

Thème: **fuzzing de l'entree source**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
entry main at app/demo { return 0 }
```

Lecture ligne par ligne:
1. `entry main at app/demo { return 0 }` -> définit le point d'entrée du scénario.

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

Exemple approfondi pour **fuzzing entree source**: harnais complet (cas unitaires, agrégation, projection CI).

```vit
// Exemple long: flux complet et vérifiable
space demo/fuzzing-entree-source

form TestCase { id: int input: int expected: int }
pick CaseResult { case Pass(id: int) case Fail(id: int, got: int, expected: int) }

proc subject(x: int) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if x < 0 { give 0 }
  // Garde: bloque un cas invalide avant de continuer
  if x > 100 { give 100 }
  // Sortie locale: valeur retournee par la procedure
  give x
}

// Exécute un cas de test et retourne un résultat typé
proc run(c: TestCase) -> CaseResult {
  let got: int = subject(c.input)
  // Garde: bloque un cas invalide avant de continuer
  if got == c.expected { give CaseResult.Pass(c.id) }
  // Sortie locale: valeur retournee par la procedure
  give CaseResult.Fail(c.id, got, c.expected)
}

// Agrège les résultats et projette un code CI
proc ci_exit(a: CaseResult, b: CaseResult, c: CaseResult) -> int {
  let ok: int = 0
  // Match: decision explicite selon l'etat
  match a { case Pass(_) { set ok = ok + 1 } otherwise { } }
  // Match: decision explicite selon l'etat
  match b { case Pass(_) { set ok = ok + 1 } otherwise { } }
  // Match: decision explicite selon l'etat
  match c { case Pass(_) { set ok = ok + 1 } otherwise { } }
  // Garde: bloque un cas invalide avant de continuer
  if ok == 3 { give 0 }
  // Garde: bloque un cas invalide avant de continuer
  if ok == 2 { give 11 }
  // Garde: bloque un cas invalide avant de continuer
  if ok == 1 { give 12 }
  // Sortie locale: valeur retournee par la procedure
  give 13
}

// Orchestration: enchaîne les étapes sans logique cachée
entry main at core/app {
  let c1: TestCase = TestCase(1, -3, 0)
  let c2: TestCase = TestCase(2, 42, 42)
  let c3: TestCase = TestCase(3, 160, 100)
  let r1: CaseResult = run(c1)
  let r2: CaseResult = run(c2)
  let r3: CaseResult = run(c3)
  // Sortie programme: code de retour observable
  return ci_exit(r1, r2, r3)
}
```

Scénarios recommandés (fuzzing entree source):
- 3 sur 3 réussis -> sortie 0.
- Régression partielle -> sortie 11.
- Échec global -> sortie 13.
