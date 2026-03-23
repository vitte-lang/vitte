# 58. Pipeline CI pour un projet Vitte

Niveau: Avancé

Prérequis: `book/chapters/18-tests.md`, `book/chapters/20-repro.md`.
Voir aussi: à définir.

## Objectif

Mettre en place une CI fiable qui valide build, tests et qualité documentaire a chaque changement.

## Étapes clés

1. Build compilateur/projet.
2. Tests unitaires et intégration.
3. Checks grammaire/diagnostics.
4. Vérification docs et style.

## Checklist

1. Pipeline deterministic.
2. Resultats lisibles en moins de 10 minutes.
3. Blocage merge en cas de regression.

## Exemples progressifs (N1 -> N3)

### N1 (base): pipeline compile + test

Snippet Vitte:

```vit
entry main at app/ci {
  // Sortie programme: code de retour observable
  return 0
}
```

Commandes CI:

```bash
make grammar-check
make grammar-test
```

### N2 (intermediaire): pipeline avec docs et diagnostics

Snippet Vitte (cas invalide de controle):

```vit
emit 1
```

Commandes CI:

```bash
make grammar-gate
make book-qa
```

### N3 (avance): gate complet avant merge

Snippet Vitte (cas nominal):

```vit
entry main at app/ci {
  let x: int = 1
  // Sortie programme: code de retour observable
  return x
}
```

Commandes CI:

```bash
make grammar-gate
make book-qa-strict
```

### Anti-exemple

```text
Pipeline vert sans tests invalid/diagnostics ni check documentation.
```

## Validation rapide

1. N1: compile + corpus OK.
2. N2: cas invalide detecte et diagnostique.
3. N3: gate complet vert avant merge.

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

Thème: **pipeline ci pour un projet vitte**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
entry main at app/ci {
  // Sortie programme: code de retour observable
  return 0
}
```

Lecture ligne par ligne:
1. `entry main at app/ci {` -> définit le point d'entrée du scénario.
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

Exemple approfondi pour **pipeline ci projet vitte**: flux applicatif complet (entrée, politique métier, persistance simulée, code de sortie).

```vit
// Exemple long: flux complet et vérifiable
space demo/pipeline-ci-projet-vitte

form Request { id: int amount: int quota: int }
pick Result { case Accepted(total: int) case Rejected(code: int) }

// Entrée applicative: validation des invariants de requête
proc parse_request(r: Request) -> Result {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if r.id <= 0 { give Result.Rejected(91) }
  // Garde: bloque un cas invalide avant de continuer
  if r.quota < 0 { give Result.Rejected(92) }
  // Garde: bloque un cas invalide avant de continuer
  if r.amount < 0 { give Result.Rejected(93) }
  // Sortie locale: valeur retournee par la procedure
  give Result.Accepted(r.amount)
}

// Politique métier: applique les règles de décision
proc apply_policy(total: int, quota: int) -> Result {
  let capped: int = total
  if capped > quota { set capped = quota }
  // Garde: bloque un cas invalide avant de continuer
  if capped < 5 { give Result.Rejected(94) }
  // Sortie locale: valeur retournee par la procedure
  give Result.Accepted(capped)
}

// Persistance simulée: matérialise un résultat sans I/O réel
proc persist_sim(x: Result) -> Result {
  // Bloc logique: decision par branches explicites
  // Match: decision explicite selon l'etat
  match x {
    case Accepted(v) {
      // Garde: bloque un cas invalide avant de continuer
  if v % 13 == 0 { give Result.Rejected(95) }
      // Sortie locale: valeur retournee par la procedure
  give Result.Accepted(v)
    }
    case Rejected(c) { give Result.Rejected(c) }
    otherwise { give Result.Rejected(70) }
  }
}

// Projection finale: convertit l'état métier en code de sortie
proc to_exit(x: Result) -> int {
  // Bloc logique: decision par branches explicites
  // Match: decision explicite selon l'etat
  match x {
    case Accepted(_) { give 0 }
    case Rejected(c) { give c }
    otherwise { give 70 }
  }
}

// Orchestration: enchaîne les étapes sans logique cachée
entry main at core/app {
  let req: Request = Request(7, 12, 15)
  let p: Result = parse_request(req)
  let d: Result = apply_policy(12, req.quota)
  let s: Result = persist_sim(d)
  let _probe: int = to_exit(p)
  // Sortie programme: code de retour observable
  return to_exit(s)
}
```

Scénarios recommandés (pipeline ci projet vitte):
- Requête nominale -> sortie 0.
- Entrée invalide id<=0 -> sortie 91.
- Refus métier valeur<5 -> sortie 94.
