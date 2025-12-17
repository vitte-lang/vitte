# Project Status — Vitte

Document de statut **factuel** pour le repo `vitte` : où on en est, ce qui est stable, ce qui manque, et la prochaine marche.

> Statut global : **expérimental** (API / formats / sémantique susceptibles d’évoluer).

---

## TL;DR

- Le repo contient les bases **spec + grammaire + code** (selon layout actuel) et une infra **bench** en cours de stabilisation.
- Priorités court terme : **build reproductible + CI minimale + frontend (lexer/parser) stable**.
- La feuille de route opérationnelle est dans `TODO.md`.

---

## Périmètre du repo

Ce repo est le **noyau** du projet Vitte :

- **Langage** : syntaxe “core + phrase” (convention `.end` côté exemples)
- **Parser/Grammaire** : `grammar/` (ex: `vitte.pest`) + éventuellement `spec/`
- **Toolchain** : frontend (lexer/parser/AST), IR/passes, backend (selon milestone)
- **Runtime/Stdlib** : `runtime/`, `std/` (si présents)
- **Bench** : `bench/` + runner `run_benchmarks.sh`
- **Scripts** : `tools/scripts/` (si présents)

---

## Points d’entrée

- `README_START_HERE.md` : navigation “start here”
- `README.md` : vue d’ensemble + quickstart
- `TODO.md` : backlog priorisé (source de vérité opérationnelle)
- `grammar/` : grammaire parser
- `spec/` : documents de sémantique/ABI/modules (si présents)

---

## Ce qui est en place (actuel)

### Documentation
- Entrées repo : `README.md`, `README_START_HERE.md`
- Backlog structuré : `TODO.md`

### Bench
- Runner “reproductible” : `run_benchmarks.sh`
  - build optionnel CMake, warmup/repeat
  - métadonnées git/système (optionnel)
  - outputs normalisés (`dist/bench/<run_id>/...`)

### Build
- Base CMake supposée via `CMakeLists.txt` (si présent) avec out-of-tree build (`build/`).

---

## État technique (à stabiliser)

### Build / CI
- Objectif : **clean build** reproductible sur macOS arm64 + Linux x86_64.
- CI minimale : **build + tests + format** (même si tests partiels au début).

### Langage / Grammaire
- Source grammaire : `grammar/vitte.pest`.
- Objectif : synchroniser la doc `spec/...ebnf` avec la grammaire effective (si 2 sources coexistent).

### Frontend (Lexer / Parser)
- Objectif : AST stable + golden tests + récupération d’erreurs.

---

## Ce qui n’est PAS affirmé ici

Ce document **n’affirme pas** (sans preuve dans le repo) :

- “SDK complet”, “production-ready”, “ABI stable”, “30k lignes”, “265 exemples”, “validated/syntax-checked”, “14 targets”, etc.

Si ces éléments existent réellement, ils doivent être :
- présents dans l’arborescence,
- référencés précisément (chemins),
- vérifiables (scripts/tests/CI).

---

## Prochaine étape (milestone suggéré)

### Milestone M0 — « Repo usable »

Livrables :
- [ ] Build reproductible (clean build)
- [ ] CI minimale (build + tests existants)
- [ ] Commandes “happy path” documentées (README)
- [ ] Frontend : lexer/parser au moins testés sur un corpus minimal
- [ ] Bench : runner OK + un binaire de bench détectable + 3 micro-benches

---

## Commandes pratiques

Lister les exécutables après build :

```bash
find build -maxdepth 5 -type f -perm -111 | sort | head -n 200
```

Bench standard :

```bash
chmod +x ./run_benchmarks.sh
./run_benchmarks.sh --build --repeat 5 --warmup 1
```

---

## Liens internes

- Backlog : `TODO.md`
- Entry-point : `README_START_HERE.md`
- Quickstart : `README.md`

---

## Version

- Source de vérité : `VERSION` (si présent)

