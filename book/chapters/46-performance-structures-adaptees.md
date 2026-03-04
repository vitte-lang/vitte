# 46. Performance: structures de donnees adaptees

Niveau: Avance

Prerequis: `book/chapters/08-structures.md`, `book/chapters/11-collections.md`.

## Objectif

Choisir la structure de donnees selon les operations dominantes.

## Regle simple

1. Lectures indexees frequentes: structure indexable.
2. Recherche par cle frequente: map.
3. Donnees heterogenes stables: form/pick.
4. Flux sequentiel: liste/slice optimisee.

## Checklist

1. Operation dominante identifiee.
2. Complexite attendue explicitee.
3. Mesure avant/apres validee.


## Exemples progressifs (N1 -> N3)

### N1 (base): liste adaptee iteration

```vit
proc first(xs: [int]) -> int { give 0 }
```

### N2 (intermediaire): map adaptee lookup

```vit
form User { id: int name: string }
proc get_user_id(u: User) -> int { give u.id }
```

### N3 (avance): choix motive

```vit
pick LookupMode { case Linear, case Keyed }
proc pick_mode(by_key: bool) -> LookupMode {
  if by_key { give Keyed }
  give Linear
}
```

### Anti-exemple

```vit
proc choose_unknown() -> int { give 0 }
# aucun critere de choix structure
```

## Validation rapide

1. Identifier operation dominante.
2. Justifier structure.
3. Verifier performance cible.
