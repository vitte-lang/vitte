# Compilateur Vitte

[![Vitte CI](https://github.com/vitte-lang/vitte/actions/workflows/ci.yml/badge.svg)](https://github.com/vitte-lang/vitte/actions/workflows/ci.yml)

Ce dépôt contient un compilateur en transition vers un pilote natif en Vitte.
Le pipeline Vitte couvre le lexer, le parser, l’analyse sémantique et l’émission
C17 directe depuis l’AST pour `compile` et `run` :

```text
source -> CLI hôte C -> pipeline Vitte: lexer/parser/sema -> AST vers C17
       -> compilateur C hôte, édition de liens et exécution
```

Le pipeline actif ne passe pas par HIR/IR natifs et le backend C17 IR séparé
n’est pas branché. `compile` et `run` s’arrêtent sur les échecs du frontend
Vitte au lieu de relancer le chemin bootstrap. `check`, `--emit-ir`, les outils
d’inspection, le rendu des diagnostics et l’orchestration du compilateur C
hôte utilisent encore des étapes C. Le bootstrap C compile aussi les modules
Vitte au stage0; le compilateur n’a pas encore passé un cycle autonome de
reconstruction puis d’exécution.

## Build

Prérequis : un compilateur C17 et `make`.

```sh
make
vitte --version
```

La version du compilateur est `0.1.1`. Les diagnostics proposent une aide
contextuelle ; `vitte explain E0501` détaille un code d’erreur.

Le binaire expose `compile` (commande par défaut), `check`, `run`, `lex`,
`parse`, `explain` et `completion`. `check` effectue l’analyse sémantique et
s’arrête avant la génération de code. `compile` et `run` utilisent le backend
AST vers C17 pour le sous-ensemble pris en charge; ils requièrent un compilateur
C hôte.

La configuration suit la priorité `défauts < fichier de projet < environnement
< CLI`. Les détails de chaque commande, option et conflit sont générés dans
[`src/bootstrap/cli/README.md`](src/bootstrap/cli/README.md) depuis la table
commune du bootstrap.

```sh
vitte check examples/hello.vit
vitte compile examples/hello.vit
vitte explain E0501
vitte completion bash > vitte.bash
```

Lancez la suite de régression avec `make test`. CMake est également pris en
charge avec `cmake -S . -B build && cmake --build build`.

La couverture actuelle du langage, du compilateur et de la CLI ainsi que leurs
limites connues sont répertoriées dans
[`src/docs/language_coverage.md`](src/docs/language_coverage.md). Les publics
visés et la politique de langue des documents sont définis dans
[`src/docs/documentation_policy.md`](src/docs/documentation_policy.md).
