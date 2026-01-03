

# DROP_IN — src/asm

Ce fichier explique comment **intégrer (`drop-in`)** le dossier `src/asm` dans un projet (ou dans une autre arborescence Vitte) sans casser le build.

Le module `asm/` sert à fournir :
- des **implémentations optimisées** (ASM) de primitives bas niveau (ex: validation UTF‑8, mem ops, etc.)
- un **dispatch** runtime/compile-time selon l’architecture/OS
- une **surface C** stable (callable depuis le reste du runtime/compilateur)

---

## 1) Arborescence attendue

```text
src/asm/
  DROP_IN.md
  include/
    vitte_asm.h                # API publique C
  src/
    vitte/
      asm_dispatch.c           # sélection impl (feature/cpu)
      asm_verify.c             # vérifications / self-tests optionnels
      cpu.c                    # détection CPU (x86_64, arm64, …)
    asm/
      x86_64/
        utf8/
          vitte_utf8_validate_stub.S
          ... autres .S/.asm
      aarch64/
        ...
  tests/
    ...
```

Règles :
- `include/` contient **uniquement** l’API exposée aux autres modules.
- `src/vitte/*.c` contient le “glue code” (dispatch + wrappers).
- `src/asm/<arch>/...` contient les fichiers ASM.

---

## 2) API publique (contrat)

Le reste du projet ne doit dépendre que de :

- `#include "vitte_asm.h"`

L’API doit être :
- **C ABI** (pas de types exotiques)
- **stable** (versionnable)
- **thread-safe** (la détection CPU peut être lazy mais doit être safe)

Exemples de primitives typiques :
- `vitte_utf8_validate(ptr, len) -> bool`
- `vitte_memchr`, `vitte_memcmp`, `vitte_memcpy_fast` (si besoin)

---

## 3) Compilation (C + ASM)

### 3.1 Principe

Le build doit :
1. Compiler les `.c` de `src/vitte/`
2. Assembler les fichiers `.S`/`.asm` présents pour l’arch cible
3. Linker le tout dans le runtime / toolchain

### 3.2 Flags recommandés

- ASM GNU (`.S`) :
  - `-c` (compile only)
  - `-x assembler-with-cpp` si tu utilises des macros C

- C (glue) :
  - `-O2` (ou profil)
  - `-fvisibility=hidden` + exports explicites si cdylib

### 3.3 macOS

- Sur macOS, le toolchain par défaut est `clang`.
- Les fichiers `.S` (avec CPP) passent bien via `clang -c file.S`.

### 3.4 Windows

Deux options :
- assembler via `clang`/`lld` (recommandé si tu veux homogénéiser)
- ou via MASM/NASM (complexité + formats)

Si tu ne supportes pas encore Windows ASM, garde un **fallback C** (stub) activé par défaut.

---

## 4) Dispatch (sélection runtime)

Le dispatch est central : il choisit la meilleure implémentation disponible.

Approche recommandée :
- `cpu.c` : détecte les features (x86_64: SSE2/SSSE3/SSE4.2/AVX2, arm64: NEON, etc.)
- `asm_dispatch.c` :
  - expose des pointeurs de fonctions “actifs”
  - initialise une fois (lazy init) et bascule vers la meilleure variante

Pattern typique :

```c
// pseudo
static bool g_inited;
static bool (*g_utf8_validate)(const uint8_t*, size_t);

static void init_once(void) {
  if (g_inited) return;
  // détecte CPU
  // g_utf8_validate = impl_avx2 ? ... : impl_sse2 ? ... : impl_c;
  g_inited = true;
}

bool vitte_utf8_validate(const uint8_t* p, size_t n) {
  init_once();
  return g_utf8_validate(p, n);
}
```

Important :
- `init_once()` doit être thread-safe (mutex, atomics, `call_once`, etc.)
- si pas de thread-safe dispo, initialiser au démarrage du programme.

---

## 5) Fallback C (obligatoire)

Chaque primitive ASM doit avoir :
- une implémentation C **référence** (correctness first)
- une ou plusieurs implémentations ASM **optionnelles**

Le fallback C sert :
- sur plateformes non supportées
- quand les features CPU ne sont pas présentes
- comme oracle de test (comparaison résultats)

---

## 6) Ajouter une nouvelle primitive ASM

Checklist :

1. Définir l’API dans `include/vitte_asm.h`
2. Implémenter le fallback C (dans `src/vitte/`)
3. Ajouter les variantes ASM :
   - `src/asm/<arch>/<domain>/.../*.S`
4. Déclarer les symboles ASM côté C (prototypes extern)
5. Mettre à jour `asm_dispatch.c` pour sélectionner la bonne variante
6. Ajouter un test dans `tests/` (comparaison fallback vs ASM)

Conventions :
- nommage : `vitte_<feature>_<primitive>` (ex: `vitte_avx2_utf8_validate`)
- éviter de changer la signature ABI d’une primitive déjà publiée.

---

## 7) Ajouter une nouvelle architecture

1. Créer `src/asm/<arch>/...`
2. Étendre `cpu.c` pour détecter les features pertinentes
3. Étendre `asm_dispatch.c` : mapping features → impl
4. Assurer qu’un fallback C existe (sinon build cassé)

Exemples d’arch :
- `x86_64`
- `aarch64`
- `riscv64` (plus tard)

---

## 8) Tests et vérification

### 8.1 Tests unitaires

Chaque primitive doit être testée sur :
- tailles petites/grandes
- cas alignés / non alignés
- cas limites (0, 1, N-1, N)

### 8.2 Vérification croisée

Option recommandée :
- exécuter fallback C et ASM sur les mêmes entrées
- comparer le résultat

`asm_verify.c` peut contenir :
- self-tests compilables en debug
- assertions d’intégrité

---

## 9) Intégration dans ton build (Muffin / driver)

Le driver/toolchain doit traiter `src/asm` comme un sous-module C/ASM.

Règles :
- Si le backend est `c`/`llvm` : compiler comme une “lib interne” du runtime.
- Si le backend est `vm` : tu peux quand même linker `asm` côté runtime (VM runtime), pas dans le bytecode.

Recommandation : générer un “plan” :
- `obj/asm_dispatch.o`, `obj/cpu.o`, `obj/asm_verify.o`...
- `obj/asm/x86_64/utf8/*.o` selon la cible

---

## 10) FAQ rapide

### Pourquoi un stub ASM `vitte_utf8_validate_stub.S` ?

- Sert de placeholder minimal
- garantit que le pipeline asm compile même si les optimisations ne sont pas encore codées

### Pourquoi `.end` (pas `{}`) n’apparaît pas ici ?

- `src/asm` est un composant **C/ASM**, donc indépendant de la syntaxe Vitte.

---

## 11) TODO (évolutions)

- Ajouter des variantes optimisées (SSE4.2/AVX2/NEON)
- Centraliser les features dans une table (`cpu_features_t`)
- Rendre le dispatch lock-free (atomics) si nécessaire
- Ajouter fuzz tests (UTF‑8) avec corpus
