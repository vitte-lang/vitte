# Source Vitte

Organisation du dépôt, flux de contribution, et points d’entrée pour les développeurs.

## Navigation

- [Accueil](index.md)
- [Documentation](doc.md)
- [Téléchargement](download.md)
- [Source](source.md)
- [Communauté](community.md)
- [Actualités](news.md)
- [Diagnostics](diagnostics.md)
- [Suggestions](suggestions.md)

## Comprendre le compilateur

Le compilateur Vitte se lit comme une suite de couches simples: parser, résolution,
IR, puis backend.

- **Analyseur** : forme le code et construit l’AST.
- **Résolution** : relie noms, imports et symboles.
- **IR** : abaisse vers une représentation intermédiaire.
- **Backend** : produit le code cible.

## Schémas

- Flux principal: `source -> parser -> AST -> resolve -> IR -> backend -> sortie`
- Lecture des erreurs: `diagnostic -> couche -> cause -> correction minimale -> rerun`

## Références utiles

- [Pipeline compilateur](../book/chapters/15-pipeline.md)
- [Architecture globale](../book/chapters/20a-architecture-globale.md)
- [Mapping AST -> IR](../book/chapters/64-mapping-ast-vers-ir.md)
- [Diagrammes de grammaire](../book/chapters/61-generation-diagrammes-grammaire.md)
- [Grammaire du langage](../book/chapters/27-grammaire.md)
- [Railroad SVG](../book/grammar/railroad/README.md)

## Organisation

- Compilateur
- Exécution et stdlib
- Documentation et pages publiques

## Contribuer

- [Lire CONTRIBUTING.md](../CONTRIBUTING.md)
- [Lire la documentation](doc.md)
- Préparer un patch court, puis relancer la validation

## Références

- [Discuter](community.md)
- [Déboguer](diagnostics.md)
- [Suivre les changements](news.md)
- [Lire SUPPORT.md](../SUPPORT.md)
