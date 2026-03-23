# 50. Tests d'integration orientes scenario

Niveau: Avance

Prérequis: `book/chapters/18-tests.md`, `book/chapters/21-projet-cli.md`.
Voir aussi: à définir.

## Objectif

Verifier des parcours complets utilisateur/systeme plutot que des fonctions isolees.

## Demarche

1. Definir scenario nominal et scenario echec.
2. Monter un environnement proche production.
3. Verifier sortie metier et effets de bord.
4. Nettoyer et rerendre le test idempotent.

## Checklist

1. Donnees de test versionnees.
2. Resultat attendu stable.
3. Execution possible en CI.

## Exemples progressifs (N1 -> N3)

### N1 (base): scenario nominal

```vit
entry main at app/cli {
  // Sortie programme: code de retour observable
  return 0
}
```

### N2 (intermediaire): scenario erreur

```vit
entry main at app/cli {
  // Sortie programme: code de retour observable
  return missing
}
```

### N3 (avance): scenario idempotent

```vit
proc run_once(ok: bool) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if not ok { give 1 }
  // Sortie locale: valeur retournee par la procedure
  give 0
}
```

### Anti-exemple

```vit
# scenario depend d'un etat manuel externe
```

## Validation rapide

1. Nominal + erreur couverts.
2. Sortie observable verifiee.
3. Reexecution sans effet parasite.

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

Thème: **tests d'integration orientes scenario**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
entry main at app/cli {
  // Sortie programme: code de retour observable
  return 0
}
```

Lecture ligne par ligne:
1. `entry main at app/cli {` -> définit le point d'entrée du scénario.
2. `return 0` -> renvoie la sortie vérifiable.
3. `}` -> participe au déroulé du traitement.

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

Exemple approfondi pour **tests integration scenario**: harnais complet (cas unitaires, agrégation, projection CI).

```vit
// Exemple long: flux complet et vérifiable
space demo/tests-integration-scenario

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

Scénarios recommandés (tests integration scenario):
- 3 sur 3 réussis -> sortie 0.
- Régression partielle -> sortie 11.
- Échec global -> sortie 13.
