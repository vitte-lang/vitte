# Compilateur Vitte

[![Vitte CI](https://github.com/vitte-lang/vitte/actions/workflows/ci.yml/badge.svg)](https://github.com/vitte-lang/vitte/actions/workflows/ci.yml)

Ce dépôt contient le compilateur officiel de Vitte, écrit en C17. Son pipeline
est conservé en place :

```text
source -> lexer -> parser/AST -> semantic analysis -> HIR -> IR -> C17
```

## Build

Prérequis : un compilateur C17 et `make`.

```sh
make
./build/bin/vitte --version
```

Le binaire expose `check`, `compile`, `run`, `lex` et `parse`. `check` valide
actuellement l'UTF-8, les tokens et la syntaxe. L'analyse sémantique est
disponible dans le driver, notamment avec `compile --stop-after-sema`.
`compile` et `run` complets restent bloqués par l'absence d'abaissement HIR/IR
et de génération C17; ils échouent explicitement au lieu d'annoncer une
compilation ou une exécution réussie.

```sh
./build/bin/vitte check examples/hello.vit
./build/bin/vitte compile examples/hello.vit
```

Lancez la suite de régression avec `make test`. CMake est également pris en
charge avec `cmake -S . -B build && cmake --build build`.

La couverture actuelle du langage et ses limites connues sont répertoriées dans
`src/docs/language_coverage.md`.
