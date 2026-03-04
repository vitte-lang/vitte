# Mot-clé `pull`

Niveau: Avancé.

## Lecture rapide (30s)

- Ce que c’est: ce mot-clé exprime une intention précise dans le flux Vitte.
- Quand l’utiliser: quand il rend la lecture du contrat plus directe.
- Erreur classique: l’utiliser au mauvais niveau (top-level vs bloc).

## Pourquoi (métier)

`pull` réduit l’ambiguïté dans le code de production.

Vous l’utilisez pour rendre la règle métier explicite dès la lecture.
Cela simplifie les revues et accélère le diagnostic en cas d’erreur.
Le but est un comportement stable, lisible et testable.

## Définition

`pull` importe un module en top-level, avec alias optionnel.

## Syntaxe

Forme canonique: `pull module/path [as alias]`.

## Exemple nominal

Entrée:
- Import top-level avec alias.

```vit
space api/v1
pull core/math as math
proc sum(a: int, b: int) -> int {
  give math.add(a, b)
}
```

Sortie observable:
- Le module `core/math` est résolu sous l’alias `math`.

## Exemple invalide

Entrée:
- Ordre syntaxique invalide.

```vit
pull as core/math
# invalide: `as` vient après le chemin module.
```

Sortie observable:
- La déclaration est rejetée par le parseur.

Diagnostic attendu:
- Code: `VITTE-XXXX` (ou code compilateur `E000X` correspondant).
- Position: `ligne 1, colonne 1` (ajustez selon le snippet réel).
- Message: motif stable orienté correction.

## Différences proches

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `use` | `pull` importe un module; `use` sélectionne des symboles depuis un chemin. |

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

- Importer avec alias trop génériques.
- Déclarer `pull` dans un bloc.
- Utiliser un chemin relatif non documenté.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour importer un module projet clairement nommé.
- Quand l’éviter: dans un bloc d’instructions ou avec un alias ambigu.

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near pull` | Ordre `pull`/chemin/alias invalide. | Respecter `pull path [as alias]`. |
| `unknown module` | Chemin module introuvable. | Vérifier le `module_path` importé. |
| `duplicate import alias` | Alias déjà utilisé. | Renommer l’alias pour éviter collision. |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `use` | `pull` importe un module; `use` sélectionne des symboles depuis un chemin. |

## Utilisé dans les chapitres

- `docs/book/chapters/07-controle.md`.
- `docs/book/chapters/27-grammaire.md`.
- `docs/book/chapters/31-erreurs-build.md`.

## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/use.md`.
- `docs/book/keywords/space.md`.
- `docs/book/chapters/09-modules.md`.

## Score de complétude

coverage: syntaxe/exemples/invalides/diagnostics/liens = 4/5
