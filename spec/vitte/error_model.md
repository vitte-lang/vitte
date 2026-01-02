# TODO: error model

# Error Model — Vitte (compiler, runtime, tools)

Ce document définit le **modèle d’erreurs** de Vitte :
- erreurs de compilation (lex/parse/type/check)
- erreurs d’exécution (runtime/VM)
- erreurs outillage (muffin, vittec, fmt, lsp)

Objectifs :
- Erreurs **stables** : codes et classes cohérents.
- Messages **actionnables** : contexte, hints, spans.
- Déterminisme : ordre stable, mêmes diagnostics pour les mêmes entrées.
- Interop : mapping avec `rt_abi.md` (ABI runtime) et `pal_contract.md`.

Non-objectifs :
- Définir la totalité des messages texte (localisation future).

---

## 1) Terminologie

- **Diagnostic** : un message utilisateur (erreur/warn/note) avec span.
- **Error code** : ident stable (ex: `E0001`).
- **Severity** : `error | warning | note | help`.
- **Span** : intervalle dans un fichier source.
- **Primary span** : span principal (cause).
- **Secondary span** : spans secondaires (contexte).

---

## 2) Principes généraux

### 2.1 Zéro exception

- En C : pas d’exception ; erreurs via `Result`/codes.
- En Vitte (langage) : erreurs runtime via mécanisme défini par VM (trap/panic/Result), mais le modèle de diagnostics reste cohérent.

### 2.2 Codes stables

- Chaque diagnostic “utilisateur” a un **code stable**.
- Les codes ne changent pas de signification.
- Les codes peuvent être dépréciés mais doivent rester reconnus.

### 2.3 Hiérarchie

- Une erreur interne (`ICE`) est distincte des erreurs utilisateur.
- Les erreurs système (OS) sont encapsulées (PAL/runtime) et mappées sur un code stable.

### 2.4 Déterminisme

- Tri stable des diagnostics : `(file, start_byte, severity, code)`.
- Un même input doit produire les mêmes codes et ordres (à options égales).

---

## 3) Structure d’un diagnostic

### 3.1 Format logique

Un diagnostic est un objet structuré :

- `code` : string (ex: `E0301`, `W0201`)
- `severity` : enum
- `message` : string courte
- `primary_span` : Span
- `secondary_spans[]` : Span + label
- `notes[]` : strings
- `help[]` : strings
- `causes[]` : (optionnel) chaînage interne

### 3.2 Span

Span canonical :
- `file_id` (ou path)
- `start_byte` (offset bytes UTF-8)
- `end_byte` (offset bytes UTF-8)
- `line/column` (dérivable, optionnel)

Règle :
- offsets **byte** en UTF-8 (pas en “char count”).

### 3.3 Labels

- Chaque secondary span peut avoir un label : `"defined here"`, `"expected type"`, etc.

---

## 4) Classification : codes et namespaces

Convention recommandée :

- `E` : erreurs (build échoue)
- `W` : warnings (build continue)
- `N` : notes (non bloquant)
- `H` : help (suggestions)
- `ICE` : internal compiler error (toujours fatal)

### 4.1 Plages (suggestion)

- `E0xxx` : lexer/parser
- `E1xxx` : résolution modules/imports
- `E2xxx` : types, inference, generics
- `E3xxx` : borrow/ownership (si applicable)
- `E4xxx` : const-eval / macros
- `E5xxx` : codegen/ABI
- `E6xxx` : runtime loader/link
- `E7xxx` : VM/traps
- `E8xxx` : muffin/packaging
- `E9xxx` : outils (fmt, lsp)

Même logique pour `Wxxxx`.

### 4.2 Exemple de codes

- `E0001` : caractère invalide
- `E0103` : import introuvable
- `E2007` : mismatch de type
- `E5002` : symbole ABI manquant
- `E7001` : division par zéro (trap)
- `E8003` : lock conflict
- `ICE0001` : invariant interne violé

---

## 5) Erreurs compilation (front-end)

### 5.1 Lexer

Caractéristiques :
- produire des erreurs localisées (span exact)
- tenter de continuer (récup) pour montrer plus d’erreurs

Erreurs typiques :
- caractères non reconnus
- string non terminée
- escape invalide
- nombre invalide / overflow lexical

