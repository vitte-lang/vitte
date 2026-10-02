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
./target/vitte/vitte --version
```

Le binaire produit prend en charge `check`, `emit-c`, `build` et `run`.

```sh
./target/vitte/vitte check examples/hello.vit
./target/vitte/vitte run examples/hello.vit
```

Lancez la suite de régression complète avec `make test`. CMake est également
pris en charge avec `cmake -S . -B build && cmake --build build`.

La couverture actuelle du langage et ses limites connues sont répertoriées dans
`src/docs/language_coverage.md`.
