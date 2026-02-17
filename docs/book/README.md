# Apprendre Vitte

Ce livre est organisé en trois couches complémentaires. Les chapitres portent la progression conceptuelle complète. La section logique isole les mécanismes d'exécution et les traces de code. La section mots-clés documente chaque élément lexical du langage avec exemples courts.

Point d'entrée principal: `docs/book/SUMMARY.md`.

Point d'entrée technique rapide: `docs/book/INDEX-technique.md`.

Guide de style éditorial: `docs/book/STYLE.md`.

Si l'objectif est l'apprentissage linéaire, suivre les chapitres `01` a `30`. Si l'objectif est de résoudre un problème technique local, passer par l'index puis naviguer vers la page logique ou mot-clé correspondante.

## Où apprendre quoi

- Démarrage et bases: `docs/book/chapters/00-avant-propos.md`, `docs/book/chapters/01-demarrer.md`, `docs/book/chapters/04-syntaxe.md`.
- Types et données: `docs/book/chapters/05-types.md`, `docs/book/chapters/08-structures.md`, `docs/book/chapters/11-collections.md`.
- Contrôle et logique: `docs/book/chapters/06-procedures.md`, `docs/book/chapters/07-controle.md`, `docs/book/logique/README.md`.
- Modules et architecture: `docs/book/chapters/09-modules.md`, `docs/book/chapters/15-pipeline.md`, `docs/book/chapters/20a-architecture-globale.md`.
- Qualité et exécution: `docs/book/chapters/10-diagnostics.md`, `docs/book/chapters/18-tests.md`, `docs/book/chapters/20-repro.md`.
- Projets complets: `docs/book/chapters/21-projet-cli.md` à `docs/book/chapters/26-projet-editor.md`.

## Charte éditoriale

- Ton: précis, pédagogique, orienté exécution concrète.
- Niveau de détail: expliciter l'intention, la garde, la sortie et l'effet d'état.
- Trame obligatoire: `Objectif -> Exemple -> Pourquoi -> Test mental -> À faire -> Corrigé minimal`.
- Longueur cible: voir les seuils détaillés dans `docs/book/STYLE.md`.
- Règle anti-répétition: ne pas redéfinir les bases (`int`, `proc`, etc.) hors ouverture et glossaire.
