# 36. Strategies de nommage en Vitte

Niveau: Intermediaire

Prerequis: `book/chapters/28-conventions.md`.
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
