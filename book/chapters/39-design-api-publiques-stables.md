# 39. Design d'API publiques stables

Niveau: Avance

Prérequis: `book/chapters/06-procedures.md`, `book/chapters/17-stdlib.md`.
Voir aussi: `book/chapters/40-compatibilite-breaking.md`.

## Objectif

Definir des contrats publics qui evoluent sans casser inutilement les utilisateurs.

## Principes

1. Surface minimale: exposer seulement le necessaire.
2. Contrats explicites: types, erreurs, invariants.
3. Evolution additive d'abord.
4. Deprecation documentee avant suppression.

## Exemple minimal

```vit
# API publique
proc parse_port(s: string) -> int { .. }

# extension additive
proc parse_port_with_default(s: string, d: int) -> int { .. }
```

## Checklist

1. Le contrat est-il testable depuis l'exterieur?
2. Le changement est-il additif?
3. La migration est-elle documentee?

## Exemples progressifs (N1 -> N3)

### N1 (base): API publique simple

```vit
proc parse_port(s: string) -> int { give 0 }
```

### N2 (intermediaire): extension additive

```vit
proc parse_port_with_default(s: string, d: int) -> int { give d }
```

### N3 (avance): coexistence versionnee

```vit
proc parse_port_v1(s: string) -> int { give 0 }
proc parse_port_v2(s: string, d: int) -> int { give d }
```

### Anti-exemple

```vit
proc parse_port(s: string) -> bool { give true }
```

## Validation rapide

1. Priorite aux changements additifs.
2. Documenter deprecation.
3. Tester v1 + v2.

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

Thème: **design d'api publiques stables**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
# API publique
proc parse_port(s: string) -> int { .. }

# extension additive
proc parse_port_with_default(s: string, d: int) -> int { .. }
```

Lecture ligne par ligne:
1. `# API publique` -> participe au déroulé du traitement.
2. `proc parse_port(s: string) -> int { .. }` -> pose un contrat clair de fonction.
3. `# extension additive` -> participe au déroulé du traitement.
4. `proc parse_port_with_default(s: string, d: int) -> int { .. }` -> pose un contrat clair de fonction.

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

Exemple approfondi pour **design api publiques stables**: pipeline validation -> transformation -> décision -> projection.

```vit
// Exemple long: flux complet et vérifiable
space demo/design-api-publiques-stables

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

Scénarios recommandés (design api publiques stables):
- Cas nominal -> sortie 0.
- Cas quota strict -> comportement déterministe.
- Cas invalide id<=0 -> sortie 21.
