# Tetris en Vitte

Version modulaire du Tetris du dépôt, avec plateau 10 × 20, sept tétriminos,
rotations, gravité, verrouillage, suppression de lignes, score, aperçu, pièce
de réserve et rendu ANSI. Le jeu lit les touches au clavier dans un terminal
POSIX.

## Compiler et lancer

Depuis la racine du dépôt :

```sh
make -C games/tetris check
make -C games/tetris build
make -C games/tetris run
```

Le Makefile utilise par défaut `build/bin/vitte` et le backend C17. Pour choisir
un autre compilateur, passer `VITTEC=/chemin/vers/vitte` à `make`.
`vitte.toml` contient les réglages plats reconnus par le compilateur ; les
métadonnées du jeu sont conservées dans `tetris-manifest.toml`.

## Contrôles

| Touche | Action |
| --- | --- |
| `A` / `D` | Déplacer à gauche / à droite |
| `S` | Descente rapide |
| `W` / `Z` | Rotation horaire / antihoraire |
| `Espace` | Chute immédiate |
| `C` | Garder ou échanger la pièce |
| `P` | Pause / reprise |
| `R` | Recommencer |
| `Q` | Quitter |

`make check` vérifie les fichiers puis analyse sémantiquement le point d’entrée
et ses modules importés. Les blocs `test` présents dans les sources ne sont pas
exécutés par le CLI actuel ; `make test` indique cette limite après les checks.
