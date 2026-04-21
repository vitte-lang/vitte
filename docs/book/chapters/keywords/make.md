# Mot-clé `make`

Niveau: Débutant.

## Lecture rapide

Repère concret: `make` sert à décider un chemin d'exécution de façon lisible et vérifiable.
Utilisez `make` quand il sert à structurer le flux d'exécution; évitez-le s'il n'apporte aucune différence observable sur la branche ou la sortie.

## Pourquoi (métier)

En pratique algorithmique, `make` sert à transformer une condition en branche exécutable, sans ambiguïté de lecture.
Règle pratique: si retirer `make` ne change ni le chemin exécuté ni la sortie, simplifiez le bloc.

## Définition

`make` déclare une variable (globale ou locale) avec type optionnel via `as`.

## Syntaxe

Forme canonique: `make nom [as type] = expr`.

## Lecture algorithmique

Lecture conseillée: traquez où `make` intervient dans le flux, puis vérifiez son effet sur l'exécution réelle.

## Exemple nominal

Entrée:
- Initialisation locale valide.

```vit
// Exemple concret: cas nominal puis cas invalide

proc init() -> int {
  make counter as int = 0
  // Valeur retournee par cette branche
  give counter
}
```

Sortie observable:
- `counter` est initialisé puis retourné.

## Exemple invalide

Entrée:
- Usage de `make` comme expression.

```vit
// Exemple concret: cas nominal puis cas invalide

proc bad() -> int {
  let c: int = make 0
  // Valeur retournee par cette branche
  give c
}
# invalide: `make` est une déclaration, pas une expression.
```

Sortie observable:
- Le parseur rejette l’instruction.

Diagnostic attendu:
- Code: `VITTE-XXXX` (ou code compilateur `E000X` correspondant).
- Position: `ligne 1, colonne 1` (ajustez selon le snippet réel).
- Message: motif stable orienté correction.

## Refactor rapide

Avant:
```vit
// Exemple concret: cas nominal puis cas invalide
# usage fragile à corriger
```

Après:
```vit
// Exemple concret: cas nominal puis cas invalide
# usage clair et testable
```

## Pièges

- Confondre `make` et constructeur de valeur.
- Omettre `=` dans la déclaration.
- Mélanger conventions `let`/`make` dans un même bloc sans règle.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour expliciter une création de variable avec contrainte de type.
- Quand l’éviter: comme expression (`make` n’est pas un littéral).

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

## Utilisé dans les chapitres

- `docs/book/chapters/07-controle.md`.
- `docs/book/chapters/27-grammaire.md`.
- `docs/book/chapters/31-erreurs-build.md`.

## Voir aussi

- `docs/book/chapters/keywords/erreurs-compilateur.md`.
- `docs/book/chapters/keywords/let.md`.
- `docs/book/chapters/keywords/set.md`.
- `docs/book/chapters/04-syntaxe.md`.

## Score de complétude

coverage: syntaxe/exemples/invalides/diagnostics/liens = 3/5
