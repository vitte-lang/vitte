---

rfc: 0003
title: "Memory Model & Ownership"
status: proposed
authors: \["Vincent"]
shepherd: "Core Team"
reviewers: \["@vm", "@typer", "@ffi", "@concurrency"]
created: 2025-09-02
updated: 2025-09-02
tracking-issues: \["vitte-lang/vitte#3"]
requires: \["0001-core-syntax-and-keywords", "0002-module-system"]
supersedes: \[]
superseded-by: \[]
discussions-to: "[https://github.com/vitte-lang/vitte/discussions](https://github.com/vitte-lang/vitte/discussions)"
target-release: "v0.1"
labels: \["lang", "memory", "ownership", "safety", "concurrency"]
-----------------------------------------------------------------

> **Résumé** — Ce RFC définit le **modèle mémoire** de Vitte et le système **ownership/borrow** de base : règles d’aliasing, déplacements (`move`), copies (`copy`), emprunts immuables/mutables (`&T` / `&mut T`), durée de vie (lifetimes) **inférées**, destruction déterministe (RAII) et intégration avec la **concurrence** (mémoire atomique et `happens‑before`).

---

## 1) Principes directeurs

* **Sécurité par défaut** : UB interdit en code sûr.
* **Détermination** : libération via RAII à la fin de portée → pas de GC global (le débat avancé reste en RFC 0021).
* **Prévisibilité perf** : zéro‑cost abstractions, aliasing clair.
* **Interop** : frontières FFI explicites (pointeurs nus marqués `unsafe`).

---

## 2) Espace mémoire & objets

* **Pile** : activation des fonctions, allocations à durée lexicale.
* **Tas** : objets dynamiques via allocateurs (`Box<T>`), smart‑pointers (`Rc<T>`, `Arc<T>` en v0.2+), buffers (`Vec<T>`, `String`).
* **Segments** : code, données statiques (`static`, `const`).
* **Objets** : bloc mémoire avec **type** et **invariants**. La destruction appelle `drop` (si implémenté).

---

## 3) Ownership (propriété)

* Chaque valeur a **un propriétaire unique**.
* Au passage de valeur, on a soit **move** (transfert de propriété), soit **copy** (duplication bit‑à‑bit autorisée pour les types `Copy`).
* Après un **move**, l’ancien binding devient **invalide** (interdit de l’utiliser).

### 3.1 `Copy` vs `Move`

* Sont `Copy` par défaut (liste v0.1) : entiers, flottants, bool, char, pointeurs non‑possédants (`&T`, `&mut T`), tuples/structs **composés uniquement** de `Copy`.
* Sont **move‑only** : `String`, `Vec<T>`, `Box<T>`, tout type avec `drop`.

Exemples :

```vitte
let a = 1
let b = a      // copy, a reste utilisable

let s = String::from("hi")
let t = s      // move, s n'est plus valide
print(s)       // ❌ erreur: use after move
```

---

## 4) Borrow (emprunt)

* `&T` : **emprunt immuable** (lecture seule, aliasing autorisé).
* `&mut T` : **emprunt mutable exclusif** (pas d’alias simultané, écriture autorisée).
* Les emprunts ont une **durée de vie** bornée par la portée où ils sont valides.

**Règle d’aliasing** (**exclusivité mutable**) :

* 0..N emprunts `&T` **ou** 0..1 emprunt `&mut T` à la fois sur la même région mémoire.

### 4.1 Durées de vie (lifetimes)

* Les durées de vie sont **inférées** par défaut.
* Énoncé utilisateur via paramètres implicites `<'a, 'b>` (v0.2+) — **hors surface v0.1**.
* **Élision** :

  * Paramètres méthode/fn : si un seul emprunt en entrée → return hérite.
  * Plusieurs entrées empruntées → aucun héritage automatique du retour (annotation requise v0.2+).

Exemples :

```vitte
fn head(xs: &Vec<i32>) -> &i32 { xs[0] }
// l'emprunt de retour est lié à la vie de xs
```

---

## 5) RAII & `drop`

* Tout type peut définir un destructeur `drop(self)` appelé à **fin de portée** ou lors d’un **move out** partiel.
* **Ordre** : LIFO, champs détruits avant leur conteneur complet.
* **Double free** évité par ownership.

```vitte
struct File { handle: RawFd }
impl File {
  fn drop(self) { close(self.handle) }
}
```

---

## 6) Références, slices & pointeurs

* `&T` / `&mut T` : références sûres, non nulles, alignées.
* `&[T]` / `&mut [T]` : **slices** (ptr + len) non nulles.
* `*const T` / `*mut T` : pointeurs nus (peuvent être nuls/mal alignés) → **`unsafe`** requis pour déréférencer.
* `Box<T>` : allocation possédée sur le tas, libérée par `drop`.

---

## 7) Initialisation & moves partiels

* Lecture d’une variable **non initialisée** : erreur compile‑time.
* Déplacement partiel : possible sur champs move‑only, le reste doit être accessible uniquement via pattern qui préserve les invariants.

```vitte
struct Pair { a: String, b: String }
let p = Pair { a: String::from("x"), b: String::from("y") }
let x = move p.a  // déplace a
print(p.b)        // OK
print(p.a)        // ❌ use after move
```

