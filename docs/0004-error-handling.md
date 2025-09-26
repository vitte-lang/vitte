---

rfc: 0004
title: "Error Handling"
status: proposed
authors: \["Vincent"]
shepherd: "Core Team"
reviewers: \["@typer", "@vm", "@lsp", "@stdlib"]
created: 2025-09-02
updated: 2025-09-02
tracking-issues: \["vitte-lang/vitte#4"]
requires: \["0001-core-syntax-and-keywords", "0003-memory-model-and-ownership"]
supersedes: \[]
superseded-by: \[]
discussions-to: "[https://github.com/vitte-lang/vitte/discussions](https://github.com/vitte-lang/vitte/discussions)"
target-release: "v0.1"
labels: \["lang", "errors", "result", "panic", "tooling"]
---------------------------------------------------------

> **Résumé** — Ce RFC spécifie le **modèle d’erreurs** de Vitte : résultats explicites `Result<T, E>`, option `Option<T>`, opérateur de **propagation** `?`, politique de **panic** configurable (abort/unwind), **traits d’erreur** standard, **contexte** et **backtrace** optionnels, intégration **VM/bytecode**, **FFI**, **async**, **LSP** et **formatter**.

---

## 1) Motivation

* **Sécurité** : pas d’exceptions implicites ni de crashs silencieux.
* **Lisibilité** : les échecs sont **visibles dans les types**.
* **Perf** : pas de coût caché (zéro *stack unwinding* en chemin nominal).
* **Interop** : FFI simple (codes/enum), mapping clair côté VM.

## 2) Objectifs / Non‑objectifs

**Objectifs**

* O1 — `Result<T, E>`, `Option<T>`, `?` sur `Result`/`Option`.
* O2 — `panic` : stratégies **abort**/**unwind** configurables.
* O3 — `std::error::Error` : affichage (`Display`), cause (`source()`), **backtrace** (optionnelle), **code**.
* O4 — Contexte & conversion : `From<E> for F`, `with_context`, `bail!`, `ensure!`.
* O5 — Outillage : `#[must_use]`, lints, diags LSP et **exit codes**.

**Non‑objectifs**

* N1 — Macros dérive (`derive(Error)`) → RFC 0008 (macros).
* N2 — *Try blocks* génériques → **futur** v0.2.

---

## 3) Types de base

```vitte
enum Result<T, E> { Ok(T), Err(E) }
enum Option<T> { Some(T), None }
```

* `Result<T, E>` : porteur de succès/échec.
* `Option<T>` : présence/absence sans message d’erreur.
* Les deux sont **`#[must_use]`** (lint si ignorés).

### 3.1 Trait `Error`

Interface standard pour les erreurs **dynamiques** (std‑only), dégradée en `core::error::ErrorCore` en `no_std`.

```vitte
trait Display { fn fmt(&self) -> String }
trait Error /*: Display*/ {
  fn source(&self) -> Option<&Error> { None }
  fn code(&self) -> Option<&'static str> { None } // ex: "EIO", "EINVAL"
  fn backtrace(&self) -> Option<&Backtrace> { None } // std‑only
}
```

> `Display` est la représentation humaine; `code()` fournit un identifiant stable machine‑lisible.

### 3.2 Erreurs standard (survol)

* `IoError { kind: IoErrorKind, path?: String }`
* `ParseError { at: Span, msg: String }`
* `Utf8Error`, `Timeout`, `Canceled`, `Unsupported`

---

## 4) Propagation avec `?`

### 4.1 Sémantique

`expr?` sur un `Result<T, E>` équivaut à :

```vitte
match expr { Ok(v) => v, Err(e) => return Err(From::from(e)) }
```

* Sur `Option<T>` : `None` → `return None`.
* **Chaining** : `From` permet de **remonter** des erreurs hétérogènes.

### 4.2 Grammaire (complément à RFC 0001)

```ebnf
postfix ::= primary { call | index | field | try_qm } ;
try_qm  ::= '?' ;
```

---

## 5) `panic` — philosophie & stratégies

* Réservé aux **invariants violés** (bugs, corruption), **pas** au contrôle nominal.
* Deux stratégies compilateur :

  * **`panic = abort`** : arrêt immédiat (par défaut en `no_std`, `release`, embarqué).
  * **`panic = unwind`** : déroulage de pile avec exécution des destructeurs **RAII** (par défaut en `std`/debug desktop).
* Sélection par **manifest** (ex.) :

```toml
[package]
panic = "abort"   # ou "unwind"
```

> En `unwind`, la VM fournit des **tables d’unwind**; en `abort`, `panic` émet un **trap**.

### 5.1 API

```vitte
fn panic(msg: &str) -> !
fn panicf(fmt: &str, args: ...) -> !  // formaté
```

---

## 6) Contexte, conversions et helpers

### 6.1 Contexte

Ajoute une couche d’info sans perdre la cause :

```vitte
trait ResultExt<T, E> {
  fn context(self, msg: &str) -> Result<T, Box<dyn Error>>
  fn with_context<F: FnOnce()->String>(self, f: F) -> Result<T, Box<dyn Error>>
}
```

### 6.2 Conversions

```vitte
trait From<Src> { fn from(Src) -> Self }
impl From<IoError> for AppError { ... }
```

### 6.3 Raccourcis

```vitte
macro ensure(cond, msg) { if !cond { return Err(error(msg)) } }
macro bail(msg) { return Err(error(msg)) }
```

> `error(msg)` construit une erreur simple `Box<dyn Error>` (std‑only). En `no_std`, privilégier des **enums**.

---

## 7) Backtrace

* Capturée **à la construction** de l’erreur si :

  * build `std` **et** variable d’environnement `VITTE_BACKTRACE=1` **ou** flag `--backtrace` de la runtime.
* Dans `no_std` : non disponible.
* Affichage multi‑lignes formaté côté CLI.

---

## 8) `main` qui retourne un `Result`

```vitte
fn main() -> Result<(), Box<dyn Error>> {
  run()?; Ok(())
}
```

* La runtime **imprime** l’erreur (`Display`, éventuellement `source()` chain + backtrace) et **mappe** en **exit code**.
* Mapping par défaut : `Ok`→`0`, `IoErrorKind::NotFound`→`2`, `Permission`→`13`, `Timeout`→`124`, sinon `1`.

---

## 9) FFI

### 9.1 ABI de `Result` minimal (C‑friendly)

```c
// T simplifié sur u32, erreur sur code u32
typedef struct { bool ok; union { uint32_t val; uint32_t err; }; } vitte_result_u32;
```

* Côté Vitte, un *shim* convertit `Result<T,E>` ↔ struct C.
* Alternative : renvoyer `bool` + out‑param (pointeur) + code d’erreur global `errno`‑like (optionnel).

### 9.2 Nullabilité

* `null` **n’est jamais** une référence Vitte sûre ; utiliser `Option<&T>` ou `*T` nu en `unsafe`.

---

## 10) Async & concurrence

* Les futures véhiculent `Result<T, E>` ; `JoinHandle<T>` → `Result<T, JoinError>`.
* **Cancellation** utilise `Err(Canceled)`.
* Les erreurs d’ordonnancement peuvent être promues en **panic** selon politique runtime (configurable).

---

## 11) VM / Bytecode

* Le bytecode **ne modélise pas** d’exceptions checked : la propagation est **structurelle** (retours).
* `?` se compile en *branch* vers un **bloc de retour d’erreur** (SSA‑friendly).
* `panic` compile :

  * en **`trap`** (abort) ;
  * en **unwind** avec tables → destructeurs insérés aux bords de blocs.

---

## 12) Tooling (LSP / Formatter / Lints)

* `#[must_use]` sur `Result`/`Option` ; lint : **Résultat ignoré** (quick‑fix : ajouter `?` ou `match`).
* LSP : code‑actions `wrap in Result`, `introduce ?`, `add context(...)`.
* Formatter : style standard pour chaînage :

