# Pipeline (vittec)

Ce document décrit le **pipeline de compilation** de Vitte, dans sa version **bootstrap-friendly**.

Objectif : une chaîne **simple**, **déterministe**, **audit-able**, qui peut compiler un sous-ensemble utile dès maintenant, puis évoluer par phases.

---

## Vue d’ensemble

```
.vitte source
  ↓
read (SourceMap)
  ↓
lex (TokenStream)
  ↓
parse (bootstrap: top-level index + skip .end)
  ↓
(sema optionnelle)
  ↓
emit_c (C portable)
  ↓
cc (clang/gcc/msvc) → binaire / lib
```

En mode bootstrap, l’étape `parse` peut être volontairement “partielle” :
- on indexe les items top-level (`module`, `import`, `export`, `fn`, …)
- on peut ignorer le corps des fonctions en sautant jusqu’à `.end`

---

## 1) Read — ingestion des sources

### Entrées
- fichier(s) `.vitte`
- (optionnel) manifest Muffin `.muf` qui décrit le graphe de modules

### Sortie
- `SourceMap` :
  - `FileId` pour chaque fichier
  - buffer source (bytes)
  - index de lignes pour convertir `(offset → line/col)`

### Contrats
- chemins normalisés (relatifs workspace si possible)
- encodage : UTF-8 recommandé (mais le lexer doit pouvoir diagnostiquer le non-UTF8 si nécessaire)

---

## 2) Lex — lexer / tokenisation

### Entrée
- buffer source + `FileId`

### Sortie
- `TokenStream` : liste de tokens + spans
- diagnostics de lexing (ex: char invalide, string non terminée)

### Token minimal
Chaque token contient :
- `kind` (ident, keyword, punct, literal, newline, …)
- `span { file, lo, hi }`
- payload optionnel (ident/literal)

### Règles importantes
- le lexer ne “devine” pas la grammaire : il produit des unités stables
- tous les tokens doivent être **spanés** (même les erreurs)

---

## 3) Parse — parsing bootstrap (index top-level)

### Pourquoi un parser “stub” au début
- bootstrap rapide (tu obtiens vite un binaire utilisable)
- diagnostics propres sur le top-level
- génération C déterministe sans implémenter toute la grammaire

### Entrée
- `TokenStream`

### Sortie (bootstrap)
Un **TopLevelIndex** (structure légère) :
- `module_name` (si trouvé)
- `imports[]` (représentation légère)
- `exports[]`
- `fns[]` : nom + span signature + (optionnel) slice tokens du corps

### Stratégie : scan top-level
Pseudo approche :
1) itérer tokens
2) détecter keywords au niveau 0
3) sur `fn`, capturer la signature, puis :
   - soit parser le bloc minimal
   - soit **skip** jusqu’au `.end` (délimitation canonique)

Règle : si `.end` est absent → diagnostic `E0011`.

---

## 4) Sema — validations (optionnel / progressif)

En bootstrap, `sema` peut être :
- désactivée
- ou limitée à des checks “safe” non-complexes

Checks typiques (phase 1) :
- unicité des noms top-level
- cohérence import/export (structure, pas résolution complète)
- duplication d’export

Sortie :
- index enrichi (ex: table symboles minimale)
- diagnostics supplémentaires

---

## 5) Emit C — génération C portable

### Entrée
- index top-level (bootstrap) ou AST (plus tard)

### Sortie
- `.c` + `.h` générés dans un répertoire `build/` ou `out/`

### Mode bootstrap (stubs)
Si le corps n’est pas compilé :
- générer des prototypes
- générer des stubs (ex: `TODO` + return par défaut)

Si le corps est compilé (phase 1+) :
- traduire un sous-ensemble de statements/expressions

### Contraintes
- sortie C lisible, stable
- ordre déterministe (tri stable si nécessaire)
- mapping de noms (mangling) défini et documenté

---

## 6) CC — compilation du C

### But
Externaliser l’étape machine code vers un toolchain C :
- clang/gcc (Linux/macOS)
- MSVC/clang-cl (Windows)

### Contrats
- flags contrôlés (reproductibilité)
- logs toolchain capturés et reformatés si besoin

---

## Diagnostics dans le pipeline

Règle : chaque étape peut produire des diagnostics.

Ordre recommandé :
- accumuler diagnostics
- arrêter si `has_errors == true`
- émettre en human (par défaut) et JSON si demandé

Liens : voir `docs/diagnostics.md`.

---

## Modes d’exécution (pratiques)

### `check`
- read + lex + parse (+ sema) sans codegen
- but : diagnostics rapides

### `build`
- pipeline complet jusqu’à binaire

### `emit-c`
- stop après `emit_c` (debug)

---

## Déterminisme et reproductibilité

- mêmes inputs → mêmes outputs (tokens, C, ordering)
- éviter toute source de hasard observable
- normaliser les chemins et les fins de ligne

---

## Tests recommandés par étape

- Read: tests paths + sourcemap line/col
- Lex: golden tests tokens + spans
- Parse bootstrap: golden tests top-level index + `.end` manquant
- Emit C: golden tests sur `.c` + compilation/exec

---

## Évolution (phases)

### Phase 0 (actuelle)
- lexer solide
- parse top-level + skip `.end`
- emit C stubs

### Phase 1
- AST statements/expr minimal
- back C pour `if/while/return/let`

### Phase 2
- sema plus complète (résolution noms, imports)

### Phase 3
- IR (optionnelle) + multi backends (C / VM)

---

## Checklist “pipeline ok”

- [ ] `benchc --all` passe (si bench intégré)
- [ ] `vittec check file.vitte` produit des spans corrects
- [ ] `.end` manquant → `E0011` avec help actionnable
- [ ] `emit-c` stable d’un run à l’autre
- [ ] compilation C externe robuste (messages capturés)
