# Téléchargement Vitte

Installer, compiler et vérifier rapidement avec des étapes courtes et concrètes.

## Navigation

- [Accueil](index.md)
- [Documentation](doc.md)
- [Téléchargement](download.md)
- [Source](source.md)
- [Communauté](community.md)
- [Actualités](news.md)
- [Diagnostics](diagnostics.md)
- [Suggestions](suggestions.md)

## Notes de version - Version 2.1.1

Date : 18 février 2026

[Référence de publication : Vitte2026_02](https://github.com/vitte-lang/vitte/releases/tag/Vitte2026_02)

- **Installateur** : [vitte-2.1.1.pkg](https://github.com/vitte-lang/vitte/releases/download/Vitte2026_02/vitte-2.1.1.pkg)
- **Désinstallateur** : [vitte-uninstall-2.1.1.pkg](https://github.com/vitte-lang/vitte/releases/download/Vitte2026_02/vitte-uninstall-2.1.1.pkg)
- **Inclus** : binaires `vitte`, `vittec`, `vitte-linker`, pages de manuel, complétions shell, runtime, et assets des éditeurs.
- **Complétions** : `completions-gen`, `completions-check`, `completions-snapshots`, `completions-snapshots-update`, `completions-lint`, `ci-completions`.
- **Publication** : `pkg-macos`, `pkg-macos-uninstall`, `release-check`, avec exclusion git de `.pkgstage/` et `pkgout/`.
- **Correctif** : le test `internal_module_denied` repasse sur le diagnostic attendu `E1016`.

## Chemin recommandé

- **Installer** : poser les outils et vérifier l’environnement.
- **Compiler** : compiler, lancer les tests, préparer la sortie.
- **Vérifier** : valider un exemple et lire les diagnostics.

## Installer

- `brew install vitte`
- `make build`
- `make install`

## Compiler

- `make build`
- `make test`
- `make core-language-gate`

## Vérifier

- `vitte check examples/first_project.vit`
- `vitte explain E0001`
- `make book-links-check`

## Raccourcis doc

- [Lire la documentation complète](doc.md)
- [Voir la migration Markdown](../MD_MIGRATION.md)
- [Lire les diagnostics](diagnostics.md)
- [Ouvrir le sommaire poche](../book/poche/SOMMAIRE.md)