```vitte
open(path)
  .with_context(|| format("open {}", path))?
  .read_to_end(&mut buf)?
```

---

## 13) Exemples

### 13.1 Division sûre

```vitte
fn div(a: i32, b: i32) -> Result<i32, String> {
  if b == 0 { return Err("division par zéro".to_string()) }
  Ok(a / b)
}
```

### 13.2 Propagation simple

```vitte
fn calc() -> Result<i32, String> {
  let x = div(10, 2)?
  let y = div(x, 0)?
  Ok(y)
}
```

### 13.3 Panic contrôlé

```vitte
fn get_value(v: Option<i32>) -> i32 {
  match v { Some(x) => x, None => panic("valeur manquante") }
}
```

### 13.4 Enum d’erreurs custom + `From`

```vitte
enum FileError { NotFound, PermissionDenied, Corrupted }
impl From<IoError> for FileError { /* mappe les kinds */ }
fn open_file(path: &String) -> Result<File, FileError> { Err(FileError::NotFound) }
```

### 13.5 Contexte

```vitte
fn read_config(path: &String) -> Result<String, Box<dyn Error>> {
  let mut s = String::new()
  open(path)
    .with_context(|| format("open {}", path))?
    .read_to_string(&mut s)
    .with_context(|| format("read {}", path))?
  Ok(s)
}
```

---

## 14) Compatibilité & phases

* **v0.1** : `Result`/`Option`, `?` (Result/Option), `panic` abort/unwind, `Error` de base, context std‑only.
* **v0.2** : *try blocks*, `derive(Error)`, lifetimes explicites côté `Error` si nécessaire, `Try` trait généralisé.

---

## 15) Lignes rouges (politiques)

* **Interdit** : exceptions invisibles en chemin nominal.
* **Recommandé** : pas de `panic` dans les bibliothèques `no_std`.
* **Documenter** : codes stables (`code()`), structure claire.

---

## 16) Questions ouvertes

* `panic` par défaut : *unwind* en debug/std et *abort* ailleurs — figer la matrice cibles ?
* Backtrace : activée par défaut en debug ? gouvernée par env seulement ?
* Doit‑on stabiliser `Try` générique tôt pour supporter d’autres conteneurs (ex. `Poll<Result<T,E>>`)

---

## 17) Références

* Rust Book/Reference — Error handling & `?`
* Go — error values
* POSIX — `errno` mapping
