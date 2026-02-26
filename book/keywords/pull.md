# Mot-clé `pull`

Niveau: Avancé.

## Définition

`pull` importe un module en top-level, avec alias optionnel.

## Syntaxe

Forme canonique: `pull module/path [as alias]`.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour importer un module projet clairement nommé.
- Quand l’éviter: dans un bloc d’instructions ou avec un alias ambigu.

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

## Pièges

- Importer avec alias trop génériques.
- Déclarer `pull` dans un bloc.
- Utiliser un chemin relatif non documenté.

## Utilisé dans les chapitres

- `docs/book/chapters/03-projet.md`.
- `docs/book/chapters/09-modules.md`.
- `docs/book/chapters/29-style.md`.

## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/use.md`.
- `docs/book/keywords/space.md`.
- `docs/book/chapters/09-modules.md`.
