# Vitte Runtime & VM – MVP

Ce document fixe le **modèle runtime/VM minimal** pour le projet Vitte, afin d’aligner le compilateur, les manifests Muffin et les futurs binaries (`vitte-run`, `vittec run`). Il s’appuie sur la spec de bytecode (`docs/bytecode-spec.md`) et vise à fournir une boucle d’exécution simple mais exploitable pour les exemples et tests smoke.

---

## 1. Objectifs

- Exécuter le bytecode Vitte produit par `vitte.compiler.codegen.bytecode` via une boucle **fetch/decode/execute** linéaire.
- Fournir une **heap linéaire** avec allocation bump-pointer et un **GC mark/sweep** simple pour les types runtime.
- Exposer une **std minimale** (I/O, string, array) suffisante pour les exemples (`tests/data/mini_project`, `examples/*`).
- Rester cohérent avec les manifests existants (`vitte.project.muf`, `bootstrap/core/mod.muf`, `bootstrap/cli/mod.muf`) qui déclarent `vitte-run` et les artefacts runtime.

---

## 2. Composants runtime

- **Loader bytecode** (`vitte.runtime.bytecode`) : valide l’en‑tête, charge le pool de constantes, la table de fonctions et le flux d’instructions conformément à `docs/bytecode-spec.md`. Définit l’entrypoint (fonction exportée `main` ou index 0).
- **VM core** (`vitte.runtime.vm`) : boucle `while` sur le flux d’instructions, dispatch via opcode table, maintient les frames, la pile VM et les registres logiques alignés sur la SSA “figée” du compilateur.
- **Heap manager** (`vitte.runtime.gc`) : heap contiguë avec bump-pointer ; fournit `alloc_heap` et déclenche un GC mark/sweep quand la réserve est basse.
- **Std hooks** (`vitte.runtime.std_hooks`) : ponts vers la std minimale (strings, arrays dynamiques, I/O basique).
- **CLI runner** (`vitte.runtime.cli.run`) : charge un bundle bytecode, installe la std minimale et exécute l’entrypoint.

---

## 3. Boucle d’exécution bytecode

- **Frames et stack** : chaque appel pousse une frame contenant `pc`, base des registres locaux, pointeurs vers les slots de paramètres/temporaires, et un lien vers la frame appelante.
- **Instruction dispatch** : boucle `for`/`while` qui lit `opcode`, décode les opérandes (const_index, local_index, offsets), puis appelle l’implémentation. Les terminators (`jmp`, `jmp_if`, `ret`) ajustent `pc` ou dépilent une frame.
- **Registres logiques** : correspondance 1:1 avec les valeurs SSA (pas d’allocation complexe au MVP). Les slots sont stockés dans la frame et adressés par index.
- **Appels** :
  - `call func_index, dst, args…` : crée une nouvelle frame, copie les arguments dans les slots de paramètres, initialise les locals à `nil`, et démarre à `code_offset`.
  - `call_indirect` optionnel : vérifie la signature, sinon lève une erreur runtime.
- **Contrôle** : `jmp offset` (relatif), `jmp_if cond, offset` (test bool), `ret value?`.
- **Mémoire** :
  - `alloc_heap ty_id, dst` : délègue à l’allocator GC pour struct/array/string/closure.
  - `load_local` / `store_local` manipulent les slots de la frame.
  - `load_field` / `store_field` adressent un champ struct/array indexé (avec vérif bornes minimale pour les exemples).
- **Diagnostics runtime** : erreurs fatales (opcode inconnu, out-of-bounds, signature mismatched) sont reportées via `std_io.stderr` et font quitter avec code non‑zéro.

---

## 4. Heap linéaire + GC mark/sweep

- **Layout** :
  - Heap contiguë, segment unique (`[base, limit)`), bump-pointer `hp`.
  - Chaque allocation stocke un header : `word tag`, `word size_bytes`, `word mark_bit`, puis le payload.
  - Types supportés : `String` (UTF‑8, longueur + données), `Array<T>` (len, cap, pointeur vers buffer), `Struct` (blocs de champs contigus), `Bytes` (optionnel pour I/O).
- **Allocation** :
  - Bump-pointer `hp += size`; si dépassement → `gc_collect()` ; si toujours insuffisant → erreur `OutOfMemory`.
  - Les buffers d’array utilisent la même heap (pas d’alloc système directe dans le MVP).
- **Racines (roots)** :
  - Frames actives (slots registres + paramètres),
  - Pool de constantes objets (strings),
  - Globals runtime (ex. handles std_io).
- **Mark** :
  - Parcours des racines, push sur une petite mark-stack,
  - Suivi des pointeurs pour `Struct`/`Array`/`String` (strings n’ont pas de sous‑références),
  - Positionne `mark_bit` sur chaque objet visité.
- **Sweep** :
  - Scan linéaire de la heap, récupère les blocs non marqués dans une free-list simple,
  - Compactage absent (fragmentation acceptée pour le MVP),
  - Réinitialise `mark_bit` des blocs conservés.
- **Déclenchement** :
  - Avant chaque `alloc_heap`, si `hp + requested > limit`, déclenche `gc_collect`,
  - Compte de collections exposé à la CLI/test pour le reporting smoke.

---

## 5. Std minimale (I/O, string, array)

- **String** :
  - Constructeurs depuis constantes (pool) ou concat via helper `std_string.concat` (réalloue dans la heap),
  - Opérations minimales : length, slice basique (bound-checked), comparaison.
- **Array** :
  - `Array<T>` dynamique avec `(len, cap, data_ptr)` ; `push`, `pop`, `get/set` bornés,
  - Réallocation doublant la capacité dans la même heap (copie élémentaire).
- **I/O** :
  - `print/println` via stdout,
  - `read_line` via stdin, retourne `String`,
  - `read_file`/`write_file` optionnels, stub si l’hôte ne les autorise pas dans les tests.
- Ces primitives sont suffisantes pour les exemples `examples/cli`, `examples/algorithms` et le mini_projet de tests.

---

## 6. Intégration CLI et manifests

- `vitte-run` (et `vittec run`) :
  - lit un manifest Muffin (`vitte.project.muf` ou manifest du projet utilisateur) pour localiser le bundle bytecode à charger,
  - charge la std minimale (ou stub) déclarée dans `src/std/mod.muf`,
  - exécute l’entrypoint `main` via la boucle VM décrite ci‑dessus.
- `vittec build` :
  - produit un bundle bytecode (`target/core/bytecode/` ou `target/debug/`) structuré selon `vitte.project.muf`,
  - enregistre les métadonnées runtime (entrypoint, version bytecode) dans un répertoire aligné avec les artefacts Muffin.
- `vittec test` (smoke) :
  - compile les fixtures (mini_project) puis lance `vitte-run` pour vérifier l’exécution,
  - rapporte dans `target/core/tests/report.txt`.
- `vittec fmt` :
  - placeholder qui confirme la disponibilité de la commande et pointera vers `vitte.tools.format` quand il sera implémenté.

---

## 7. Invariants et compatibilité

- La VM est **strictement alignée** sur la spec de bytecode : un changement d’opcode ou de layout doit être répercuté dans `docs/bytecode-spec.md` et les modules `vitte.runtime.bytecode`.
- Le GC est non compactant ; les pointeurs retournés restent valides tant que l’objet est marqué.
- Les exemples et tests smoke ne doivent utiliser que les primitives std décrites ici tant que la std élargie n’est pas disponible.
- Les manifests Muffin existants restent la **source de vérité** pour les chemins d’artefacts et l’exposition des binaries (`vittec`, `vitte-run`).
