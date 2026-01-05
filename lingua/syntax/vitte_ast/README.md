

# vitte_ast

Crate de définition et de manipulation de l’AST (Abstract Syntax Tree) de Vitte.

Objectifs :
- fournir un **modèle canonique** (types + invariants) pour le parser et les passes du compilateur ;
- exposer des outils standard de parcours (**visit** en lecture, **mut_visit** en réécriture) ;
- centraliser des briques transverses (spans, tokens, token stream, node ids, format args).

Ce crate **ne fait pas** :
- le lexing (c’est le rôle du lexer : il produit `Vec[Token]`) ;
- le parsing (c’est le rôle du parser : il consomme `TokenStream/Cursor` et construit `AstFile`).

---

## Structure

```
compiler/vitte_ast/
  src/
    ast.vit          # définitions AST (items, expr, stmt, types, patterns, spans, node ids basiques)
    format.vit       # AST niveau "format args" (template + placeholders + options)
    node_id.vit      # NodeIdGen + NodeIdRange (allocation/ranges)
    token.vit        # TokenKind/Token + trivia + keywords/punct + helpers
    tokenstream.vit  # TokenStream + Cursor + erreurs + helpers (peek/expect/skip_group)
    visit.vit        # visitor read-only (pré-ordre)
    mut_visit.vit    # visitor mutable (post-ordre)
    entry.vit        # surface d’import stable (re-export)
    lib.vit          # surface crate (re-export + tests_run)
    mod.vit          # arbre de modules + re-export
```

---

## Concepts

### Span

Un `Span` pointe vers une zone source : `file`, `start`, `end`. Il sert à :
- diagnostics (erreurs, warnings) ;
- mapping AST ⇄ source ;
- reconstruction de ranges lors de transformations.

### NodeId

`NodeId` est un identifiant stable et cheap (entier) pour indexer des nœuds AST.

Le module `node_id.vit` fournit :
- `NodeIdGen` : allocateur monotone (`fresh`, `reserve`, `fresh_many`) ;
- `NodeIdRange` : utilitaires de plage (`len`, `contains`).

### Tokens

`token.vit` définit :
- `Trivia` (whitespace/newline/comments) ;
- `Keyword` et `Punct` (classification rapide) ;
- `TokenKind` + `TokenPayload` ;
- `Token` (avec `leading`/`trailing` trivia).

Invariants :
- `Token.kind` détermine la validité de `payload` ;
- `span` est toujours présent (y compris `Error`/`Eof`).

### TokenStream + Cursor

`tokenstream.vit` définit :
- `TokenStream` : wrapper de `Vec[Token]` ;
- `Cursor` : navigation (`current`, `peek`, `bump`, `mark/rewind`) ;
- `expect_*` / `eat_*` : patterns classiques du parser ;
- `skip_group()` : saut d’un groupe délimité équilibré.

### Format args

`format.vit` modélise un appel de formatting au niveau AST :
- `FormatArgs` : `template` (pièces literal/placeholder) + `arguments` ;
- `FormatPlaceholder` : position + trait + options (width/precision/align/etc.).

Invariants importants (`FormatArguments`) :
- les args non nommés ne sont comptés que **avant** le premier arg nommé ;
- les args `Captured` sont ajoutés **après** tous les args explicites ;
- `names` mappe `name -> index`.

---

## Parcours AST

### `visit.vit` (read-only)

Parcours **pré-ordre** : hook sur le nœud courant, puis descente dans les enfants.

Cas d’usage :
- analyse (collecte, stats, validation) ;
- construction de tables (symboles, références) ;
- diagnostics non-mutants.

### `mut_visit.vit` (mutable)

Parcours **post-ordre** : descente d’abord, hook ensuite.

Cas d’usage :
- réécriture bottom-up (désucrage, normalisation) ;
- remplacement de sous-arbres ;
- transformations qui dépendent de la forme des enfants déjà transformés.

---

## Import surface

Pour les crates downstream, l’import recommandé passe par `vitte_ast` (surface stable) :

```vitte
use vitte_ast
use vitte_ast.ast
use vitte_ast.token
use vitte_ast.tokenstream

fn example(ts: &tokenstream::TokenStream)
  let c = tokenstream::cursor_new(ts)
  # parser code...
.end
```

`entry.vit`, `lib.vit` et `mod.vit` assurent les re-exports.

---

## Tests

Chaque module expose un `tests_run()` (smoke tests structurels).

Au niveau crate :
- `src/lib.vit` appelle les `tests_run()` des sous-modules ;
- `src/mod.vit` propose une agrégation équivalente (utile tooling).

Les tests ici sont volontairement "cheap" : ils garantissent la cohérence de surface et la compilation.
Les tests sémantiques plus lourds (parser/resolve/lower) doivent vivre dans les crates de passes.

---

## Contrats de compatibilité

Ce crate se veut **canonique** :
- si l’AST évolue, les invariants et les visitors doivent rester cohérents ;
- les enums `Keyword`/`Punct` doivent rester alignés avec la grammaire officielle ;
- les helpers (`*_span`, `cursor_*`, etc.) doivent produire des spans déterministes.

---

## Roadmap (indicatif)

- enrichir `tokenstream` : support de groupes imbriqués hétérogènes ((), [], {}) + `ExpectedSet` pour diagnostics ;
- compléter l’AST `format` : parsing des specs (`width$`, `precision$`, `*`, flags) côté parser/lower ;
- stabiliser les API `Vec_*` / `Map_*` utilisées (ou adapter via wrappers) pour garantir compile-ready.