# Vitte Packages Roadmap

Ce document décrit l'organisation cible en **packages Vitte**.
Terminologie imposée: on parle de *packages* (pas de crates).

Convention d'import dans le langage:
- nom package: `vitte-core` (documentation/architecture).
- chemin dans `use`: `vitte/core` (syntaxe Vitte).

## 1) Fondation langage

### `vitte-core`
- types primitifs.
- gestion erreurs (`Result`, `Option`).
- traits de base.
- alloc abstraite.

### `vitte-alloc`
- allocateur global.
- bump / arena.
- slab.
- fallback `no_std`.

### `vitte-collections`
- `Vec`.
- `Map`.
- `Set`.
- `RingBuffer`.
- `Rope` (utile pour PlatonEditor).

### `vitte-math`
- int / float.
- traits numeriques.
- random.
- SIMD wrapper.

## 2) Async / Concurrence

### `vitte-async`
- `Future`.
- `Executor`.
- `Task`.
- `Scheduler`.

### `vitte-net`
- TCP / UDP.
- sockets non bloquants.
- HTTP minimal.

### `vitte-runtime`
- runtime complet (type Tokio).
- reactor + scheduler.
- timers.
- io driver.

## 3) Systeme & bas niveau

### `vitte-fs`
- abstraction fichiers.
- mmap.
- watchers.

### `vitte-process`
- spawn.
- pipes.
- sandbox.

### `vitte-abi`
- calling conventions.
- register sets.
- lowering ABI.

### `vitte-ffi`
- binding C.
- generation header.
- safe wrappers.

## 4) Compilation / Toolchain

- `vitte-ast`.
- `vitte-hir`.
- `vitte-mir`.
- `vitte-monomorphize`.
- `vitte-borrow`.
- `vitte-typeck`.
- `vitte-const-eval`.
- `vitte-codegen-llvm`.
- `vitte-codegen-cranelift`.
- `vitte-codegen-gcc`.

Reference structurelle: inspiree de `rustc`, adaptee au pipeline Vitte.

## 5) Modules standards (std)

### `vitte-std`
- re-export core + alloc + collections.
- fs.
- net.
- io.
- thread.

### `vitte-io`
- `Reader`.
- `Writer`.
- `BufReader`.
- ANSI (type `std::io` + `std::fmt`).

## 6) Dev tools

### `vitte-test`
- runner.
- expect.
- snapshot.

### `vitte-bench`
- micro bench.
- flamegraph export.

### `vitte-lint`
- regles style Vitte.
- lints custom (style `.end`, etc.).

## 7) Ecosysteme avance

### `vitte-serialize`
- json.
- toml.
- bincode.
- derive macros.

### `vitte-graph`
- graph algorithms.
- topo sort.
- dijkstra.

### `vitte-audio`
- wav.
- synth.
- dsp de base.

### `vitte-gui`
- backend abstraction.
- OpenGL / Vulkan wrapper.

## 8) Data / Scientifique

### `vitte-array`
- ndarray N-dim.
- slicing.
- broadcasting.
- backend BLAS optionnel.

### `vitte-stats`
- mean / variance.
- regression lineaire.
- distributions.

### `vitte-plot`
- plotting simple.
- export SVG / PNG.

### `vitte-ml`
- perceptron.
- k-means.
- logistic regression.
- mini autodiff.

## 9) Scripting / Dynamic tools

### `vitte-eval`
- eval runtime.
- sandboxed interpreter.
- plugin loader.

### `vitte-plugin`
- dynamic modules.
- versioning.
- hot reload.

### `vitte-shell`
- mini shell scripting.
- pipelines.
- env tools.

## 10) Web avance

### `vitte-ws`
- WebSocket server/client.

### `vitte-template`
- moteur templates.
- compile-time rendering.

### `vitte-openapi`
- generation spec OpenAPI.
- doc auto.

## 11) Base de donnees

### `vitte-sql`
- driver SQLite / Postgres.
- async support.

### `vitte-migrate`
- migrations versionnees.
- diff schema.

### `vitte-cache`
- LRU.
- TTL.
- redis-like memory store.

## 12) Securite

### `vitte-jwt`
- encode / decode.
- HMAC / RSA.

### `vitte-tls`
- TLS abstraction.
- backend openssl / rustls-like.

### `vitte-auth`
- session manager.
- role system.

## 13) Concurrence avancee

### `vitte-channel`
- mpsc.
- broadcast.
- bounded / unbounded.

### `vitte-actor`
- actor model.
- supervision tree.

## 14) Dev / Packaging

### `vitte-pack`
- builder artifacts.
- metadata.

### `vitte-registry`
- client registry.
- publish / fetch.

### `vitte-doc`
- generateur doc HTML.
- search index.

## 15) Multimedia

### `vitte-image`
- PNG / JPEG.
- resize.
- color space.

### `vitte-video`
- wrapper ffmpeg.

## 16) Meta-programming

### `vitte-macro`
Chemin Vitte: `use vitte/macros`.
- derive.
- attribute macros.

### `vitte-reflect`
- runtime reflection.
- type metadata.

## 17) Infrastructure moderne

### `vitte-config-derive`
- derive config struct.

### `vitte-env`
- gestion variables env.

### `vitte-scheduler`
- cron-like tasks.

## 18) Batteries included (type Python)

### `vitte-http-client`
Chemin Vitte: `use vitte/http_client`.

### `vitte-requests`
Chemin Vitte: `use vitte/requests`.

### `vitte-jsonpath`
Chemin Vitte: `use vitte/jsonpath`.

### `vitte-yaml`
Chemin Vitte: `use vitte/yaml`.

### `vitte-pickle`
Chemin Vitte: `use vitte/pickle`.

### `vitte-datetime`
Chemin Vitte: `use vitte/datetime`.

### `vitte-pathlib`
Chemin Vitte: `use vitte/pathlib`.

### `vitte-subprocess`
Chemin Vitte: `use vitte/subprocess`.

## 19) Orientation strategique

Si Vitte vise langage systeme, priorite:
- alloc.
- async.
- net.
- fs.
- abi.
- codegen.

Si Vitte vise backend moderne, priorite:
- web.
- db.
- auth.
- json.
- async runtime.

Si Vitte vise tooling / IDE, priorite:
- rope.
- lsp.
- syntax.
- parser incremental.

Si Vitte vise data / scientifique, priorite:
- array.
- stats.
- plot.
- ml.

## Regles d'architecture

- `vitte-core` ne depend d'aucun package haut niveau.
- `vitte-alloc` depend de `vitte-core`.
- `vitte-collections` depend de `vitte-core` + `vitte-alloc`.
- `vitte-std` re-exporte; la logique lourde reste dans les packages specialises.
- `vitte-runtime` depend de `vitte-async` + `vitte-net` + `vitte-io`.
- `vitte-ffi` applique les contrats de `vitte-abi`.

## Priorite MVP (ordre recommande)

1. `vitte-core`.
2. `vitte-alloc`.
3. `vitte-collections`.
4. `vitte-io`.
5. `vitte-async`.
6. `vitte-runtime`.
7. `vitte-net`.
8. `vitte-fs` / `vitte-process`.
9. `vitte-std` (re-exports stables).
