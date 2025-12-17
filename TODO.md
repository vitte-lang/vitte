# TODO

1. [done] Task 1
2. [done] Task 2
3. [done] Task 3
4. [done] Task 4
5. [not-started] Task 5
6. [not-started] Task 6

# TODO

Ce fichier sert de backlog opérationnel (repo **vitte**). Format :
- `[done]` terminé
- `[in-progress]` en cours
- `[blocked]` bloqué / dépendance externe
- `[not-started]` à faire

---

## 0) Triage

- [not-started] Définir les objectifs du prochain jalon (milestone) et la date cible
- [not-started] Lister les tickets « bloquants » (build, CI, packaging) et prioriser
- [not-started] Normaliser les labels GitHub (bug, enhancement, rfc, build, docs, good-first-issue)

---

## 1) Build / Toolchain

- [not-started] Rendre le build reproductible (clean build) sur macOS arm64 + Linux x86_64
- [not-started] Ajouter une cible CI minimale : build + tests + format
- [not-started] Ajouter un script `tools/scripts/doctor.(sh|ps1)` qui vérifie toolchain, deps, versions
- [not-started] Gérer proprement les sorties (artifacts) : `dist/`, `out/`, `build/` (noms stables)

---

## 2) Langage / Grammaire

- [not-started] Synchroniser `spec/vitte/grammar_core_phrase.ebnf` avec la grammaire Pest `grammar/vitte.pest`
- [not-started] Définir la liste canonique des mots-clés (core + phrase) + tokens réservés
- [not-started] Définir la sémantique exacte des blocs `.end` (scopes, capture, ret implicite ou non)
- [not-started] Documenter le modèle d’erreurs (codes, messages, spans)

---

## 3) Frontend (Lexer / Parser)

- [not-started] Lexer : positions (ligne/col), spans, gestion des commentaires, strings, escapes
- [not-started] Parser : AST stable + tests de non-régression (golden files)
- [not-started] Parser : récupération d’erreurs (panic mode) sur tokens attendus

---

## 4) IR / Compilation

- [not-started] Définir une IR minimale (expressions, control-flow, fonctions) + sérialisation debug
- [not-started] Passes : name resolution (modules/use), type checking (subset), lowering vers IR
- [not-started] Génération : C backend minimal OU bytecode/VM minimal (choisir la voie du prochain milestone)

---

## 5) Runtime / VM

- [not-started] Définir l’ABI interne : calling convention, layout des types, slices/strings
- [not-started] Implémenter un runtime minimal : allocations, strings, panic/report
- [not-started] Ajouter un désassembleur / traceur (debug) pour le backend/VM

---

## 6) Stdlib (priorités)

- [not-started] `std/core` : types de base, conversions, comparaison
- [not-started] `std/io` : stdin/stdout minimal (si backend le permet)
- [not-started] `std/fs` : path + lecture fichier (si backend le permet)
- [not-started] `std/cli` : parsing argv (basique)

---

## 7) Bench / Perf

- [not-started] Stabiliser le framework `bench/` (registry + stats + time) + documentation d’usage
- [not-started] Ajouter 5 micro-benches standard (memcpy, hash, json parse, add, string ops)
- [not-started] Ajouter un export JSON/CSV des résultats + comparateur entre runs

---

## 8) Docs

- [not-started] `README.md` : quickstart (build, run, tests), architecture, roadmap
- [not-started] `spec/` : sémantique modules, erreurs, FFI (ABI), conventions
- [not-started] Ajouter un guide « contribution » (format, tests, conventions de commits)

---

## 9) Release / Distribution

- [not-started] Versioning (SemVer) + fichier `VERSION` source de vérité
- [not-started] Génération d’archives (tar.gz/zip) + checksums (sha256)
- [not-started] Homebrew formula / scripts de release (si applicable)

---

## Historique (existant)

1. [done] Task 1
2. [done] Task 2
3. [done] Task 3
4. [done] Task 4
5. [not-started] Task 5
6. [not-started] Task 6