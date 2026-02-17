# Mot-clé `make`

Niveau: Débutant.

## Définition

`make` déclare une variable (globale ou locale) avec type optionnel via `as`.

## Syntaxe

Forme canonique: `make nom [as type] = expr`.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour expliciter une création de variable avec contrainte de type.
- Quand l’éviter: comme expression (`make` n’est pas un littéral).

## Exemple nominal

Entrée:
- Initialisation locale valide.

```vit
proc init() -> int {
  make counter as int = 0
  give counter
}
```

Sortie observable:
- `counter` est initialisé puis retourné.

## Exemple invalide

Entrée:
- Usage de `make` comme expression.

```vit
proc bad() -> int {
  let c: int = make 0
  give c
}
# invalide: `make` est une déclaration, pas une expression.
```

Sortie observable:
- Le parseur rejette l’instruction.

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near make` | Nom ou `=` manquant. | Utiliser `make ident [as type] = expr`. |
| `invalid declaration context` | Déclaration mal placée. | Garder `make` dans top-level ou bloc `stmt`. |
| `type mismatch` | Type `as` incompatible avec l’expression. | Aligner expression et type déclaré. |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `let` | `let` autorise `: type`; `make` utilise `as type`. |

## Pièges

- Confondre `make` et constructeur de valeur.
- Omettre `=` dans la déclaration.
- Mélanger conventions `let`/`make` dans un même bloc sans règle.

## Utilisé dans les chapitres

- `docs/book/chapters/30-faq.md`.
- `docs/book/chapters/31-erreurs-build.md`.

## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/let.md`.
- `docs/book/keywords/set.md`.
- `docs/book/chapters/04-syntaxe.md`.
