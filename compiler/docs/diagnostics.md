# Diagnostics (vittec)

Objectif : un modèle d’erreurs **stable**, **déterministe** et **facile à exploiter** (humain + JSON), avec des **spans précis** et un affichage console lisible.

Ce document définit :
- le **modèle de diagnostic** (structures de données)
- les **règles de construction** (contrats)
- les **formats d’émission** : human (ANSI) et json
- les **codes d’erreur** et conventions

---

## Principes

- Chaque diagnostic a :
  - une **sévérité** (`note|warning|error`)
  - un **code** (ex: `E0001`)
  - un **message** court
  - au moins un **span principal**
- Zéro exception : tout remonte par valeurs (Result/err struct).
- L’affichage doit être **reproductible** (ordre stable, pas de hash random).

---

## Types (contrat)

### FileId
Identifiant de fichier interne (index dans `SourceMap`).

- `FileId` est un entier (u32 ou i32) stable pendant une compilation.

### Span
Un span localise une zone dans un fichier.

Champs (minimum) :
- `file: FileId`
- `lo: u32` (offset bytes, inclus)
- `hi: u32` (offset bytes, exclus)

Contraintes :
- `lo <= hi`
- `hi` ne dépasse pas la longueur du buffer

### Label
Un label décrit une zone et son rôle.

Champs :
- `span: Span`
- `style: primary|secondary`
- `message: string?` (optionnel)

Règles :
- un diagnostic doit avoir **exactement 1 label primary**
- labels secondaires optionnels (liens, hints sur une autre zone)

### Diagnostic
Champs :
- `severity: note|warning|error`
- `code: string` (ex: `E0001`, `W0100`)
- `message: string`
- `labels: Label[]` (>=1, contient 1 primary)
- `notes: string[]` (optionnel)
- `help: string?` (optionnel, 1 phrase actionnable)

### DiagnosticBag
Une compilation accumule une liste :
- `diag[]` (ordre de production)
- helpers : `push_error`, `push_warning`, `has_errors`

---

## SourceMap (résolution ligne/col)

Le but : convertir `Span(lo, hi)` en :
- chemin du fichier
- `(line, col)` pour `lo`
- extraits de code (lignes concernées)

Règles :
- le découpage en lignes se fait à partir du buffer brut
- compatibilité : `\n` et `\r\n`
- `col` = colonne en caractères affichables (au début, approximation bytes OK, puis UTF-8 aware)

---

## Émission Human (console)

Format inspiré Rust/Clang (simple, stable) :

```
error[E0001]: message court
  --> path/to/file.vitte:12:5
   |
12 |   let x =
   |       ^ expected expression
   |
   = help: add a literal or call a function
   = note: this parser is running in bootstrap mode
```

### Détails d’affichage

- Ligne 1 : `severity[code]: message`
- Ligne 2 : `--> file:line:col` (position = début du primary span)
- Bloc code :
  - montre 1 à 3 lignes autour
  - caret `^` sur `lo` (si `hi==lo`, caret simple)
  - underline `^^^^` sur `[lo, hi)` si possible
- S’il y a des labels secondaires : afficher des carets supplémentaires sur les lignes concernées.

### ANSI
- `error` en rouge, `warning` en jaune, `note` en bleu/gris
- Si `NO_COLOR` est défini, ou si stdout n’est pas un TTY : **désactiver ANSI**

---

## Émission JSON

But : consommable par outils (CI, IDE, LSP).

### Schéma JSON (stable)

```json
{
  "severity": "error",
  "code": "E0001",
  "message": "expected expression",
  "primary": {
    "file": "path/to/file.vitte",
    "lo": 123,
    "hi": 124,
    "line": 12,
    "col": 5
  },
  "labels": [
    {
      "style": "primary",
      "file": "path/to/file.vitte",
      "lo": 123,
      "hi": 124,
      "line": 12,
      "col": 5,
      "message": "expected expression"
    }
  ],
  "help": "add a literal or call a function",
  "notes": ["bootstrap parser: body skipped until .end"]
}
```

Règles :
- Toujours inclure `severity`, `code`, `message`, `labels`
- `file` doit être un chemin normalisé (relatif au workspace si possible)
- `line/col` sont 1-based

---

## Conventions de codes

### Erreurs (`E`)
- `E0001`… : erreurs de lexing/parsing bas niveau
- `E01xx`… : erreurs import/export/module
- `E02xx`… : erreurs de types (plus tard)

### Warnings (`W`)
- `W0100`… : unused imports / unused vars (plus tard)
- `W01xx`… : style / compat (plus tard)

Notes :
- codes fixes : ne pas recycler un code pour une autre signification
- documenter les nouveaux codes au fur et à mesure

---

## Règles de production (compilo)

### Lexer
- sur caractère illégal : `E0001` + span sur 1 byte
- sur string non terminée : `E0002` + span depuis `"` jusqu’à EOF/ligne

### Parser bootstrap
- si top-level inattendu : `E0010` + span sur token
- si `.end` manquant : `E0011` + span sur début de bloc (et note “expected .end”)

### Sema (plus tard)
- double définition : `E0100` + primary sur 2e définition, secondary sur 1ère

---

## Tri / ordre (déterminisme)

- Ordre de base : ordre d’apparition dans le flux source.
- À émission : trier par `(file, lo, severity)` si nécessaire, mais garder stable.

Recommandation :
- conserver un `seq` interne (monotonic) pour stabiliser encore plus.

---

## API minimale (C) — suggestion

Sans figer l’implémentation, une API typique :

- `diag_error(code, span, msg)`
- `diag_add_label(diag, span, style, msg?)`
- `diag_add_note(diag, note)`
- `diag_emit_human(diags, sourcemap, use_color)`
- `diag_emit_json(diags, sourcemap, out)`

---

## Checklist qualité

- [ ] Chaque `error` a un span primary valide
- [ ] Les offsets `lo/hi` ne sortent jamais du buffer
- [ ] Le format human n’a pas de dépendance au terminal (si no-color)
- [ ] Le JSON est stable (champs fixes, ordering stable)
- [ ] Tests snapshots sur diagnostics (golden)

---

## Exemples

### Exemple 1 — token invalide
- `E0001`: `unexpected character '@'`
- primary span : sur `@`

### Exemple 2 — `.end` manquant
- `E0011`: `missing .end to close block`
- primary : sur le début du `fn`
- note : `block started here`
- help : `add .end at the end of the function`
