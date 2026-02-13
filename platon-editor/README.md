# Platon Editor (base Vitte)

`platon-editor/editor_core.vit` est une base compilable pour un futur éditeur terminal en Vitte.

## Build

```sh
./bin/vitte build platon-editor/editor_core.vit -o platon-editor/editor_core
```

## Run

```sh
platon-editor/editor_core
echo $?
```

Le binaire exécute `self_test()` et retourne `0` si le noyau est cohérent.

## Fonctionnalités de la base

- Buffer texte ligne par ligne (`insert`, `delete`, `replace`).
- Curseur borné (`editor_move_cursor`).
- Historique borné (base pour undo/redo).
- Modes (`normal`, `insert`, `command`).
- Catalogue interne de 30 langages.

## Pourquoi pas encore Vim/Emacs complet

Le repo a encore des limitations backend/runtime sur certaines APIs std nécessaires à un éditeur interactif complet (I/O terminal brut, parsing texte riche). Le plan d'extension est dans `platon-editor/ROADMAP.md`.
