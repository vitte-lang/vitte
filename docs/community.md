# Communauté Vitte

Questions, patchs, signaux techniques, et retours rapides sur le projet.

## Navigation

- [Accueil](index.md)
- [Documentation](doc.md)
- [Téléchargement](download.md)
- [Source](source.md)
- [Communauté](community.md)
- [Actualités](news.md)
- [Diagnostics](diagnostics.md)
- [Suggestions](suggestions.md)

## Tri rapide

- **Tendance**
- **Nouveaux**
- **Aide**
- **Dév**
- **Signaux**

## Messages utiles

- [Épinglé] Commencer par le bon fil
  - Aide pour les blocages, contribution pour les patches, diagnostics pour les erreurs, source pour l'architecture.
  - [Ouvrir les diagnostics](diagnostics.md) · [Voir la source](source.md)
- [@_admin] Avancée globale du projet
  - Le livre est passé sous `docs/book`, la documentation est mieux reliée aux chapitres essentiels, et les diagnostics sont plus lisibles.
  - [Relire la documentation](doc.md) · [Voir la publication 2.1.1](download.md) · [Lire la source](source.md)
- [@_admin] Stdlib étendue: fil de référence
  - La stdlib couvre maintenant un ensemble large de familles: `core`, `kernel`, `collections`, `compression`, `crypto`, `datetime`, `encoding`, `io`, `json`, `math`, `path`, `regex`, `strings`, et `sysinfo`.
  - Le détail utile reste groupé par sous-modules: `kernel` pour `device`, `fileio`, `memory`, `network`, `process`, `signals`, `sync`, `threads`, `users`; `core` pour `algorithms`, `memory`, `strings`, `types`, `utils`; `io` pour `file`, `stdio`; `collections` pour `vector`, `stack`, `queue`, `linkedlist`, `hashmap`, `hashset`; `math` pour les familles d’arithmétique, géométrie, matrices, statistiques, et trigonométrie.
  - Le bon réflexe: un seul fil de référence pour la vue d’ensemble, puis des liens vers la source et le contrat quand un sous-module mérite un traitement dédié.
  - [Post précédent: arbre source stdlib](https://github.com/vitte-lang/vitte/tree/main/src/vitte/stdlib)
  - [Lire la stdlib](../book/chapters/17-stdlib.md) · [Voir le contrat](../book/compiler-stdlib-contract.md) · [Lire la source](source.md)
- [@_admin] La CLI, expliquée simplement
  - `vitte explain` pour comprendre un diagnostic, `vitte check` pour valider un morceau de code, et les cibles Make pour aller plus loin.
  - [Voir les diagnostics](diagnostics.md) · [Aller au téléchargement](download.md) · [Lire la documentation](doc.md)
- [@_vincent.rs] Stdlib, source et contrat de lecture
  - La stdlib est la surface la plus touchée quand on écrit du concret ou qu’on relie un projet à ses imports.
  - [Relire la stdlib](../book/chapters/17-stdlib.md) · [Voir le contrat](../book/compiler-stdlib-contract.md) · [Retour à la documentation](doc.md)
- [@_roussov] stdlib/kernel: syntaxe nouvelle, exemple complet
  - Exemple court pour relire la nouvelle syntaxe sans chercher le contexte ailleurs.
  - [Relire la stdlib](../book/chapters/17-stdlib.md) · [Voir le contrat](../book/compiler-stdlib-contract.md) · [Lire la source](source.md)
- [@_roussov] kernel: cas d'erreur et diagnostic attendu
  - L’import doit remonter un message qui dit quoi corriger, pas seulement que quelque chose a cassé.
  - [Voir les codes repères](diagnostics.md) · [Relire la stdlib](../book/chapters/17-stdlib.md) · [Retour à la publication](download.md)

## Tendance

- [Version 2.1.1](download.md)
- [Documentation](doc.md)
- [Codes repères](diagnostics.md)

## Nouveaux

- [Aide](../SUPPORT.md)
- [Contribution](../CONTRIBUTING.md)
- [Suggestions](suggestions.md)

## Aide

- [Consulter SUPPORT.md](../SUPPORT.md)
- [Lire les diagnostics](diagnostics.md)
- [Suivre les annonces](news.md)

## Dév

- [Lire CONTRIBUTING.md](../CONTRIBUTING.md)
- [Source](source.md)
- [Lire la documentation](doc.md)

## Flairs

- **Question** : blocage, erreur, aide
- **Patch** : modif courte, ciblée
- **Signal** : doc, source, ou release

## Contributeurs

- [GoGinx](https://github.com/GoGinx)
- [roussov](https://github.com/roussov)
- [Kapra-foster](https://github.com/Kapra-foster)

## Règles du fil

- Un post, un sujet.
- Un exemple minimal vaut mieux qu'un long bloc flou.
- Quand c'est une erreur, commencer par `vitte explain`.