### 5.2 Parser

- erreurs de token attendu
- récupération via synchronisation (ex: fin de statement / `.end`)
- limiter les cascades : si erreur majeure, éviter 1000 erreurs secondaires

### 5.3 Résolution modules/imports

- module inexistant
- import ambigu
- export absent
- cycle d’imports (si interdit)

### 5.4 Typage

- mismatch type
- symbol non défini
- generic arity
- pattern non exhaustif (warning ou error selon policy)

### 5.5 Suggestions

Les diagnostics doivent inclure des **hints** si possible :
- “voulais-tu dire … ?” (typos)
- “ajoute `.end`”
- “import `std/io`”

---

## 6) Erreurs runtime (VM)

### 6.1 Deux catégories

- **Traps** (déterministes, liés au programme) : ex division par zéro, out-of-bounds.
- **Faults** (environnement/OS) : ex fichier introuvable, permission.

### 6.2 Représentation runtime

Le runtime expose un code stable (aligné sur `rt_abi.md`) :
- `VITTE_E*` (ex: `VITTE_ENOENT`, `VITTE_EACCES`, `VITTE_EIO`, etc.)

Pour les traps VM :
- `VITTE_TRAP_*` (namespace séparé) ou mapping sur `E7xxx` côté diagnostics.

### 6.3 Stack traces

Recommandation :
- une erreur runtime doit pouvoir fournir une trace : frames + (file,line) si debug info.
- la trace est optionnelle mais le format doit être stable.

---

## 7) Erreurs outillage (muffin, vittec)

### 7.1 Muffin

Erreurs typiques :
- `E8001` parse MUF
- `E8002` include cycle
- `E8003` lock conflict
- `E8004` checksum mismatch
- `E8005` target inconnu

### 7.2 CLI tools

- erreurs d’arguments : code stable (ex `E9001`)
- erreurs I/O : mappées sur `VITTE_E*` puis surfacées en diagnostic

Règle :
- stdout = sorties normales
- stderr = diagnostics
- code de retour process :
  - `0` succès
  - `1` erreur utilisateur (diagnostics)
  - `2` erreur interne (ICE)

---

## 8) ICE (Internal errors)

Un ICE :
- a un code `ICE####`
- inclut un message pour rapport bug
- inclut une “trace interne” (si dispo)
- indique la version du compilateur + target

Règles :
- Un ICE ne doit pas être déclenché par un input valide.
- Un ICE doit être fatal.

---

## 9) Mapping : OS ↔ PAL ↔ Runtime ↔ Diagnostics

### 9.1 OS → PAL

- POSIX : errno → `VITTE_PAL_E*`
- Windows : GetLastError/WSAGetLastError → `VITTE_PAL_E*`

### 9.2 PAL → Runtime

- PAL renvoie `VITTE_PAL_E*`.
- Runtime mappe sur `VITTE_E*` (mêmes classes, souscode conservé en debug).

### 9.3 Runtime → User diagnostics

- Erreurs système : message “fichier introuvable” + code stable + path.
- Traps : message “index out of bounds” + span (si debug info) + code stable.

---

## 10) Format de rendu (CLI)

Le rendu CLI doit supporter :
- mode “human” (avec couleurs si TTY)
- mode “json” (machine readable)

### 10.1 Human

Exemple :

- `error[E2007]: type mismatch`
- `--> src/main.vitte:12:5`
- snippet + caret
- notes / help

### 10.2 JSON

Recommandation : format “one diagnostic per line” (JSONL) :

```json
{"code":"E2007","severity":"error","message":"type mismatch","spans":[{"file":"src/main.vitte","start":123,"end":130,"label":"expected i32"}]}
```

---

## 11) Politiques (warnings-as-errors)

Support attendu :
- `--deny warnings` (tout warning devient error)
- `--warn <code>` / `--deny <code>` / `--allow <code>`

Règles :
- la policy ne change pas le **code** du diagnostic
- elle change la **severity**

---

## 12) Checklist implémentation

- Codes stables + table centrale.
- Tri stable.
- Span en bytes UTF-8.
- Recovery lexer/parser.
- JSON output stable.
- Mapping PAL/runtime cohérent.
