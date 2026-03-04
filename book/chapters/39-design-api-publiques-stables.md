# 39. Design d'API publiques stables

Niveau: Avance

Prerequis: `book/chapters/06-procedures.md`, `book/chapters/17-stdlib.md`.
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
proc parse_port(s: string) -> int { ... }

# extension additive
proc parse_port_with_default(s: string, d: int) -> int { ... }
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