---

## 8) Concurrence & modèle mémoire

### 8.1 Threads & partage

* Partager **en lecture** : `&T` (si `T` immuable thread‑safe).
* Partager **en écriture** : nécessite synchronisation (futures types `Mutex<T>`, `RwLock<T>`, `Atomic*`).

### 8.2 Atomiques (v0.1 minimal)

* Fournir `AtomicBool`, `AtomicI32`, `AtomicUsize`, etc., avec ordres mémoire : `Relaxed`, `Acquire`, `Release`, `AcqRel`, `SeqCst`.
* **Happens‑before** conforme aux modèles usuels (C++11/Rust) — précision formelle en doc VM.

### 8.3 Data‑races

* Interdits en code sûr : un data‑race est **UB**.
* Synchronisation via primitives de la std (futures RFC 0009/0012/0016).

---

## 9) `unsafe` — périmètre

* Bloque `unsafe { ... }` autorise :

  * déréférence de `*const T` / `*mut T`;
  * FFI `extern` et conversions brutes;
  * accéder/écrire à des invariants internes documentés.
* `unsafe` **n’ôte pas** les vérifications de type : il restreint seulement certaines erreurs au run‑time si l’invariant n’est pas tenu.

---

## 10) FFI & nullabilité

* Vitte expose `extern "C"` (surface en RFC 0005) et des **pointeurs nus** pour interop.
* Le littéral `null` n’est **jamais** une référence sûre ; il s’emploie avec `*T` ou avec un futur `Option<&T>`.

---

## 11) Erreurs & diagnostics (typer/borrow‑checker)

* Erreurs typiques :

  * use‑after‑move
  * use of moved value: field `x` moved out
  * borrowed value does not live long enough
  * cannot borrow `x` as mutable because it is also borrowed as immutable
* **Aides** : suggestions pour cloner (`.clone()`), introduire des scopes, utiliser des `&mut` temporaires.

---

## 12) Bytecode/VM : contrat

* Les instructions VM doivent respecter le **contrat d’aliasing** :

  * pas de mutations observables via deux références actives aliasées (`&` vs `&mut`).
* Les **barrières mémoire** (`fence`, ordres d’atomiques) sont exposées dans l’IR.
* **Drop** devient des opcodes/handlers insérés aux sorties de blocs.

---

## 13) Spécification formelle (esquisse)

### 13.1 Règle d’exclusivité

> À tout instant `t`, pour une région `R`,
> \#{ `&mut` actifs sur R } ∈ {0,1} **et** (si 1) alors #{ `&` actifs sur R } = 0.

### 13.2 Sous‑typage & variances (v0.1)

* **Aucune subtyping** structurel par défaut.
* Variance future sur références : `&T` covariant en `T`, `&mut T` invariant (RFC ultérieure si besoin).

---

## 14) Exemples canons

### 14.1 Emprunts simples

```vitte
fn len(s: &String) -> usize { s.length() }
fn push_excl(s: &mut String) { s.push('x') }
```

### 14.2 Réparation d’un conflit d’emprunt

```vitte
let mut s = String::from("abc")
let r = &s
// push_excl(&mut s) // ❌ alias mutable
print(r)
{
  push_excl(&mut s) // ✅ r est hors‑scope
}
```

### 14.3 RAII sur un handle

```vitte
struct Socket { raw: RawSock }
impl Socket {
  fn drop(self) { close_socket(self.raw) }
}
```

---

## 15) Compatibilité & migrations

* `v0.1` : ownership/borrow minimal, lifetimes **inférés** uniquement.
* `v0.2+` : lifetimes explicites, smart‑pointers partagés (`Rc`, `Arc`), pinning (immobilité), interior mutability contrôlée (`Cell`, `RefCell`).

---

## 16) Interactions

* **Erreur/`Result`** (RFC 0004) : destruction propre lors du désempaquetage / propagation.
* **Async** (RFC 0006) : exigences `Send`/`Sync` futures pour passer entre threads.
* **Typer** : produit des **contraintes d’emprunt**; le borrow‑checker s’appuie sur les `NodeId`.

---

## 17) Alternatives considérées

* **GC par défaut** : plus simple pour l’API, coût d’exécution, pauses, moins de contrôle système.
* **Références nullables par défaut** : sources de bugs (NPE) → refusé.
* **Unique mutable par type (Linéalité stricte)** : trop contraignant pour l’ergonomie.

---

## 18) Sécurité / STRIDE (résumé)

| Catégorie       | Risque                    | Mitigation                 |
| --------------- | ------------------------- | -------------------------- |
| Tampering       | Use‑after‑free            | Ownership + RAII           |
| Info Disclosure | Aliasing non intentionnel | Règle d’exclusivité        |
| DoS             | Fuite mémoire             | `drop` déterministe, lints |
| Elevation       | FFI non sûr               | `unsafe` scellé, revue     |

---

## 19) Questions ouvertes

* Surface exacte des **lifetimes explicites** v0.2.
* **Interior mutability** stable en v0.2 ou v0.3 ?
* **Pinning** requis pour certains IO/async ?

---

## 20) Références

* C++11/C++20 Memory Model
* Rust Reference & Nomicon (ownership/borrows)
* Java/C# GC (pour contre‑exemples & comparaison)
