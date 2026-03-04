# Mot-clé `make`

Niveau: Débutant.

## Lecture rapide (30s)

- Ce que c’est: ce mot-clé exprime une intention précise dans le flux Vitte.
- Quand l’utiliser: quand il rend la lecture du contrat plus directe.
- Erreur classique: l’utiliser au mauvais niveau (top-level vs bloc).

## Pourquoi (métier)

`make` réduit l’ambiguïté dans le code de production.

Vous l’utilisez pour rendre la règle métier explicite dès la lecture.
Cela simplifie les revues et accélère le diagnostic en cas d’erreur.
Le but est un comportement stable, lisible et testable.

## Définition

`make` déclare une variable (globale ou locale) avec type optionnel via `as`.

## Syntaxe

Forme canonique: `make nom [as type] = expr`.

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

Diagnostic attendu:
- Code: `VITTE-XXXX` (ou code compilateur `E000X` correspondant).
- Position: `ligne 1, colonne 1` (ajustez selon le snippet réel).
- Message: motif stable orienté correction.

## Différences proches

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `let` | `let` autorise `: type`; `make` utilise `as type`. |

## Refactor rapide

Avant:
```vit
# usage fragile à corriger
```

Après:
```vit
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

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/let.md`.
- `docs/book/keywords/set.md`.
- `docs/book/chapters/04-syntaxe.md`.

## Score de complétude

coverage: syntaxe/exemples/invalides/diagnostics/liens = 3/5
